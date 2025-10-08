# ADR-001: Format Layer Zero-Coupling Design

**Status**: Accepted

**Date**: 2025-10-08

**Context**: asyncle 項目需要支援 JSON 解析和序列化功能，但 simdjson 和 Glaze 等外部庫不應該與 asyncle 核心產生耦合。

---

## Decision

採用**雙層架構 + 零耦合設計**：

1. **Foundation Layer** (`format::*`)
   - 提供對外部庫的薄封裝
   - 使用 Type Alias 和 CPO 模式
   - 通過 CMake feature flags 選擇實現

2. **Integration Layer** (`asyncle::format::*`)
   - 完全不知道具體實現（simdjson/Glaze）
   - 只依賴 foundation layer 的抽象接口
   - 提供 operation-based API

---

## Problem Statement

### 初始需求

1. 需要高效的 JSON 解析（考慮 simdjson）
2. 需要通用的序列化/反序列化（考慮 Glaze）
3. 希望支援 async data flow pipeline（未來）

### 設計挑戰

**問題 1**: 如果直接在 asyncle 中使用 simdjson/Glaze，會造成：
- ❌ asyncle 依賴特定實現
- ❌ 難以替換或擴展
- ❌ 用戶被迫使用特定庫

**問題 2**: 如何在保持零拷貝、高性能的同時，又保持抽象？
- 虛函數有性能開銷
- 類型擦除增加複雜度

**問題 3**: 如何設計讓用戶代碼與實現無關？

---

## Solution

### 機制 1: Type Alias (JSON Parsing)

**Foundation Layer** 選擇實現：

```cpp
// format/json/parser.hpp
#ifdef FORMAT_HAS_SIMDJSON
    using parser = simdjson_document;  // Zero-overhead
#elif defined(FORMAT_HAS_GLAZE)
    using parser = glaze_document;
#else
    using parser = stub_parser;  // Compile-time error
#endif
```

**Integration Layer** 使用抽象：

```cpp
// asyncle/format/json.hpp
using ::format::json::parser;  // Type alias (不知道具體類型)

class parser_operation {
    parser parse() const {
        return parser{data_};  // 返回 whatever format::json::parser is
    }
};
```

**優點**：
- ✅ 零運行時開銷（compile-time type alias）
- ✅ asyncle 完全不知道 simdjson
- ✅ 實現可以自由替換

### 機制 2: CPO + ADL (Serialization)

**Foundation Layer** 定義 CPO：

```cpp
// format/serialize.hpp
namespace detail {
    struct save_fn {
        template <typename T, typename Tag>
        auto operator()(T const& obj, Tag tag) const {
            return save_impl(obj, tag);  // ADL lookup
        }
    };
}
inline constexpr detail::save_fn save{};
```

**Implementation** 提供實現（在 format layer）：

```cpp
// format/serialize/glaze.hpp
namespace format::serialize {
    template <typename T>
    auto save_impl(T const& obj, json_tag) -> result<std::string> {
        return glz::write_json(obj, buffer);
    }
}
```

**Integration Layer** 轉發：

```cpp
// asyncle/format/serialize.hpp
template <typename T, typename Tag>
inline auto save(T const& obj, Tag tag) noexcept {
    return ::format::serialize::save(obj, tag);  // Forward to CPO
}
```

**優點**：
- ✅ ADL 查找，零開銷
- ✅ asyncle 永遠不引用 Glaze
- ✅ 用戶可以自定義 save_impl

---

## Consequences

### Positive

1. **完全隔離**
   - asyncle 代碼中零 simdjson/Glaze 引用
   - 可以用 grep 驗證
   - 測試：`test_format_isolation.cpp`

2. **自由替換**
   ```bash
   # 使用 simdjson
   cmake -DFORMAT_ENABLE_SIMDJSON=ON ..

   # 使用自定義實現
   # 只需在 format/json/parser.hpp 修改 type alias
   ```

3. **性能無損**
   - Type alias: 零運行時開銷
   - CPO + ADL: 零運行時開銷
   - 編譯器完全內聯

4. **用戶友好**
   ```cpp
   // 用戶代碼完全不知道實現
   auto parser = asyncle::format::json::make_parser()
       .source(json)
       .make();
   auto doc = parser.parse();
   ```

5. **未來擴展**
   - 為 async pipeline 預留空間
   - 實現選擇在 foundation layer
   - asyncle 代碼無需修改

### Negative

1. **間接層增加**
   - Foundation + Integration 兩層
   - 但對用戶透明

2. **Type alias 的限制**
   - 同一時間只能有一個默認 parser
   - 解決方案：用戶可以直接使用 format layer 的多個實現

3. **編譯時選擇**
   - 不能在運行時切換實現
   - 但這是合理的（零開銷要求）

---

## Architecture Diagram

```
User Code
   │
   ↓ asyncle::format::json::make_parser()
┌──────────────────────────────────────┐
│ asyncle::format::json                 │
│                                       │
│ ✓ 無 #ifdef                           │
│ ✓ 無 simdjson/Glaze 引用              │
│ ✓ 使用 format::json::parser (alias)   │
└──────────────┬───────────────────────┘
               │ 透明轉發
┌──────────────▼───────────────────────┐
│ format::json::parser (Type Alias)     │
│                                       │
│ #ifdef FORMAT_HAS_SIMDJSON            │
│   using parser = simdjson_document;   │
│ #endif                                │
└──────────────┬───────────────────────┘
               │
      ┌────────┴─────────┐
      │                  │
┌─────▼──────┐    ┌──────▼────────┐
│ simdjson   │    │ Custom Parser │
└────────────┘    └───────────────┘
```

---

## Implementation Details

### File Structure

```
include/
├── format/                          # Foundation Layer
│   ├── json.hpp                     # 主接口
│   ├── json/
│   │   ├── types.hpp               # 錯誤、結果類型
│   │   ├── concepts.hpp            # JSON parser 概念
│   │   ├── parser.hpp              # ★ Type alias 選擇實現
│   │   ├── simdjson.hpp            # simdjson adapter (可選)
│   │   └── glaze.hpp               # Glaze adapter (可選)
│   ├── serialize.hpp                # CPO 主接口
│   └── serialize/
│       ├── concepts.hpp            # Serializer 概念
│       └── glaze.hpp               # Glaze adapter (可選)
│
└── asyncle/format/                  # Integration Layer
    ├── json.hpp                     # ★ JSON 操作（零耦合）
    └── serialize.hpp                # ★ 序列化操作（零耦合）
```

### Key Files

**Foundation Layer**:
- `format/json/parser.hpp` - Type alias 定義
- `format/serialize.hpp` - CPO 定義

**Integration Layer**:
- `asyncle/format/json.hpp` - 使用 `format::json::parser`
- `asyncle/format/serialize.hpp` - 轉發到 CPO

### Verification

測試文件驗證零耦合：
- `tests/test_format_isolation.cpp`

```bash
# 驗證：asyncle 頭文件不含實現
grep -r "simdjson" include/asyncle/format/
# 結果：無匹配

grep -r "glaze" include/asyncle/format/
# 結果：無匹配
```

---

## Alternatives Considered

### Alternative 1: Virtual Interface

```cpp
class json_document {
    virtual result<string_view> get_string(string_view path) = 0;
    // ...
};
```

**拒絕原因**：
- ❌ 虛函數調用開銷
- ❌ 無法內聯
- ❌ 違背 zero-overhead 原則

### Alternative 2: Template Everything

```cpp
template <typename Parser>
class parser_operation {
    Parser parse() const { return Parser{data_}; }
};
```

**拒絕原因**：
- ❌ 暴露實現細節（用戶需要知道 Parser 類型）
- ❌ 代碼散佈到頭文件

### Alternative 3: Direct Dependency

```cpp
// 直接在 asyncle 中使用 simdjson
#include <simdjson.h>
```

**拒絕原因**：
- ❌ 硬依賴
- ❌ 無法替換
- ❌ 耦合嚴重

---

## Related Decisions

- Follows the **platform/asyncle pattern**:
  - `platform::file` → `asyncle::io::file`
  - `format::json` → `asyncle::format::json`

- Complements **ADR-000** (hypothetical): "asyncle uses operation-based design"

---

## References

- [format_architecture.md](../format_architecture.md) - 整體架構
- [format_zero_coupling.md](../format_zero_coupling.md) - 零耦合設計詳解
- [format_customization.md](../format_customization.md) - 自定義指南

---

## Verification Checklist

- [x] asyncle 頭文件無 simdjson/Glaze 引用
- [x] 編譯通過（無外部庫時有 stub）
- [x] 測試驗證零耦合 (`test_format_isolation`)
- [x] 性能無損（type alias + ADL）
- [x] 用戶代碼與實現無關
- [x] 實現可以自由替換
- [x] 文檔完整

---

## Notes

此 ADR 記錄了 asyncle format 層從**有耦合設計**到**零耦合設計**的演進。

**舊設計問題**：
- `asyncle/format/json.hpp` 含有 `#ifdef FORMAT_HAS_SIMDJSON`
- 暴露 `simdjson_tag` 等實現細節
- 用戶代碼需要 `parser<simdjson_tag>()`

**新設計解決**：
- Type alias 隱藏實現
- 無條件編譯
- 用戶代碼 `make_parser()` 即可
