#ifdef __APPLE__

#include "../../include/asyncle/hardware/memory.hpp"

namespace asyncle::hardware {

cache_info detect_cache_info() noexcept {
    cache_info info;

    // TODO: Implement macOS cache detection using sysctl
    // For now, return conservative defaults

    return info;
}

}  // namespace asyncle::hardware

#endif  // __APPLE__
