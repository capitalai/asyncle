#ifndef ASYNCLE_ERROR_CONCEPTS_HPP
#define ASYNCLE_ERROR_CONCEPTS_HPP

#include "basic_concepts.hpp"
#include <type_traits>

namespace asyncle {

// Basic error type concept - accepts both enum and struct error types
template <typename T>
concept has_error_type = requires { typename T::error_type; };

// Legacy concept for enum-based errors (for backward compatibility)
template <typename T>
concept has_enum_error_type = has_error_type<T> && std::is_enum_v<typename T::error_type>;

// Concept for structured errors (like file_error, memory_error, process_error)
template <typename T>
concept has_struct_error_type = has_error_type<T> && std::is_class_v<typename T::error_type>;

// Error checking concepts
template <typename T>
concept can_has_error = has_error_type<T> && requires(T t) {
    { t.has_error() } -> testable;
};

template <typename T>
concept can_get_error = can_has_error<T> && requires(T t) {
    { t.error() } -> just_value<typename T::error_type>;
};

// Result type concept - for types that can represent success or error (like expected)
template <typename T>
concept is_result_type = requires(T t) {
    typename T::value_type;
    typename T::error_type;
    { t.has_value() } -> testable;
    { t.value() } -> just_value<typename T::value_type>;
    { t.error() } -> just_value<typename T::error_type>;
};

}  // namespace asyncle

#endif
