// Test format::json foundation layer
// Note: This test requires simdjson and/or Glaze to be installed
// Enable with: -DFORMAT_ENABLE_SIMDJSON=ON -DFORMAT_ENABLE_GLAZE=ON

#include <cassert>
#include <format/json.hpp>
#include <iostream>
#include <string_view>

#ifdef FORMAT_HAS_SIMDJSON

void test_simdjson_basic_parsing() {
    std::string_view json = R"({
        "symbol": "2330.TW",
        "price": 580.0,
        "volume": 1000
    })";

    // Create document holder
    format::json::simdjson_document doc(json);

    // Iterate to get document
    auto result = doc.iterate();
    if(result.error()) {
        std::cerr << "FAIL: simdjson basic parsing - " << simdjson::error_message(result.error()) << '\n';
        std::exit(1);
    }

    std::cout << "PASS: simdjson basic parsing\n";

    // Verify values
    auto symbol = result["symbol"].get_string();
    if(symbol.error() || symbol.value() != "2330.TW") {
        std::cerr << "FAIL: symbol value check\n";
        std::exit(1);
    }

    auto price_result = result["price"].get_double();
    if(price_result.error() || price_result.value() != 580.0) {
        std::cerr << "FAIL: price value check\n";
        std::exit(1);
    }

    auto volume_result = result["volume"].get_int64();
    if(volume_result.error() || volume_result.value() != 1000) {
        std::cerr << "FAIL: volume value check\n";
        std::exit(1);
    }

    std::cout << "PASS: simdjson value extraction\n";
}

void test_simdjson_invalid_json() {
    std::string_view invalid = R"({"key": "value",})";  // Trailing comma

    format::json::simdjson_document doc(invalid);
    auto                            result = doc.iterate();

    if(!result.error()) {
        std::cerr << "FAIL: simdjson invalid JSON detection - expected error but succeeded\n";
        std::exit(1);
    }

    std::cout << "PASS: simdjson invalid JSON detection\n";
}

void test_simdjson_nested() {
    std::string_view json = R"({
        "order": {
            "id": 12345,
            "details": {
                "symbol": "2330.TW",
                "quantity": 500
            }
        }
    })";

    format::json::simdjson_document doc(json);
    auto                            result = doc.iterate();

    if(result.error()) {
        std::cerr << "FAIL: simdjson nested parsing\n";
        std::exit(1);
    }

    std::cout << "PASS: simdjson nested parsing\n";

    // Access nested values
    auto order = result["order"].get_object();
    if(order.error()) {
        std::cerr << "FAIL: access order object\n";
        std::exit(1);
    }

    auto id = order["id"].get_int64();
    if(id.error() || id.value() != 12345) {
        std::cerr << "FAIL: order id check\n";
        std::exit(1);
    }

    std::cout << "PASS: simdjson nested value extraction\n";
}

void test_simdjson_capabilities() {
    auto caps = format::json::simdjson_document::caps();

    assert(caps.zero_copy && "simdjson should be zero-copy");
    assert(caps.lazy_parsing && "simdjson should support lazy parsing");
    assert(caps.simd_optimized && "simdjson should be SIMD optimized");
    assert(!caps.structured_binding && "simdjson doesn't support structured binding");

    std::cout << "PASS: simdjson capabilities check\n";
}

#endif  // FORMAT_HAS_SIMDJSON

#ifdef FORMAT_HAS_GLAZE

// Test struct for Glaze
struct TestConfig {
    std::string name;
    int         port;
    bool        enabled;
};

// Glaze reflection metadata
template <>
struct glz::meta<TestConfig> {
    using T                     = TestConfig;
    static constexpr auto value = object("name", &T::name, "port", &T::port, "enabled", &T::enabled);
};

void test_glaze_basic_parsing() {
    std::string_view json = R"({
        "name": "production",
        "port": 8080,
        "enabled": true
    })";

    format::json::glaze_parser parser;
    auto                       config_result = parser.parse<TestConfig>(json);

    if(!config_result) {
        std::cerr << "FAIL: glaze basic parsing\n";
        std::exit(1);
    }

    std::cout << "PASS: glaze basic parsing\n";

    auto& config = config_result.value();
    assert(config.name == "production" && "name check");
    assert(config.port == 8080 && "port check");
    assert(config.enabled == true && "enabled check");

    std::cout << "PASS: glaze value extraction\n";
}

void test_glaze_write() {
    TestConfig config { .name = "staging", .port = 9090, .enabled = false };

    format::json::glaze_parser parser;
    auto                       json_result = parser.write(config);

    if(!json_result) {
        std::cerr << "FAIL: glaze write\n";
        std::exit(1);
    }

    std::cout << "PASS: glaze write\n";
    std::cout << "JSON: " << json_result.value() << '\n';
}

void test_glaze_capabilities() {
    auto caps = format::json::glaze_parser::caps();

    assert(caps.structured_binding && "glaze should support structured binding");
    assert(caps.compile_time_reflection && "glaze should use compile-time reflection");
    assert(caps.can_use_stack && "glaze can use stack allocation");

    std::cout << "PASS: glaze capabilities check\n";
}

#endif  // FORMAT_HAS_GLAZE

int main() {
    std::cout << "=== Testing format::json foundation layer ===\n\n";

#ifdef FORMAT_HAS_SIMDJSON
    std::cout << "--- simdjson tests ---\n";
    test_simdjson_basic_parsing();
    test_simdjson_invalid_json();
    test_simdjson_nested();
    test_simdjson_capabilities();
    std::cout << "\n";
#else
    std::cout << "simdjson tests SKIPPED (not enabled)\n\n";
#endif

#ifdef FORMAT_HAS_GLAZE
    std::cout << "--- Glaze tests ---\n";
    test_glaze_basic_parsing();
    test_glaze_write();
    test_glaze_capabilities();
    std::cout << "\n";
#else
    std::cout << "Glaze tests SKIPPED (not enabled)\n\n";
#endif

#if !defined(FORMAT_HAS_SIMDJSON) && !defined(FORMAT_HAS_GLAZE)
    std::cerr << "ERROR: No parsers enabled. Enable with -DFORMAT_ENABLE_SIMDJSON=ON or -DFORMAT_ENABLE_GLAZE=ON\n";
    return 1;
#endif

    std::cout << "=== All tests passed! ===\n";
    return 0;
}
