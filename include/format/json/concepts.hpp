#ifndef FORMAT_JSON_CONCEPTS_HPP
#define FORMAT_JSON_CONCEPTS_HPP

#include "types.hpp"
#include <concepts>
#include <string_view>
#include <type_traits>

namespace format::json {

// Concept: A JSON parser that can parse string data into a document
// Requirements:
// - Must have a nested document_type
// - Must provide caps() returning parser_caps
// - Must be constructible from string_view
// - Must provide iterate() method returning result<document_view>
template <typename P>
concept json_parser = requires {
    typename P::document_type;
    { P::caps() } -> std::same_as<parser_caps>;
} && requires(P parser) {
    { parser.iterate() };  // Returns parser-specific document type
};

// Concept: A JSON document view (result of parsing)
// This is what you get after calling parser.iterate()
// Different parsers return different document types (simdjson::ondemand::document, etc.)
template <typename D>
concept json_document = requires(D doc) {
    // Document must be usable in some way - exact API depends on implementation
    // We don't enforce specific methods here to allow different parser styles
    requires std::movable<D>;
};

// Concept: A JSON value that can be accessed
// This represents a node in the JSON tree (object, array, number, string, etc.)
template <typename V>
concept json_value = requires(V val) {
    requires std::movable<V>;
    // Actual access methods (get_string, get_int64, etc.) are implementation-specific
};

// Type trait: Extract document type from a parser
template <json_parser P>
using parser_document_t = typename P::document_type;

// Type trait: Check if a parser supports zero-copy
template <json_parser P>
constexpr bool is_zero_copy_parser_v = P::caps().zero_copy;

// Type trait: Check if a parser supports lazy parsing
template <json_parser P>
constexpr bool is_lazy_parser_v = P::caps().lazy_parsing;

// Type trait: Check if a parser supports SIMD
template <json_parser P>
constexpr bool is_simd_parser_v = P::caps().simd_optimized;

}  // namespace format::json

#endif  // FORMAT_JSON_CONCEPTS_HPP
