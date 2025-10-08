#ifndef ASYNCLE_FORMAT_SERIALIZE_REDESIGN_HPP
#define ASYNCLE_FORMAT_SERIALIZE_REDESIGN_HPP

// asyncle::format - Implementation-agnostic serialization
//
// This layer has ZERO knowledge of Glaze or any specific serialization library.
// It only uses format::serialize CPOs which handle implementation selection.
//
// Design:
//   asyncle::format::save/load ──> format::serialize::save/load (CPO)
//                                           ↓
//                               ┌───────────┴──────────────┐
//                               │                          │
//                          save_impl (Glaze)        save_impl (custom)
//                          (selected via ADL)       (selected via ADL)
//
// Usage:
//   // Using default JSON format
//   auto json = asyncle::format::to_json(config);
//   auto config = asyncle::format::from_json<Config>(json);
//
//   // Using explicit format tags (from format layer)
//   auto json = asyncle::format::save(config, format::serialize::json_tag{});
//   auto data = asyncle::format::load<Config>(json, format::serialize::json_tag{});

#include "../base/cpo.hpp"
#include "../concepts/operation_concepts.hpp"
#include <format/serialize.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace asyncle::format {

// Re-export foundation layer types and tags
using ::format::serialize::error;
using ::format::serialize::serializer_caps;

// Format tags (defined by format layer, not asyncle)
using ::format::serialize::beve_tag;
using ::format::serialize::csv_tag;
using ::format::serialize::json_tag;
using ::format::serialize::xml_tag;
using ::format::serialize::yaml_tag;

template <typename T>
using result = ::format::serialize::result<T>;

// Convenience functions - forward to format::serialize CPOs
// No knowledge of implementation, just forwards to foundation layer

// Save with explicit format tag
template <typename T, typename Tag>
inline auto save(T const& obj, Tag tag) noexcept {
    return ::format::serialize::save(obj, tag);
}

// Load with explicit format tag
template <typename T, typename Tag>
inline auto load(std::string_view data, Tag tag) noexcept
requires ::format::serialize::text_format<Tag>
{
    return ::format::serialize::load<T>(data, tag);
}

template <typename T, typename Tag>
inline auto load(std::span<std::byte const> data, Tag tag) noexcept
requires ::format::serialize::binary_format<Tag>
{
    return ::format::serialize::load<T>(data, tag);
}

// Convenience: JSON-specific functions (most common use case)
template <typename T>
inline auto to_json(T const& obj) noexcept {
    return save(obj, json_tag {});
}

template <typename T>
inline auto from_json(std::string_view json) noexcept {
    return load<T>(json, json_tag {});
}

// Convenience: Binary format functions
template <typename T>
inline auto to_binary(T const& obj) noexcept {
    return save(obj, beve_tag {});
}

template <typename T>
inline auto from_binary(std::span<std::byte const> data) noexcept {
    return load<T>(data, beve_tag {});
}

// Serializer operation builder (for more complex scenarios)
template <typename T>
class serializer_operation {
    public:
    serializer_operation() = default;

    // Save to JSON
    result<std::string> to_json(T const& obj) const noexcept { return ::format::serialize::save(obj, json_tag {}); }

    // Load from JSON
    result<T> from_json(std::string_view data) const noexcept {
        return ::format::serialize::load<T>(data, json_tag {});
    }

    // Save to binary
    result<std::vector<std::byte>> to_binary(T const& obj) const noexcept {
        return ::format::serialize::save(obj, beve_tag {});
    }

    // Load from binary
    result<T> from_binary(std::span<std::byte const> data) const noexcept {
        return ::format::serialize::load<T>(data, beve_tag {});
    }

    // Generic save with format selection
    template <typename Tag>
    auto save(T const& obj, Tag tag) const noexcept {
        return ::format::serialize::save(obj, tag);
    }

    // Generic load with format selection
    template <typename Tag>
    auto load(std::string_view data, Tag tag) const noexcept
    requires ::format::serialize::text_format<Tag>
    {
        return ::format::serialize::load<T>(data, tag);
    }

    template <typename Tag>
    auto load(std::span<std::byte const> data, Tag tag) const noexcept
    requires ::format::serialize::binary_format<Tag>
    {
        return ::format::serialize::load<T>(data, tag);
    }

    // Future: async operations
    // auto to_json_async(T const& obj) -> async_result<std::string>;
    // auto from_json_async(std::string_view data) -> async_result<T>;
};

// Factory for serializer operation
template <typename T>
inline serializer_operation<T> serializer() noexcept {
    return serializer_operation<T> {};
}

}  // namespace asyncle::format

#endif  // ASYNCLE_FORMAT_SERIALIZE_REDESIGN_HPP
