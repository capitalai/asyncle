#ifndef FORMAT_SERIALIZE_HPP
#define FORMAT_SERIALIZE_HPP

// format::serialize - Serialization/Deserialization CPO layer
//
// This layer provides Customization Point Objects (CPOs) for serializing
// and deserializing C++ objects to/from various formats.
//
// Design philosophy:
// 1. Format-agnostic interface through tag dispatch
// 2. Pluggable implementations via ADL or direct customization
// 3. Zero-overhead abstractions - compiles to direct calls
// 4. Support for multiple serialization libraries
//
// Supported formats (when corresponding libraries are available):
// - JSON (via Glaze)
// - BEVE (Binary Efficient Versatile Encoding, via Glaze)
// - CSV (via Glaze)
// - XML (future)
// - YAML (future)
//
// Usage:
//   struct Config { int port; std::string host; };
//
//   // Serialize to JSON
//   auto json = format::serialize::save(config, format::serialize::json_tag{});
//
//   // Deserialize from JSON
//   auto config = format::serialize::load<Config>(json_str, format::serialize::json_tag{});
//
//   // Serialize to binary format
//   auto bytes = format::serialize::save(config, format::serialize::beve_tag{});

#include "serialize/concepts.hpp"
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Include implementations based on feature flags
#ifdef FORMAT_HAS_GLAZE
#include "serialize/glaze.hpp"
#endif

namespace format::serialize {

// CPO: save - Serialize an object to specified format
//
// This is a Customization Point Object that allows users to:
// 1. Use built-in adapters (Glaze, etc.)
// 2. Provide custom implementations via ADL
// 3. Specialize for their own types
//
// Text formats return result<std::string>
// Binary formats return result<std::vector<std::byte>>

// save_impl and load_impl are customization points
// Users can provide overloads in format::serialize namespace
// or rely on ADL by implementing in their own namespace

// CPO implementation
namespace detail {
struct save_fn {
    // Text format overload
    template <typename T, typename Tag>
    requires text_format<Tag>
    auto operator()(T const& obj, Tag tag) const -> result<std::string> {
        // Unqualified call for ADL
        return save_impl(obj, tag);
    }

    // Binary format overload
    template <typename T, typename Tag>
    requires binary_format<Tag>
    auto operator()(T const& obj, Tag tag) const -> result<std::vector<std::byte>> {
        return save_impl(obj, tag);
    }
};

template <typename T>
struct load_fn {
    // Text format overload
    template <typename Tag>
    requires text_format<Tag>
    auto operator()(std::string_view data, Tag tag) const -> result<T> {
        return load_impl<T>(data, tag);
    }

    // Binary format overload
    template <typename Tag>
    requires binary_format<Tag>
    auto operator()(std::span<std::byte const> data, Tag tag) const -> result<T> {
        return load_impl<T>(data, tag);
    }
};
}  // namespace detail

// CPO objects
inline constexpr detail::save_fn save {};

// Load CPO - template variable pattern for proper template argument deduction
template <typename T>
inline constexpr detail::load_fn<T> load {};

// Convenience: Check if type is serializable to format at compile time
template <typename T, typename Tag>
constexpr bool is_serializable_v = requires(T const& obj, Tag tag) {
    { save(obj, tag) } -> std::same_as<result<std::string>>;
} || requires(T const& obj, Tag tag) {
    { save(obj, tag) } -> std::same_as<result<std::vector<std::byte>>>;
};

// Convenience: Check if type is deserializable from format at compile time
template <typename T, typename Tag>
constexpr bool is_deserializable_v = requires(std::string_view data, Tag tag) {
    { load<T>(data, tag) } -> std::same_as<result<T>>;
} || requires(std::span<std::byte const> data, Tag tag) {
    { load<T>(data, tag) } -> std::same_as<result<T>>;
};

}  // namespace format::serialize

#endif  // FORMAT_SERIALIZE_HPP
