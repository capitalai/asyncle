#ifndef ASYNCLE_HARDWARE_ARCH_GENERIC_HPP
#define ASYNCLE_HARDWARE_ARCH_GENERIC_HPP

#include <cstddef>

namespace asyncle::hardware::arch {

// Generic fallback architecture constants
// Conservative values that should work on most architectures

struct generic_info {
    // Cache line sizes (bytes) - conservative 64-byte default
    static constexpr size_t cache_line_size    = 64;
    static constexpr size_t l1_cache_line_size = 64;
    static constexpr size_t l2_cache_line_size = 64;
    static constexpr size_t l3_cache_line_size = 64;

    // Typical cache sizes (bytes) - conservative estimates
    static constexpr size_t typical_l1_cache_size = 32 * 1024;        // 32 KB
    static constexpr size_t typical_l2_cache_size = 256 * 1024;       // 256 KB
    static constexpr size_t typical_l3_cache_size = 8 * 1024 * 1024;  // 8 MB

    // Page sizes
    static constexpr size_t page_size       = 4096;     // 4 KB - nearly universal
    static constexpr size_t large_page_size = 2097152;  // 2 MB - common
    static constexpr size_t huge_page_size  = 0;        // Unknown

    // Architecture features - assume none
    static constexpr bool has_cache_flush  = false;
    static constexpr bool has_prefetch     = false;
    static constexpr bool has_memory_fence = false;

    static constexpr const char* arch_name = "generic";
};

}  // namespace asyncle::hardware::arch

#endif  // ASYNCLE_HARDWARE_ARCH_GENERIC_HPP
