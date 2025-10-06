#ifdef __APPLE__

#include "../../../include/asyncle/hardware/platform/cache_detection.hpp"

namespace asyncle::hardware::platform {

cache_info detect_cache_info() noexcept {
    cache_info info;

    // TODO: Implement macOS cache detection using sysctl
    // For now, return architecture-specific defaults (128 bytes for Apple Silicon)

    return info;
}

}  // namespace asyncle::hardware::platform

#endif  // __APPLE__
