#ifndef FORMAT_JSON_GLAZE_HPP
#define FORMAT_JSON_GLAZE_HPP

#include "types.hpp"
#include <string>
#include <string_view>
#include <type_traits>

#ifdef FORMAT_HAS_GLAZE
#include <glaze/glaze.hpp>

namespace format::json {

// Glaze capabilities
inline constexpr parser_caps glaze_caps {
    .zero_copy         = true,
    .lazy_parsing      = false,  // Parses to struct immediately
    .lightweight_index = true,
    .full_dom          = false,  // Creates struct, not generic DOM

    .streaming        = false,
    .random_access    = true,
    .multiple_cursors = true,  // Multiple struct members accessible

    .simd_optimized          = false,
    .swar_optimized          = true,
    .compile_time_reflection = true,

    .typical_overhead_pct = 0,  // Can use stack allocation for known types
    .can_use_stack        = true,
    .requires_mutable     = false,

    .structured_binding = true,
    .validates_utf8     = false,  // Assumes valid UTF-8
    .validates_json     = true
};

// Convert Glaze errors to format::json::error
inline error convert_error(glz::error_ctx err) noexcept {
    using glz::error_code;
    switch(err.ec) {
    case error_code::none                      : return error::none;
    case error_code::syntax_error              :
    case error_code::unexpected_end            :
    case error_code::expected_brace            :
    case error_code::expected_bracket          :
    case error_code::expected_quote            :
    case error_code::expected_comma            :
    case error_code::expected_colon            : return error::invalid_syntax;
    case error_code::unknown_key               :
    case error_code::missing_key               : return error::key_not_found;
    case error_code::exceeded_static_array_size:
    case error_code::array_element_not_found   : return error::index_out_of_bounds;
    case error_code::invalid_flag_input        :
    case error_code::invalid_nullable_read     : return error::type_mismatch;
    default                                    : return error::invalid_syntax;
    }
}

// Glaze adapter for structured JSON parsing
// Best for: configuration files, API responses, known structures at compile time
class glaze_parser {
    public:
    glaze_parser() noexcept = default;

    // Non-copyable, moveable
    glaze_parser(const glaze_parser&)                = delete;
    glaze_parser& operator=(const glaze_parser&)     = delete;
    glaze_parser(glaze_parser&&) noexcept            = default;
    glaze_parser& operator=(glaze_parser&&) noexcept = default;

    ~glaze_parser() = default;

    static constexpr parser_caps caps() noexcept { return glaze_caps; }

    // Parse JSON into a struct of type T
    template <typename T>
    result<T> parse(std::string_view json) noexcept {
        T    value {};
        auto err = glz::read_json(value, json);
        if(err) { return unexpected(convert_error(err)); }
        return value;
    }

    // Parse JSON into existing object (in-place, avoids allocation)
    template <typename T>
    void_result parse_into(T& value, std::string_view json) noexcept {
        auto err = glz::read_json(value, json);
        if(err) { return unexpected(convert_error(err)); }
        return {};
    }

    // Parse with explicit lifetime hint (for API consistency with simdjson)
    template <typename T>
    result<T> parse(std::string_view json, source_lifetime lifetime) noexcept {
        // Glaze always copies/parses into struct, so lifetime doesn't matter
        return parse<T>(json);
    }

    // Write struct to JSON string
    template <typename T>
    result<std::string> write(const T& value) noexcept {
        std::string result;
        auto        err = glz::write_json(value, result);
        if(err) { return unexpected(convert_error(err)); }
        return result;
    }

    // Write struct to JSON string (pretty-printed)
    template <typename T>
    result<std::string> write_pretty(const T& value, int indent = 2) noexcept {
        std::string result;
        auto        err = glz::write<glz::opts { .prettify = true }>(value, result);
        if(err) { return unexpected(convert_error(err)); }
        return result;
    }

    // Validate JSON syntax without parsing
    void_result validate(std::string_view json) noexcept {
        auto err = glz::validate_json(json);
        if(err) { return unexpected(convert_error(err)); }
        return {};
    }
};

// Dynamic JSON parser for cases where structure is unknown at compile time
// Uses Glaze's json_t type (type-erased value)
class glaze_dynamic_parser {
    public:
    using native_value_type = glz::json_t;

    glaze_dynamic_parser() noexcept = default;

    glaze_dynamic_parser(const glaze_dynamic_parser&)                = delete;
    glaze_dynamic_parser& operator=(const glaze_dynamic_parser&)     = delete;
    glaze_dynamic_parser(glaze_dynamic_parser&&) noexcept            = default;
    glaze_dynamic_parser& operator=(glaze_dynamic_parser&&) noexcept = default;

    ~glaze_dynamic_parser() = default;

    static constexpr parser_caps caps() noexcept { return glaze_caps; }

    // Parse to dynamic JSON value
    result<native_value_type> parse(std::string_view json) noexcept {
        native_value_type value {};
        auto              err = glz::read_json(value, json);
        if(err) { return unexpected(convert_error(err)); }
        return value;
    }

    // Parse into existing json_t
    void_result parse_into(native_value_type& value, std::string_view json) noexcept {
        auto err = glz::read_json(value, json);
        if(err) { return unexpected(convert_error(err)); }
        return {};
    }

    // Write json_t to string
    result<std::string> write(const native_value_type& value) noexcept {
        std::string result;
        auto        err = glz::write_json(value, result);
        if(err) { return unexpected(convert_error(err)); }
        return result;
    }

    void_result validate(std::string_view json) noexcept {
        auto err = glz::validate_json(json);
        if(err) { return unexpected(convert_error(err)); }
        return {};
    }
};

}  // namespace format::json

#endif  // FORMAT_HAS_GLAZE

#endif  // FORMAT_JSON_GLAZE_HPP
