#ifndef FORMAT_SERIALIZE_CONCEPTS_HPP
#define FORMAT_SERIALIZE_CONCEPTS_HPP

#include "../json/types.hpp"
#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace format::serialize {

// Import expected from json
using format::json::error;
using format::json::expected;

// Result types for serialization
template <typename T>
using result = expected<T, error>;

// Format tag types - used to select serialization format
// These are empty tag types for compile-time dispatch
struct json_tag {};

struct beve_tag {};  // Binary Efficient Versatile Encoding (Glaze)

struct csv_tag {};

struct xml_tag {};

struct yaml_tag {};

// Concept: A format tag
template <typename T>
concept format_tag =
  std::same_as<T, json_tag> || std::same_as<T, beve_tag> || std::same_as<T, csv_tag> || std::same_as<T, xml_tag>
  || std::same_as<T, yaml_tag>;

// Format type traits - can be specialized by users
template <typename Tag>
struct is_text_format: std::false_type {};

template <>
struct is_text_format<json_tag>: std::true_type {};

template <>
struct is_text_format<csv_tag>: std::true_type {};

template <>
struct is_text_format<xml_tag>: std::true_type {};

template <>
struct is_text_format<yaml_tag>: std::true_type {};

template <typename Tag>
struct is_binary_format: std::false_type {};

template <>
struct is_binary_format<beve_tag>: std::true_type {};

// Concept: A text-based serialization format (produces string)
template <typename Tag>
concept text_format = is_text_format<Tag>::value;

// Concept: A binary serialization format (produces bytes)
template <typename Tag>
concept binary_format = is_binary_format<Tag>::value;

// Concept: A type that can be serialized with a specific format
// This checks if save_fn can be called with T and Tag
template <typename T, typename Tag>
concept serializable = format_tag<Tag> && requires(T const& obj, Tag tag) {
    // Will be satisfied by ADL or CPO customization
    // Actual check happens when save is invoked
    requires std::is_object_v<T>;
};

// Concept: A type that can be deserialized from a specific format
template <typename T, typename Tag>
concept deserializable = format_tag<Tag> && requires(Tag tag) {
    requires std::is_object_v<T>;
    requires std::is_default_constructible_v<T> || std::is_move_constructible_v<T>;
};

// Serialization capabilities
struct serializer_caps {
    // Format characteristics
    bool text_format;      // Produces human-readable text
    bool binary_format;    // Produces binary data
    bool self_describing;  // Includes schema/type information

    // Features
    bool supports_reflection;  // Uses compile-time reflection
    bool supports_schema;      // Can validate against schema
    bool streaming_output;     // Supports incremental output

    // Performance
    bool zero_allocation;  // Can serialize without heap allocation (for known types)
    bool compile_time;     // Can perform serialization at compile time

    const char* format_name;
    const char* mime_type;
};

// Concept: A serializer implementation
// This is what adapter libraries must implement
template <typename S, typename Tag>
concept serializer_for = format_tag<Tag> && requires {
    { S::caps() } -> std::same_as<serializer_caps>;
    // Must support save and load operations (checked at call site)
};

}  // namespace format::serialize

#endif  // FORMAT_SERIALIZE_CONCEPTS_HPP
