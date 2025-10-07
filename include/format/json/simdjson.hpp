#ifndef FORMAT_JSON_SIMDJSON_HPP
#define FORMAT_JSON_SIMDJSON_HPP

#include "types.hpp"
#include <string_view>

#ifdef FORMAT_HAS_SIMDJSON
#include <simdjson.h>

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

// RAII wrapper for simdjson on-demand parser
// Following platform layer pattern: minimal abstraction, preserve native functionality
class simdjson_parser {
    public:
    using native_document_type = simdjson::ondemand::document;
    using native_parser_type   = simdjson::ondemand::parser;

    simdjson_parser() noexcept: parser_ {}, padded_string_ {} {}

    // Non-copyable, moveable (following platform pattern)
    simdjson_parser(const simdjson_parser&)                = delete;
    simdjson_parser& operator=(const simdjson_parser&)     = delete;
    simdjson_parser(simdjson_parser&&) noexcept            = default;
    simdjson_parser& operator=(simdjson_parser&&) noexcept = default;

    ~simdjson_parser() = default;

    // Get parser capabilities
    static constexpr parser_caps caps() noexcept { return simdjson_caps; }

    // Parse JSON from string_view
    // Note: simdjson requires padding, so this will create padded_string internally
    result<native_document_type> parse(std::string_view json) noexcept {
        auto padded_result = simdjson::padded_string::load(json);
        if(padded_result.error() != simdjson::SUCCESS) { return unexpected(convert_error(padded_result.error())); }

        padded_string_ = std::move(padded_result.value_unsafe());
        return parse_padded(padded_string_);
    }

    // Parse JSON from padded_string (truly zero-copy)
    result<native_document_type> parse_padded(simdjson::padded_string& padded) noexcept {
        auto doc_result = parser_.iterate(padded);
        if(doc_result.error() != simdjson::SUCCESS) { return unexpected(convert_error(doc_result.error())); }
        return doc_result.value_unsafe();
    }

    // Parse with explicit lifetime hint
    result<native_document_type> parse(std::string_view json, source_lifetime lifetime) noexcept {
        // For simdjson, we always need padding, so lifetime hint doesn't change behavior
        // But this interface allows future optimizations
        return parse(json);
    }

    // Access native simdjson parser for advanced usage
    native_parser_type& native() noexcept { return parser_; }

    const native_parser_type& native() const noexcept { return parser_; }

    // Get internal padded string (for inspection)
    const simdjson::padded_string& padded_data() const noexcept { return padded_string_; }

    private:
    native_parser_type      parser_;
    simdjson::padded_string padded_string_;
};

// Streaming parser for NDJSON (newline-delimited JSON)
class simdjson_stream_parser {
    public:
    using native_stream_type = simdjson::ondemand::document_stream;
    using native_parser_type = simdjson::ondemand::parser;

    simdjson_stream_parser() noexcept: parser_ {} {}

    simdjson_stream_parser(const simdjson_stream_parser&)                = delete;
    simdjson_stream_parser& operator=(const simdjson_stream_parser&)     = delete;
    simdjson_stream_parser(simdjson_stream_parser&&) noexcept            = default;
    simdjson_stream_parser& operator=(simdjson_stream_parser&&) noexcept = default;

    ~simdjson_stream_parser() = default;

    static constexpr parser_caps caps() noexcept { return simdjson_caps; }

    // Parse multiple JSON documents (e.g., NDJSON)
    result<native_stream_type>
      parse_many(std::string_view json, size_t batch_size = simdjson::dom::DEFAULT_BATCH_SIZE) noexcept {
        auto padded_result = simdjson::padded_string::load(json);
        if(padded_result.error() != simdjson::SUCCESS) { return unexpected(convert_error(padded_result.error())); }

        padded_string_     = std::move(padded_result.value_unsafe());
        auto stream_result = parser_.iterate_many(padded_string_, batch_size);

        if(stream_result.error() != simdjson::SUCCESS) { return unexpected(convert_error(stream_result.error())); }

        return stream_result.value_unsafe();
    }

    native_parser_type& native() noexcept { return parser_; }

    const native_parser_type& native() const noexcept { return parser_; }

    private:
    native_parser_type      parser_;
    simdjson::padded_string padded_string_;
};

}  // namespace format::json

#endif  // FORMAT_HAS_SIMDJSON

#endif  // FORMAT_JSON_SIMDJSON_HPP
