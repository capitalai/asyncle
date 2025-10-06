#ifdef _WIN32

#include "../../../include/asyncle/hardware/platform/cache_detection.hpp"

namespace asyncle::hardware::platform {

cache_info detect_cache_info() noexcept {
    cache_info info;

    // TODO: Implement Windows cache detection using GetLogicalProcessorInformation
    // For now, return architecture-specific defaults

    return info;
}

}  // namespace asyncle::hardware::platform

#endif  // _WIN32
