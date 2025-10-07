// Test format::json foundation layer
// Note: This test requires simdjson and/or Glaze to be installed
// Enable with: -DFORMAT_ENABLE_SIMDJSON=ON -DFORMAT_ENABLE_GLAZE=ON

#include <cassert>
#include <format/json.hpp>
#include <iostream>
#include <string_view>

// Test helpers
template <typename T>
void assert_success(const format::json::result<T>& result, const char* context) {
    if(!result) {
        std::cerr << "FAIL: " << context << " - " << format::json::error_string(result.error()) << '\n';
        std::exit(1);
    }
    std::cout << "PASS: " << context << '\n';
}

void assert_failure(const format::json::void_result& result, const char* context) {
    if(result) {
        std::cerr << "FAIL: " << context << " - expected error but succeeded\n";
        std::exit(1);
    }
    std::cout << "PASS: " << context << '\n';
}

#ifdef FORMAT_HAS_SIMDJSON

void test_simdjson_basic_parsing() {
    format::json::simdjson_parser parser;

    std::string_view json = R"({
        "symbol": "2330.TW",
        "price": 580.0,
        "volume": 1000
    })";

    auto doc_result = parser.parse(json);
    assert_success(doc_result, "simdjson basic parsing");

    auto doc = std::move(doc_result.value());

    // Verify values
    auto symbol = doc["symbol"].get_string();
    assert(symbol.value() == "2330.TW" && "symbol value check");

    double price = doc["price"].get_double();
    assert(price == 580.0 && "price value check");

    int64_t volume = doc["volume"].get_int64();
    assert(volume == 1000 && "volume value check");

    std::cout << "PASS: simdjson value extraction\n";
}

void test_simdjson_invalid_json() {
    format::json::simdjson_parser parser;

    std::string_view invalid = R"({"key": "value",})";  // Trailing comma

    auto result = parser.parse(invalid);
    assert_failure(result.transform([](auto) { return; }), "simdjson invalid JSON detection");
}

void test_simdjson_nested() {
    format::json::simdjson_parser parser;

    std::string_view json = R"({
        "order": {
            "id": 12345,
            "details": {
                "symbol": "2330.TW",
                "quantity": 500
            }
        }
    })";

    auto doc_result = parser.parse(json);
    assert_success(doc_result, "simdjson nested parsing");

    auto doc = std::move(doc_result.value());

    auto    order = doc["order"];
    int64_t id    = order["id"];
    assert(id == 12345 && "nested id check");

    auto details = order["details"];
    auto symbol  = details["symbol"].get_string();
    assert(symbol.value() == "2330.TW" && "nested symbol check");

    std::cout << "PASS: simdjson nested object access\n";
}

void test_simdjson_stream() {
    format::json::simdjson_stream_parser stream_parser;

    std::string_view ndjson = R"(
{"trade_id": 1, "price": 100.0}
{"trade_id": 2, "price": 101.5}
{"trade_id": 3, "price": 102.0}
)";

    auto stream_result = stream_parser.parse_many(ndjson);
    assert_success(stream_result, "simdjson stream parsing");

    auto stream = std::move(stream_result.value());

    int count = 0;
    for(auto doc: stream) {
        if(doc.error()) continue;

        int64_t trade_id = doc["trade_id"];
        double  price    = doc["price"];

        assert(trade_id == count + 1 && "stream trade_id check");
        count++;
    }

    assert(count == 3 && "stream document count");
    std::cout << "PASS: simdjson stream processing (" << count << " documents)\n";
}

void test_simdjson_capabilities() {
    constexpr auto caps = format::json::simdjson_parser::caps();

    static_assert(caps.zero_copy, "simdjson should be zero-copy");
    static_assert(caps.lazy_parsing, "simdjson should support lazy parsing");
    static_assert(caps.streaming, "simdjson should support streaming");
    static_assert(caps.simd_optimized, "simdjson should be SIMD optimized");
    static_assert(!caps.multiple_cursors, "simdjson doesn't support multiple cursors");

    std::cout << "PASS: simdjson capabilities check\n";
}

void run_simdjson_tests() {
    std::cout << "\n=== simdjson Tests ===\n";
    test_simdjson_basic_parsing();
    test_simdjson_invalid_json();
    test_simdjson_nested();
    test_simdjson_stream();
    test_simdjson_capabilities();
}

#endif  // FORMAT_HAS_SIMDJSON

#ifdef FORMAT_HAS_GLAZE

// Test structure
struct TestConfig {
    std::string              api_key;
    double                   risk_limit;
    std::vector<std::string> symbols;
};

// Glaze metadata
template <>
struct glz::meta<TestConfig> {
    using T = TestConfig;
    static constexpr auto value =
      glz::object("api_key", &T::api_key, "risk_limit", &T::risk_limit, "symbols", &T::symbols);
};

void test_glaze_structured_parsing() {
    format::json::glaze_parser parser;

    std::string_view json = R"({
        "api_key": "test_key",
        "risk_limit": 10000.0,
        "symbols": ["2330.TW", "2317.TW"]
    })";

    auto config_result = parser.parse<TestConfig>(json);
    assert_success(config_result, "Glaze structured parsing");

    auto config = std::move(config_result.value());

    assert(config.api_key == "test_key" && "api_key check");
    assert(config.risk_limit == 10000.0 && "risk_limit check");
    assert(config.symbols.size() == 2 && "symbols size check");
    assert(config.symbols[0] == "2330.TW" && "symbol[0] check");

    std::cout << "PASS: Glaze value extraction\n";
}

void test_glaze_inplace_parsing() {
    format::json::glaze_parser parser;

    std::string_view json = R"({
        "api_key": "inplace_key",
        "risk_limit": 5000.0,
        "symbols": ["2454.TW"]
    })";

    TestConfig config;
    auto       result = parser.parse_into(config, json);
    assert_success(result, "Glaze in-place parsing");

    assert(config.api_key == "inplace_key" && "in-place api_key check");
    assert(config.risk_limit == 5000.0 && "in-place risk_limit check");

    std::cout << "PASS: Glaze in-place parsing\n";
}

void test_glaze_write() {
    format::json::glaze_parser parser;

    TestConfig config {
        .api_key    = "write_test",
        .risk_limit = 15000.0,
        .symbols    = { "2330.TW", "2317.TW", "2454.TW" }
    };

    auto json_result = parser.write(config);
    assert_success(json_result, "Glaze JSON writing");

    auto json = std::move(json_result.value());
    assert(!json.empty() && "written JSON not empty");

    std::cout << "PASS: Glaze JSON writing\n";
    std::cout << "  Generated: " << json << '\n';
}

void test_glaze_validation() {
    format::json::glaze_parser parser;

    std::string_view valid_json = R"({"key": "value"})";
    auto             result1    = parser.validate(valid_json);
    assert_success(result1, "Glaze validation of valid JSON");

    std::string_view invalid_json = R"({"key": "value",})";
    auto             result2      = parser.validate(invalid_json);
    assert_failure(result2, "Glaze validation of invalid JSON");
}

void test_glaze_capabilities() {
    constexpr auto caps = format::json::glaze_parser::caps();

    static_assert(caps.zero_copy, "Glaze should be zero-copy");
    static_assert(caps.structured_binding, "Glaze should support structured binding");
    static_assert(caps.compile_time_reflection, "Glaze should use compile-time reflection");
    static_assert(caps.can_use_stack, "Glaze should support stack allocation");
    static_assert(!caps.streaming, "Glaze doesn't support streaming");

    std::cout << "PASS: Glaze capabilities check\n";
}

void run_glaze_tests() {
    std::cout << "\n=== Glaze Tests ===\n";
    test_glaze_structured_parsing();
    test_glaze_inplace_parsing();
    test_glaze_write();
    test_glaze_validation();
    test_glaze_capabilities();
}

#endif  // FORMAT_HAS_GLAZE

int main() {
    std::cout << "format::json Test Suite\n";
    std::cout << "========================\n";

#ifdef FORMAT_HAS_SIMDJSON
    run_simdjson_tests();
#else
    std::cout << "\nSimdjson tests SKIPPED (not enabled)\n";
    std::cout << "Enable with: cmake -DFORMAT_ENABLE_SIMDJSON=ON\n";
#endif

#ifdef FORMAT_HAS_GLAZE
    run_glaze_tests();
#else
    std::cout << "\nGlaze tests SKIPPED (not enabled)\n";
    std::cout << "Enable with: cmake -DFORMAT_ENABLE_GLAZE=ON\n";
#endif

#if !defined(FORMAT_HAS_SIMDJSON) && !defined(FORMAT_HAS_GLAZE)
    std::cout << "\nNo JSON parsers enabled. At least one parser is required.\n";
    return 1;
#endif

    std::cout << "\n=========================\n";
    std::cout << "All tests PASSED\n";
    return 0;
}
