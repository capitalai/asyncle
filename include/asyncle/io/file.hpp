#ifndef ASYNCLE_IO_FILE_HPP
#define ASYNCLE_IO_FILE_HPP

#include "../../platform/file.hpp"
#include "result.hpp"
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

// Standardized result types for file operations
template <typename T>
using file_result = result<T, file_error>;

using file_void_result = void_result<file_error>;

// Single RAII file class with full capabilities
class file {
    public:
    // Type aliases for result types and error handling
    using error_type = file_error;
    template <typename T>
    using result_type = file_result<T>;
    using void_result_type = file_void_result;

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
    file_result<file_handle> open(const char* path, const file_request& request) noexcept {
        close();
        auto result = platform::file::open_file(path, request);
        if(result) { handle_ = result.value(); }
        return result;
    }

    file_result<file_handle> open(const char* path, access_mode mode = access_mode::read_only) noexcept {
        file_request req {};
        req.access = mode;
        return open(path, req);
    }

    file_result<file_handle> create_temp(const char* dir = nullptr, const file_request& request = {}) noexcept {
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
    file_result<io_result> read(const io_request& request) const noexcept {
        if(!is_open()) { return file_result<io_result>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::read_file(handle_, request);
    }

    file_result<size_t> read(void* buffer, size_t length, uint64_t offset = static_cast<uint64_t>(-1)) const noexcept {
        io_request req {};
        req.buffer = buffer;
        req.length = length;
        req.offset = offset;
        auto result = read(req);
        if(result) { return file_result<size_t>(result.value().bytes_transferred); }
        return file_result<size_t>(unexpect, result.error());
    }

    file_result<io_result> write(const io_request& request) noexcept {
        if(!is_open()) { return file_result<io_result>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::write_file(handle_, request);
    }

    file_result<size_t> write(const void* buffer, size_t length, uint64_t offset = static_cast<uint64_t>(-1)) noexcept {
        io_request req {};
        req.buffer = const_cast<void*>(buffer);
        req.length = length;
        req.offset = offset;
        auto result = write(req);
        if(result) { return file_result<size_t>(result.value().bytes_transferred); }
        return file_result<size_t>(unexpect, result.error());
    }

    // Vectored I/O
    file_result<io_result> readv(const io_request* requests, size_t count) const noexcept {
        if(!is_open()) { return file_result<io_result>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::read_vectored(handle_, requests, count);
    }

    file_result<io_result> writev(const io_request* requests, size_t count) noexcept {
        if(!is_open()) { return file_result<io_result>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::write_vectored(handle_, requests, count);
    }

    // File positioning
    file_result<uint64_t> seek(int64_t offset, seek_origin origin = seek_origin::begin) noexcept {
        if(!is_open()) { return file_result<uint64_t>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::seek_file(handle_, offset, origin);
    }

    file_result<uint64_t> tell() const noexcept {
        if(!is_open()) { return file_result<uint64_t>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::tell_file(handle_);
    }

    // Synchronization
    file_void_result sync(sync_flags flags = sync_flags::full_sync) noexcept {
        if(!is_open()) { return file_void_result(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::sync_file(handle_, flags);
    }

    file_void_result sync_range(uint64_t offset, uint64_t length, sync_flags flags = sync_flags::full_sync) noexcept {
        if(!is_open()) { return file_void_result(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::sync_range(handle_, offset, length, flags);
    }

    // File manipulation
    file_void_result truncate(uint64_t size) noexcept {
        if(!is_open()) { return file_void_result(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::truncate_file(handle_, size);
    }

    file_void_result allocate(uint64_t offset, uint64_t length) noexcept {
        if(!is_open()) { return file_void_result(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::allocate_file(handle_, offset, length);
    }

    file_void_result deallocate(uint64_t offset, uint64_t length) noexcept {
        if(!is_open()) { return file_void_result(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::deallocate_file(handle_, offset, length);
    }

    // File locking
    file_void_result lock(const file_lock& lock) noexcept {
        if(!is_open()) { return file_void_result(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::lock_file(handle_, lock);
    }

    file_result<file_lock> test_lock(const file_lock& lock) const noexcept {
        if(!is_open()) { return file_result<file_lock>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::test_lock(handle_, lock);
    }

    // File advice
    file_void_result advise(uint64_t offset, uint64_t length, file_advice advice) noexcept {
        if(!is_open()) { return file_void_result(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::advise_file(handle_, offset, length, advice);
    }

    // Zero-copy operations
    file_result<size_t> splice_to(file& out, uint64_t* in_offset, uint64_t* out_offset, size_t length, uint32_t flags = 0) noexcept {
        if(!is_open() || !out.is_open()) {
            return file_result<size_t>(unexpect, file_error(error_code::invalid_argument));
        }
        return platform::file::splice_files(handle_, in_offset, out.handle_, out_offset, length, flags);
    }

    file_result<size_t> sendfile_to(file& out, uint64_t* offset, size_t count) noexcept {
        if(!is_open() || !out.is_open()) {
            return file_result<size_t>(unexpect, file_error(error_code::invalid_argument));
        }
        return platform::file::sendfile_op(out.handle_, handle_, offset, count);
    }

    // File information
    file_result<file_info> stat() const noexcept {
        if(!is_open()) { return file_result<file_info>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::stat_file(handle_);
    }

    file_result<uint64_t> size() const noexcept {
        if(!is_open()) { return file_result<uint64_t>(unexpect, file_error(error_code::invalid_argument)); }
        return platform::file::get_file_size(handle_);
    }

    // Static utilities
    static file_result<file_info> stat(const char* path, bool follow_symlinks = true) noexcept {
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