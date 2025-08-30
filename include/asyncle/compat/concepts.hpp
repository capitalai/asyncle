#ifndef ASYNCLE_COMPAT_CONCEPTS_HPP
#define ASYNCLE_COMPAT_CONCEPTS_HPP

#include <concepts>
#include <type_traits>

namespace asyncle {

// Compatibility for missing std::same_as
#ifdef __cpp_lib_concepts
using std::same_as;
using std::convertible_to;
using std::integral;
using std::floating_point;
#else
template <class T, class U>
concept same_as = std::is_same_v<T, U> && std::is_same_v<U, T>;

template <class From, class To>  
concept convertible_to = std::is_convertible_v<From, To> &&
    requires { static_cast<To>(std::declval<From>()); };

template <class T>
concept integral = std::is_integral_v<T>;

template <class T>
concept floating_point = std::is_floating_point_v<T>;
#endif

} // namespace asyncle

// Import into std namespace for compatibility
namespace std {
#ifndef __cpp_lib_concepts
using asyncle::same_as;
using asyncle::convertible_to;
using asyncle::integral;
using asyncle::floating_point;
#endif
}

#endif