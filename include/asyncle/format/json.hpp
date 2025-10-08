#ifndef ASYNCLE_FORMAT_JSON_REDESIGN_HPP
#define ASYNCLE_FORMAT_JSON_REDESIGN_HPP

// asyncle::format::json - Implementation-agnostic JSON operations
//
// This layer has ZERO knowledge of:
// - simdjson
// - Glaze
// - Any other specific JSON library
//
// It only depends on format::json::parser which is a type alias
// configured by the format library at compile time.
//
// Design principle:
//   asyncle::format::json ──> format::json::parser (type alias)
//                                      ↓
//                        ┌─────────────┴──────────────┐
//                        │                            │
//                  simdjson_document            glaze_document
//                  (selected by format)         (selected by format)
//
// Usage:
//   auto parser = asyncle::format::json::parser()
//       .source(json_str)
//       .make();
//
//   auto doc = parser.parse();  // Returns format::json::parser (whatever it is)

#include "../base/cpo.hpp"
#include "../concepts/operation_concepts.hpp"
#include <format/json/parser.hpp>  // Unified parser interface
#include <string_view>

namespace asyncle::format::json {

// Re-export foundation types (all abstract)
using ::format::json::error;
using ::format::json::error_string;
using ::format::json::parser;  // The unified parser type (implementation-agnostic)
using ::format::json::parser_caps;
using ::format::json::source_lifetime;

template <typename T>
using result = ::format::json::result<T>;

// Parser operation - wraps format::json::parser
// No knowledge of implementation, just uses the unified parser type
class parser_operation {
    public:
    explicit parser_operation(std::string_view data, source_lifetime lifetime = source_lifetime::stable):
        data_(data),
        lifetime_(lifetime) {}

    // Access configuration
    std::string_view data() const noexcept { return data_; }

    source_lifetime lifetime() const noexcept { return lifetime_; }

    // Get capabilities of the underlying parser (whatever it is)
    static constexpr parser_caps capabilities() noexcept { return ::format::json::parser_capabilities; }

    // Parse operation - returns format::json::parser instance
    // Implementation is selected by format library, not asyncle
    parser parse() const { return parser { data_ }; }

    // Alternative: could return result if we want error handling here
    // result<parser> try_parse() const { ... }

    // Future: async parse
    // auto parse_async() -> async_operation<parser>;

    private:
    std::string_view data_;
    source_lifetime  lifetime_;
};

// Builder for parser operation
class parser_builder {
    public:
    parser_builder() = default;

    parser_builder& source(std::string_view data) noexcept {
        data_ = data;
        return *this;
    }

    parser_builder& lifetime(source_lifetime hint) noexcept {
        lifetime_ = hint;
        return *this;
    }

    parser_operation make() && { return parser_operation { data_, lifetime_ }; }

    private:
    std::string_view data_;
    source_lifetime  lifetime_ = source_lifetime::stable;
};

// Factory function - NO template parameters, NO implementation knowledge!
inline parser_builder make_parser() noexcept { return parser_builder {}; }

// Query capabilities - forwarded from format layer
inline constexpr parser_caps capabilities() noexcept { return ::format::json::parser_capabilities; }

// Check if parser implementation is available
inline constexpr bool has_parser() noexcept { return ::format::json::has_parser_impl; }

// Convenience: Direct parse (bypasses builder)
inline parser parse(std::string_view data) { return ::format::json::parse(data); }

}  // namespace asyncle::format::json

#endif  // ASYNCLE_FORMAT_JSON_REDESIGN_HPP
