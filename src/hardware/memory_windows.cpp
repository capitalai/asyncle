#ifdef _WIN32

#include "../../include/asyncle/hardware/memory.hpp"

namespace asyncle::hardware {

cache_info detect_cache_info() noexcept {
    cache_info info;

    // TODO: Implement Windows cache detection using GetLogicalProcessorInformation
    // For now, return conservative defaults

    return info;
}

}  // namespace asyncle::hardware

#endif  // _WIN32
