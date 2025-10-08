# Format Library Design

> **⚠️ SUPERSEDED**: This document has been superseded by:
> - [ADR-001: Format Layer Zero-Coupling Design](adr/001-format-layer-zero-coupling.md)
> - [format_architecture.md](format_architecture.md)
>
> **Date**: 2025-10-08
>
> Kept for historical reference only.

## Overview

The `format` library is a foundation layer (similar to `platform`) that provides thin wrappers around external data format parsers. It follows these principles:

1. **Minimal abstraction** - Preserve native library functionality
2. **Unified interface** - Standardize common operations where libraries overlap
3. **Capability exposure** - Let users query parser characteristics at compile time
4. **No premature integration** - Operation-based async design deferred to higher layers

## Architecture

```
┌─────────────────────────────────────────┐
│          asyncle::json (future)         │  ← High-level async API
│  Builder pattern, make/try_take/try_push│     (not yet implemented)
├─────────────────────────────────────────┤
│           format::json                   │  ← Foundation layer (current)
│      Thin wrappers, unified types       │
├─────────────────────────────────────────┤
│    simdjson          Glaze               │  ← External libraries
│  (streaming)    (structured)            │
└─────────────────────────────────────────┘
```

This mirrors the platform/asyncle architecture:
```
platform::file      →  asyncle::io::file
platform::mmap      →  asyncle::io::mmap
platform::process   →  asyncle::io::process
format::json        →  asyncle::json (future)
```

## format::json Design

### Core Types

```cpp
namespace format::json {

// Unified error type
enum class error {
    none,
    invalid_syntax,
    type_mismatch,
    key_not_found,
    // ...
};

// Result type following asyncle convention
template <typename T>
using result = expected<T, error>;

// Capability structure (like platform::file_caps)
struct parser_caps {
    bool zero_copy;
    bool lazy_parsing;
    bool streaming;
    bool simd_optimized;
    size_t typical_overhead_pct;
    // ...
};

// Source lifetime hint for future optimizations
enum class source_lifetime {
    transient,    // Must copy immediately
    stable,       // Available during parsing
    persistent    // Available for program lifetime
};

}
```

### simdjson Adapter

```cpp
#include <format/json.hpp>

format::json::simdjson_parser parser;

// Basic parsing
std::string_view json = R"({"price": 123.45, "volume": 1000})";
auto doc_result = parser.parse(json);

if (doc_result) {
    auto doc = std::move(doc_result.value());

    // Access native simdjson document
    double price = doc["price"].get_double();
    int64_t volume = doc["volume"].get_int64();
}

// Query capabilities
constexpr auto caps = format::json::simdjson_parser::caps();
static_assert(caps.zero_copy);
static_assert(caps.lazy_parsing);
static_assert(caps.simd_optimized);

// Access native parser for advanced usage
auto& native = parser.native();
```

### Glaze Adapter

```cpp
#include <format/json.hpp>

// Define structure
struct Config {
    std::string api_key;
    double risk_limit;
    std::vector<std::string> symbols;
};

// Glaze uses compile-time reflection (define meta)
template <>
struct glz::meta<Config> {
    using T = Config;
    static constexpr auto value = object(
        "api_key", &T::api_key,
        "risk_limit", &T::risk_limit,
        "symbols", &T::symbols
    );
};

format::json::glaze_parser parser;

// Parse to struct
auto config_result = parser.parse<Config>(json);
if (config_result) {
    Config config = std::move(config_result.value());
}

// In-place parsing (no allocation for struct)
Config config;
auto result = parser.parse_into(config, json);

// Write JSON
auto json_result = parser.write(config);
auto pretty_result = parser.write_pretty(config);

// Query capabilities
constexpr auto caps = format::json::glaze_parser::caps();
static_assert(caps.structured_binding);
static_assert(caps.can_use_stack);
```

### Streaming Parser (simdjson only)

```cpp
#include <format/json.hpp>

format::json::simdjson_stream_parser stream_parser;

// Parse NDJSON (newline-delimited JSON)
std::string_view ndjson = R"(
{"trade_id": 1, "price": 100.0}
{"trade_id": 2, "price": 101.5}
{"trade_id": 3, "price": 102.0}
)";

auto stream_result = stream_parser.parse_many(ndjson);
if (stream_result) {
    auto stream = std::move(stream_result.value());

    for (auto doc : stream) {
        if (doc.error()) continue;

        int64_t trade_id = doc["trade_id"];
        double price = doc["price"];

        process_trade(trade_id, price);
    }
}
```

## Unified Interface

Where functionality overlaps between libraries, `format::json` provides consistent interfaces:

| Operation | simdjson | Glaze | Common Interface |
|-----------|----------|-------|------------------|
| Parse | ✓ | ✓ | `result<T> parse(string_view)` |
| Validate | ✓ | ✓ | `void_result validate(string_view)` |
| Error handling | ✓ | ✓ | `format::json::error` |
| Capabilities | ✓ | ✓ | `static constexpr parser_caps caps()` |
| Native access | ✓ | ✓ | `native()` method |

Where functionality differs, preserve native interfaces:

| Operation | Library | Interface |
|-----------|---------|-----------|
| Streaming | simdjson only | `simdjson_stream_parser::parse_many()` |
| Structured binding | Glaze only | `glaze_parser::parse<T>()` |
| JSON writing | Glaze only | `glaze_parser::write()` |
| Zero-copy padded | simdjson only | `simdjson_parser::parse_padded()` |

## Design Rationale

### Why Not Integrate with work() Yet?

The current design deliberately **avoids** premature integration with asyncle's operation model (`work()`, `make()`, `try_take()`) because:

1. **Async design missing**: Current wrappers are synchronous. True async requires:
   - Builder pattern for configuration
   - Query objects for field selection
   - State tracking for completion checking
   - Non-blocking interfaces

2. **Parser semantics differ**:
   - simdjson: Single-cursor streaming
   - Glaze: Immediate struct population
   - These require different async patterns

3. **Foundation first**: Like platform layer, establish stable low-level wrappers before building high-level abstractions

### Future asyncle::json Design (Sketch)

The future high-level API might look like:

```cpp
namespace asyncle::json {

// Builder for JSON parsing configuration
class json_builder {
public:
    // Set storage strategy
    json_builder& set_storage(storage_strategy);
    json_builder& set_stack_buffer(span<byte>);

    // Set source and lifetime
    json_builder& set_source(string_view json, source_lifetime);

    // Set field filter (selective parsing)
    json_builder& set_filter(std::initializer_list<string_view> keys);

    // Choose underlying parser
    json_builder& use_parser(parser_type);
};

// JSON object with async query interface
class json_object {
public:
    // Query interface
    template <typename T>
    query_handle<T> query(string_view key);

    // Check completion
    bool is_ready(const query_handle<T>&);

    // Retrieve result (blocking if not ready)
    result<T> take(query_handle<T>&&);

    // Non-blocking retrieval
    optional<result<T>> try_take(query_handle<T>&&);
};

// Usage
json_builder builder;
builder.set_source(market_data, source_lifetime::transient)
       .set_filter({"price", "volume", "timestamp"})
       .use_parser(parser_type::simdjson);

auto json_obj = make(builder, source);  // Returns json_object

auto price_query = json_obj.query<double>("price");
auto volume_query = json_obj.query<int64_t>("volume");

// Can do other work here...

if (json_obj.is_ready(price_query)) {
    auto price = json_obj.take(std::move(price_query));
}
```

This design would enable:
- **Non-blocking parsing**: Submit parse request, continue work, check later
- **Selective field access**: Only parse requested fields (true lazy)
- **Memory control**: Choose stack vs heap allocation
- **Copy control**: Specify source lifetime to minimize copies

## Comparison with Platform Layer

| Aspect | platform::file | format::json |
|--------|---------------|--------------|
| Purpose | OS file operations | Format parsing |
| Abstraction | Thin RAII wrapper | Thin adapter |
| Native access | ✓ | ✓ |
| Capability system | file_caps | parser_caps |
| Error handling | expected<T, error> | expected<T, error> |
| Higher layer | asyncle::io::file | asyncle::json (future) |
| Async support | In higher layer | In higher layer (future) |

## Usage Recommendations

### Use format::json when:
- Building foundation/infrastructure code
- Need direct control over parser behavior
- Working with parser-specific features
- Prototyping before async integration

### Wait for asyncle::json when:
- Need non-blocking parsing
- Want unified async interface
- Building high-level application code
- Need selective field parsing

## Build Configuration

```cmake
# Enable format library with JSON parsers
option(FORMAT_ENABLE_SIMDJSON "Enable simdjson" OFF)
option(FORMAT_ENABLE_GLAZE "Enable Glaze" OFF)

# In your project
target_link_libraries(your_target PRIVATE format)
```

```bash
# Build with simdjson
cmake -DFORMAT_ENABLE_SIMDJSON=ON ..

# Build with Glaze
cmake -DFORMAT_ENABLE_GLAZE=ON ..

# Build with both
cmake -DFORMAT_ENABLE_SIMDJSON=ON -DFORMAT_ENABLE_GLAZE=ON ..
```

## Implementation Status

- [x] format::json foundation layer
- [x] simdjson adapter with unified error types
- [x] Glaze adapter with unified error types
- [x] Capability system (parser_caps)
- [x] CMake integration with optional dependencies
- [ ] asyncle::json builder pattern
- [ ] Async query interface
- [ ] Selective field parsing
- [ ] Integration with asyncle operation model

## Related Documentation

- [JSON_PARSER_ANALYSIS.md](JSON_PARSER_ANALYSIS.md) - Library comparison and selection rationale
- [HARDWARE_MODULE_DESIGN.md](HARDWARE_MODULE_DESIGN.md) - Similar layered architecture pattern
- Platform layer documentation - Parallel design philosophy
