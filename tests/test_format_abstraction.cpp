// Test: Format layer abstraction and customization
//
// This test verifies:
// 1. Concepts work correctly
// 2. Default implementations (simdjson, Glaze) work
// 3. Custom implementations can be plugged in
// 4. CPOs dispatch correctly based on tags

#include <cassert>
#include <format/json.hpp>
#include <format/serialize.hpp>
#include <iostream>

// Test structure
struct TestData {
    int         id;
    std::string name;
    double      value;
};

// Custom minimal parser for testing concept satisfaction
namespace custom {

struct minimal_document {
    std::string data;
};

struct minimal_parser {
    using document_type = minimal_document;

    static constexpr format::json::parser_caps caps() noexcept {
        return {
            .zero_copy               = false,
            .lazy_parsing            = false,
            .lightweight_index       = false,
            .full_dom                = true,
            .streaming               = false,
            .random_access           = true,
            .multiple_cursors        = true,
            .simd_optimized          = false,
            .swar_optimized          = false,
            .compile_time_reflection = false,
            .typical_overhead_pct    = 200,
            .can_use_stack           = false,
            .requires_mutable        = false,
            .structured_binding      = false,
            .validates_utf8          = false,
            .validates_json          = false
        };
    }

    explicit minimal_parser(std::string_view json): json_(json) {}

    format::json::result<document_type> iterate() {
        return format::json::result<document_type>(minimal_document { std::string(json_) });
    }

    private:
    std::string_view json_;
};

}  // namespace custom

// Test: Custom parser satisfies concept
static_assert(format::json::json_parser<custom::minimal_parser>, "Custom parser should satisfy json_parser concept");

// Test: Capability queries work
static_assert(!format::json::is_zero_copy_parser_v<custom::minimal_parser>, "Custom parser is not zero-copy");

#ifdef FORMAT_HAS_SIMDJSON
static_assert(
  format::json::json_parser<format::json::simdjson_document>,
  "simdjson_document should satisfy json_parser concept");

static_assert(format::json::is_zero_copy_parser_v<format::json::simdjson_document>, "simdjson should be zero-copy");

static_assert(format::json::is_simd_parser_v<format::json::simdjson_document>, "simdjson should be SIMD-optimized");
#endif

// Test: Format tags
static_assert(format::serialize::format_tag<format::serialize::json_tag>);
static_assert(format::serialize::format_tag<format::serialize::beve_tag>);
static_assert(format::serialize::text_format<format::serialize::json_tag>);
static_assert(format::serialize::binary_format<format::serialize::beve_tag>);
static_assert(!format::serialize::text_format<format::serialize::beve_tag>);

// Custom serializer implementation
namespace custom {

struct custom_format_tag {};

// ADL-findable implementations in the tag's namespace
template <typename T>
auto save_impl(T const& obj [[maybe_unused]], custom_format_tag) -> format::serialize::result<std::string> {
    // Dummy implementation for testing
    return format::serialize::result<std::string>(std::string("custom_serialized"));
}

template <typename T>
auto load_impl(std::string_view data [[maybe_unused]], custom_format_tag) -> format::serialize::result<T> {
    // Dummy implementation for testing
    return format::serialize::result<T>(T {});
}

}  // namespace custom

// Extend format::serialize with custom format
namespace format::serialize {

// Mark as text format by specializing is_text_format
template <>
struct is_text_format<custom::custom_format_tag>: std::true_type {};

}  // namespace format::serialize

void test_custom_parser() {
    std::cout << "Testing custom parser...\n";

    custom::minimal_parser parser(R"({"test": 123})");
    auto                   doc = parser.iterate();

    assert(doc.has_value());
    assert(doc->data == R"({"test": 123})");

    std::cout << "  ✓ Custom parser works\n";
}

#ifdef FORMAT_HAS_SIMDJSON
void test_simdjson_parser() {
    std::cout << "Testing simdjson parser...\n";

    const char*                     json = R"({"name": "Alice", "age": 30})";
    format::json::simdjson_document doc(json);

    auto result = doc.iterate();
    assert(!result.error());

    auto name = result["name"].get_string();
    assert(!name.error());
    assert(name.value() == "Alice");

    auto age = result["age"].get_int64();
    assert(!age.error());
    assert(age.value() == 30);

    std::cout << "  ✓ simdjson parser works\n";
    std::cout << "  ✓ Zero-copy: " << (doc.caps().zero_copy ? "yes" : "no") << "\n";
    std::cout << "  ✓ SIMD: " << (doc.caps().simd_optimized ? "yes" : "no") << "\n";
}
#endif

#ifdef FORMAT_HAS_GLAZE
void test_glaze_serializer() {
    std::cout << "Testing Glaze serializer...\n";

    // Note: Glaze requires reflection metadata for structs
    // For this test, use a simple type
    struct Simple {
        int         x;
        std::string s;
    };

    Simple data { 42, "test" };

    // Serialize to JSON
    auto json = format::serialize::save(data, format::serialize::json_tag {});
    if(json) {
        std::cout << "  ✓ JSON serialization works: " << *json << "\n";
    } else {
        std::cout << "  ✗ JSON serialization failed\n";
    }

    // Serialize to BEVE
    auto beve = format::serialize::save(data, format::serialize::beve_tag {});
    if(beve) {
        std::cout << "  ✓ BEVE serialization works (size: " << beve->size() << " bytes)\n";
    } else {
        std::cout << "  ✗ BEVE serialization failed\n";
    }
}
#endif

void test_custom_serializer() {
    std::cout << "Testing custom serializer...\n";

    TestData data { 1, "test", 3.14 };

    auto result = format::serialize::save(data, custom::custom_format_tag {});
    assert(result.has_value());
    assert(*result == "custom_serialized");

    auto loaded = format::serialize::load<TestData>(*result, custom::custom_format_tag {});
    assert(loaded.has_value());

    std::cout << "  ✓ Custom serializer works\n";
}

int main() {
    std::cout << "=== Format Layer Abstraction Tests ===\n\n";

    test_custom_parser();

#ifdef FORMAT_HAS_SIMDJSON
    test_simdjson_parser();
#else
    std::cout << "Skipping simdjson tests (not enabled)\n";
#endif

#ifdef FORMAT_HAS_GLAZE
    test_glaze_serializer();
#else
    std::cout << "Skipping Glaze tests (not enabled)\n";
#endif

    test_custom_serializer();

    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
