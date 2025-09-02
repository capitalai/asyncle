#ifndef PLATFORM_FILE_LINUX_HPP
#define PLATFORM_FILE_LINUX_HPP

#include "file.hpp"

// Platform-specific includes
#ifdef __linux__
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

// Linux-specific headers
#include <sys/sendfile.h>
#include <sys/syscall.h>

namespace platform::file::linux_impl {

// Import expected types for cleaner usage
using cxx23_compat::expected;
using cxx23_compat::unexpect;

// Linux-specific implementation details
namespace detail {

// Convert platform errno to our flattened error system
constexpr file_error make_system_error(int errno_val) noexcept {
    error_code code = error_code::io_error;  // default

    switch(errno_val) {
    case EINVAL    : code = error_code::invalid_argument; break;
    case ENOMEM    : code = error_code::no_memory; break;
    case EACCES    :
    case EPERM     : code = error_code::permission_denied; break;
    case ENOENT    : code = error_code::file_not_found; break;
    case EEXIST    : code = error_code::file_exists; break;
    case EISDIR    : code = error_code::is_directory; break;
    case ENOTDIR   : code = error_code::not_directory; break;
    case EMFILE    :
    case ENFILE    : code = error_code::too_many_files; break;
    case EFBIG     : code = error_code::file_too_large; break;
    case ENOSPC    : code = error_code::no_space; break;
    case ESPIPE    : code = error_code::invalid_seek; break;
    case EROFS     : code = error_code::read_only_fs; break;
    case EPIPE     : code = error_code::broken_pipe; break;
    case EAGAIN    : code = error_code::would_block; break;
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK: code = error_code::would_block; break;
#endif
    case EINTR     : code = error_code::interrupted; break;
    case ENOSYS    : code = error_code::not_supported; break;
    default        : code = error_code::io_error; break;
    }

    return file_error(error_domain::system, code, static_cast<uint8_t>(errno_val));
}

// Convert access_mode to O_* flags
constexpr int to_open_flags(access_mode access) noexcept {
    int flags = 0;
    
    uint8_t mode = static_cast<uint8_t>(access);
    
    // Basic access mode
    if((mode & 0x03) == static_cast<uint8_t>(access_mode::read_write)) {
        flags = O_RDWR;
    } else if(mode & static_cast<uint8_t>(access_mode::write_only)) {
        flags = O_WRONLY;
    } else {
        flags = O_RDONLY;
    }
    
    // Additional flags
    if(mode & static_cast<uint8_t>(access_mode::append)) flags |= O_APPEND;
    if(mode & static_cast<uint8_t>(access_mode::truncate)) flags |= O_TRUNC;
    if(mode & static_cast<uint8_t>(access_mode::create)) flags |= O_CREAT;
    if(mode & static_cast<uint8_t>(access_mode::exclusive)) flags |= O_EXCL;
    if(mode & static_cast<uint8_t>(access_mode::direct)) flags |= O_DIRECT;
    if(mode & static_cast<uint8_t>(access_mode::sync)) flags |= O_SYNC;
    
    return flags;
}

// Convert seek_origin to SEEK_* constants
constexpr int to_seek_whence(seek_origin origin) noexcept {
    switch(origin) {
    case seek_origin::begin  : return SEEK_SET;
    case seek_origin::current: return SEEK_CUR;
    case seek_origin::end    : return SEEK_END;
#ifdef SEEK_DATA
    case seek_origin::data   : return SEEK_DATA;
#endif
#ifdef SEEK_HOLE
    case seek_origin::hole   : return SEEK_HOLE;
#endif
    default                  : return SEEK_SET;
    }
}

// Convert stat structure to file_info
inline file_info stat_to_info(const struct stat& st) noexcept {
    file_info info{};
    info.size = st.st_size;
    info.blocks = st.st_blocks;
    info.inode = st.st_ino;
    info.device = st.st_dev;
    info.atime_sec = st.st_atime;
    info.mtime_sec = st.st_mtime;
    info.ctime_sec = st.st_ctime;
    info.mode = st.st_mode;
    info.uid = st.st_uid;
    info.gid = st.st_gid;
    info.nlink = st.st_nlink;
    
    // Determine file type
    if(S_ISREG(st.st_mode))       info.type = file_type::regular;
    else if(S_ISDIR(st.st_mode))  info.type = file_type::directory;
    else if(S_ISLNK(st.st_mode))  info.type = file_type::symlink;
    else if(S_ISBLK(st.st_mode))  info.type = file_type::block;
    else if(S_ISCHR(st.st_mode))  info.type = file_type::character;
    else if(S_ISFIFO(st.st_mode)) info.type = file_type::fifo;
    else if(S_ISSOCK(st.st_mode)) info.type = file_type::socket;
    else                           info.type = file_type::unknown;
    
    return info;
}

// Convert lock_type to F_* constants
constexpr short to_lock_type(lock_type type) noexcept {
    switch(type) {
    case lock_type::shared   : return F_RDLCK;
    case lock_type::exclusive: return F_WRLCK;
    case lock_type::unlock   : return F_UNLCK;
    default                  : return F_UNLCK;
    }
}

// Convert lock_cmd to F_* constants
constexpr int to_lock_cmd(lock_cmd cmd) noexcept {
    switch(cmd) {
    case lock_cmd::set_wait: return F_SETLKW;
    case lock_cmd::set     : return F_SETLK;
    case lock_cmd::get     : return F_GETLK;
    default                : return F_SETLK;
    }
}

// Convert file_advice to POSIX_FADV_* constants
constexpr int to_fadvise_advice(file_advice advice) noexcept {
    switch(advice) {
#ifdef POSIX_FADV_NORMAL
    case file_advice::normal    : return POSIX_FADV_NORMAL;
    case file_advice::sequential: return POSIX_FADV_SEQUENTIAL;
    case file_advice::random    : return POSIX_FADV_RANDOM;
    case file_advice::no_reuse  : return POSIX_FADV_NOREUSE;
    case file_advice::will_need : return POSIX_FADV_WILLNEED;
    case file_advice::dont_need : return POSIX_FADV_DONTNEED;
#endif
    default                     : return 0;
    }
}

}  // namespace detail

// Linux implementation functions

inline expected<file_handle, file_error>
open_file_impl(const char* path, const file_request& request) noexcept {
    int flags = detail::to_open_flags(request.access);
    if(request.native_flags != 0) {
        flags |= request.native_flags;
    }
    
    int fd = ::open(path, flags, request.permissions);
    if(fd < 0) {
        return expected<file_handle, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    return expected<file_handle, file_error>(file_handle(fd, flags));
}

inline expected<file_handle, file_error>
create_temp_impl(const char* dir_path, const file_request& request) noexcept {
    char template_path[PATH_MAX];
    if(dir_path) {
        snprintf(template_path, PATH_MAX, "%s/tmp.XXXXXX", dir_path);
    } else {
        snprintf(template_path, PATH_MAX, "/tmp/tmp.XXXXXX");
    }
    
    int fd = ::mkstemp(template_path);
    if(fd < 0) {
        return expected<file_handle, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    // Remove the temporary file immediately (it remains accessible via fd)
    ::unlink(template_path);
    
    return expected<file_handle, file_error>(file_handle(fd, O_RDWR));
}

inline void close_file_impl(file_handle& handle) noexcept {
    if(handle.is_valid()) {
        ::close(handle.fd);
        handle.fd = -1;
    }
}

inline expected<file_info, file_error>
stat_file_impl(const file_handle& handle) noexcept {
    struct stat st;
    if(::fstat(handle.fd, &st) < 0) {
        return expected<file_info, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<file_info, file_error>(detail::stat_to_info(st));
}

inline expected<file_info, file_error>
stat_path_impl(const char* path, bool follow_symlinks) noexcept {
    struct stat st;
    int result = follow_symlinks ? ::stat(path, &st) : ::lstat(path, &st);
    if(result < 0) {
        return expected<file_info, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<file_info, file_error>(detail::stat_to_info(st));
}

inline expected<uint64_t, file_error>
get_file_size_impl(const file_handle& handle) noexcept {
    struct stat st;
    if(::fstat(handle.fd, &st) < 0) {
        return expected<uint64_t, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<uint64_t, file_error>(st.st_size);
}

inline expected<io_result, file_error>
read_file_impl(const file_handle& handle, const io_request& request) noexcept {
    ssize_t result;
    
    if(request.offset != static_cast<uint64_t>(-1)) {
        // Use pread for specific offset
        result = ::pread(handle.fd, request.buffer, request.length, request.offset);
    } else {
        // Use regular read for current position
        result = ::read(handle.fd, request.buffer, request.length);
    }
    
    if(result < 0) {
        return expected<io_result, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    // Get current file position
    off_t new_pos = ::lseek(handle.fd, 0, SEEK_CUR);
    if(new_pos < 0) new_pos = 0;
    
    return expected<io_result, file_error>(io_result(result, new_pos));
}

inline expected<io_result, file_error>
write_file_impl(const file_handle& handle, const io_request& request) noexcept {
    ssize_t result;
    
    if(request.offset != static_cast<uint64_t>(-1)) {
        // Use pwrite for specific offset
        result = ::pwrite(handle.fd, request.buffer, request.length, request.offset);
    } else {
        // Use regular write for current position
        result = ::write(handle.fd, request.buffer, request.length);
    }
    
    if(result < 0) {
        return expected<io_result, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    // Get current file position
    off_t new_pos = ::lseek(handle.fd, 0, SEEK_CUR);
    if(new_pos < 0) new_pos = 0;
    
    return expected<io_result, file_error>(io_result(result, new_pos));
}

inline expected<io_result, file_error>
read_vectored_impl(const file_handle& handle, const io_request* requests, size_t count) noexcept {
    if(count > IOV_MAX) {
        return expected<io_result, file_error>(unexpect, file_error(error_code::invalid_argument));
    }
    
    struct iovec iov[IOV_MAX];
    for(size_t i = 0; i < count; ++i) {
        iov[i].iov_base = requests[i].buffer;
        iov[i].iov_len = requests[i].length;
    }
    
    ssize_t result = ::readv(handle.fd, iov, count);
    if(result < 0) {
        return expected<io_result, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    off_t new_pos = ::lseek(handle.fd, 0, SEEK_CUR);
    if(new_pos < 0) new_pos = 0;
    
    return expected<io_result, file_error>(io_result(result, new_pos));
}

inline expected<io_result, file_error>
write_vectored_impl(const file_handle& handle, const io_request* requests, size_t count) noexcept {
    if(count > IOV_MAX) {
        return expected<io_result, file_error>(unexpect, file_error(error_code::invalid_argument));
    }
    
    struct iovec iov[IOV_MAX];
    for(size_t i = 0; i < count; ++i) {
        iov[i].iov_base = requests[i].buffer;
        iov[i].iov_len = requests[i].length;
    }
    
    ssize_t result = ::writev(handle.fd, iov, count);
    if(result < 0) {
        return expected<io_result, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    off_t new_pos = ::lseek(handle.fd, 0, SEEK_CUR);
    if(new_pos < 0) new_pos = 0;
    
    return expected<io_result, file_error>(io_result(result, new_pos));
}

inline expected<uint64_t, file_error>
seek_file_impl(const file_handle& handle, int64_t offset, seek_origin origin) noexcept {
    off_t result = ::lseek(handle.fd, offset, detail::to_seek_whence(origin));
    if(result < 0) {
        return expected<uint64_t, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<uint64_t, file_error>(result);
}

inline expected<uint64_t, file_error>
tell_file_impl(const file_handle& handle) noexcept {
    off_t result = ::lseek(handle.fd, 0, SEEK_CUR);
    if(result < 0) {
        return expected<uint64_t, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<uint64_t, file_error>(result);
}

inline expected<void, file_error>
sync_file_impl(const file_handle& handle, sync_flags flags) noexcept {
    int result;
    if(flags == sync_flags::data_only) {
        result = ::fdatasync(handle.fd);
    } else {
        result = ::fsync(handle.fd);
    }
    
    if(result < 0) {
        return expected<void, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<void, file_error>();
}

inline expected<void, file_error>
sync_range_impl(const file_handle& handle, uint64_t offset, uint64_t length, sync_flags flags) noexcept {
#ifdef __linux__
    unsigned int sync_flags = SYNC_FILE_RANGE_WRITE;
    if(flags == sync_flags::full_sync) {
        sync_flags |= SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WAIT_AFTER;
    }
    
    if(::sync_file_range(handle.fd, offset, length, sync_flags) < 0) {
        return expected<void, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<void, file_error>();
#else
    return expected<void, file_error>(unexpect, file_error(error_domain::feature, error_code::not_supported));
#endif
}

inline expected<void, file_error>
truncate_file_impl(const file_handle& handle, uint64_t size) noexcept {
    if(::ftruncate(handle.fd, size) < 0) {
        return expected<void, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<void, file_error>();
}

inline expected<void, file_error>
allocate_file_impl(const file_handle& handle, uint64_t offset, uint64_t length) noexcept {
#ifdef __linux__
    if(::fallocate(handle.fd, 0, offset, length) < 0) {
        return expected<void, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<void, file_error>();
#else
    return expected<void, file_error>(unexpect, file_error(error_domain::feature, error_code::not_supported));
#endif
}

inline expected<void, file_error>
deallocate_file_impl(const file_handle& handle, uint64_t offset, uint64_t length) noexcept {
#ifdef __linux__
    if(::fallocate(handle.fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, offset, length) < 0) {
        return expected<void, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<void, file_error>();
#else
    return expected<void, file_error>(unexpect, file_error(error_domain::feature, error_code::not_supported));
#endif
}

inline expected<void, file_error>
lock_file_impl(const file_handle& handle, const file_lock& lock) noexcept {
    struct flock fl{};
    fl.l_type = detail::to_lock_type(lock.type);
    fl.l_whence = SEEK_SET;
    fl.l_start = lock.start;
    fl.l_len = lock.length;
    fl.l_pid = 0;
    
    if(::fcntl(handle.fd, detail::to_lock_cmd(lock.command), &fl) < 0) {
        return expected<void, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<void, file_error>();
}

inline expected<file_lock, file_error>
test_lock_impl(const file_handle& handle, const file_lock& lock) noexcept {
    struct flock fl{};
    fl.l_type = detail::to_lock_type(lock.type);
    fl.l_whence = SEEK_SET;
    fl.l_start = lock.start;
    fl.l_len = lock.length;
    fl.l_pid = 0;
    
    if(::fcntl(handle.fd, F_GETLK, &fl) < 0) {
        return expected<file_lock, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    file_lock result = lock;
    if(fl.l_type == F_UNLCK) {
        result.type = lock_type::unlock;
    } else {
        result.type = (fl.l_type == F_RDLCK) ? lock_type::shared : lock_type::exclusive;
        result.start = fl.l_start;
        result.length = fl.l_len;
        result.pid = fl.l_pid;
    }
    
    return expected<file_lock, file_error>(result);
}

inline expected<void, file_error>
advise_file_impl(const file_handle& handle, uint64_t offset, uint64_t length, file_advice advice) noexcept {
#ifdef POSIX_FADV_NORMAL
    if(::posix_fadvise(handle.fd, offset, length, detail::to_fadvise_advice(advice)) != 0) {
        return expected<void, file_error>(unexpect, detail::make_system_error(errno));
    }
    return expected<void, file_error>();
#else
    return expected<void, file_error>(unexpect, file_error(error_domain::feature, error_code::not_supported));
#endif
}

inline expected<size_t, file_error>
splice_files_impl(const file_handle& in, uint64_t* in_offset,
                  const file_handle& out, uint64_t* out_offset,
                  size_t length, uint32_t flags) noexcept {
#ifdef __linux__
    loff_t in_off = in_offset ? *in_offset : 0;
    loff_t out_off = out_offset ? *out_offset : 0;
    
    ssize_t result = ::splice(in.fd, in_offset ? &in_off : nullptr,
                              out.fd, out_offset ? &out_off : nullptr,
                              length, flags);
    
    if(result < 0) {
        return expected<size_t, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    if(in_offset) *in_offset = in_off;
    if(out_offset) *out_offset = out_off;
    
    return expected<size_t, file_error>(result);
#else
    return expected<size_t, file_error>(unexpect, file_error(error_domain::feature, error_code::not_supported));
#endif
}

inline expected<size_t, file_error>
sendfile_op_impl(const file_handle& out, const file_handle& in,
                uint64_t* offset, size_t count) noexcept {
#ifdef __linux__
    off_t off = offset ? *offset : 0;
    
    ssize_t result = ::sendfile(out.fd, in.fd, offset ? &off : nullptr, count);
    
    if(result < 0) {
        return expected<size_t, file_error>(unexpect, detail::make_system_error(errno));
    }
    
    if(offset) *offset = off;
    
    return expected<size_t, file_error>(result);
#else
    return expected<size_t, file_error>(unexpect, file_error(error_domain::feature, error_code::not_supported));
#endif
}

inline file_caps query_file_caps_impl() noexcept {
    file_caps caps{};
    
#ifdef __linux__
    caps.supports_direct_io = true;
    caps.supports_async_io = true;
    caps.supports_splice = true;
    caps.supports_fallocate = true;
#ifdef POSIX_FADV_NORMAL
    caps.supports_fadvise = true;
#endif
    caps.supports_mmap = true;
    caps.supports_lock = true;
#ifdef SEEK_DATA
    caps.supports_extended_seek = true;
#endif
    
    // Query system limits
    long max_files = sysconf(_SC_OPEN_MAX);
    if(max_files > 0) {
        caps.max_open_files = max_files;
    }
    
    // Typical Linux values
    caps.max_file_size = LLONG_MAX;
    caps.pipe_buffer_size = 65536;  // Default pipe buffer size
#endif
    
    return caps;
}

}  // namespace platform::file::linux_impl

#endif  // __linux__

#endif  // PLATFORM_FILE_LINUX_HPP