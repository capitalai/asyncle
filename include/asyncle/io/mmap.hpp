#ifndef ASYNCLE_IO_MMAP_HPP
#define ASYNCLE_IO_MMAP_HPP

#include "../../platform/mmap.hpp"
#include "file.hpp"
#include "result.hpp"
#include <cstddef>
#include <memory>
#include <utility>

namespace asyncle::io {

// Import platform types for mmap (avoid conflicts with file module)
using platform::mmap::access_pattern;
using platform::mmap::backing_type;
using platform::mmap::expected;
using platform::mmap::locking_strategy;
using platform::mmap::memory_caps;
using platform::mmap::memory_error;
using platform::mmap::memory_region;
using platform::mmap::memory_request;
using platform::mmap::placement_strategy;
using platform::mmap::sharing_mode;
using platform::mmap::unexpect;

// Use explicit namespace for mmap-specific types
namespace mmap_access = platform::mmap;

// Result type aliases are now defined in result.hpp
// and reference platform::mmap::result directly

// Single RAII mmap class with full capabilities
class mmap {
    public:
    // Type aliases for result types and error handling
    using error_type = memory_error;
    template <typename T>
    using result_type      = mmap_result<T>;
    using void_result_type = mmap_void_result;

    private:
    memory_region region_;

    public:
    // Constructors
    mmap() noexcept = default;

    // Map anonymous memory with request
    explicit mmap(const memory_request& request) noexcept {
        auto result = platform::mmap::map_memory(-1, request);
        if(result) { region_ = result.value(); }
    }

    // Map anonymous memory with simple parameters
    mmap(size_t length, mmap_access::access_mode access = mmap_access::access_mode::read_write) noexcept {
        memory_request req {};
        req.length  = length;
        req.backing = backing_type::anonymous;
        req.access  = access;
        req.sharing = sharing_mode::private_cow;
        auto result = platform::mmap::map_memory(-1, req);
        if(result) { region_ = result.value(); }
    }

    // Map file with request
    mmap(const file& f, const memory_request& request) noexcept {
        if(f.is_open()) {
            auto result = platform::mmap::map_memory(f.fd(), request);
            if(result) { region_ = result.value(); }
        }
    }

    // Map file with simple parameters
    mmap(
      const file&              f,
      size_t                   length,
      size_t                   offset = 0,
      mmap_access::access_mode access = mmap_access::access_mode::read) noexcept {
        if(f.is_open()) {
            memory_request req {};
            req.length  = length;
            req.offset  = offset;
            req.backing = backing_type::file_backed;
            req.access  = access;
            req.sharing = sharing_mode::shared;
            auto result = platform::mmap::map_memory(f.fd(), req);
            if(result) { region_ = result.value(); }
        }
    }

    // Map from file descriptor with request
    mmap(int fd, const memory_request& request) noexcept {
        auto result = platform::mmap::map_memory(fd, request);
        if(result) { region_ = result.value(); }
    }

    // Create from existing region
    explicit mmap(memory_region region) noexcept: region_(region) {}

    // Move semantics
    mmap(mmap&& other) noexcept: region_(std::exchange(other.region_, memory_region {})) {}

    mmap& operator=(mmap&& other) noexcept {
        if(this != &other) {
            unmap();
            region_ = std::exchange(other.region_, memory_region {});
        }
        return *this;
    }

    // No copy
    mmap(const mmap&)            = delete;
    mmap& operator=(const mmap&) = delete;

    // Destructor
    ~mmap() { unmap(); }

    // Core operations
    mmap_result<memory_region> map(const memory_request& request, int fd = -1) noexcept {
        unmap();
        auto result = platform::mmap::map_memory(fd, request);
        if(result) { region_ = result.value(); }
        return result;
    }

    mmap_result<memory_region> map(const file& f, const memory_request& request) noexcept {
        if(!f.is_open()) {
            return mmap_result<memory_region>(unexpect, memory_error(mmap_access::error_code::invalid_argument));
        }
        return map(request, f.fd());
    }

    mmap_result<memory_region>
      map_anonymous(size_t length, mmap_access::access_mode access = mmap_access::access_mode::read_write) noexcept {
        memory_request req {};
        req.length  = length;
        req.backing = backing_type::anonymous;
        req.access  = access;
        req.sharing = sharing_mode::private_cow;
        return map(req, -1);
    }

    mmap_result<memory_region> map_file(
      const file&              f,
      size_t                   length,
      size_t                   offset = 0,
      mmap_access::access_mode access = mmap_access::access_mode::read) noexcept {
        if(!f.is_open()) {
            return mmap_result<memory_region>(unexpect, memory_error(mmap_access::error_code::invalid_argument));
        }

        memory_request req {};
        req.length  = length;
        req.offset  = offset;
        req.backing = backing_type::file_backed;
        req.access  = access;
        req.sharing = sharing_mode::shared;
        return map(req, f.fd());
    }

    void unmap() noexcept {
        if(is_mapped()) {
            platform::mmap::unmap_memory(region_);
            region_ = memory_region {};
        }
    }

    // Memory synchronization
    mmap_void_result sync(bool invalidate_caches = false) noexcept {
        if(!is_mapped()) { return mmap_void_result(unexpect, memory_error(mmap_access::error_code::invalid_argument)); }
        return platform::mmap::sync_memory(region_, invalidate_caches);
    }

    // Memory advice
    mmap_void_result advise(access_pattern pattern) noexcept {
        if(!is_mapped()) { return mmap_void_result(unexpect, memory_error(mmap_access::error_code::invalid_argument)); }
        return platform::mmap::advise_memory(region_, pattern);
    }

    // Memory locking
    mmap_void_result lock(locking_strategy strategy = locking_strategy::lock_resident) noexcept {
        if(!is_mapped()) { return mmap_void_result(unexpect, memory_error(mmap_access::error_code::invalid_argument)); }
        return platform::mmap::lock_memory(region_, strategy);
    }

    mmap_void_result unlock() noexcept {
        if(!is_mapped()) { return mmap_void_result(unexpect, memory_error(mmap_access::error_code::invalid_argument)); }
        return platform::mmap::unlock_memory(region_);
    }

    // Memory prefetch
    mmap_void_result prefetch(size_t offset = 0, size_t length = 0) noexcept {
        if(!is_mapped()) { return mmap_void_result(unexpect, memory_error(mmap_access::error_code::invalid_argument)); }
        return platform::mmap::prefetch_memory(region_, offset, length);
    }

    // Static utilities
    static memory_caps capabilities() noexcept { return platform::mmap::query_capabilities(); }

    static size_t page_size() noexcept {
        auto caps = capabilities();
        return caps.system_page_size;
    }

    static size_t align_to_page(size_t size) noexcept {
        size_t ps = page_size();
        return (size + ps - 1) & ~(ps - 1);
    }

    // Accessors
    void* data() noexcept { return region_.address; }

    const void* data() const noexcept { return region_.address; }

    size_t size() const noexcept { return region_.length; }

    const memory_region& region() const noexcept { return region_; }

    memory_region& region() noexcept { return region_; }

    bool is_mapped() const noexcept { return region_.address != nullptr; }

    explicit operator bool() const noexcept { return is_mapped(); }

    // Access as typed pointer
    template <typename T>
    T* as() noexcept {
        return static_cast<T*>(region_.address);
    }

    template <typename T>
    const T* as() const noexcept {
        return static_cast<const T*>(region_.address);
    }

    // Access with bounds checking
    template <typename T>
    T* at(size_t index) noexcept {
        if(index * sizeof(T) >= region_.length) { return nullptr; }
        return static_cast<T*>(region_.address) + index;
    }

    template <typename T>
    const T* at(size_t index) const noexcept {
        if(index * sizeof(T) >= region_.length) { return nullptr; }
        return static_cast<const T*>(region_.address) + index;
    }

    // Info accessors
    bool is_file_backed() const noexcept { return region_.file_descriptor >= 0; }

    bool is_anonymous() const noexcept { return region_.file_descriptor < 0; }

    bool is_locked() const noexcept { return region_.is_locked; }

    bool supports_sync() const noexcept { return region_.supports_sync; }

    int fd() const noexcept { return region_.file_descriptor; }
};

}  // namespace asyncle::io

#endif  // ASYNCLE_IO_MMAP_HPP
