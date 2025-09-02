#ifndef ASYNCLE_IO_L0_LINUX_HPP
#define ASYNCLE_IO_L0_LINUX_HPP

#include "l0_interface.hpp"

using ::unexpect;

// Platform-specific includes
#ifdef __linux__
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace asyncle::io::linux_impl {

// Linux-specific implementation details
namespace detail {

// Convert access_mode to PROT_* flags
constexpr int to_prot_flags(access_mode access) noexcept {
    int prot = PROT_NONE;
    if(static_cast<uint8_t>(access) & static_cast<uint8_t>(access_mode::read)) { prot |= PROT_READ; }
    if(static_cast<uint8_t>(access) & static_cast<uint8_t>(access_mode::write)) { prot |= PROT_WRITE; }
    if(static_cast<uint8_t>(access) & static_cast<uint8_t>(access_mode::execute)) { prot |= PROT_EXEC; }
    return prot;
}

// Convert various parameters to MAP_* flags
constexpr int to_map_flags(const io_request& req) noexcept {
    int flags = 0;

    // Sharing mode
    switch(req.sharing) {
    case sharing_mode::shared     : flags |= MAP_SHARED; break;
    case sharing_mode::private_cow: flags |= MAP_PRIVATE; break;
    }

    // Backing type
    if(req.backing == backing_type::anonymous) { flags |= MAP_ANONYMOUS; }

    // Placement strategy
    switch(req.placement) {
    case placement_strategy::fixed_address: flags |= MAP_FIXED; break;
    case placement_strategy::fixed_no_clobber:
#ifdef MAP_FIXED_NOREPLACE
        flags |= MAP_FIXED_NOREPLACE;
#endif
        break;
    case placement_strategy::any_address:
    case placement_strategy::hint_address:
    default:
        // No additional flags needed
        break;
    }

    // Large pages
    if(req.page_pref == page_preference::prefer_large || req.page_pref == page_preference::require_large) {
#ifdef MAP_HUGETLB
        flags |= MAP_HUGETLB;

        // Try to set specific huge page size if requested
        if(req.large_page_size != 0) {
#ifdef MAP_HUGE_2MB
            if(req.large_page_size == 2 * 1024 * 1024) {
                flags |= MAP_HUGE_2MB;
            }
#endif
#ifdef MAP_HUGE_1GB
            else if(req.large_page_size == 1024 * 1024 * 1024) {
                flags |= MAP_HUGE_1GB;
            }
#endif
        }
#endif
    }

    // Population/commit strategy
    if(req.populate == populate_strategy::prefault || req.commit == commit_strategy::pre_commit) {
#ifdef MAP_POPULATE
        flags |= MAP_POPULATE;
#endif
    }

    // Native flags override if enabled
    if(req.enable_native) { flags |= static_cast<int>(req.native_flags); }

    return flags;
}

// Get system page size
std::size_t get_page_size() noexcept {
    static std::size_t page_size = 0;
    if(page_size == 0) {
        long ps   = sysconf(_SC_PAGESIZE);
        page_size = (ps > 0) ? static_cast<std::size_t>(ps) : 4096;
    }
    return page_size;
}

// Check if address is properly aligned
bool is_aligned(const void* addr, std::size_t alignment) noexcept {
    if(alignment == 0) alignment = get_page_size();
    return (reinterpret_cast<uintptr_t>(addr) % alignment) == 0;
}

// Apply memory advice using madvise
expected<void, std::error_code> apply_madvise(void* addr, std::size_t length, int advice) noexcept {
    if(::madvise(addr, length, advice) == 0) { return expected<void, std::error_code>(); }
    return expected<void, std::error_code>(unexpect, std::error_code(errno, std::system_category()));
}

// Apply memory locking
expected<void, std::error_code> apply_mlock(void* addr, std::size_t length, locking_strategy strategy) noexcept {
    int result = -1;

    switch(strategy) {
    case locking_strategy::lock_resident: result = ::mlock(addr, length); break;
    case locking_strategy::lock_on_fault:
#ifdef MLOCK_ONFAULT
        result = ::mlock2(addr, length, MLOCK_ONFAULT);
#else
        return expected<void, std::error_code>(unexpect, std::error_code(ENOSYS, std::system_category()));
#endif
        break;
    case locking_strategy::no_lock:
    default                       : return expected<void, std::error_code>();
    }

    if(result == 0) { return expected<void, std::error_code>(); }
    return expected<void, std::error_code>(unexpect, std::error_code(errno, std::system_category()));
}

// Query large page sizes
void query_large_page_sizes(io_caps& caps) noexcept {
    std::size_t idx = 0;

    // Check common large page sizes
    const std::size_t common_sizes[] = {
        2 * 1024 * 1024,     // 2MB
        1024 * 1024 * 1024,  // 1GB
        0
    };

    for(const auto* size_ptr = common_sizes; *size_ptr && idx < 7; ++size_ptr) {
        // Try to determine if this size is supported
        // This is a simplified check - in practice, you might need to
        // read /proc/meminfo or /sys/kernel/mm/hugepages/
        caps.large_page_sizes[idx++] = *size_ptr;
    }

    caps.large_page_sizes[idx] = 0;  // Null terminate
}

}  // namespace detail

// Linux implementation of L0 interface functions

inline expected<io_region, std::error_code> l0_map_impl(int file_descriptor, const io_request& request) noexcept {
    // Validate parameters
    if(request.length == 0) {
        return expected<io_region, std::error_code>(unexpect, std::error_code(EINVAL, std::system_category()));
    }

    // Ensure file offset is page-aligned
    if(request.offset % detail::get_page_size() != 0) {
        return expected<io_region, std::error_code>(unexpect, std::error_code(EINVAL, std::system_category()));
    }

    // Convert parameters to system calls
    int prot_flags = detail::to_prot_flags(request.access);
    int map_flags  = detail::to_map_flags(request);

    // Handle address hint
    void* hint_addr = request.address_hint;
    if(request.placement == placement_strategy::any_address) { hint_addr = nullptr; }

    // Perform the mapping
    void* mapped_addr = ::mmap(
      hint_addr,
      request.length,
      prot_flags,
      map_flags,
      (request.backing == backing_type::anonymous) ? -1 : file_descriptor,
      static_cast<off_t>(request.offset));

    if(mapped_addr == MAP_FAILED) {
        return expected<io_region, std::error_code>(unexpect, std::error_code(errno, std::system_category()));
    }

    // Create region descriptor
    io_region region;
    region.address          = mapped_addr;
    region.length           = request.length;
    region.actual_page_size = detail::get_page_size();
    region.file_descriptor  = (request.backing == backing_type::anonymous) ? -1 : file_descriptor;
    region.file_offset      = request.offset;
    region.actual_access    = request.access;
    region.actual_sharing   = request.sharing;
    region.actual_pages     = page_preference::system_default;  // May be updated below
    region.is_locked        = false;
    region.supports_sync    = (request.backing == backing_type::file_backed);

    // Apply post-mapping operations
    bool            cleanup_needed = false;
    std::error_code error;

    // Apply memory advice for access patterns
    if(request.pattern != access_pattern::normal_access) {
        int advice = MADV_NORMAL;
        switch(request.pattern) {
        case access_pattern::sequential_access: advice = MADV_SEQUENTIAL; break;
        case access_pattern::random_access    : advice = MADV_RANDOM; break;
        default                               : break;
        }

        auto advice_result = detail::apply_madvise(mapped_addr, request.length, advice);
        if(!advice_result && request.pattern != access_pattern::normal_access) {
            // Non-fatal, continue
        }
    }

    // Apply memory locking if requested
    if(request.locking != locking_strategy::no_lock) {
        auto lock_result = detail::apply_mlock(mapped_addr, request.length, request.locking);
        if(!lock_result) {
            cleanup_needed = true;
            error          = lock_result.error();
        } else {
            region.is_locked = true;
        }
    }

    // Apply population hints if not already done via MAP_POPULATE
    if(request.populate == populate_strategy::hint_willneed && !(map_flags & MAP_POPULATE)) {
        detail::apply_madvise(mapped_addr, request.length, MADV_WILLNEED);
        // Non-fatal if this fails
    }

    // Check for large pages if requested
#ifdef MAP_HUGETLB
    if(request.page_pref != page_preference::system_default) {
        // In a real implementation, you might check /proc/self/smaps
        // to determine if large pages were actually used
        region.actual_pages = request.page_pref;
    }
#endif

    // Cleanup on error
    if(cleanup_needed) {
        ::munmap(mapped_addr, request.length);
        return expected<io_region, std::error_code>(unexpect, error);
    }

    return expected<io_region, std::error_code>(region);
}

inline expected<void, std::error_code> l0_sync_impl(const io_region& region, bool invalidate_caches) noexcept {
    if(!region.supports_sync || region.file_descriptor < 0) {
        return expected<void, std::error_code>(unexpect, std::error_code(ENODEV, std::system_category()));
    }

    int flags = MS_SYNC;
    if(invalidate_caches) { flags |= MS_INVALIDATE; }

    if(::msync(region.address, region.length, flags) == 0) { return expected<void, std::error_code>(); }

    return expected<void, std::error_code>(unexpect, std::error_code(errno, std::system_category()));
}

inline void l0_unmap_impl(const io_region& region) noexcept {
    if(region.address && region.length > 0) { ::munmap(region.address, region.length); }
}

inline io_caps l0_query_caps_impl() noexcept {
    io_caps caps;

    caps.system_page_size       = detail::get_page_size();
    caps.allocation_granularity = caps.system_page_size;

    // Query large page support
#ifdef MAP_HUGETLB
    caps.supports_large_pages = true;
    detail::query_large_page_sizes(caps);
#endif

    // Check for MAP_FIXED_NOREPLACE support (Linux >= 4.17)
#ifdef MAP_FIXED_NOREPLACE
    caps.supports_fixed_no_clobber = true;
#endif

    // Check for mlock2 support (Linux >= 4.4)
#ifdef MLOCK_ONFAULT
    caps.supports_lock_on_fault = true;
#endif

    // Check for MAP_SYNC support (requires DAX filesystem)
#ifdef MAP_SYNC
    // This is filesystem-dependent, so we set it optimistically
    caps.supports_durable_sync = true;
#endif

    caps.supports_prefetch    = true;  // madvise(MADV_WILLNEED)
    caps.supports_memory_lock = true;  // mlock/mlock2
    caps.supports_anonymous   = true;  // MAP_ANONYMOUS
    caps.supports_execute     = true;  // PROT_EXEC (subject to security policies)

    return caps;
}

inline expected<void, std::error_code> l0_advise_impl(const io_region& region, access_pattern pattern) noexcept {
    int advice = MADV_NORMAL;

    switch(pattern) {
    case access_pattern::sequential_access: advice = MADV_SEQUENTIAL; break;
    case access_pattern::random_access    : advice = MADV_RANDOM; break;
    case access_pattern::normal_access    :
    default                               : advice = MADV_NORMAL; break;
    }

    return detail::apply_madvise(region.address, region.length, advice);
}

inline expected<void, std::error_code> l0_lock_impl(const io_region& region, locking_strategy strategy) noexcept {
    return detail::apply_mlock(region.address, region.length, strategy);
}

inline expected<void, std::error_code> l0_unlock_impl(const io_region& region) noexcept {
    if(::munlock(region.address, region.length) == 0) { return expected<void, std::error_code>(); }

    return expected<void, std::error_code>(unexpect, std::error_code(errno, std::system_category()));
}

inline expected<void, std::error_code>
  l0_prefetch_impl(const io_region& region, std::size_t offset, std::size_t length) noexcept {
    void*       addr = static_cast<char*>(region.address) + offset;
    std::size_t size = (length == 0) ? (region.length - offset) : length;

    if(offset >= region.length || offset + size > region.length) {
        return expected<void, std::error_code>(unexpect, std::error_code(EINVAL, std::system_category()));
    }

    return detail::apply_madvise(addr, size, MADV_WILLNEED);
}

}  // namespace asyncle::io::linux_impl

#endif  // __linux__

#endif
