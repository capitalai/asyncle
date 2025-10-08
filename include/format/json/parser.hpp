#ifndef FORMAT_JSON_PARSER_HPP
#define FORMAT_JSON_PARSER_HPP

// format::json::parser - Unified parser interface
//
// This provides a single, implementation-independent parser type that
// asyncle can use without knowing about simdjson, Glaze, etc.
//
// Design:
// - Single parser type: format::json::parser
// - Implementation selected at compile time via CMake flags
// - If no implementation available, provides error stub
// - asyncle only sees format::json::parser (never simdjson_document, etc.)

#include "types.hpp"
#include <string_view>
#include <utility>

// Include selected implementation
#ifdef FORMAT_HAS_SIMDJSON
#include "simdjson.hpp"
#endif

// Note: For serialization (not parsing), use format::serialize with Glaze.
// JSON parsing uses simdjson for optimal performance (zero-copy, SIMD).

namespace format::json {

// Unified parser type - implementation selected at compile time
// This is THE parser that asyncle uses
#if defined(FORMAT_HAS_SIMDJSON)
// Use simdjson as default parser
using parser                                     = simdjson_document;
inline constexpr parser_caps parser_capabilities = simdjson_caps;

#else
// No parser available - provide stub that produces errors
class parser {
    public:
    using native_document_type = void;
    using native_parser_type   = void;

    explicit parser(std::string_view) {}

    parser(parser&&) noexcept            = default;
    parser& operator=(parser&&) noexcept = default;
    parser(const parser&)                = delete;
    parser& operator=(const parser&)     = delete;

    static constexpr parser_caps caps() noexcept { return parser_caps {}; }

    result<void> iterate() noexcept { return result<void>(unexpect, error::uninitialized); }
};

inline constexpr parser_caps parser_capabilities = parser_caps {};
#endif

// Convenience: Check if a parser implementation is available
inline constexpr bool has_parser_impl =
#if defined(FORMAT_HAS_SIMDJSON)
  true;
#else
  false;
#endif

// Parse function - unified interface for asyncle
// This is what asyncle calls, without knowing the implementation
inline auto parse(std::string_view json_data) { return parser { json_data }; }

}  // namespace format::json

#endif  // FORMAT_JSON_PARSER_HPP
