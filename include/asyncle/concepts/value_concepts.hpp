#ifndef ASYNCLE_VALUE_CONCEPTS_HPP
#define ASYNCLE_VALUE_CONCEPTS_HPP

#include "basic_concepts.hpp"

namespace asyncle {

template <typename T>
concept has_value_type = requires { typename T::value_type; };

template <typename T>
concept can_has_value = has_value_type<T> && requires(T t) {
    { t.has_value() } -> testable;
};

template <typename T>
concept can_get_value = can_has_value<T> && requires(T t) {
    { t.value() } -> just_value<typename T::value_type>;
};

}  // namespace asyncle

#endif
