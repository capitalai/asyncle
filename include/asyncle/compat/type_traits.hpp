#ifndef ASYNCLE_COMPAT_TYPE_TRAITS_HPP
#define ASYNCLE_COMPAT_TYPE_TRAITS_HPP

#include <type_traits>

namespace asyncle {

// Compatibility for std::is_aggregate_v if not available
#ifdef __cpp_lib_is_aggregate
using std::is_aggregate_v;
#else
template <typename T>
constexpr bool is_aggregate_v = std::is_class_v<T> && std::is_trivially_copyable_v<T> && !std::is_polymorphic_v<T>;
#endif

// Compatibility for other missing type traits if needed
#ifndef __cpp_lib_remove_cvref
template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#else
using std::remove_cvref_t;
#endif

} // namespace asyncle

#endif