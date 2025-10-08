#ifndef FORMAT_JSON_TYPES_HPP
#define FORMAT_JSON_TYPES_HPP

#include "../../asyncle/compat.hpp"
#include <cstddef>
#include <string_view>

namespace format::json {

// Import expected/unexpect from asyncle
using asyncle::expected;
using asyncle::unexpect;

// Helper struct to create unexpected results (similar to std::unexpected)
template <typename E>
struct unexpected {
    E error_;

    constexpr explicit unexpected(E err): error_(std::move(err)) {}

    template <typename T>
    constexpr operator expected<T, E>() const {
        return expected<T, E>(unexpect, error_);
    }
};

// JSON error types
enum class error {
    none = 0,
    invalid_syntax,
    type_mismatch,
    key_not_found,
    index_out_of_bounds,
    io_error,
    capacity_exceeded,
    utf8_error,
    uninitialized
};

// Result type for JSON operations
template <typename T>
using result = expected<T, error>;

using void_result = expected<void, error>;

// Convert error to string
inline const char* error_string(error err) noexcept {
    switch(err) {
    case error::none               : return "Success";
    case error::invalid_syntax     : return "Invalid JSON syntax";
    case error::type_mismatch      : return "Type mismatch";
    case error::key_not_found      : return "Key not found";
    case error::index_out_of_bounds: return "Index out of bounds";
    case error::io_error           : return "I/O error";
    case error::capacity_exceeded  : return "Capacity exceeded";
    case error::utf8_error         : return "Invalid UTF-8 encoding";
    case error::uninitialized      : return "Uninitialized parser";
    }
    return "Unknown error";
}

// Parser capabilities (following platform::file_caps pattern)
struct parser_caps {
    // Core characteristics
    bool zero_copy;          // Operates directly on input without copying
    bool lazy_parsing;       // Only parses accessed fields
    bool lightweight_index;  // Minimal memory overhead for indexing
    bool full_dom;           // Builds complete Document Object Model

    // Access patterns
    bool streaming;         // Single-pass forward-only iteration
    bool random_access;     // Can access any field in any order
    bool multiple_cursors;  // Can have multiple active iterators

    // Performance characteristics
    bool simd_optimized;           // Uses SIMD instructions
    bool swar_optimized;           // Uses SWAR (SIMD Within A Register)
    bool compile_time_reflection;  // Uses compile-time type information

    // Memory characteristics
    size_t typical_overhead_pct;  // Typical memory overhead as percentage of JSON size
                                  // 0 = pure zero-copy, 50-100 = lightweight, 200-300 = full DOM
    bool can_use_stack;           // Can avoid heap allocations for known types
    bool requires_mutable;        // Requires mutable input buffer

    // API characteristics
    bool structured_binding;  // Can bind directly to C++ structs
    bool validates_utf8;      // Validates UTF-8 encoding
    bool validates_json;      // Validates JSON structure during parsing
};

// Data source lifetime hints
enum class source_lifetime {
    transient,  // Must copy immediately (e.g., temporary buffer)
    stable,     // Available during parsing but may change later
    persistent  // Available for lifetime of program (e.g., mmap'd file)
};

}  // namespace format::json

#endif  // FORMAT_JSON_TYPES_HPP
