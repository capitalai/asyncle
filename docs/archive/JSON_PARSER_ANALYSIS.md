# JSON Parser Analysis for Asyncle

> **⚠️ SUPERSEDED**: This analysis has been superseded by [ADR-001](adr/001-format-layer-zero-coupling.md).
> The final design is documented in [format_zero_coupling.md](format_zero_coupling.md).
>
> **Date**: 2025-10-08

**Status**: Analysis complete. Implementation as `format::json` foundation layer.

See also:
- [FORMAT_LIBRARY.md](FORMAT_LIBRARY.md) - Current implementation documentation
- [FORMAT_JSON_EXAMPLES.md](FORMAT_JSON_EXAMPLES.md) - Usage examples
- [FUTURE_ASYNCLE_JSON.md](FUTURE_ASYNCLE_JSON.md) - Future async API design

## Design Philosophy Comparison

This document analyzes JSON parsers against asyncle's core design principles:
- **Zero-copy**: Operate directly on original data
- **Lazy parsing**: Only parse what's accessed
- **Lightweight index**: Minimal memory overhead
- **No unnecessary DOM**: Avoid full tree construction

## Detailed Library Analysis

### 1. simdjson ★★★★★

**Design Philosophy**: Pure streaming, zero-copy, lazy parsing

#### Architecture
```cpp
Parse Mode: Lazy iterator (On-Demand API)
Memory Model: Zero-copy with lightweight tape index
Access Pattern: Single-pointer forward iteration
Allocation Strategy: ~50-100% of JSON size for index
```

#### Capabilities
```cpp
✓ Zero-copy: YES (operates on original bytes)
✓ Lazy parsing: YES (only parses accessed fields)
✓ Lightweight index: YES (tape-based index)
✓ No full DOM: YES (never builds tree)
✓ SIMD optimized: YES (2-3 GB/s)
✓ Strict validation: YES (full JSON compliance)

✗ Random access: NO (single pointer)
✗ Multi-cursor: NO (must rewind)
✗ Modification: NO (read-only)
```

#### Memory Overhead
- Index: ~50-100% of JSON size
- No additional allocations for parsed values
- Can parse in-place with padding

#### Ideal Use Cases
```cpp
// Streaming large JSON arrays
for (auto item : json_array) {
    process(item["price"], item["volume"]);
}

// Sequential field access
auto price = doc["market"]["bid"];
auto volume = doc["market"]["volume"];
```

#### Limitations
```cpp
// Cannot do this (requires backtracking):
auto it1 = doc["quotes"][0];
auto it2 = doc["quotes"][1];  // ✗ it1 is now invalidated
compare(it1, it2);

// Must do this instead:
auto val1 = materialize(doc["quotes"][0]);
auto it2 = doc["quotes"][1];
compare(val1, it2);
```

---

### 2. Glaze ★★★★★ (Different Philosophy)

**Design Philosophy**: Compile-time reflection, direct memory mapping

#### Architecture
```cpp
Parse Mode: Eager deserialization into C++ objects
Memory Model: Direct write to object memory
Access Pattern: Hash-based key lookup
Allocation Strategy: User-provided C++ structures (stack or heap)
```

#### Capabilities
```cpp
✓ Zero-copy: PARTIAL (can use stack-allocated references)
✓ Lazy parsing: LIMITED (can skip unused fields with validate_skipped=false)
✓ Lightweight: YES (no intermediate DOM if using typed structs)
✗ No full allocation: NO (allocates target C++ objects)

✓ Compile-time reflection: YES (automatic struct mapping)
✓ Out-of-order keys: YES (hash-based lookup)
✓ Random access: YES (direct struct member access)
✓ Modification: YES (can serialize back)
✓ SWAR optimized: YES (~1.2 GB/s read, 1.4 GB/s write)
```

#### Memory Overhead
- Known structure: Only C++ object size (minimal!)
- Generic JSON (glz::json_t): Similar to DOM parsers
- Stack allocation: Zero heap allocations possible

#### Ideal Use Cases
```cpp
// Direct mapping to C++ structs
struct Market {
    double bid;
    double ask;
    uint64_t volume;
};

Market market;
glz::read_json(market, json_string);  // Direct deserialization
auto spread = market.ask - market.bid;  // Native C++ access

// Out-of-order keys handled efficiently
// Keys can appear in any order in JSON
```

#### Limitations
```cpp
// Must know structure at compile time
// For truly dynamic JSON, falls back to glz::json_t (allocates)

// Not ideal for skipping large portions of JSON
// (simdjson's iterator skips faster)
```

---

### 3. yyjson ★★★☆☆

**Design Philosophy**: Fast DOM construction

#### Architecture
```cpp
Parse Mode: Eager DOM construction
Memory Model: Immutable DOM tree
Access Pattern: Tree traversal
Allocation Strategy: Full DOM allocation (~2-3x JSON size)
```

#### Capabilities
```cpp
✗ Zero-copy: NO (builds DOM)
✗ Lazy parsing: NO (parses everything)
✗ Lightweight: NO (full tree in memory)
✓ Fast: YES (fastest DOM parser)
✓ Random access: YES
✓ Read-only optimization: YES
```

**Verdict**: Does not match asyncle's design philosophy (full DOM allocation)

---

### 4. jsmn / flatjson ★★★★☆

**Design Philosophy**: Minimal token-based parsing

#### Architecture
```cpp
Parse Mode: Index-only (tokens)
Memory Model: Array of offset/length tokens
Access Pattern: Manual token traversal
Allocation Strategy: Fixed token array (can be stack-allocated)
```

#### Capabilities
```cpp
✓ Zero-copy: YES (only stores offsets)
✓ Lazy access: YES (tokens are just indexes)
✓ Lightweight: YES (minimal memory)
✗ Convenience: NO (very low-level API)
✗ Type conversion: NO (user must implement)
```

**Verdict**: Great for embedded systems, too low-level for asyncle

---

## Comparison Matrix

| Feature | simdjson | Glaze | yyjson | jsmn |
|---------|----------|-------|--------|------|
| Zero-copy | ✓✓✓ | ✓✓ | ✗ | ✓✓✓ |
| Lazy parsing | ✓✓✓ | ✓ | ✗ | ✓✓✓ |
| Lightweight index | ✓✓✓ | ✓✓ | ✗ | ✓✓✓ |
| No full DOM | ✓✓✓ | ✓✓ | ✗ | ✓✓✓ |
| Random access | ✗ | ✓✓✓ | ✓✓✓ | ✓ |
| Multi-cursor | ✗ | ✓✓✓ | ✓✓✓ | ✓ |
| Out-of-order keys | ✗ | ✓✓✓ | ✓✓✓ | ✓ |
| Compile-time types | ✗ | ✓✓✓ | ✗ | ✗ |
| Modification | ✗ | ✓✓✓ | ✗ | ✗ |
| Performance | ✓✓✓ | ✓✓ | ✓✓✓ | ✓ |
| API convenience | ✓✓ | ✓✓✓ | ✓✓ | ✗ |

## Design Philosophy Alignment

### simdjson
```
Asyncle Philosophy Match: ★★★★★ (95%)

Pros:
+ Perfect zero-copy implementation
+ True lazy parsing (only parse what you touch)
+ Minimal memory overhead
+ No DOM construction
+ Production-proven (Facebook, ClickHouse)

Cons:
- Single-cursor limitation
- Poor performance with out-of-order keys
- Read-only (cannot modify)
```

### Glaze
```
Asyncle Philosophy Match: ★★★★☆ (80%)

Pros:
+ Compile-time reflection (matches asyncle's concept-driven design)
+ Direct memory mapping (no intermediate representation)
+ Excellent with out-of-order keys
+ Can use stack allocation (zero heap allocations)
+ Random access and multi-cursor support
+ Can modify and serialize back

Cons:
- Requires known structure at compile time
- Not true lazy parsing (deserializes into C++ objects)
- For generic JSON, falls back to allocations
```

## Recommendations

### For asyncle Core Library

**Implementation Status**: ✅ Implemented as `format::json` foundation layer

The recommendation has been implemented following the platform layer pattern:

```cpp
// format/json/types.hpp (Implemented)
namespace format::json {

struct parser_caps {
    bool zero_copy;
    bool lazy_parsing;
    bool lightweight_index;
    bool full_dom;
    bool streaming;
    bool random_access;
    bool multiple_cursors;
    bool simd_optimized;
    bool swar_optimized;
    bool compile_time_reflection;
    size_t typical_overhead_pct;
    // ...
};

// Implemented adapters
class simdjson_parser { /*...*/ };
class glaze_parser { /*...*/ };
class simdjson_stream_parser { /*...*/ };
class glaze_dynamic_parser { /*...*/ };

}
```

See [FORMAT_LIBRARY.md](FORMAT_LIBRARY.md) for complete implementation details.

**Option 2: Build Custom Parser** (Long-term consideration)
```cpp
Goals:
- Zero-copy like simdjson
- Multi-cursor support like glaze
- SIMD optimization (requires asyncle/hardware/simd.hpp)
- Flexible compile-time or runtime typing

Timeline: After Phase 3 (SIMD module) completion
Effort: 3-6 months development + testing
```

### For noemix-trader (Immediate)

**Recommended: Use BOTH strategically via format::json**

```cpp
// High-frequency market data (streaming)
// Use simdjson adapter for zero-copy, lazy parsing
#include <format/json.hpp>

format::json::simdjson_parser parser;
auto doc_result = parser.parse(market_tick);
if (doc_result) {
    auto doc = std::move(doc_result.value());
    process(doc["price"].get_double(), doc["volume"].get_int64());
}

// Configuration, API responses (structured data)
// Use Glaze adapter for type-safe, convenient mapping
#include <format/json.hpp>

struct Config {
    std::string api_key;
    double risk_limit;
    std::vector<std::string> symbols;
};

format::json::glaze_parser config_parser;
auto config_result = config_parser.parse<Config>(config_file);
if (config_result) {
    Config config = std::move(config_result.value());
    // Use config...
}
```

**Rationale**:
- simdjson adapter: Perfect for high-throughput market data streams
- Glaze adapter: Perfect for structured config and API responses
- Different tools for different jobs
- Unified error handling via `format::json::error`
- Consistent capability queries via `parser_caps`

See [FORMAT_JSON_EXAMPLES.md](FORMAT_JSON_EXAMPLES.md) for complete usage examples.

## Capability System Design

Based on platform::file_caps pattern:

```cpp
namespace asyncle::format {

struct parser_caps {
    // Memory characteristics
    bool is_zero_copy;
    bool requires_dom_allocation;
    bool supports_in_place_parse;
    bool supports_stack_allocation;

    // Parsing strategy
    enum class parse_mode {
        eager_dom,           // yyjson
        lazy_iterator,       // simdjson
        compile_time_typed,  // glaze with structs
        index_only           // jsmn
    } mode;

    // Access patterns
    bool supports_random_access;
    bool supports_rewind;
    bool supports_multi_cursor;
    bool handles_out_of_order_keys_efficiently;

    // Type system
    bool requires_compile_time_types;
    bool supports_runtime_types;
    bool supports_schema_validation;

    // Performance
    size_t typical_memory_overhead_percent;
    size_t parse_speed_mbps;

    // Features
    bool supports_streaming;
    bool supports_modification;
    bool supports_serialization;

    constexpr parser_caps() noexcept = default;
};

// Query capabilities at compile time
template<typename Parser>
constexpr parser_caps get_parser_caps() {
    return parser_traits<Parser>::capabilities();
}

}
```

## Conclusion

**For Asyncle**:
- Provide capability abstraction for multiple parsers
- Do not reinvent the wheel yet (simdjson and glaze already excellent)
- Revisit custom parser after Phase 3 SIMD completion

**For noemix-trader**:
- Use simdjson for streaming market data
- Use glaze for structured configs and API responses
- Get best of both worlds

**Long-term**:
- Consider custom parser only if neither library meets needs
- Focus on completing hardware/simd.hpp first
- Capability system allows easy parser switching
