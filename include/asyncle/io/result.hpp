#ifndef ASYNCLE_IO_RESULT_HPP
#define ASYNCLE_IO_RESULT_HPP

#include "../../platform/file.hpp"
#include "../../platform/mmap.hpp"
#include "../../platform/process.hpp"

namespace asyncle::io {

// Import expected types from cxx23_compat
using cxx23_compat::expected;
using cxx23_compat::unexpect;
using cxx23_compat::unexpect_t;

// Result type aliases that reference platform layer result types
// This provides a unified interface while reusing platform definitions

// File result types - reference platform::file::result
template <typename T>
using file_result      = platform::file::result<T>;
using file_void_result = platform::file::void_result;

// Memory mapping result types - reference platform::mmap::result
template <typename T>
using mmap_result      = platform::mmap::result<T>;
using mmap_void_result = platform::mmap::void_result;

// Process result types - reference platform::process::result
template <typename T>
using process_result      = platform::process::result<T>;
using process_void_result = platform::process::void_result;

}  // namespace asyncle::io

#endif  // ASYNCLE_IO_RESULT_HPP
