#ifndef ASYNCLE_COMPAT_CXX23_HPP
#define ASYNCLE_COMPAT_CXX23_HPP

// C++23 compatibility header - provides missing features for older standard libraries

// Include standard headers first
#include "type_traits.hpp"
#include <stdexcept>
#include <type_traits>
#include <utility>

// Check for C++23 features and provide fallbacks
namespace asyncle::compat {

// std::expected compatibility - use the implementation from expected.hpp
#include "expected.hpp"

// Create alias in compat namespace for internal compatibility layer use
template <class T, class E = std::exception>
using expected = asyncle::expected<T, E>;

// std::is_aggregate_v compatibility - use the implementation from type_traits.hpp
using ::asyncle::is_aggregate_v;

// std::same_as compatibility - always use our implementations to avoid conflicts
template <class T, class U>
concept same_as = std::is_same_v<T, U>;

template <class From, class To>
concept convertible_to = std::is_convertible_v<From, To>;

}  // namespace asyncle::compat

// Don't import into std namespace to avoid conflicts with native implementations

#endif
