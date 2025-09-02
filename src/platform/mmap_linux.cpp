#include "../../include/platform/mmap.hpp"

#ifdef __linux__
#include "../../include/platform/mmap_linux.hpp"

namespace platform::mmap {

// Import expected types for cleaner usage
using cxx23_compat::expected;
using cxx23_compat::unexpect;

// Linux implementation of the cross-platform mmap interface
// These functions delegate to the platform-specific implementations

expected<memory_region, memory_error>
map_memory(int file_descriptor, const memory_request& request) noexcept {
    return linux_impl::map_memory_impl(file_descriptor, request);
}

expected<void, memory_error>
sync_memory(const memory_region& region, bool invalidate_caches) noexcept {
    return linux_impl::sync_memory_impl(region, invalidate_caches);
}

void unmap_memory(const memory_region& region) noexcept {
    linux_impl::unmap_memory_impl(region);
}

memory_caps query_capabilities() noexcept {
    return linux_impl::query_capabilities_impl();
}

expected<void, memory_error>
advise_memory(const memory_region& region, access_pattern pattern) noexcept {
    return linux_impl::advise_memory_impl(region, pattern);
}

expected<void, memory_error>
lock_memory(const memory_region& region, locking_strategy strategy) noexcept {
    return linux_impl::lock_memory_impl(region, strategy);
}

expected<void, memory_error>
unlock_memory(const memory_region& region) noexcept {
    return linux_impl::unlock_memory_impl(region);
}

expected<void, memory_error>
prefetch_memory(const memory_region& region, std::size_t offset, std::size_t length) noexcept {
    return linux_impl::prefetch_memory_impl(region, offset, length);
}

} // namespace platform::mmap

#else

// Stub implementation for non-Linux platforms
namespace platform::mmap {

expected<memory_region, memory_error>
map_memory(int file_descriptor, const memory_request& request) noexcept {
    return expected<memory_region, memory_error>(
        unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

expected<void, memory_error>
sync_memory(const memory_region& region, bool invalidate_caches) noexcept {
    return expected<void, memory_error>(
        unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

void unmap_memory(const memory_region& region) noexcept {
    // No-op for unsupported platforms
}

memory_caps query_capabilities() noexcept {
    return memory_caps{}; // Return default capabilities (all disabled)
}

expected<void, memory_error>
advise_memory(const memory_region& region, access_pattern pattern) noexcept {
    return expected<void, memory_error>(
        unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

expected<void, memory_error>
lock_memory(const memory_region& region, locking_strategy strategy) noexcept {
    return expected<void, memory_error>(
        unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

expected<void, memory_error>
unlock_memory(const memory_region& region) noexcept {
    return expected<void, memory_error>(
        unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

expected<void, memory_error>
prefetch_memory(const memory_region& region, std::size_t offset, std::size_t length) noexcept {
    return expected<void, memory_error>(
        unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

} // namespace platform::mmap

#endif