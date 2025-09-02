#ifndef ASYNCLE_IO_L0_INTERFACE_HPP
#define ASYNCLE_IO_L0_INTERFACE_HPP

#include "../compat/expected.hpp"
#include <cstddef>
#include <cstdint>
#include <system_error>

namespace asyncle::io {

// Forward declarations
struct io_region;
struct io_caps;
struct io_request;

// L0 Interface - Low-level I/O operations without capability loss
// All structures use flattened data layout for maximum performance

// Basic access permissions
enum class access_mode : uint8_t {
    read       = 0x01,
    write      = 0x02,
    execute    = 0x04,
    read_write = read | write,
    read_exec  = read | execute,
    all_access = read | write | execute
};

// Memory sharing semantics
enum class sharing_mode : uint8_t {
    shared      = 0,  // MAP_SHARED - changes are visible to other processes
    private_cow = 1   // MAP_PRIVATE - copy-on-write semantics
};

// Memory backing source
enum class backing_type : uint8_t {
    file_backed = 0,  // File-backed mapping
    anonymous   = 1   // Anonymous memory (no file)
};

// Address placement strategy
enum class placement_strategy : uint8_t {
    any_address      = 0,  // Let system choose address
    hint_address     = 1,  // Suggest address (non-binding)
    fixed_address    = 2,  // Force specific address
    fixed_no_clobber = 3   // Fixed address but don't replace existing mapping
};

// Page size preferences
enum class page_preference : uint8_t {
    system_default = 0,  // Use system default page size
    prefer_large   = 1,  // Try large pages, fallback to normal
    require_large  = 2   // Require large pages or fail
};

// Memory commitment strategy
enum class commit_strategy : uint8_t {
    lazy_commit = 0,  // Commit pages on first access
    pre_commit  = 1   // Commit all pages immediately
};

// Population/prefaulting behavior
enum class populate_strategy : uint8_t {
    none          = 0,  // No prefaulting
    prefault      = 1,  // Prefault pages into memory
    hint_willneed = 2   // Hint that pages will be needed soon
};

// Memory locking behavior
enum class locking_strategy : uint8_t {
    no_lock       = 0,  // Don't lock pages in memory
    lock_resident = 1,  // Lock pages in physical memory
    lock_on_fault = 2   // Lock pages when they fault in
};

// Synchronization semantics for file-backed mappings
enum class sync_semantics : uint8_t {
    normal_sync  = 0,  // Standard synchronization
    durable_sync = 1   // Durable sync if supported by filesystem
};

// Access pattern hints
enum class access_pattern : uint8_t {
    normal_access     = 0,  // No specific pattern
    sequential_access = 1,  // Sequential access pattern
    random_access     = 2   // Random access pattern
};

// Flattened I/O request structure
struct io_request {
    // Core mapping parameters
    std::size_t length;           // Size of mapping in bytes
    std::size_t offset;           // Offset in file (must be page-aligned)
    void*       address_hint;     // Suggested address for placement
    std::size_t alignment;        // Alignment requirement (0 = system page size)
    std::size_t large_page_size;  // Specific large page size (0 = any)

    // Behavior flags (flattened enums for cache efficiency)
    access_mode        access;     // Access permissions
    sharing_mode       sharing;    // Sharing semantics
    backing_type       backing;    // Memory backing type
    placement_strategy placement;  // Address placement strategy
    page_preference    page_pref;  // Page size preference
    commit_strategy    commit;     // Memory commitment strategy
    populate_strategy  populate;   // Population/prefaulting behavior
    locking_strategy   locking;    // Memory locking behavior
    sync_semantics     sync;       // Synchronization semantics
    access_pattern     pattern;    // Access pattern hints

    // Platform-specific escape hatch (flattened for efficiency)
    uint64_t native_flags;       // Platform-specific flags
    uint64_t native_protection;  // Platform-specific protection bits
    uint64_t native_view_flags;  // Platform-specific view flags (Windows)
    bool     enable_native;      // Whether to use native flags

    // Default constructor with sensible defaults
    constexpr io_request() noexcept:
        length(0),
        offset(0),
        address_hint(nullptr),
        alignment(0),
        large_page_size(0),
        access(access_mode::read),
        sharing(sharing_mode::shared),
        backing(backing_type::file_backed),
        placement(placement_strategy::any_address),
        page_pref(page_preference::system_default),
        commit(commit_strategy::lazy_commit),
        populate(populate_strategy::none),
        locking(locking_strategy::no_lock),
        sync(sync_semantics::normal_sync),
        pattern(access_pattern::normal_access),
        native_flags(0),
        native_protection(0),
        native_view_flags(0),
        enable_native(false) {}
};

// Flattened I/O region structure representing a mapped region
struct io_region {
    void*       address;           // Starting address of mapped region
    std::size_t length;            // Length of mapped region
    std::size_t actual_page_size;  // Actual page size used
    int         file_descriptor;   // File descriptor (-1 for anonymous)
    std::size_t file_offset;       // Offset in file

    // Flattened flags for what was actually granted
    access_mode     actual_access;   // Actual access permissions
    sharing_mode    actual_sharing;  // Actual sharing mode
    page_preference actual_pages;    // Actual page size used
    bool            is_locked;       // Whether region is locked in memory
    bool            supports_sync;   // Whether durable sync is supported

    constexpr io_region() noexcept:
        address(nullptr),
        length(0),
        actual_page_size(0),
        file_descriptor(-1),
        file_offset(0),
        actual_access(access_mode::read),
        actual_sharing(sharing_mode::shared),
        actual_pages(page_preference::system_default),
        is_locked(false),
        supports_sync(false) {}
};

// Flattened capability structure for platform features
struct io_caps {
    // Page size information
    std::size_t system_page_size;        // System default page size
    std::size_t large_page_sizes[8];     // Supported large page sizes (0-terminated)
    std::size_t allocation_granularity;  // Minimum allocation granularity

    // Feature flags (flattened booleans for cache efficiency)
    bool supports_fixed_no_clobber;  // MAP_FIXED_NOREPLACE support
    bool supports_large_pages;       // Large/huge page support
    bool supports_lock_on_fault;     // Lock-on-fault support
    bool supports_durable_sync;      // Durable sync support (MAP_SYNC)
    bool supports_prefetch;          // Memory prefetch APIs
    bool supports_memory_lock;       // Memory locking support
    bool supports_anonymous;         // Anonymous mapping support
    bool supports_execute;           // Execute permission support

    constexpr io_caps() noexcept:
        system_page_size(4096),
        large_page_sizes {},
        allocation_granularity(4096),
        supports_fixed_no_clobber(false),
        supports_large_pages(false),
        supports_lock_on_fault(false),
        supports_durable_sync(false),
        supports_prefetch(false),
        supports_memory_lock(false),
        supports_anonymous(true),
        supports_execute(false) {}
};

// L0 Interface Functions
// These are implemented by platform-specific modules

// Map a region of memory according to the request
expected<io_region, std::error_code> l0_map(int file_descriptor, const io_request& request) noexcept;

// Synchronize a mapped region to storage
expected<void, std::error_code> l0_sync(const io_region& region, bool invalidate_caches = false) noexcept;

// Unmap a previously mapped region
void l0_unmap(const io_region& region) noexcept;

// Query platform capabilities
io_caps l0_query_caps() noexcept;

// Apply memory advice/hints to a region
expected<void, std::error_code> l0_advise(const io_region& region, access_pattern pattern) noexcept;

// Lock/unlock memory region
expected<void, std::error_code> l0_lock(const io_region& region, locking_strategy strategy) noexcept;

expected<void, std::error_code> l0_unlock(const io_region& region) noexcept;

// Prefetch memory region
expected<void, std::error_code>
  l0_prefetch(const io_region& region, std::size_t offset = 0, std::size_t length = 0) noexcept;

}  // namespace asyncle::io

#endif
