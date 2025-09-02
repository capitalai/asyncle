#ifndef ASYNCLE_IO_FILE_HPP
#define ASYNCLE_IO_FILE_HPP

#include "../../platform/file.hpp"
#include <memory>
#include <span>
#include <string_view>
#include <utility>

namespace asyncle::io {

// Import platform types
using platform::file::access_mode;
using platform::file::error_code;
using platform::file::error_domain;
using platform::file::expected;
using platform::file::file_advice;
using platform::file::file_caps;
using platform::file::file_error;
using platform::file::file_handle;
using platform::file::file_info;
using platform::file::file_lock;
using platform::file::file_request;
using platform::file::file_type;
using platform::file::io_request;
using platform::file::io_result;
using platform::file::lock_cmd;
using platform::file::lock_type;
using platform::file::seek_origin;
using platform::file::sync_flags;
using platform::file::unexpect;

// Single RAII file class with full capabilities
class file {
    private:
    file_handle handle_;

    public:
    // Constructors
    file() noexcept = default;

    // Open file with path and request
    file(const char* path, const file_request& request) noexcept {
        auto result = platform::file::open_file(path, request);
        if(result) { handle_ = result.value(); }
    }

    // Open file with simple mode
    file(const char* path, access_mode mode = access_mode::read_only) noexcept {
        file_request req {};
        req.access = mode;
        auto result = platform::file::open_file(path, req);
        if(result) { handle_ = result.value(); }
    }

    // Create from existing handle
    explicit file(file_handle h) noexcept: handle_(h) {}

    // Move semantics
    file(file&& other) noexcept: handle_(std::exchange(other.handle_, file_handle {})) {}

    file& operator=(file&& other) noexcept {
        if(this != &other) {
            close();
            handle_ = std::exchange(other.handle_, file_handle {});
        }
        return *this;
    }

    // No copy
    file(const file&)            = delete;
    file& operator=(const file&) = delete;

    // Destructor
    ~file() { close(); }

    // Core operations
    expected<file_handle, file_error> open(const char* path, const file_request& request) noexcept {
        close();
        auto result = platform::file::open_file(path, request);
        if(result) { handle_ = result.value(); }
        return result;
    }

    expected<file_handle, file_error> open(const char* path, access_mode mode = access_mode::read_only) noexcept {
        file_request req {};
        req.access = mode;
        return open(path, req);
    }

    expected<file_handle, file_error> create_temp(const char* dir = nullptr, const file_request& request = {}) noexcept {
        close();
        auto result = platform::file::create_temp(dir, request);
        if(result) { handle_ = result.value(); }
        return result;
    }

    void close() noexcept {
        if(handle_.is_valid()) {
            platform::file::close_file(handle_);
            handle_ = file_handle {};
        }
    }

    // I/O operations
    expected<io_result, file_error> read(const io_request& request) const noexcept {
        if(!is_open()) { return expected<io_result, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::read_file(handle_, request);
    }

    expected<size_t, file_error> read(void* buffer, size_t length, uint64_t offset = static_cast<uint64_t>(-1)) const noexcept {
        io_request req {};
        req.buffer = buffer;
        req.length = length;
        req.offset = offset;
        auto result = read(req);
        if(result) { return expected<size_t, file_error>(result.value().bytes_transferred); }
        return expected<size_t, file_error>(unexpect, result.error());
    }

    expected<io_result, file_error> write(const io_request& request) noexcept {
        if(!is_open()) { return expected<io_result, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::write_file(handle_, request);
    }

    expected<size_t, file_error> write(const void* buffer, size_t length, uint64_t offset = static_cast<uint64_t>(-1)) noexcept {
        io_request req {};
        req.buffer = const_cast<void*>(buffer);
        req.length = length;
        req.offset = offset;
        auto result = write(req);
        if(result) { return expected<size_t, file_error>(result.value().bytes_transferred); }
        return expected<size_t, file_error>(unexpect, result.error());
    }

    // Vectored I/O
    expected<io_result, file_error> readv(const io_request* requests, size_t count) const noexcept {
        if(!is_open()) { return expected<io_result, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::read_vectored(handle_, requests, count);
    }

    expected<io_result, file_error> writev(const io_request* requests, size_t count) noexcept {
        if(!is_open()) { return expected<io_result, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::write_vectored(handle_, requests, count);
    }

    // File positioning
    expected<uint64_t, file_error> seek(int64_t offset, seek_origin origin = seek_origin::begin) noexcept {
        if(!is_open()) { return expected<uint64_t, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::seek_file(handle_, offset, origin);
    }

    expected<uint64_t, file_error> tell() const noexcept {
        if(!is_open()) { return expected<uint64_t, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::tell_file(handle_);
    }

    // Synchronization
    expected<void, file_error> sync(sync_flags flags = sync_flags::full_sync) noexcept {
        if(!is_open()) { return expected<void, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::sync_file(handle_, flags);
    }

    expected<void, file_error> sync_range(uint64_t offset, uint64_t length, sync_flags flags = sync_flags::full_sync) noexcept {
        if(!is_open()) { return expected<void, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::sync_range(handle_, offset, length, flags);
    }

    // File manipulation
    expected<void, file_error> truncate(uint64_t size) noexcept {
        if(!is_open()) { return expected<void, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::truncate_file(handle_, size);
    }

    expected<void, file_error> allocate(uint64_t offset, uint64_t length) noexcept {
        if(!is_open()) { return expected<void, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::allocate_file(handle_, offset, length);
    }

    expected<void, file_error> deallocate(uint64_t offset, uint64_t length) noexcept {
        if(!is_open()) { return expected<void, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::deallocate_file(handle_, offset, length);
    }

    // File locking
    expected<void, file_error> lock(const file_lock& lock) noexcept {
        if(!is_open()) { return expected<void, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::lock_file(handle_, lock);
    }

    expected<file_lock, file_error> test_lock(const file_lock& lock) const noexcept {
        if(!is_open()) { return expected<file_lock, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::test_lock(handle_, lock);
    }

    // File advice
    expected<void, file_error> advise(uint64_t offset, uint64_t length, file_advice advice) noexcept {
        if(!is_open()) { return expected<void, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::advise_file(handle_, offset, length, advice);
    }

    // Zero-copy operations
    expected<size_t, file_error> splice_to(file& out, uint64_t* in_offset, uint64_t* out_offset, size_t length, uint32_t flags = 0) noexcept {
        if(!is_open() || !out.is_open()) {
            return expected<size_t, file_error>(unexpect, file_error(error_code::invalid_argument));
        }
        return platform::file::splice_files(handle_, in_offset, out.handle_, out_offset, length, flags);
    }

    expected<size_t, file_error> sendfile_to(file& out, uint64_t* offset, size_t count) noexcept {
        if(!is_open() || !out.is_open()) {
            return expected<size_t, file_error>(unexpect, file_error(error_code::invalid_argument));
        }
        return platform::file::sendfile_op(out.handle_, handle_, offset, count);
    }

    // File information
    expected<file_info, file_error> stat() const noexcept {
        if(!is_open()) { return expected<file_info, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::stat_file(handle_);
    }

    expected<uint64_t, file_error> size() const noexcept {
        if(!is_open()) { return expected<uint64_t, file_error>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::get_file_size(handle_);
    }

    // Static utilities
    static expected<file_info, file_error> stat(const char* path, bool follow_symlinks = true) noexcept {
        return platform::file::stat_path(path, follow_symlinks);
    }

    static file_caps capabilities() noexcept { return platform::file::query_file_caps(); }

    // Accessors
    const file_handle& handle() const noexcept { return handle_; }
    file_handle&       handle() noexcept { return handle_; }
    bool               is_open() const noexcept { return handle_.is_valid(); }
    explicit operator bool() const noexcept { return is_open(); }
    int fd() const noexcept { return handle_.fd; }
};

}  // namespace asyncle::io

#endif  // ASYNCLE_IO_FILE_HPP