# Future asyncle::json Design Notes

> **⚠️ SUPERSEDED**: This document has been superseded by [format_architecture.md](format_architecture.md).
> The current implementation is `asyncle::format::json`, not `asyncle::json`.
>
> **Date**: 2025-10-08

## Vision

The future `asyncle::json` module will be a high-level async API built on top of `format::json`, providing:

1. **Builder pattern** for configuration
2. **Non-blocking parsing** with query objects
3. **Selective field extraction** (true lazy parsing)
4. **Memory control** (stack/heap allocation choice)
5. **Integration with asyncle operation model** (work/make/try_take/try_push)

## Design Principles

### 1. Builder Pattern for Configuration

```cpp
json_builder builder;

// Storage configuration
builder.set_storage(storage_strategy::stack);
builder.set_stack_buffer(my_buffer);  // Or let it allocate

// Source configuration
builder.set_source(json_data, source_lifetime::transient);  // Must copy
builder.set_source(mmap_json, source_lifetime::persistent); // Zero-copy safe

// Parsing configuration
builder.set_filter({"price", "volume"});  // Only parse these fields
builder.set_parser(parser_type::simdjson);
builder.set_validation(validation_level::syntax_only);

// Create JSON object
auto json_obj = make(builder, json_source);
```

### 2. Async Query Interface

```cpp
class json_object {
public:
    // Submit query (non-blocking)
    template <typename T>
    query_handle<T> query(string_view key);

    template <typename T>
    query_handle<T> query_path(std::initializer_list<string_view> path);

    // Check completion
    bool is_ready(const query_handle<T>&) const;

    // Blocking retrieval
    result<T> take(query_handle<T>&&);

    // Non-blocking retrieval
    optional<result<T>> try_take(query_handle<T>&&);

    // Batch queries
    template <typename... Ts>
    query_batch<Ts...> query_batch(pairs<string_view, type_tag<Ts>>...);
};
```

Usage example:
```cpp
json_object json_obj = /* ... */;

// Submit multiple queries
auto price_q = json_obj.query<double>("price");
auto volume_q = json_obj.query<int64_t>("volume");
auto symbol_q = json_obj.query<string_view>("symbol");

// Do other work...
process_other_data();

// Check and retrieve when ready
if (json_obj.is_ready(price_q)) {
    auto price = json_obj.take(std::move(price_q));
    if (price) {
        update_price(price.value());
    }
}

// Or use non-blocking
if (auto result = json_obj.try_take(std::move(volume_q))) {
    if (result->has_value()) {
        update_volume(result->value());
    }
}
```

### 3. Integration with Operation Model

```cpp
// Command-based interface
struct json_parse_command : command<json_error, ...> {};
struct json_query_command : command<json_error, ...> {};

// CPO integration
auto result = work(json_builder, json_parse_command{}, source);
auto value = work(json_object, json_query_command{}, query_spec);
```

### 4. Selective Field Parsing

```cpp
// Only parse specified fields (true lazy)
json_builder builder;
builder.set_filter({"bid", "ask", "last", "timestamp"});

auto json_obj = make(builder, market_tick);

// Only these 4 fields are parsed, rest ignored
// Significant performance gain for large JSON with many fields
```

### 5. Memory Control

```cpp
// Stack allocation (when possible)
alignas(64) byte buffer[4096];
builder.set_storage(storage_strategy::stack);
builder.set_stack_buffer(span{buffer});

// Heap allocation with custom allocator
builder.set_storage(storage_strategy::heap);
builder.set_allocator(my_allocator);

// Let parser decide
builder.set_storage(storage_strategy::auto_select);
```

## Implementation Phases

### Phase 1: Foundation (✓ Complete)
- [x] format::json layer with thin wrappers
- [x] Unified error types
- [x] Capability system
- [x] CMake integration

### Phase 2: Query System (Not Started)
- [ ] query_handle<T> design
- [ ] Field extraction without full parse
- [ ] Query state tracking
- [ ] Completion checking

### Phase 3: Builder Pattern (Not Started)
- [ ] json_builder interface
- [ ] Storage strategy implementation
- [ ] Source lifetime handling
- [ ] Filter specification

### Phase 4: Async Integration (Not Started)
- [ ] json_object with async queries
- [ ] Integration with asyncle operation model
- [ ] Command definitions
- [ ] CPO implementations (work/make/try_take)

### Phase 5: Optimizations (Not Started)
- [ ] Selective field parsing
- [ ] Stack allocation fast path
- [ ] Zero-copy for persistent sources
- [ ] Batch query optimization

## Technical Challenges

### 1. Parser Heterogeneity

simdjson and Glaze have fundamentally different access patterns:

**simdjson**: Single-cursor, forward-only
```cpp
// Can't do this:
auto price_cursor = doc["price"];  // Get cursor
auto volume_cursor = doc["volume"]; // Invalidates price_cursor!
// price_cursor is now invalid
```

**Glaze**: Immediate struct population
```cpp
// Must know type at compile time
struct Data { double price; int64_t volume; };
auto data = parser.parse<Data>(json);  // All fields parsed immediately
```

**Solution**: Adapter layer that:
- For simdjson: Cache field positions during initial scan, replay for queries
- For Glaze: Use json_t for dynamic queries, or require compile-time types

### 2. Async State Management

Need to track:
- Which fields have been queried
- Which queries are complete
- Which parts of JSON have been parsed
- When to invalidate cached data

**Possible design**:
```cpp
class json_object_state {
    bitset<256> queried_fields_;
    bitset<256> completed_fields_;
    vector<cached_value> value_cache_;
    parser_state parser_state_;
};
```

### 3. Memory Lifetime

Complex interaction between:
- Source data lifetime
- Parsed value lifetime
- Query handle lifetime

**Rules to establish**:
1. If source is transient → must copy or parse immediately
2. If source is persistent → can defer parsing until query
3. Query handle must not outlive json_object
4. Taken values must be independent of source

### 4. Type Erasure for Dynamic Queries

```cpp
// Challenge: How to store query results of different types?
auto price_q = json_obj.query<double>("price");       // query_handle<double>
auto symbol_q = json_obj.query<string_view>("symbol"); // query_handle<string_view>

// Internal storage needs type erasure
class query_handle_base {
    virtual ~query_handle_base() = default;
    virtual bool is_ready() const = 0;
};

template <typename T>
class query_handle : public query_handle_base {
    // ...
};
```

## API Sketch

### Core Types

```cpp
namespace asyncle::json {

// Storage strategies
enum class storage_strategy {
    auto_select,  // Let implementation choose
    stack,        // Use provided stack buffer
    heap          // Use heap allocation
};

// Validation levels
enum class validation_level {
    none,         // No validation (unsafe but fast)
    syntax_only,  // Syntax validation only
    full          // Full validation including UTF-8
};

// Query state
enum class query_state {
    pending,      // Query submitted, not started
    in_progress,  // Parser working on this query
    completed,    // Result available
    failed        // Query failed (e.g., key not found)
};

// Builder
class json_builder {
public:
    json_builder& set_storage(storage_strategy);
    json_builder& set_stack_buffer(span<byte>);
    json_builder& set_allocator(allocator_type);

    json_builder& set_source(string_view, source_lifetime);
    json_builder& set_parser(parser_type);

    json_builder& set_filter(initializer_list<string_view> keys);
    json_builder& set_validation(validation_level);

    // Query builder capabilities
    static constexpr bool supports_selective_parsing = true;
    static constexpr bool supports_async_queries = true;
};

// Query handle
template <typename T>
class query_handle {
public:
    query_handle(query_handle&&) noexcept;
    query_handle& operator=(query_handle&&) noexcept;

    query_state state() const noexcept;
    bool is_ready() const noexcept { return state() == query_state::completed; }

    // Can only take once (moves out result)
    result<T> take() &&;

private:
    friend class json_object;
    // Internal state
};

// JSON object
class json_object {
public:
    json_object(json_object&&) noexcept;
    json_object& operator=(json_object&&) noexcept;

    // Query interface
    template <typename T>
    query_handle<T> query(string_view key);

    template <typename T>
    query_handle<T> query_path(initializer_list<string_view> path);

    // Check query state
    template <typename T>
    bool is_ready(const query_handle<T>&) const;

    // Blocking take
    template <typename T>
    result<T> take(query_handle<T>&&);

    // Non-blocking take
    template <typename T>
    optional<result<T>> try_take(query_handle<T>&&);

    // Parser info
    parser_caps caps() const noexcept;
    parser_type active_parser() const noexcept;

private:
    // Implementation details
    unique_ptr<json_object_impl> impl_;
};

// Make function (operation-based)
json_object make(json_builder& builder, string_view source);

// CPO integration
struct json_parse_command : command<json_error,
                                     entry<parse_spec, json_object>> {};

struct json_query_command : command<json_error,
                                     entry<query_spec<double>, double>,
                                     entry<query_spec<string_view>, string_view>
                                     /* ... */> {};

}  // namespace asyncle::json
```

### Usage Examples

#### Example 1: Market Data Processing

```cpp
#include <asyncle/json.hpp>

using namespace asyncle::json;

// Setup reusable builder
json_builder tick_builder;
tick_builder.set_parser(parser_type::simdjson)
            .set_filter({"symbol", "bid", "ask", "last", "volume", "timestamp"})
            .set_storage(storage_strategy::stack)
            .set_validation(validation_level::syntax_only);

alignas(64) byte buffer[2048];
tick_builder.set_stack_buffer(buffer);

void process_market_tick(string_view json) {
    auto tick = make(tick_builder, json);

    // Submit queries
    auto symbol = tick.query<string_view>("symbol");
    auto bid = tick.query<double>("bid");
    auto ask = tick.query<double>("ask");
    auto volume = tick.query<int64_t>("volume");

    // Can do other work here if parsing is async...

    // Retrieve results
    if (auto s = tick.take(std::move(symbol))) {
        if (auto b = tick.take(std::move(bid))) {
            if (auto a = tick.take(std::move(ask))) {
                if (auto v = tick.take(std::move(volume))) {
                    update_order_book(s.value(), b.value(), a.value(), v.value());
                }
            }
        }
    }
}
```

#### Example 2: Configuration Loading

```cpp
#include <asyncle/json.hpp>

using namespace asyncle::json;

json_builder config_builder;
config_builder.set_parser(parser_type::glaze)
              .set_source(config_json, source_lifetime::persistent)
              .set_validation(validation_level::full);

auto config_obj = make(config_builder, config_json);

// For Glaze, might still use compile-time types
// But through json_object interface for consistency
auto api_key = config_obj.query<string>("api_key");
auto risk_limit = config_obj.query<double>("risk_limit");

// Or use Glaze directly for structured data
format::json::glaze_parser glaze;
auto config = glaze.parse<TradingConfig>(config_json);
```

## Open Questions

1. **Query lifetime**: Should queries keep json_object alive? Or require explicit ownership?

2. **Error granularity**: Should queries return `result<T>` or throw? Or provide both?

3. **Batch optimization**: Should batch queries be explicit or automatic?

4. **Parser selection**: Auto-select parser based on query pattern? Or always explicit?

5. **Threading**: Can multiple threads submit queries? How to synchronize?

6. **Cancellation**: Can queries be cancelled before completion?

## Timeline

This design is **exploratory** and **not scheduled** for immediate implementation. Priority order:

1. ✅ **Phase 1** (Complete): Foundation layer working
2. 🔄 **Validate design** with real usage in noemix-trader
3. ⏳ **Phase 2-3**: Implement if async needs become clear
4. ⏳ **Phase 4-5**: Optimize based on profiling

The format::json layer is sufficient for current needs. The asyncle::json async design will be driven by actual requirements from the trading system.

## References

- [FORMAT_LIBRARY.md](FORMAT_LIBRARY.md) - Current implementation
- [JSON_PARSER_ANALYSIS.md](JSON_PARSER_ANALYSIS.md) - Parser comparison
- asyncle operation concepts - Command/CPO design patterns
