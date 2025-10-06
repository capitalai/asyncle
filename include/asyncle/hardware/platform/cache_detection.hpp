#ifndef ASYNCLE_HARDWARE_PLATFORM_CACHE_DETECTION_HPP
#define ASYNCLE_HARDWARE_PLATFORM_CACHE_DETECTION_HPP

#include "../arch/current.hpp"
#include <cstddef>

namespace asyncle::hardware::platform {

// Cache information structure
struct cache_info {
    size_t l1_line_size;
    size_t l2_line_size;
    size_t l3_line_size;
    size_t l1_cache_size;
    size_t l2_cache_size;
    size_t l3_cache_size;

    constexpr cache_info() noexcept:
        l1_line_size(current_arch::l1_cache_line_size),
        l2_line_size(current_arch::l2_cache_line_size),
        l3_line_size(current_arch::l3_cache_line_size),
        l1_cache_size(current_arch::typical_l1_cache_size),
        l2_cache_size(current_arch::typical_l2_cache_size),
        l3_cache_size(current_arch::typical_l3_cache_size) {}
};

// Platform-specific cache detection
// Implemented in platform-specific source files
cache_info detect_cache_info() noexcept;

}  // namespace asyncle::hardware::platform

#endif  // ASYNCLE_HARDWARE_PLATFORM_CACHE_DETECTION_HPP
