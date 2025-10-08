# Format Layer Documentation

## Quick Start

### Basic Usage

```cpp
#include <asyncle/format/json.hpp>
#include <asyncle/format/serialize.hpp>

// JSON Parsing
auto parser = asyncle::format::json::make_parser()
    .source(json_string)
    .make();
auto doc = parser.parse();

// Serialization
struct Config { int port; std::string host; };
auto json = asyncle::format::to_json(config);
auto config = asyncle::format::from_json<Config>(json);
```

### CMake Integration

```cmake
# Enable optional libraries
option(FORMAT_ENABLE_SIMDJSON "Enable simdjson for JSON parsing" ON)
option(FORMAT_ENABLE_GLAZE "Enable Glaze for serialization" ON)

target_link_libraries(your_app PRIVATE asyncle)
```

---

## Architecture

### Two-Layer Design

```
┌─────────────────────────────────────────────────┐
│  User Application Code                          │
└───────────────┬─────────────────────────────────┘
                │
    ┌───────────┴───────────┐
    │                       │
┌───▼────────────────┐  ┌──▼──────────────────────┐
│ Foundation Layer    │  │ Integration Layer        │
│ (format::*)         │  │ (asyncle::format::*)     │
│                     │  │                          │
│ - format::json      │  │ - asyncle::format::json  │
│ - format::serialize │  │ - asyncle::format::...   │
│                     │  │                          │
│ Sync, Pluggable     │◄─┤ Async, Operation-based   │
│ Thin wrappers       │  │ Builder patterns         │
└─────────┬───────────┘  └──────────────────────────┘
          │
    ┌─────┴──────┐
    │            │
┌───▼───┐   ┌───▼────┐
│simdjson│   │ Glaze  │
│(optional│   │(optional│
└────────┘   └────────┘
```

### Foundation Layer (`format::*`)

**Location**: `include/format/`

**Purpose**:
- Thin wrappers around external libraries (simdjson, Glaze)
- Define unified concepts and interfaces
- Completely synchronous, no async abstractions
- Pluggable implementations via feature flags

**Components**:

#### JSON Parsing (`format::json`)
```cpp
#include <format/json.hpp>

format::json::parser doc(json_string);
auto result = doc.iterate();
```

**Key mechanism**: Type alias
```cpp
// format/json/parser.hpp
#ifdef FORMAT_HAS_SIMDJSON
    using parser = simdjson_document;  // Selected at compile time
#else
    using parser = stub_parser;
#endif
```

#### Serialization (`format::serialize`)
```cpp
#include <format/serialize.hpp>

auto json = format::serialize::save(obj, json_tag{});
auto result = format::serialize::load<T>(json, json_tag{});
```

**Key mechanism**: CPO (Customization Point Objects) + ADL

### Integration Layer (`asyncle::format::*`)

**Location**: `include/asyncle/format/`

**Purpose**:
- High-level, user-friendly API
- Builder patterns for configuration
- Future async operation support
- Zero coupling to implementation libraries

**Zero Coupling Verification**:
```bash
grep -r "simdjson" include/asyncle/format/  # No matches
grep -r "glaze" include/asyncle/format/     # No matches
```

---

## File Structure

```
include/
├── format/                          # Foundation Layer
│   ├── json.hpp                     # Convenience header
│   ├── json/
│   │   ├── types.hpp               # Error, result types
│   │   ├── concepts.hpp            # json_parser concept
│   │   ├── parser.hpp              # ⭐ Type alias (key!)
│   │   └── simdjson.hpp            # simdjson adapter
│   ├── serialize.hpp               # CPO definitions
│   └── serialize/
│       ├── concepts.hpp            # Serializer concepts
│       └── glaze.hpp               # Glaze adapter
│
└── asyncle/format/                  # Integration Layer
    ├── json.hpp                     # ⭐ Recommended entry
    └── serialize.hpp                # ⭐ Recommended entry
```

### Which Header to Use

| Use Case | Header | Why |
|----------|--------|-----|
| **Application code** | `asyncle/format/json.hpp` | Zero-coupled, builder patterns |
| **Application serialization** | `asyncle/format/serialize.hpp` | Zero-coupled, convenient API |
| **Benchmarking** | `format/json/simdjson.hpp` | Direct access to implementation |
| **Custom parser** | `format/json/concepts.hpp` | Define your own parser |

---

## Customization

### Custom JSON Parser

```cpp
#include <format/json/concepts.hpp>

class my_parser {
public:
    my_parser(std::string_view json);

    auto iterate() const -> format::json::result<my_iterator>;
    auto get_error() const -> format::json::parse_error;
};

// Verify it satisfies the concept
static_assert(format::json::json_parser<my_parser>);
```

Then configure at compile time:
```cpp
// format/json/parser.hpp
#ifdef FORMAT_HAS_MY_PARSER
    using parser = my_parser;
#endif
```

### Custom Serializer

```cpp
#include <format/serialize/concepts.hpp>

// Implement save_impl via ADL
namespace my_namespace {
    struct MyData { int value; };

    auto save_impl(MyData const& obj, format::serialize::json_tag)
        -> format::serialize::result<std::string>
    {
        return std::format("{{\"value\":{}}}", obj.value);
    }
}

// Now works with CPO
auto json = format::serialize::save(my_data, json_tag{});
```

---

## Design Rationale

### Why Zero Coupling?

**Problem**: Direct dependency on simdjson/Glaze couples user code to specific implementations.

**Solution**:
1. **Type Alias** for JSON parsing - compile-time selection
2. **CPO + ADL** for serialization - runtime customization

**Benefits**:
- Replace simdjson with another parser without changing asyncle code
- Support multiple serialization formats (JSON, BSON, MessagePack)
- Test with stub implementations
- No runtime overhead (zero-cost abstraction)

### Why Two Layers?

| Layer | Purpose | Users |
|-------|---------|-------|
| **Foundation** (`format::*`) | Sync wrappers, can be used standalone | Low-level code, tests, benchmarks |
| **Integration** (`asyncle::format::*`) | Async-ready, builder patterns | Application code |

**Analogy**: Similar to `platform::file` (sync I/O) vs `asyncle::io::file` (async operations)

---

## Implementation Details

### JSON Parsing: Type Alias Pattern

```cpp
// Foundation layer selects implementation
namespace format::json {
    #ifdef FORMAT_HAS_SIMDJSON
        using parser = simdjson_document;
    #else
        using parser = stub_parser;
    #endif
}

// Integration layer uses the alias
namespace asyncle::format::json {
    using ::format::json::parser;  // No knowledge of simdjson

    class parser_operation {
        parser parse() const {
            return parser{data_};  // Whatever parser is
        }
    };
}
```

**Key insight**: asyncle only sees `format::json::parser`, never `simdjson_document`.

### Serialization: CPO + ADL Pattern

```cpp
// Foundation layer defines CPO
namespace format::serialize {
    namespace detail {
        struct save_fn {
            template <typename T, typename Format>
            auto operator()(T const& obj, Format fmt) const {
                return save_impl(obj, fmt);  // Unqualified call → ADL
            }
        };
    }
    inline constexpr detail::save_fn save{};
}

// Implementation found via ADL
namespace format::serialize {
    auto save_impl(auto const& obj, json_tag) -> result<std::string> {
        // Glaze implementation
    }
}
```

**Key insight**: CPO finds implementation via ADL, no direct calls to Glaze.

---

## Testing

```bash
# Build with implementations
cmake -B build -DFORMAT_ENABLE_SIMDJSON=ON -DFORMAT_ENABLE_GLAZE=ON
ninja -C build

# Run tests
./build/test_format_isolation      # Verify zero coupling
./build/test_format_abstraction    # Test custom implementations
./build/test_asyncle_format        # Integration tests
```

---

## Further Reading

- [ADR-001: Zero Coupling Design](adr/001-format-layer-zero-coupling.md) - Detailed design decision
- [EXTERNAL_DEPENDENCIES.md](EXTERNAL_DEPENDENCIES.md) - Installing simdjson/Glaze
- [Archive](archive/) - Historical documents from development process
