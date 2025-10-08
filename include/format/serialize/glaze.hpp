#ifndef FORMAT_SERIALIZE_GLAZE_HPP
#define FORMAT_SERIALIZE_GLAZE_HPP

#include "concepts.hpp"
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifdef FORMAT_HAS_GLAZE
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <glaze/glaze.hpp>
#pragma GCC diagnostic pop

namespace format::serialize {

// Glaze capabilities for JSON
inline constexpr serializer_caps glaze_json_caps {
    .text_format         = true,
    .binary_format       = false,
    .self_describing     = true,
    .supports_reflection = true,
    .supports_schema     = true,
    .streaming_output    = false,
    .zero_allocation     = false,
    .compile_time        = false,
    .format_name         = "JSON",
    .mime_type           = "application/json"
};

// Glaze capabilities for BEVE
inline constexpr serializer_caps glaze_beve_caps {
    .text_format         = false,
    .binary_format       = true,
    .self_describing     = true,
    .supports_reflection = true,
    .supports_schema     = true,
    .streaming_output    = false,
    .zero_allocation     = false,
    .compile_time        = false,
    .format_name         = "BEVE",
    .mime_type           = "application/octet-stream"
};

// Convert Glaze error to format::serialize::error
inline error convert_glaze_error(glz::error_ctx const& ctx) noexcept {
    if(!ctx) return error::none;

    // Map common Glaze errors
    // Note: Glaze uses error_ctx which contains detailed error information
    // For now, we map to generic errors
    return error::invalid_syntax;  // Most Glaze errors are syntax/type related
}

// Implementation for save with Glaze + JSON
template <typename T>
auto save_impl(T const& obj, json_tag) -> result<std::string> {
    std::string buffer;
    auto        ec = glz::write_json(obj, buffer);

    if(ec) { return result<std::string>(unexpect, convert_glaze_error(ec)); }

    return result<std::string>(std::move(buffer));
}

// Implementation for load with Glaze + JSON
template <typename T>
auto load_impl(std::string_view data, json_tag) -> result<T> {
    T    obj {};
    auto ec = glz::read_json(obj, data);

    if(ec) { return result<T>(unexpect, convert_glaze_error(ec)); }

    return result<T>(std::move(obj));
}

// Implementation for save with Glaze + BEVE
template <typename T>
auto save_impl(T const& obj, beve_tag) -> result<std::vector<std::byte>> {
    std::vector<std::byte> buffer;
    auto                   ec = glz::write_binary(obj, buffer);

    if(ec) { return result<std::vector<std::byte>>(unexpect, convert_glaze_error(ec)); }

    return result<std::vector<std::byte>>(std::move(buffer));
}

// Implementation for load with Glaze + BEVE
template <typename T>
auto load_impl(std::span<std::byte const> data, beve_tag) -> result<T> {
    T    obj {};
    auto ec = glz::read_binary(obj, data);

    if(ec) { return result<T>(unexpect, convert_glaze_error(ec)); }

    return result<T>(std::move(obj));
}

// Glaze serializer type for querying capabilities
struct glaze_json_serializer {
    static constexpr serializer_caps caps() noexcept { return glaze_json_caps; }
};

struct glaze_beve_serializer {
    static constexpr serializer_caps caps() noexcept { return glaze_beve_caps; }
};

}  // namespace format::serialize

#endif  // FORMAT_HAS_GLAZE

#endif  // FORMAT_SERIALIZE_GLAZE_HPP
