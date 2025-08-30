#ifndef ASYNCLE_BASIC_CONCEPTS_HPP
#define ASYNCLE_BASIC_CONCEPTS_HPP

#include <concepts>

namespace asyncle {

template <typename T, typename U = int>
concept just_value = std::convertible_to<T, U>;

template <typename T, typename U>
concept same_type = std::same_as<std::remove_cvref_t<T>, U>;

template <typename T>
concept testable = just_value<T, bool>;

template <typename T>
concept object = std::is_aggregate_v<T>;

}  // namespace asyncle

#endif
