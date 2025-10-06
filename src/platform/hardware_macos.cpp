#ifdef __APPLE__

#include "../../include/platform/hardware.hpp"

namespace platform::hardware {

cache_info detect_cache_info() noexcept {
    cache_info info;

    // TODO: Implement macOS cache detection using sysctl
    // For now, return architecture-specific defaults (128 bytes for Apple Silicon)

    return info;
}

}  // namespace platform::hardware

#endif  // __APPLE__
