#ifndef ASYNCLE_IO_RESULT_HPP
#define ASYNCLE_IO_RESULT_HPP

#include "../../cxx23_compat/expected.hpp"

namespace asyncle::io {

// Import expected types
using cxx23_compat::expected;
using cxx23_compat::unexpect;
using cxx23_compat::unexpect_t;

// Common result type template for I/O operations
// Provides a standardized way to express expected<T, Error> results
template <typename T, typename Error>
using result = expected<T, Error>;

// Void result type for operations that return no value on success
template <typename Error>
using void_result = expected<void, Error>;

// Result type helpers for convenience
// These allow writing: file_result<size_t> instead of expected<size_t, file_error>

}  // namespace asyncle::io

#endif  // ASYNCLE_IO_RESULT_HPP
