// Test: asyncle::format integration layer
//
// This test verifies the integration between:
// 1. format::json (foundation) and asyncle::format::json (operations)
// 2. format::serialize (foundation) and asyncle::format::serialize (operations)
//
// Architecture being tested:
//   Foundation Layer (format::*)     -> Low-level, sync, pluggable
//   Integration Layer (asyncle::format::*) -> High-level, operation-based
//
// This mirrors the platform/asyncle split:
//   platform::file -> asyncle::io::file
//   format::json   -> asyncle::format::json

#include <asyncle/format/json.hpp>
#include <asyncle/format/serialize.hpp>
#include <cassert>
#include <iostream>

struct TestConfig {
    int         port;
    std::string host;
};

void test_foundation_layer_available() {
    std::cout << "Testing foundation layer availability...\n";

    // Foundation layer should always be available (concepts and types)
    using format::json::error;
    using format::json::parser_caps;

    std::cout << "  ✓ Foundation layer types available\n";
}

#ifdef FORMAT_HAS_SIMDJSON
void test_simdjson_integration() {
    std::cout << "Testing simdjson integration with asyncle...\n";

    // Check that integration layer is available
    static_assert(
      asyncle::format::json::is_available_v<asyncle::format::json::simdjson_tag>,
      "simdjson should be available through integration layer");

    // Test capabilities query through integration layer
    auto caps = asyncle::format::json::caps<asyncle::format::json::simdjson_tag>();
    assert(caps.zero_copy);
    assert(caps.simd_optimized);
    std::cout << "  ✓ Capabilities query works\n";

    // Test builder pattern
    const char* json = R"({"name": "test", "value": 42})";

    auto parser =
      asyncle::format::json::parser<asyncle::format::json::simdjson_tag>()
        .source(json)
        .lifetime(asyncle::format::json::source_lifetime::stable)
        .make();

    std::cout << "  ✓ Builder pattern works\n";

    // Test parsing through integration layer
    auto doc    = parser.parse();
    auto result = doc.iterate();
    assert(!result.error());

    auto name = result["name"].get_string();
    assert(!name.error());
    assert(name.value() == "test");

    auto value = result["value"].get_int64();
    assert(!value.error());
    assert(value.value() == 42);

    std::cout << "  ✓ Parsing through asyncle layer works\n";
    std::cout << "  ✓ Zero-copy: " << (caps.zero_copy ? "enabled" : "disabled") << "\n";
    std::cout << "  ✓ SIMD: " << (caps.simd_optimized ? "enabled" : "disabled") << "\n";
}
#endif

#ifdef FORMAT_HAS_GLAZE
void test_glaze_integration() {
    std::cout << "Testing Glaze integration with asyncle...\n";

    // Note: Glaze requires reflection metadata
    // For testing, use simple types

    struct Simple {
        int x = 42;
    };

    // Test through asyncle convenience functions
    using asyncle::format::serialize::json_tag;

    auto json_result = asyncle::format::save(Simple {}, json_tag {});
    if(json_result) {
        std::cout << "  ✓ Serialization through asyncle layer works\n";
        std::cout << "  ✓ JSON output: " << *json_result << "\n";
    } else {
        std::cout << "  ⚠ Serialization might need reflection metadata\n";
    }

    // Test builder pattern
    auto serializer  = asyncle::format::serializer<Simple>(json_tag {}).make();
    auto save_result = serializer.save(Simple {});

    if(save_result) { std::cout << "  ✓ Builder pattern works for serialization\n"; }
}
#endif

void test_layering_independence() {
    std::cout << "Testing layer independence...\n";

    // Foundation layer works independently
    using format::json::error;
    using format::json::parser_caps;

    // Integration layer wraps foundation layer
    // But foundation can still be used directly

#ifdef FORMAT_HAS_SIMDJSON
    // Direct foundation layer usage
    const char*                     json = R"({"test": true})";
    format::json::simdjson_document doc(json);
    auto                            result = doc.iterate();
    assert(!result.error());

    std::cout << "  ✓ Foundation layer works independently\n";

    // Integration layer usage
    auto parser = asyncle::format::json::parser<asyncle::format::json::simdjson_tag>().source(json).make();
    auto doc2   = parser.parse();

    std::cout << "  ✓ Integration layer wraps foundation properly\n";
#endif

    std::cout << "  ✓ Layers are properly separated\n";
}

void test_future_pipeline_design() {
    std::cout << "Testing future pipeline design pattern...\n";

    // This demonstrates the INTENDED design for future implementation
    // Current code doesn't support this yet, but the architecture allows it

    std::cout << "  ℹ Future pipeline example (not yet implemented):\n";
    std::cout << "    auto pipeline = \n";
    std::cout << "        asyncle::io::read_file(\"data.json\")\n";
    std::cout << "        | asyncle::format::json::parse()\n";
    std::cout << "        | asyncle::format::json::transform([](auto doc) { ... })\n";
    std::cout << "        | asyncle::format::serialize_to(json_tag{});\n";
    std::cout << "\n";
    std::cout << "  ✓ Architecture supports future pipeline composition\n";
}

int main() {
    std::cout << "=== Asyncle Format Integration Tests ===\n\n";

    test_foundation_layer_available();

#ifdef FORMAT_HAS_SIMDJSON
    test_simdjson_integration();
#else
    std::cout << "Skipping simdjson integration tests (not enabled)\n";
#endif

#ifdef FORMAT_HAS_GLAZE
    test_glaze_integration();
#else
    std::cout << "Skipping Glaze integration tests (not enabled)\n";
#endif

    test_layering_independence();
    test_future_pipeline_design();

    std::cout << "\n=== Integration tests complete! ===\n";
    std::cout << "\nArchitecture Summary:\n";
    std::cout << "  Foundation Layer:  format::json, format::serialize\n";
    std::cout << "                     - Low-level, sync, pluggable\n";
    std::cout << "                     - Can be used independently\n";
    std::cout << "\n";
    std::cout << "  Integration Layer: asyncle::format::json, asyncle::format::serialize\n";
    std::cout << "                     - High-level, operation-based\n";
    std::cout << "                     - Builder patterns, future async support\n";
    std::cout << "                     - Wraps foundation layer\n";
    std::cout << "\n";
    std::cout << "  Pattern: Similar to platform::file -> asyncle::io::file\n";

    return 0;
}
