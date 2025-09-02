#ifndef ASYNCLE_IO_FILE_HPP
#define ASYNCLE_IO_FILE_HPP

#include "../compat.hpp"
#include "../../platform/file.hpp"
#include "../concepts/operation_concepts.hpp"
#include "../base/object.hpp"
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace asyncle::io {

// Import platform types
using platform::file::file_handle;
using platform::file::file_info;
using platform::file::file_error;
using platform::file::file_request;
using platform::file::file_caps;
using platform::file::io_request;
using platform::file::io_result;
using platform::file::access_mode;
using platform::file::seek_origin;
using platform::file::sync_flags;
using platform::file::file_advice;
using platform::file::file_lock;
using platform::file::lock_type;
using platform::file::lock_cmd;

// Asyncle file object - RAII wrapper for file handle
class file : public object {
private:
    file_handle handle_;
    std::string path_;
    
public:
    // Constructors
    file() noexcept = default;
    
    explicit file(file_handle h, std::string_view path = "") noexcept 
        : handle_(h), path_(path) {}
    
    // Move constructor and assignment
    file(file&& other) noexcept 
        : handle_(std::exchange(other.handle_, file_handle())), 
          path_(std::move(other.path_)) {}
    
    file& operator=(file&& other) noexcept {
        if(this != &other) {
            close();
            handle_ = std::exchange(other.handle_, file_handle());
            path_ = std::move(other.path_);
        }
        return *this;
    }
    
    // No copy
    file(const file&) = delete;
    file& operator=(const file&) = delete;
    
    // Destructor
    ~file() { close(); }
    
    // File operations
    void close() noexcept {
        if(handle_.is_valid()) {
            platform::file::close_file(handle_);
        }
    }
    
    // Accessors
    const file_handle& handle() const noexcept { return handle_; }
    file_handle& handle() noexcept { return handle_; }
    const std::string& path() const noexcept { return path_; }
    bool is_open() const noexcept { return handle_.is_valid(); }
    explicit operator bool() const noexcept { return is_open(); }
};

// File open operation
template<typename Receiver>
struct open_file_op : operation<open_file_op<Receiver>> {
    std::string path_;
    file_request request_;
    Receiver receiver_;
    
    open_file_op(std::string path, file_request req, Receiver r) noexcept
        : path_(std::move(path)), request_(req), receiver_(std::move(r)) {}
    
    void start() noexcept {
        auto result = platform::file::open_file(path_.c_str(), request_);
        if(result) {
            set_value(std::move(receiver_), file(result.value(), path_));
        } else {
            set_error(std::move(receiver_), result.error());
        }
    }
};

// File read operation
template<typename Receiver>
struct read_file_op : operation<read_file_op<Receiver>> {
    file* file_;
    std::span<std::byte> buffer_;
    uint64_t offset_;
    Receiver receiver_;
    
    read_file_op(file& f, std::span<std::byte> buf, uint64_t off, Receiver r) noexcept
        : file_(&f), buffer_(buf), offset_(off), receiver_(std::move(r)) {}
    
    void start() noexcept {
        if(!file_->is_open()) {
            set_error(std::move(receiver_), file_error(platform::file::error_code::invalid_argument));
            return;
        }
        
        io_request req{};
        req.buffer = buffer_.data();
        req.length = buffer_.size();
        req.offset = offset_;
        
        auto result = platform::file::read_file(file_->handle(), req);
        if(result) {
            set_value(std::move(receiver_), result.value().bytes_transferred);
        } else {
            set_error(std::move(receiver_), result.error());
        }
    }
};

// File write operation
template<typename Receiver>
struct write_file_op : operation<write_file_op<Receiver>> {
    file* file_;
    std::span<const std::byte> buffer_;
    uint64_t offset_;
    Receiver receiver_;
    
    write_file_op(file& f, std::span<const std::byte> buf, uint64_t off, Receiver r) noexcept
        : file_(&f), buffer_(buf), offset_(off), receiver_(std::move(r)) {}
    
    void start() noexcept {
        if(!file_->is_open()) {
            set_error(std::move(receiver_), file_error(platform::file::error_code::invalid_argument));
            return;
        }
        
        io_request req{};
        req.buffer = const_cast<void*>(static_cast<const void*>(buffer_.data()));
        req.length = buffer_.size();
        req.offset = offset_;
        
        auto result = platform::file::write_file(file_->handle(), req);
        if(result) {
            set_value(std::move(receiver_), result.value().bytes_transferred);
        } else {
            set_error(std::move(receiver_), result.error());
        }
    }
};

// File sync operation
template<typename Receiver>
struct sync_file_op : operation<sync_file_op<Receiver>> {
    file* file_;
    sync_flags flags_;
    Receiver receiver_;
    
    sync_file_op(file& f, sync_flags flags, Receiver r) noexcept
        : file_(&f), flags_(flags), receiver_(std::move(r)) {}
    
    void start() noexcept {
        if(!file_->is_open()) {
            set_error(std::move(receiver_), file_error(platform::file::error_code::invalid_argument));
            return;
        }
        
        auto result = platform::file::sync_file(file_->handle(), flags_);
        if(result) {
            set_value(std::move(receiver_));
        } else {
            set_error(std::move(receiver_), result.error());
        }
    }
};

// File sender types
template<typename Op>
struct file_sender {
    using operation_type = Op;
    
    template<typename Receiver>
    auto connect(Receiver r) && noexcept {
        return std::apply([&r](auto&&... args) {
            return Op(std::forward<decltype(args)>(args)..., std::move(r));
        }, std::move(data_));
    }
    
    std::tuple<typename Op::args_type...> data_;
};

// Convenience functions to create senders

inline auto open(std::string_view path, access_mode mode = access_mode::read_only) noexcept {
    file_request req{};
    req.access = mode;
    return file_sender<open_file_op<void>>{std::string(path), req};
}

inline auto open(std::string_view path, file_request req) noexcept {
    return file_sender<open_file_op<void>>{std::string(path), req};
}

inline auto read(file& f, std::span<std::byte> buffer, uint64_t offset = static_cast<uint64_t>(-1)) noexcept {
    return file_sender<read_file_op<void>>{f, buffer, offset};
}

inline auto write(file& f, std::span<const std::byte> buffer, uint64_t offset = static_cast<uint64_t>(-1)) noexcept {
    return file_sender<write_file_op<void>>{f, buffer, offset};
}

inline auto sync(file& f, sync_flags flags = sync_flags::full_sync) noexcept {
    return file_sender<sync_file_op<void>>{f, flags};
}

// Synchronous convenience functions

inline expected<file, file_error> open_sync(std::string_view path, access_mode mode = access_mode::read_only) noexcept {
    file_request req{};
    req.access = mode;
    auto result = platform::file::open_file(path.data(), req);
    if(result) {
        return expected<file, file_error>(file(result.value(), path));
    }
    return expected<file, file_error>(unexpect, result.error());
}

inline expected<size_t, file_error> read_sync(file& f, std::span<std::byte> buffer, uint64_t offset = static_cast<uint64_t>(-1)) noexcept {
    if(!f.is_open()) {
        return expected<size_t, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    
    io_request req{};
    req.buffer = buffer.data();
    req.length = buffer.size();
    req.offset = offset;
    
    auto result = platform::file::read_file(f.handle(), req);
    if(result) {
        return expected<size_t, file_error>(result.value().bytes_transferred);
    }
    return expected<size_t, file_error>(unexpect, result.error());
}

inline expected<size_t, file_error> write_sync(file& f, std::span<const std::byte> buffer, uint64_t offset = static_cast<uint64_t>(-1)) noexcept {
    if(!f.is_open()) {
        return expected<size_t, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    
    io_request req{};
    req.buffer = const_cast<void*>(static_cast<const void*>(buffer.data()));
    req.length = buffer.size();
    req.offset = offset;
    
    auto result = platform::file::write_file(f.handle(), req);
    if(result) {
        return expected<size_t, file_error>(result.value().bytes_transferred);
    }
    return expected<size_t, file_error>(unexpect, result.error());
}

inline expected<void, file_error> sync_sync(file& f, sync_flags flags = sync_flags::full_sync) noexcept {
    if(!f.is_open()) {
        return expected<void, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    return platform::file::sync_file(f.handle(), flags);
}

// Utility functions

inline expected<uint64_t, file_error> file_size(const file& f) noexcept {
    if(!f.is_open()) {
        return expected<uint64_t, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    return platform::file::get_file_size(f.handle());
}

inline expected<uint64_t, file_error> seek(file& f, int64_t offset, seek_origin origin = seek_origin::begin) noexcept {
    if(!f.is_open()) {
        return expected<uint64_t, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    return platform::file::seek_file(f.handle(), offset, origin);
}

inline expected<uint64_t, file_error> tell(const file& f) noexcept {
    if(!f.is_open()) {
        return expected<uint64_t, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    return platform::file::tell_file(f.handle());
}

inline expected<void, file_error> truncate(file& f, uint64_t size) noexcept {
    if(!f.is_open()) {
        return expected<void, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    return platform::file::truncate_file(f.handle(), size);
}

// File info
inline expected<file_info, file_error> stat(const file& f) noexcept {
    if(!f.is_open()) {
        return expected<file_info, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    return platform::file::stat_file(f.handle());
}

inline expected<file_info, file_error> stat(std::string_view path, bool follow_symlinks = true) noexcept {
    return platform::file::stat_path(path.data(), follow_symlinks);
}

// Zero-copy operations
inline expected<size_t, file_error> splice(file& in, uint64_t* in_offset,
                                           file& out, uint64_t* out_offset,
                                           size_t length, uint32_t flags = 0) noexcept {
    if(!in.is_open() || !out.is_open()) {
        return expected<size_t, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    return platform::file::splice_files(in.handle(), in_offset, out.handle(), out_offset, length, flags);
}

inline expected<size_t, file_error> sendfile(file& out, file& in,
                                             uint64_t* offset, size_t count) noexcept {
    if(!in.is_open() || !out.is_open()) {
        return expected<size_t, file_error>(unexpect, file_error(platform::file::error_code::invalid_argument));
    }
    return platform::file::sendfile_op(out.handle(), in.handle(), offset, count);
}

// Query capabilities
inline file_caps query_caps() noexcept {
    return platform::file::query_file_caps();
}

}  // namespace asyncle::io

#endif  // ASYNCLE_IO_FILE_HPP