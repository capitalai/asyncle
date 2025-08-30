#ifndef ASYNCLE_ERROR_CONCEPTS_HPP
#define ASYNCLE_ERROR_CONCEPTS_HPP

#include "basic_concepts.hpp"
#include <type_traits>

namespace asyncle {

template <typename T>
concept has_error_type = requires { typename T::error_type; } && std::is_enum_v<typename T::error_type>;

template <typename T>
concept can_has_error = has_error_type<T> && requires(T t) {
    { t.has_error() } -> testable;
};

template <typename T>
concept can_get_error = can_has_error<T> && requires(T t) {
    { t.error() } -> just_value<typename T::error_type>;
};

}  // namespace asyncle

#endif
