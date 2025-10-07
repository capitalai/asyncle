# format::json Usage Examples

## Overview

`format::json` is a foundation layer providing thin wrappers around external JSON libraries. This document provides practical usage examples.

**Important**: This is NOT the high-level async API. For future async design, see [FUTURE_ASYNCLE_JSON.md](FUTURE_ASYNCLE_JSON.md).

## Quick Start

### Enable JSON Support

```bash
# CMake configuration
cmake -DFORMAT_ENABLE_SIMDJSON=ON -DFORMAT_ENABLE_GLAZE=ON ..
```

### Include Header

```cpp
#include <format/json.hpp>
```

## simdjson Examples

### Example 1: Basic Parsing

```cpp
#include <format/json.hpp>
#include <iostream>

void basic_parsing() {
    format::json::simdjson_parser parser;

    std::string_view json = R"({
        "symbol": "2330.TW",
        "price": 580.0,
        "volume": 15234
    })";

    auto doc_result = parser.parse(json);
    if (!doc_result) {
        std::cerr << "Parse error: "
                  << format::json::error_string(doc_result.error()) << '\n';
        return;
    }

    auto doc = std::move(doc_result.value());

    // Zero-copy access to fields
    auto symbol = doc["symbol"].get_string();
    double price = doc["price"].get_double();
    int64_t volume = doc["volume"].get_int64();

    std::cout << "Symbol: " << symbol << '\n'
              << "Price: " << price << '\n'
              << "Volume: " << volume << '\n';
}
```

### Example 2: Stream Processing (NDJSON)

```cpp
#include <format/json.hpp>
#include <iostream>

void stream_processing() {
    format::json::simdjson_stream_parser stream_parser;

    // Newline-delimited JSON
    std::string_view ndjson = R"(
{"trade_id": 1, "price": 100.0, "qty": 50}
{"trade_id": 2, "price": 101.5, "qty": 75}
{"trade_id": 3, "price": 102.0, "qty": 100}
)";

    auto stream_result = stream_parser.parse_many(ndjson);
    if (!stream_result) {
        std::cerr << "Stream parse error\n";
        return;
    }

    auto stream = std::move(stream_result.value());

    for (auto doc : stream) {
        if (doc.error()) {
            std::cerr << "Document error\n";
            continue;
        }

        int64_t trade_id = doc["trade_id"];
        double price = doc["price"];
        int64_t qty = doc["qty"];

        std::cout << "Trade " << trade_id
                  << ": price=" << price
                  << " qty=" << qty << '\n';
    }
}
```

### Example 3: Error Handling

```cpp
#include <format/json.hpp>
#include <iostream>

void error_handling() {
    format::json::simdjson_parser parser;

    std::string_view invalid_json = R"({"key": "value",})";  // Trailing comma

    auto result = parser.parse(invalid_json);

    if (!result) {
        auto err = result.error();

        switch (err) {
            case format::json::error::invalid_syntax:
                std::cerr << "Invalid JSON syntax\n";
                break;
            case format::json::error::utf8_error:
                std::cerr << "Invalid UTF-8 encoding\n";
                break;
            default:
                std::cerr << "Parse error: "
                          << format::json::error_string(err) << '\n';
        }
        return;
    }

    // Process valid document
    auto doc = std::move(result.value());
    // ...
}
```

### Example 4: Nested Objects

```cpp
#include <format/json.hpp>
#include <iostream>

void nested_objects() {
    format::json::simdjson_parser parser;

    std::string_view json = R"({
        "order": {
            "id": 12345,
            "details": {
                "symbol": "2330.TW",
                "quantity": 1000,
                "price": 580.0
            }
        }
    })";

    auto doc_result = parser.parse(json);
    if (!doc_result) return;

    auto doc = std::move(doc_result.value());

    // Navigate nested structure
    auto order = doc["order"];
    int64_t id = order["id"];

    auto details = order["details"];
    auto symbol = details["symbol"].get_string();
    int64_t quantity = details["quantity"];
    double price = details["price"];

    std::cout << "Order " << id << ": "
              << quantity << " shares of " << symbol
              << " at " << price << '\n';
}
```

### Example 5: Query Capabilities

```cpp
#include <format/json.hpp>
#include <iostream>

void query_capabilities() {
    constexpr auto caps = format::json::simdjson_parser::caps();

    std::cout << "simdjson capabilities:\n"
              << "  Zero-copy: " << caps.zero_copy << '\n'
              << "  Lazy parsing: " << caps.lazy_parsing << '\n'
              << "  Streaming: " << caps.streaming << '\n'
              << "  SIMD optimized: " << caps.simd_optimized << '\n'
              << "  Random access: " << caps.random_access << '\n'
              << "  Multiple cursors: " << caps.multiple_cursors << '\n'
              << "  Overhead: ~" << caps.typical_overhead_pct << "% of JSON size\n";
}
```

## Glaze Examples

### Example 1: Structured Parsing

```cpp
#include <format/json.hpp>
#include <string>
#include <vector>
#include <iostream>

// Define your data structure
struct Config {
    std::string api_key;
    std::string api_secret;
    double risk_limit;
    std::vector<std::string> symbols;
};

// Glaze uses compile-time reflection
template <>
struct glz::meta<Config> {
    using T = Config;
    static constexpr auto value = glz::object(
        "api_key", &T::api_key,
        "api_secret", &T::api_secret,
        "risk_limit", &T::risk_limit,
        "symbols", &T::symbols
    );
};

void structured_parsing() {
    format::json::glaze_parser parser;

    std::string_view json = R"({
        "api_key": "your_key",
        "api_secret": "your_secret",
        "risk_limit": 10000.0,
        "symbols": ["2330.TW", "2317.TW", "2454.TW"]
    })";

    auto config_result = parser.parse<Config>(json);
    if (!config_result) {
        std::cerr << "Parse error\n";
        return;
    }

    Config config = std::move(config_result.value());

    std::cout << "API Key: " << config.api_key << '\n'
              << "Risk Limit: " << config.risk_limit << '\n'
              << "Symbols: ";
    for (const auto& sym : config.symbols) {
        std::cout << sym << ' ';
    }
    std::cout << '\n';
}
```

### Example 2: In-Place Parsing

```cpp
#include <format/json.hpp>
#include <iostream>

struct Trade {
    int64_t trade_id;
    std::string symbol;
    double price;
    int32_t quantity;
};

template <>
struct glz::meta<Trade> {
    using T = Trade;
    static constexpr auto value = glz::object(
        "trade_id", &T::trade_id,
        "symbol", &T::symbol,
        "price", &T::price,
        "quantity", &T::quantity
    );
};

void inplace_parsing() {
    format::json::glaze_parser parser;

    std::string_view json = R"({
        "trade_id": 12345,
        "symbol": "2330.TW",
        "price": 580.0,
        "quantity": 1000
    })";

    // Parse into existing object (avoids allocation for Trade itself)
    Trade trade;
    auto result = parser.parse_into(trade, json);

    if (!result) {
        std::cerr << "Parse error\n";
        return;
    }

    std::cout << "Trade " << trade.trade_id << ": "
              << trade.quantity << " shares of " << trade.symbol
              << " at " << trade.price << '\n';
}
```

### Example 3: Writing JSON

```cpp
#include <format/json.hpp>
#include <iostream>

struct Order {
    int64_t order_id;
    std::string symbol;
    std::string side;  // "buy" or "sell"
    double price;
    int32_t quantity;
};

template <>
struct glz::meta<Order> {
    using T = Order;
    static constexpr auto value = glz::object(
        "order_id", &T::order_id,
        "symbol", &T::symbol,
        "side", &T::side,
        "price", &T::price,
        "quantity", &T::quantity
    );
};

void writing_json() {
    format::json::glaze_parser parser;

    Order order {
        .order_id = 67890,
        .symbol = "2330.TW",
        .side = "buy",
        .price = 580.0,
        .quantity = 500
    };

    // Compact JSON
    auto json_result = parser.write(order);
    if (json_result) {
        std::cout << "Compact: " << json_result.value() << '\n';
    }

    // Pretty-printed JSON
    auto pretty_result = parser.write_pretty(order);
    if (pretty_result) {
        std::cout << "Pretty:\n" << pretty_result.value() << '\n';
    }
}
```

### Example 4: Dynamic JSON

```cpp
#include <format/json.hpp>
#include <iostream>

void dynamic_json() {
    format::json::glaze_dynamic_parser parser;

    std::string_view json = R"({
        "status": "ok",
        "data": {
            "count": 42,
            "items": [1, 2, 3]
        }
    })";

    auto value_result = parser.parse(json);
    if (!value_result) {
        std::cerr << "Parse error\n";
        return;
    }

    glz::json_t value = std::move(value_result.value());

    // Access using glaze's json_t API
    if (value.contains("status")) {
        // Note: Exact API depends on glaze's json_t implementation
        std::cout << "Status field exists\n";
    }

    // Write back to JSON
    auto write_result = parser.write(value);
    if (write_result) {
        std::cout << "JSON: " << write_result.value() << '\n';
    }
}
```

### Example 5: Validation

```cpp
#include <format/json.hpp>
#include <iostream>

void validation() {
    format::json::glaze_parser parser;

    std::string_view valid_json = R"({"key": "value"})";
    std::string_view invalid_json = R"({"key": "value",})";

    auto result1 = parser.validate(valid_json);
    if (result1) {
        std::cout << "Valid JSON\n";
    }

    auto result2 = parser.validate(invalid_json);
    if (!result2) {
        std::cout << "Invalid JSON: "
                  << format::json::error_string(result2.error()) << '\n';
    }
}
```

## Combining Both Libraries

### Example: Use Right Tool for Right Job

```cpp
#include <format/json.hpp>
#include <iostream>

// Configuration structure (use Glaze)
struct TradingConfig {
    std::string api_endpoint;
    double max_position_size;
    std::vector<std::string> allowed_symbols;
};

template <>
struct glz::meta<TradingConfig> {
    using T = TradingConfig;
    static constexpr auto value = glz::object(
        "api_endpoint", &T::api_endpoint,
        "max_position_size", &T::max_position_size,
        "allowed_symbols", &T::allowed_symbols
    );
};

class TradingSystem {
public:
    TradingSystem() = default;

    // Load configuration with Glaze (structured data)
    bool load_config(std::string_view config_json) {
        auto result = config_parser_.parse<TradingConfig>(config_json);
        if (!result) {
            std::cerr << "Failed to parse config\n";
            return false;
        }
        config_ = std::move(result.value());
        return true;
    }

    // Process market tick with simdjson (high-frequency streaming)
    void process_tick(std::string_view tick_json) {
        auto doc_result = tick_parser_.parse(tick_json);
        if (!doc_result) {
            return;  // Skip bad tick
        }

        auto doc = std::move(doc_result.value());

        // Zero-copy, SIMD-optimized access
        auto symbol = doc["symbol"].get_string();
        double bid = doc["bid"].get_double();
        double ask = doc["ask"].get_double();
        int64_t volume = doc["volume"].get_int64();

        update_market_data(symbol, bid, ask, volume);
    }

private:
    format::json::glaze_parser config_parser_;
    format::json::simdjson_parser tick_parser_;
    TradingConfig config_;

    void update_market_data(std::string_view symbol, double bid, double ask, int64_t volume) {
        // Update order book...
    }
};

int main() {
    TradingSystem system;

    std::string_view config = R"({
        "api_endpoint": "wss://example.com",
        "max_position_size": 100000.0,
        "allowed_symbols": ["2330.TW", "2317.TW"]
    })";

    if (!system.load_config(config)) {
        return 1;
    }

    // Simulate market ticks
    std::string_view tick = R"({
        "symbol": "2330.TW",
        "bid": 579.5,
        "ask": 580.0,
        "volume": 1234
    })";

    system.process_tick(tick);

    return 0;
}
```

## Performance Tips

### simdjson
1. **Reuse parser instances** - They contain internal buffers
2. **Use `parse_many()` for streams** - More efficient than parsing individually
3. **Access fields in document order** - Forward-only cursor is fastest
4. **Avoid repeated `get_*()` calls** - Cache field values

### Glaze
1. **Use `parse_into()` when possible** - Avoids struct allocation
2. **Define structs at compile time** - Enables reflection optimization
3. **Use stack allocation for small structs** - Faster than heap
4. **Reserve vector capacity** - For array fields

## Native Access

Both adapters provide access to native parser functionality:

```cpp
// simdjson
format::json::simdjson_parser parser;
auto& native_simdjson = parser.native();
// Use simdjson::ondemand::parser directly

// Glaze - already exposes native API through methods
format::json::glaze_parser parser;
auto result = parser.parse<T>(json);  // Native Glaze behavior
```

## Integration with noemix-trader

```cpp
#include <format/json.hpp>

class MarketDataAdapter {
public:
    void on_quote(std::string_view json) {
        auto doc = quote_parser_.parse(json);
        if (!doc) return;

        // Process quote...
    }

    void load_symbols(std::string_view json) {
        auto symbols = config_parser_.parse<SymbolList>(json);
        if (symbols) {
            symbols_ = std::move(symbols.value());
        }
    }

private:
    format::json::simdjson_parser quote_parser_;  // High-frequency
    format::json::glaze_parser config_parser_;     // Configuration
    SymbolList symbols_;
};
```

## See Also

- [FORMAT_LIBRARY.md](FORMAT_LIBRARY.md) - Design documentation
- [FUTURE_ASYNCLE_JSON.md](FUTURE_ASYNCLE_JSON.md) - Future async API design
- [JSON_PARSER_ANALYSIS.md](JSON_PARSER_ANALYSIS.md) - Parser comparison
