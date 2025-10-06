#ifdef __linux__

#include "../../include/platform/hardware.hpp"
#include <fstream>
#include <string>
#include <unistd.h>

namespace platform::hardware {

cache_info detect_cache_info() noexcept {
    cache_info info;

    // Try to detect cache line size using sysconf
    long line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if(line_size > 0) {
        info.l1_line_size = static_cast<size_t>(line_size);
        info.l2_line_size = static_cast<size_t>(line_size);
        info.l3_line_size = static_cast<size_t>(line_size);
    }

    // Try to detect L1 cache size
    long l1_size = sysconf(_SC_LEVEL1_DCACHE_SIZE);
    if(l1_size > 0) { info.l1_cache_size = static_cast<size_t>(l1_size); }

    // Try to detect L2 cache size
    long l2_size = sysconf(_SC_LEVEL2_CACHE_SIZE);
    if(l2_size > 0) { info.l2_cache_size = static_cast<size_t>(l2_size); }

    // Try to detect L3 cache size
    long l3_size = sysconf(_SC_LEVEL3_CACHE_SIZE);
    if(l3_size > 0) { info.l3_cache_size = static_cast<size_t>(l3_size); }

    // Fallback: read from /sys/devices/system/cpu if sysconf fails
    if(line_size <= 0) {
        try {
            std::ifstream l1_line("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size");
            if(l1_line) {
                size_t size;
                l1_line >> size;
                if(size > 0) {
                    info.l1_line_size = size;
                    info.l2_line_size = size;
                    info.l3_line_size = size;
                }
            }
        } catch(...) {
            // Ignore errors, use defaults
        }
    }

    // Fallback: read cache sizes from /sys if sysconf fails
    if(l1_size <= 0) {
        try {
            std::ifstream l1_cache("/sys/devices/system/cpu/cpu0/cache/index0/size");
            if(l1_cache) {
                std::string size_str;
                l1_cache >> size_str;
                // Parse size string (e.g., "32K", "256K", "8M")
                size_t size = 0;
                if(sscanf(size_str.c_str(), "%zuK", &size) == 1) {
                    info.l1_cache_size = size * 1024;
                } else if(sscanf(size_str.c_str(), "%zuM", &size) == 1) {
                    info.l1_cache_size = size * 1024 * 1024;
                }
            }
        } catch(...) {
            // Ignore errors, use defaults
        }
    }

    if(l2_size <= 0) {
        try {
            std::ifstream l2_cache("/sys/devices/system/cpu/cpu0/cache/index2/size");
            if(l2_cache) {
                std::string size_str;
                l2_cache >> size_str;
                size_t      size = 0;
                if(sscanf(size_str.c_str(), "%zuK", &size) == 1) {
                    info.l2_cache_size = size * 1024;
                } else if(sscanf(size_str.c_str(), "%zuM", &size) == 1) {
                    info.l2_cache_size = size * 1024 * 1024;
                }
            }
        } catch(...) {
            // Ignore errors, use defaults
        }
    }

    if(l3_size <= 0) {
        try {
            std::ifstream l3_cache("/sys/devices/system/cpu/cpu0/cache/index3/size");
            if(l3_cache) {
                std::string size_str;
                l3_cache >> size_str;
                size_t      size = 0;
                if(sscanf(size_str.c_str(), "%zuK", &size) == 1) {
                    info.l3_cache_size = size * 1024;
                } else if(sscanf(size_str.c_str(), "%zuM", &size) == 1) {
                    info.l3_cache_size = size * 1024 * 1024;
                }
            }
        } catch(...) {
            // Ignore errors, use defaults
        }
    }

    return info;
}

}  // namespace platform::hardware

#endif  // __linux__
