#ifndef PLATFORM_PROCESS_LINUX_HPP
#define PLATFORM_PROCESS_LINUX_HPP

#include "process.hpp"

// Platform-specific includes
#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace platform::process::linux_impl {

// Import expected types for cleaner usage
using cxx23_compat::expected;
using cxx23_compat::unexpect;

// Linux-specific implementation details
namespace detail {

// Convert errno to process_error
constexpr process_error make_system_error(int errno_val) noexcept {
    error_code code = error_code::io_error;

    switch(errno_val) {
    case EACCES : code = error_code::permission_denied; break;
    case ENOENT : code = error_code::not_found; break;
    case ENOMEM : code = error_code::no_memory; break;
    case EAGAIN : code = error_code::would_block; break;
    case EINTR  : code = error_code::interrupted; break;
    case EPIPE  : code = error_code::broken_pipe; break;
    case ECHILD : code = error_code::process_not_found; break;
    case EINVAL : code = error_code::invalid_argument; break;
    case E2BIG  : code = error_code::invalid_argument; break;
    case ELOOP  : code = error_code::io_error; break;
    case ENFILE : code = error_code::too_many_processes; break;
    case EMFILE : code = error_code::too_many_processes; break;
    default     : code = error_code::io_error; break;
    }

    return process_error(error_domain::system, code, static_cast<uint8_t>(errno_val));
}

// Set file descriptor to non-blocking mode
inline bool set_nonblocking(int fd) noexcept {
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

// Set close-on-exec flag
inline bool set_cloexec(int fd) noexcept {
    int flags = fcntl(fd, F_GETFD, 0);
    if(flags == -1) return false;
    return fcntl(fd, F_SETFD, flags | FD_CLOEXEC) != -1;
}

}  // namespace detail

// Implementation functions
expected<process_handle, process_error> spawn_process_impl(const spawn_request& request, pipe_handle* stdin_pipe,
                                                             pipe_handle* stdout_pipe,
                                                             pipe_handle* stderr_pipe) noexcept;

expected<int, process_error> wait_process_impl(process_handle& handle, bool no_hang) noexcept;

expected<void, process_error> kill_process_impl(const process_handle& handle, int signal) noexcept;

expected<void, process_error> terminate_process_impl(const process_handle& handle) noexcept;

expected<io_result, process_error> read_pipe_impl(const pipe_handle& pipe, const io_request& request) noexcept;

expected<io_result, process_error> write_pipe_impl(const pipe_handle& pipe, const io_request& request) noexcept;

expected<void, process_error> close_pipe_impl(pipe_handle& pipe) noexcept;

process_caps query_process_caps_impl() noexcept;

}  // namespace platform::process::linux_impl

#endif  // __linux__

#endif  // PLATFORM_PROCESS_LINUX_HPP
