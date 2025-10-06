#ifndef PLATFORM_PROCESS_HPP
#define PLATFORM_PROCESS_HPP

#include "../cxx23_compat/expected.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace platform::process {

// Import expected types for cleaner usage
using cxx23_compat::expected;
using cxx23_compat::unexpect;

// Forward declarations
struct process_handle;
struct pipe_handle;
struct process_caps;
struct spawn_request;
struct process_error;
struct io_request;
struct io_result;

// Flattened error system - consistent with file/mmap modules
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
    not_found         = 5,
    already_exists    = 6,
    too_many_processes = 7,
    would_block       = 8,
    interrupted       = 9,
    broken_pipe       = 10,
    process_not_found = 11,
    process_terminated = 12,
    not_supported     = 200,
    platform_specific = 201
};

// Flattened error structure (4 bytes)
struct process_error {
    error_domain domain;          // 1 byte
    uint8_t      platform_errno;  // 1 byte
    error_code   code;            // 2 bytes

    constexpr process_error() noexcept: domain(error_domain::system), platform_errno(0), code(error_code::success) {}

    constexpr process_error(error_code c) noexcept: domain(error_domain::system), platform_errno(0), code(c) {}

    constexpr process_error(error_domain d, error_code c, uint8_t e = 0) noexcept: domain(d), platform_errno(e), code(c) {}
};

// Pipe modes
enum class pipe_mode : uint8_t {
    none      = 0,  // No pipe (inherit from parent or redirect to /dev/null)
    pipe      = 1,  // Create pipe for bidirectional communication
    inherit   = 2   // Inherit from parent process
};

// Process creation flags
enum class spawn_flags : uint32_t {
    none              = 0x00,
    new_process_group = 0x01,  // Create new process group
    detached          = 0x02,  // Detach from parent
    search_path       = 0x04   // Search PATH for executable
};

// Flattened pipe handle (8 bytes)
struct pipe_handle {
    int32_t  fd;     // File descriptor (POSIX) or HANDLE (Windows)
    uint32_t flags;  // Internal flags

    constexpr pipe_handle() noexcept: fd(-1), flags(0) {}

    constexpr pipe_handle(int32_t f, uint32_t fl = 0) noexcept: fd(f), flags(fl) {}

    constexpr bool is_valid() const noexcept { return fd >= 0; }

    constexpr explicit operator bool() const noexcept { return is_valid(); }
};

// Flattened process handle (16 bytes)
struct process_handle {
    int32_t  pid;        // Process ID
    uint32_t flags;      // Creation flags
    int32_t  exit_code;  // Exit code (valid after wait)
    uint32_t state;      // Internal state flags

    constexpr process_handle() noexcept: pid(-1), flags(0), exit_code(-1), state(0) {}

    constexpr process_handle(int32_t p, uint32_t f = 0) noexcept: pid(p), flags(f), exit_code(-1), state(0) {}

    constexpr bool is_valid() const noexcept { return pid > 0; }

    constexpr explicit operator bool() const noexcept { return is_valid(); }
};

// Process spawn request (96 bytes - aligned)
struct spawn_request {
    const char* executable;           // Path to executable
    const char* const* args;          // Argument array (NULL-terminated)
    const char* const* env;           // Environment array (NULL-terminated, nullptr = inherit)
    const char* working_dir;          // Working directory (nullptr = inherit)
    pipe_mode   stdin_mode;           // stdin pipe configuration
    pipe_mode   stdout_mode;          // stdout pipe configuration
    pipe_mode   stderr_mode;          // stderr pipe configuration
    uint8_t     _padding[5];          // Padding for alignment
    spawn_flags flags;                // Creation flags
    uint32_t    _reserved[16];        // Reserved for future use

    constexpr spawn_request() noexcept
        : executable(nullptr), args(nullptr), env(nullptr), working_dir(nullptr), stdin_mode(pipe_mode::inherit),
          stdout_mode(pipe_mode::inherit), stderr_mode(pipe_mode::inherit), _padding {}, flags(spawn_flags::none),
          _reserved {} {}
};

// I/O request structure (32 bytes - same as file module)
struct io_request {
    void*    buffer;   // Buffer pointer
    size_t   length;   // Number of bytes to transfer
    uint64_t timeout;  // Timeout in milliseconds (0 = non-blocking, -1 = infinite)
    uint32_t flags;    // Operation flags
    uint32_t _padding; // Padding for alignment

    constexpr io_request() noexcept: buffer(nullptr), length(0), timeout(static_cast<uint64_t>(-1)), flags(0), _padding(0) {}
};

// I/O result structure (16 bytes - same as file module)
struct io_result {
    size_t   bytes_transferred;  // Number of bytes actually transferred
    uint32_t operation_flags;    // Flags describing the operation result
    uint32_t _padding;           // Padding for alignment

    constexpr io_result() noexcept: bytes_transferred(0), operation_flags(0), _padding(0) {}
};

// Platform capabilities (16 bytes)
struct process_caps {
    bool supports_pipes;           // Platform supports pipe creation
    bool supports_detach;          // Platform supports process detachment
    bool supports_process_groups;  // Platform supports process groups
    bool supports_search_path;     // Platform supports PATH search
    uint8_t  _padding[12];         // Reserved for future capabilities

    constexpr process_caps() noexcept
        : supports_pipes(false), supports_detach(false), supports_process_groups(false), supports_search_path(false),
          _padding {} {}
};

// Core process operations - to be implemented per platform
expected<process_handle, process_error> spawn_process(const spawn_request& request, pipe_handle* stdin_pipe,
                                                       pipe_handle* stdout_pipe, pipe_handle* stderr_pipe) noexcept;

expected<int, process_error> wait_process(process_handle& handle, bool no_hang = false) noexcept;

expected<void, process_error> kill_process(const process_handle& handle, int signal) noexcept;

expected<void, process_error> terminate_process(const process_handle& handle) noexcept;

// Pipe I/O operations
expected<io_result, process_error> read_pipe(const pipe_handle& pipe, const io_request& request) noexcept;

expected<io_result, process_error> write_pipe(const pipe_handle& pipe, const io_request& request) noexcept;

expected<void, process_error> close_pipe(pipe_handle& pipe) noexcept;

// Capability query
process_caps query_process_caps() noexcept;

}  // namespace platform::process

// Include platform-specific implementation
#if defined(__linux__)
#include "process_linux.hpp"
#elif defined(_WIN32)
#include "process_windows.hpp"
#else
#error "Unsupported platform"
#endif

#endif  // PLATFORM_PROCESS_HPP
