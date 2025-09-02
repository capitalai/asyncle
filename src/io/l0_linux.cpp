#include "../../include/asyncle/io/l0_interface.hpp"

using ::unexpect;

#ifdef __linux__
#include "../../include/asyncle/io/l0_linux.hpp"

namespace asyncle::io {

// Linux implementation of the L0 interface functions
// These functions delegate to the platform-specific implementations

expected<io_region, std::error_code> l0_map(int file_descriptor, const io_request& request) noexcept {
    return linux_impl::l0_map_impl(file_descriptor, request);
}

expected<void, std::error_code> l0_sync(const io_region& region, bool invalidate_caches) noexcept {
    return linux_impl::l0_sync_impl(region, invalidate_caches);
}

void l0_unmap(const io_region& region) noexcept { linux_impl::l0_unmap_impl(region); }

io_caps l0_query_caps() noexcept { return linux_impl::l0_query_caps_impl(); }

expected<void, std::error_code> l0_advise(const io_region& region, access_pattern pattern) noexcept {
    return linux_impl::l0_advise_impl(region, pattern);
}

expected<void, std::error_code> l0_lock(const io_region& region, locking_strategy strategy) noexcept {
    return linux_impl::l0_lock_impl(region, strategy);
}

expected<void, std::error_code> l0_unlock(const io_region& region) noexcept {
    return linux_impl::l0_unlock_impl(region);
}

expected<void, std::error_code> l0_prefetch(const io_region& region, std::size_t offset, std::size_t length) noexcept {
    return linux_impl::l0_prefetch_impl(region, offset, length);
}

}  // namespace asyncle::io

#else

// Stub implementation for non-Linux platforms
namespace asyncle::io {

expected<io_region, std::error_code> l0_map(int file_descriptor, const io_request& request) noexcept {
    return expected<io_region, std::error_code>(unexpect, std::error_code(ENOSYS, std::system_category()));
}

expected<void, std::error_code> l0_sync(const io_region& region, bool invalidate_caches) noexcept {
    return expected<void, std::error_code>(unexpect, std::error_code(ENOSYS, std::system_category()));
}

void l0_unmap(const io_region& region) noexcept {
    // No-op for unsupported platforms
}

io_caps l0_query_caps() noexcept {
    return io_caps {};  // Return default capabilities (all disabled)
}

expected<void, std::error_code> l0_advise(const io_region& region, access_pattern pattern) noexcept {
    return expected<void, std::error_code>(unexpect, std::error_code(ENOSYS, std::system_category()));
}

expected<void, std::error_code> l0_lock(const io_region& region, locking_strategy strategy) noexcept {
    return expected<void, std::error_code>(unexpect, std::error_code(ENOSYS, std::system_category()));
}

expected<void, std::error_code> l0_unlock(const io_region& region) noexcept {
    return expected<void, std::error_code>(unexpect, std::error_code(ENOSYS, std::system_category()));
}

expected<void, std::error_code> l0_prefetch(const io_region& region, std::size_t offset, std::size_t length) noexcept {
    return expected<void, std::error_code>(unexpect, std::error_code(ENOSYS, std::system_category()));
}

}  // namespace asyncle::io

#endif
