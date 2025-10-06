#ifndef ASYNCLE_HARDWARE_ARCH_X86_64_HPP
#define ASYNCLE_HARDWARE_ARCH_X86_64_HPP

#include <cstddef>

namespace asyncle::hardware::arch {

// x86-64 / AMD64 architecture constants
// Both Intel and AMD modern CPUs use 64-byte cache lines

struct x86_64_info {
    // Cache line sizes (bytes)
    static constexpr size_t cache_line_size    = 64;
    static constexpr size_t l1_cache_line_size = 64;
    static constexpr size_t l2_cache_line_size = 64;
    static constexpr size_t l3_cache_line_size = 64;

    // Typical cache sizes (bytes) - these are just hints
    static constexpr size_t typical_l1_cache_size = 32 * 1024;        // 32 KB per core
    static constexpr size_t typical_l2_cache_size = 256 * 1024;       // 256 KB per core
    static constexpr size_t typical_l3_cache_size = 8 * 1024 * 1024;  // 8 MB shared

    // Page sizes
    static constexpr size_t page_size       = 4096;        // 4 KB standard page
    static constexpr size_t large_page_size = 2097152;     // 2 MB huge page
    static constexpr size_t huge_page_size  = 1073741824;  // 1 GB huge page

    // Architecture features
    static constexpr bool has_sse      = true;
    static constexpr bool has_sse2     = true;
    static constexpr bool has_clflush  = true;  // Cache line flush instruction
    static constexpr bool has_prefetch = true;  // Prefetch instructions
    static constexpr bool has_mfence   = true;  // Memory fence instructions

    static constexpr const char* arch_name = "x86-64";
};

}  // namespace asyncle::hardware::arch

#endif  // ASYNCLE_HARDWARE_ARCH_X86_64_HPP
