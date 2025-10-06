#ifdef _WIN32

#include "../../include/platform/hardware.hpp"

namespace platform::hardware {

cache_info detect_cache_info() noexcept {
    cache_info info;

    // TODO: Implement Windows cache detection using GetLogicalProcessorInformation
    // For now, return architecture-specific defaults

    return info;
}

}  // namespace platform::hardware

#endif  // _WIN32
