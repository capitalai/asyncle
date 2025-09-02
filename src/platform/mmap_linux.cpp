#include "../../include/platform/mmap.hpp"

#ifdef __linux__
#include "../../include/platform/mmap_linux.hpp"

namespace platform::mmap {

// Linux implementation of the cross-platform mmap interface
// These functions delegate to the platform-specific implementations

cxx23_compat::expected<memory_region, memory_error>
map_memory(int file_descriptor, const memory_request& request) noexcept {
    return linux_impl::map_memory_impl(file_descriptor, request);
}

cxx23_compat::expected<void, memory_error>
sync_memory(const memory_region& region, bool invalidate_caches) noexcept {
    return linux_impl::sync_memory_impl(region, invalidate_caches);
}

void unmap_memory(const memory_region& region) noexcept {
    linux_impl::unmap_memory_impl(region);
}

memory_caps query_capabilities() noexcept {
    return linux_impl::query_capabilities_impl();
}

cxx23_compat::expected<void, memory_error>
advise_memory(const memory_region& region, access_pattern pattern) noexcept {
    return linux_impl::advise_memory_impl(region, pattern);
}

cxx23_compat::expected<void, memory_error>
lock_memory(const memory_region& region, locking_strategy strategy) noexcept {
    return linux_impl::lock_memory_impl(region, strategy);
}

cxx23_compat::expected<void, memory_error>
unlock_memory(const memory_region& region) noexcept {
    return linux_impl::unlock_memory_impl(region);
}

cxx23_compat::expected<void, memory_error>
prefetch_memory(const memory_region& region, std::size_t offset, std::size_t length) noexcept {
    return linux_impl::prefetch_memory_impl(region, offset, length);
}

} // namespace platform::mmap

#else

// Stub implementation for non-Linux platforms
namespace platform::mmap {

cxx23_compat::expected<memory_region, memory_error>
map_memory(int file_descriptor, const memory_request& request) noexcept {
    return cxx23_compat::expected<memory_region, memory_error>(
        cxx23_compat::unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

cxx23_compat::expected<void, memory_error>
sync_memory(const memory_region& region, bool invalidate_caches) noexcept {
    return cxx23_compat::expected<void, memory_error>(
        cxx23_compat::unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

void unmap_memory(const memory_region& region) noexcept {
    // No-op for unsupported platforms
}

memory_caps query_capabilities() noexcept {
    return memory_caps{}; // Return default capabilities (all disabled)
}

cxx23_compat::expected<void, memory_error>
advise_memory(const memory_region& region, access_pattern pattern) noexcept {
    return cxx23_compat::expected<void, memory_error>(
        cxx23_compat::unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

cxx23_compat::expected<void, memory_error>
lock_memory(const memory_region& region, locking_strategy strategy) noexcept {
    return cxx23_compat::expected<void, memory_error>(
        cxx23_compat::unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

cxx23_compat::expected<void, memory_error>
unlock_memory(const memory_region& region) noexcept {
    return cxx23_compat::expected<void, memory_error>(
        cxx23_compat::unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

cxx23_compat::expected<void, memory_error>
prefetch_memory(const memory_region& region, std::size_t offset, std::size_t length) noexcept {
    return cxx23_compat::expected<void, memory_error>(
        cxx23_compat::unexpect, memory_error(error_domain::platform, error_code::not_supported));
}

} // namespace platform::mmap

#endif