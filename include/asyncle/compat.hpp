#ifndef ASYNCLE_COMPAT_HPP
#define ASYNCLE_COMPAT_HPP

// Clean C++23 compatibility for asyncle
// Imports compatibility types into the asyncle namespace for direct use

#include "../cxx23_compat/expected.hpp"
#include <concepts>
#include <type_traits>

namespace asyncle {

// Import expected types directly into asyncle namespace
using cxx23_compat::expected;
using cxx23_compat::unexpect_t;
using cxx23_compat::unexpect;

// Standard C++23 concepts (available in modern compilers)
using std::same_as;
using std::convertible_to;
using std::constructible_from;
using std::derived_from;

// Standard type traits
using std::is_aggregate_v;
using std::remove_cvref_t;

} // namespace asyncle

#endif