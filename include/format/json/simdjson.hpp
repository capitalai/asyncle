#ifndef FORMAT_JSON_SIMDJSON_HPP
#define FORMAT_JSON_SIMDJSON_HPP

#include "types.hpp"
#include <string_view>

#ifdef FORMAT_HAS_SIMDJSON
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <simdjson.h>
#pragma GCC diagnostic pop

namespace format::json {

// simdjson capabilities
inline constexpr parser_caps simdjson_caps {
    .zero_copy         = true,
    .lazy_parsing      = true,
    .lightweight_index = true,
    .full_dom          = false,

    .streaming        = true,
    .random_access    = false,  // Single-cursor limitation
    .multiple_cursors = false,

    .simd_optimized          = true,
    .swar_optimized          = false,
    .compile_time_reflection = false,

    .typical_overhead_pct = 50,  // Tape index ~50-100% of JSON size
    .can_use_stack        = false,
    .requires_mutable     = false,

    .structured_binding = false,
    .validates_utf8     = true,
    .validates_json     = true
};

// Convert simdjson errors to format::json::error
inline error convert_error(simdjson::error_code err) noexcept {
    using simdjson::error_code;
    switch(err) {
    case error_code::SUCCESS            : return error::none;
    case error_code::TAPE_ERROR         :
    case error_code::STRING_ERROR       :
    case error_code::T_ATOM_ERROR       :
    case error_code::F_ATOM_ERROR       :
    case error_code::N_ATOM_ERROR       :
    case error_code::NUMBER_ERROR       : return error::invalid_syntax;
    case error_code::UTF8_ERROR         : return error::utf8_error;
    case error_code::NO_SUCH_FIELD      : return error::key_not_found;
    case error_code::INDEX_OUT_OF_BOUNDS: return error::index_out_of_bounds;
    case error_code::INCORRECT_TYPE     : return error::type_mismatch;
    case error_code::CAPACITY           : return error::capacity_exceeded;
    default                             : return error::invalid_syntax;
    }
}

// RAII document holder for simdjson on-demand parsing
// This class owns the memory (padded_string) required for zero-copy parsing
// Usage: Create instance with JSON, then iterate() to get document for consumption
//
// Important: simdjson documents are forward-only iterators that can only be consumed once.
// The document returned by iterate() is only valid while this object is alive.
class simdjson_document {
    public:
    using native_document_type = simdjson::ondemand::document;
    using native_parser_type   = simdjson::ondemand::parser;

    // Construct with JSON string - prepares padded memory
    explicit simdjson_document(std::string_view json): parser_(), padded_(json) {}

    // Non-copyable, moveable
    simdjson_document(const simdjson_document&)                = delete;
    simdjson_document& operator=(const simdjson_document&)     = delete;
    simdjson_document(simdjson_document&&) noexcept            = default;
    simdjson_document& operator=(simdjson_document&&) noexcept = default;

    ~simdjson_document() = default;

    // Get parser capabilities
    static constexpr parser_caps caps() noexcept { return simdjson_caps; }

    // Iterate to get document (can only be called once per instance)
    // Returns simdjson_result which contains document view
    // Note: The returned document is only valid while this object is alive
    //
    // Example:
    //   simdjson_document doc(json_string);
    //   auto result = doc.iterate();
    //   if(result.error()) { /* handle error */ }
    //   auto name = result["name"].get_string();
    simdjson::simdjson_result<native_document_type> iterate() noexcept { return parser_.iterate(padded_); }

    // Access native parser for advanced usage
    native_parser_type& parser() noexcept { return parser_; }

    const native_parser_type& parser() const noexcept { return parser_; }

    // Get internal padded string
    const simdjson::padded_string& padded_data() const noexcept { return padded_; }

    private:
    native_parser_type      parser_;
    simdjson::padded_string padded_;
};

}  // namespace format::json

#endif  // FORMAT_HAS_SIMDJSON

#endif  // FORMAT_JSON_SIMDJSON_HPP
