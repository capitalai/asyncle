#ifndef PLATFORM_HARDWARE_HPP
#define PLATFORM_HARDWARE_HPP

#include <cstddef>

namespace platform::hardware {

// Cache information structure
// This is detected at runtime by platform-specific implementations
struct cache_info {
    size_t l1_line_size;
    size_t l2_line_size;
    size_t l3_line_size;
    size_t l1_cache_size;
    size_t l2_cache_size;
    size_t l3_cache_size;

    // Default constructor with architecture-specific defaults
    // These are overridden by detect_cache_info() at runtime
    constexpr cache_info() noexcept:
        l1_line_size(64),
        l2_line_size(64),
        l3_line_size(64),
        l1_cache_size(32 * 1024),
        l2_cache_size(256 * 1024),
        l3_cache_size(8 * 1024 * 1024) {}
};

// Platform-specific cache detection
// Implemented in platform-specific source files:
// - Linux: sysconf() + /sys/devices/system/cpu
// - Windows: GetLogicalProcessorInformation()
// - macOS: sysctl()
cache_info detect_cache_info() noexcept;

}  // namespace platform::hardware

#endif  // PLATFORM_HARDWARE_HPP
