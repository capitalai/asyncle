#ifndef ASYNCLE_HARDWARE_ARCH_AARCH64_HPP
#define ASYNCLE_HARDWARE_ARCH_AARCH64_HPP

#include "detect.hpp"
#include <cstddef>

namespace asyncle::hardware::arch {

// AArch64 (ARM 64-bit) architecture constants
// Note: Apple Silicon uses 128-byte cache lines, while most other ARM64 uses 64 bytes

struct aarch64_info {
    // Cache line sizes (bytes)
    // Apple Silicon (M1/M2/M3) uses 128-byte cache lines
    // Most other ARM64 (Cortex-A series) uses 64-byte cache lines
#if defined(ASYNCLE_ARCH_APPLE_SILICON)
    static constexpr size_t cache_line_size    = 128;
    static constexpr size_t l1_cache_line_size = 128;
    static constexpr size_t l2_cache_line_size = 128;
    static constexpr size_t l3_cache_line_size = 128;
#else
    static constexpr size_t cache_line_size    = 64;
    static constexpr size_t l1_cache_line_size = 64;
    static constexpr size_t l2_cache_line_size = 64;
    static constexpr size_t l3_cache_line_size = 64;
#endif

    // Typical cache sizes (bytes) - these vary widely
#if defined(ASYNCLE_ARCH_APPLE_SILICON)
    // Apple M1/M2 typical values
    static constexpr size_t typical_l1_cache_size = 128 * 1024;        // 128 KB per core (split I/D)
    static constexpr size_t typical_l2_cache_size = 12 * 1024 * 1024;  // 12 MB shared
    static constexpr size_t typical_l3_cache_size = 0;                 // No L3 on M1/M2
#else
    // Generic ARM Cortex-A typical values
    static constexpr size_t typical_l1_cache_size = 64 * 1024;        // 64 KB per core
    static constexpr size_t typical_l2_cache_size = 512 * 1024;       // 512 KB per cluster
    static constexpr size_t typical_l3_cache_size = 4 * 1024 * 1024;  // 4 MB system cache
#endif

    // Page sizes
    static constexpr size_t page_size       = 4096;        // 4 KB standard page
    static constexpr size_t large_page_size = 2097152;     // 2 MB huge page
    static constexpr size_t huge_page_size  = 1073741824;  // 1 GB huge page (if supported)

    // Architecture features
    static constexpr bool has_neon    = true;  // NEON SIMD
    static constexpr bool has_dc_cvac = true;  // Data cache clean (flush)
    static constexpr bool has_prfm    = true;  // Prefetch instructions
    static constexpr bool has_dmb     = true;  // Data memory barrier

    static constexpr const char* arch_name = "aarch64";
};

}  // namespace asyncle::hardware::arch

#endif  // ASYNCLE_HARDWARE_ARCH_AARCH64_HPP
