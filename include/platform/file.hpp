#ifndef PLATFORM_FILE_HPP
#define PLATFORM_FILE_HPP

#include "../cxx23_compat/expected.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace platform::file {

// Import expected types for cleaner usage
using cxx23_compat::expected;
using cxx23_compat::unexpect;

// Forward declarations
struct file_handle;
struct file_caps;
struct file_request;
struct file_error;
struct file_info;
struct io_request;
struct io_result;

// Flattened error system - consistent with mmap module
enum class error_domain : uint8_t {
    system   = 0,  // System/OS errors (errno)
    platform = 1,  // Platform-specific errors
    feature  = 2   // Feature not supported
};

enum class error_code : uint16_t {
    success           = 0,
    io_error          = 1,
    invalid_argument  = 2,
    no_memory         = 3,
    permission_denied = 4,
    file_not_found    = 5,
    file_exists       = 6,
    is_directory      = 7,
    not_directory     = 8,
    too_many_files    = 9,
    file_too_large    = 10,
    no_space          = 11,
    invalid_seek      = 12,
    read_only_fs      = 13,
    broken_pipe       = 14,
    would_block       = 15,
    interrupted       = 16,
    not_supported     = 200,
    platform_specific = 201
};

// Flattened error structure (4 bytes)
struct file_error {
    error_domain domain;          // 1 byte
    uint8_t      platform_errno;  // 1 byte
    error_code   code;            // 2 bytes

    constexpr file_error() noexcept: domain(error_domain::system), platform_errno(0), code(error_code::success) {}

    constexpr file_error(error_code c) noexcept: domain(error_domain::system), platform_errno(0), code(c) {}

    constexpr file_error(error_domain d, error_code c, uint8_t e = 0) noexcept: domain(d), platform_errno(e), code(c) {}
};

// File access modes (can be combined)
enum class access_mode : uint8_t {
    read_only  = 0x01,
    write_only = 0x02,
    read_write = 0x03,
    append     = 0x04,
    truncate   = 0x08,
    create     = 0x10,
    exclusive  = 0x20,  // Fail if file exists (with create)
    direct     = 0x40,  // O_DIRECT - bypass cache
    sync       = 0x80   // O_SYNC - synchronous I/O
};

// File seek origin
enum class seek_origin : uint8_t {
    begin   = 0,  // SEEK_SET
    current = 1,  // SEEK_CUR
    end     = 2,  // SEEK_END
    data    = 3,  // SEEK_DATA (Linux)
    hole    = 4   // SEEK_HOLE (Linux)
};

// File type information
enum class file_type : uint8_t {
    unknown   = 0,
    regular   = 1,
    directory = 2,
    symlink   = 3,
    block     = 4,
    character = 5,
    fifo      = 6,
    socket    = 7
};

// File advice hints (for posix_fadvise)
enum class file_advice : uint8_t {
    normal     = 0,  // POSIX_FADV_NORMAL
    sequential = 1,  // POSIX_FADV_SEQUENTIAL
    random     = 2,  // POSIX_FADV_RANDOM
    no_reuse   = 3,  // POSIX_FADV_NOREUSE
    will_need  = 4,  // POSIX_FADV_WILLNEED
    dont_need  = 5   // POSIX_FADV_DONTNEED
};

// Sync flags
enum class sync_flags : uint8_t {
    data_only = 0,  // fdatasync
    full_sync = 1,  // fsync
    directory = 2   // sync directory entry
};

// Lock type
enum class lock_type : uint8_t {
    shared    = 0,  // F_RDLCK
    exclusive = 1,  // F_WRLCK
    unlock    = 2   // F_UNLCK
};

// Lock command
enum class lock_cmd : uint8_t {
    set_wait = 0,  // F_SETLKW - blocking
    set      = 1,  // F_SETLK - non-blocking
    get      = 2   // F_GETLK - test
};

// Flattened file handle (8 bytes)
struct file_handle {
    int32_t  fd;     // File descriptor
    uint32_t flags;  // Open flags for reference

    constexpr file_handle() noexcept: fd(-1), flags(0) {}

    constexpr file_handle(int32_t f, uint32_t fl = 0) noexcept: fd(f), flags(fl) {}

    constexpr bool is_valid() const noexcept { return fd >= 0; }

    constexpr explicit operator bool() const noexcept { return is_valid(); }
};

// Flattened file info structure (64 bytes)
struct file_info {
    uint64_t  size;         // File size in bytes
    uint64_t  blocks;       // Number of 512-byte blocks
    uint64_t  inode;        // Inode number
    uint64_t  device;       // Device ID
    int64_t   atime_sec;    // Access time (seconds)
    int64_t   mtime_sec;    // Modification time (seconds)
    int64_t   ctime_sec;    // Status change time (seconds)
    uint32_t  mode;         // File mode/permissions
    uint32_t  uid;          // User ID
    uint32_t  gid;          // Group ID
    uint32_t  nlink;        // Number of hard links
    file_type type;         // File type
    uint8_t   reserved[3];  // Padding
};

// Flattened file open request (16 bytes)
struct file_request {
    access_mode access;        // Access mode flags
    uint8_t     reserved1;     // Padding
    uint16_t    permissions;   // Mode for file creation (e.g., 0644)
    uint32_t    native_flags;  // Platform-specific flags
    uint64_t    reserved2;     // Future use/padding

    constexpr file_request() noexcept:
        access(access_mode::read_only),
        reserved1(0),
        permissions(0644),
        native_flags(0),
        reserved2(0) {}
};

// Flattened I/O request (32 bytes)
struct io_request {
    void*    buffer;    // Data buffer
    uint64_t offset;    // File offset (-1 for current position)
    size_t   length;    // Bytes to read/write
    uint32_t flags;     // Platform-specific flags
    uint32_t reserved;  // Padding

    constexpr io_request() noexcept:
        buffer(nullptr),
        offset(static_cast<uint64_t>(-1)),
        length(0),
        flags(0),
        reserved(0) {}
};

// Flattened I/O result (16 bytes)
struct io_result {
    size_t   bytes_transferred;  // Actual bytes read/written
    uint64_t new_offset;         // New file position

    constexpr io_result() noexcept: bytes_transferred(0), new_offset(0) {}

    constexpr io_result(size_t bytes, uint64_t offset) noexcept: bytes_transferred(bytes), new_offset(offset) {}
};

// Flattened file lock structure (32 bytes)
struct file_lock {
    lock_type type;       // Lock type
    lock_cmd  command;    // Lock command
    uint16_t  reserved1;  // Padding
    uint64_t  start;      // Starting offset
    uint64_t  length;     // Number of bytes (0 = to EOF)
    int32_t   pid;        // Process ID (for F_GETLK)
    uint32_t  reserved2;  // Padding

    constexpr file_lock() noexcept:
        type(lock_type::shared),
        command(lock_cmd::set),
        reserved1(0),
        start(0),
        length(0),
        pid(0),
        reserved2(0) {}
};

// Capability structure for platform features (32 bytes)
struct file_caps {
    bool supports_direct_io;      // O_DIRECT support
    bool supports_async_io;       // AIO support
    bool supports_splice;         // splice/sendfile support
    bool supports_fallocate;      // fallocate support
    bool supports_fadvise;        // posix_fadvise support
    bool supports_mmap;           // mmap support
    bool supports_lock;           // File locking support
    bool supports_extended_seek;  // SEEK_DATA/SEEK_HOLE

    uint64_t max_file_size;       // Maximum file size
    uint32_t max_open_files;      // Maximum open files
    uint32_t pipe_buffer_size;    // Default pipe buffer size

    uint8_t reserved[8];          // Future use

    constexpr file_caps() noexcept:
        supports_direct_io(false),
        supports_async_io(false),
        supports_splice(false),
        supports_fallocate(false),
        supports_fadvise(false),
        supports_mmap(true),
        supports_lock(true),
        supports_extended_seek(false),
        max_file_size(0),
        max_open_files(0),
        pipe_buffer_size(0),
        reserved {} {}
};

// Platform interface functions

// File operations
expected<file_handle, file_error> open_file(const char* path, const file_request& request) noexcept;
expected<file_handle, file_error> create_temp(const char* dir_path, const file_request& request) noexcept;
void                              close_file(file_handle& handle) noexcept;

// File information
expected<file_info, file_error> stat_file(const file_handle& handle) noexcept;
expected<file_info, file_error> stat_path(const char* path, bool follow_symlinks = true) noexcept;
expected<uint64_t, file_error>  get_file_size(const file_handle& handle) noexcept;

// I/O operations
expected<io_result, file_error> read_file(const file_handle& handle, const io_request& request) noexcept;
expected<io_result, file_error> write_file(const file_handle& handle, const io_request& request) noexcept;
expected<io_result, file_error>
  read_vectored(const file_handle& handle, const io_request* requests, size_t count) noexcept;
expected<io_result, file_error>
  write_vectored(const file_handle& handle, const io_request* requests, size_t count) noexcept;

// File positioning
expected<uint64_t, file_error> seek_file(const file_handle& handle, int64_t offset, seek_origin origin) noexcept;
expected<uint64_t, file_error> tell_file(const file_handle& handle) noexcept;

// File synchronization
expected<void, file_error> sync_file(const file_handle& handle, sync_flags flags = sync_flags::full_sync) noexcept;
expected<void, file_error>
  sync_range(const file_handle& handle, uint64_t offset, uint64_t length, sync_flags flags) noexcept;

// File manipulation
expected<void, file_error> truncate_file(const file_handle& handle, uint64_t size) noexcept;
expected<void, file_error> allocate_file(const file_handle& handle, uint64_t offset, uint64_t length) noexcept;
expected<void, file_error> deallocate_file(const file_handle& handle, uint64_t offset, uint64_t length) noexcept;

// File locking
expected<void, file_error>      lock_file(const file_handle& handle, const file_lock& lock) noexcept;
expected<file_lock, file_error> test_lock(const file_handle& handle, const file_lock& lock) noexcept;

// File advice
expected<void, file_error>
  advise_file(const file_handle& handle, uint64_t offset, uint64_t length, file_advice advice) noexcept;

// Zero-copy operations (Linux specific)
expected<size_t, file_error> splice_files(
  const file_handle& in,
  uint64_t*          in_offset,
  const file_handle& out,
  uint64_t*          out_offset,
  size_t             length,
  uint32_t           flags = 0) noexcept;
expected<size_t, file_error>
  sendfile_op(const file_handle& out, const file_handle& in, uint64_t* offset, size_t count) noexcept;

// Capability query
file_caps query_file_caps() noexcept;

}  // namespace platform::file

#endif
