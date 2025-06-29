#ifndef ASYNCLE_VAL_HPP
#define ASYNCLE_VAL_HPP

#include <concepts>

namespace asyncle {

template <typename T, typename U = int>
concept val = std::convertible_to<T, U>;

template <typename T>
concept has_value_type = requires { typename T::value_type; };

template <typename T>
concept has_value = has_value_type<T> && requires(T t) {
    { t.value() } -> val<typename T::value_type>;
};

template <typename T>
concept has_error_type = requires { typename T::error_type; };

template <typename T>
concept has_error = has_error_type<T> && requires(T t) {
    { t.error() } -> val<typename T::error_type>;
};

template <typename T>
concept ret = val<T, bool> && has_value<T>;

template <typename T>
concept obj = std::is_aggregate_v<T>;

}  // namespace asyncle

#endif
