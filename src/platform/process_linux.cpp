#include "../../include/platform/process.hpp"

#ifdef __linux__
#include "../../include/platform/process_linux.hpp"

namespace platform::process {

// Import expected types for cleaner usage
using cxx23_compat::expected;
using cxx23_compat::unexpect;

// Linux implementation of the cross-platform process interface
// These functions delegate to the platform-specific implementations

expected<process_handle, process_error> spawn_process(const spawn_request& request, pipe_handle* stdin_pipe,
                                                        pipe_handle* stdout_pipe, pipe_handle* stderr_pipe) noexcept {
    return linux_impl::spawn_process_impl(request, stdin_pipe, stdout_pipe, stderr_pipe);
}

expected<int, process_error> wait_process(process_handle& handle, bool no_hang) noexcept {
    return linux_impl::wait_process_impl(handle, no_hang);
}

expected<void, process_error> kill_process(const process_handle& handle, int signal) noexcept {
    return linux_impl::kill_process_impl(handle, signal);
}

expected<void, process_error> terminate_process(const process_handle& handle) noexcept {
    return linux_impl::terminate_process_impl(handle);
}

expected<io_result, process_error> read_pipe(const pipe_handle& pipe, const io_request& request) noexcept {
    return linux_impl::read_pipe_impl(pipe, request);
}

expected<io_result, process_error> write_pipe(const pipe_handle& pipe, const io_request& request) noexcept {
    return linux_impl::write_pipe_impl(pipe, request);
}

expected<void, process_error> close_pipe(pipe_handle& pipe) noexcept { return linux_impl::close_pipe_impl(pipe); }

process_caps query_process_caps() noexcept { return linux_impl::query_process_caps_impl(); }

}  // namespace platform::process

// Linux implementation details
namespace platform::process::linux_impl {

using detail::make_system_error;
using detail::set_cloexec;
using detail::set_nonblocking;

expected<process_handle, process_error> spawn_process_impl(const spawn_request& request, pipe_handle* stdin_pipe,
                                                             pipe_handle* stdout_pipe,
                                                             pipe_handle* stderr_pipe) noexcept {
    if(!request.executable) {
        return expected<process_handle, process_error>(unexpect, process_error(error_code::invalid_argument));
    }

    // Create pipes if requested
    int stdin_fds[2]  = {-1, -1};
    int stdout_fds[2] = {-1, -1};
    int stderr_fds[2] = {-1, -1};

    if(request.stdin_mode == pipe_mode::pipe) {
        if(pipe(stdin_fds) != 0) {
            return expected<process_handle, process_error>(unexpect, make_system_error(errno));
        }
        set_cloexec(stdin_fds[1]);  // Parent write end
    }

    if(request.stdout_mode == pipe_mode::pipe) {
        if(pipe(stdout_fds) != 0) {
            if(stdin_fds[0] != -1) {
                ::close(stdin_fds[0]);
                ::close(stdin_fds[1]);
            }
            return expected<process_handle, process_error>(unexpect, make_system_error(errno));
        }
        set_cloexec(stdout_fds[0]);  // Parent read end
    }

    if(request.stderr_mode == pipe_mode::pipe) {
        if(pipe(stderr_fds) != 0) {
            if(stdin_fds[0] != -1) {
                ::close(stdin_fds[0]);
                ::close(stdin_fds[1]);
            }
            if(stdout_fds[0] != -1) {
                ::close(stdout_fds[0]);
                ::close(stdout_fds[1]);
            }
            return expected<process_handle, process_error>(unexpect, make_system_error(errno));
        }
        set_cloexec(stderr_fds[0]);  // Parent read end
    }

    // Fork process
    pid_t pid = fork();

    if(pid < 0) {
        // Fork failed
        int err = errno;
        if(stdin_fds[0] != -1) {
            ::close(stdin_fds[0]);
            ::close(stdin_fds[1]);
        }
        if(stdout_fds[0] != -1) {
            ::close(stdout_fds[0]);
            ::close(stdout_fds[1]);
        }
        if(stderr_fds[0] != -1) {
            ::close(stderr_fds[0]);
            ::close(stderr_fds[1]);
        }
        return expected<process_handle, process_error>(unexpect, make_system_error(err));
    }

    if(pid == 0) {
        // Child process
        // Redirect stdin
        if(request.stdin_mode == pipe_mode::pipe) {
            ::close(stdin_fds[1]);  // Close write end
            if(dup2(stdin_fds[0], STDIN_FILENO) == -1) { _exit(127); }
            ::close(stdin_fds[0]);
        } else if(request.stdin_mode == pipe_mode::none) {
            int null_fd = open("/dev/null", O_RDONLY);
            if(null_fd != -1) {
                dup2(null_fd, STDIN_FILENO);
                ::close(null_fd);
            }
        }

        // Redirect stdout
        if(request.stdout_mode == pipe_mode::pipe) {
            ::close(stdout_fds[0]);  // Close read end
            if(dup2(stdout_fds[1], STDOUT_FILENO) == -1) { _exit(127); }
            ::close(stdout_fds[1]);
        } else if(request.stdout_mode == pipe_mode::none) {
            int null_fd = open("/dev/null", O_WRONLY);
            if(null_fd != -1) {
                dup2(null_fd, STDOUT_FILENO);
                ::close(null_fd);
            }
        }

        // Redirect stderr
        if(request.stderr_mode == pipe_mode::pipe) {
            ::close(stderr_fds[0]);  // Close read end
            if(dup2(stderr_fds[1], STDERR_FILENO) == -1) { _exit(127); }
            ::close(stderr_fds[1]);
        } else if(request.stderr_mode == pipe_mode::none) {
            int null_fd = open("/dev/null", O_WRONLY);
            if(null_fd != -1) {
                dup2(null_fd, STDERR_FILENO);
                ::close(null_fd);
            }
        }

        // Change working directory if requested
        if(request.working_dir) {
            if(chdir(request.working_dir) != 0) { _exit(127); }
        }

        // Create new process group if requested
        if(static_cast<uint32_t>(request.flags) & static_cast<uint32_t>(spawn_flags::new_process_group)) {
            setpgid(0, 0);
        }

        // Execute process
        if(request.env) {
            execve(request.executable, const_cast<char* const*>(request.args), const_cast<char* const*>(request.env));
        } else {
            execv(request.executable, const_cast<char* const*>(request.args));
        }

        // If we reach here, exec failed
        _exit(127);
    }

    // Parent process
    // Close child ends of pipes
    if(stdin_fds[0] != -1) { ::close(stdin_fds[0]); }
    if(stdout_fds[1] != -1) { ::close(stdout_fds[1]); }
    if(stderr_fds[1] != -1) { ::close(stderr_fds[1]); }

    // Set parent pipe ends to non-blocking
    if(stdin_fds[1] != -1) { set_nonblocking(stdin_fds[1]); }
    if(stdout_fds[0] != -1) { set_nonblocking(stdout_fds[0]); }
    if(stderr_fds[0] != -1) { set_nonblocking(stderr_fds[0]); }

    // Return handles
    if(stdin_pipe && stdin_fds[1] != -1) { *stdin_pipe = pipe_handle(stdin_fds[1]); }
    if(stdout_pipe && stdout_fds[0] != -1) { *stdout_pipe = pipe_handle(stdout_fds[0]); }
    if(stderr_pipe && stderr_fds[0] != -1) { *stderr_pipe = pipe_handle(stderr_fds[0]); }

    return expected<process_handle, process_error>(process_handle(pid, static_cast<uint32_t>(request.flags)));
}

expected<int, process_error> wait_process_impl(process_handle& handle, bool no_hang) noexcept {
    if(!handle.is_valid()) {
        return expected<int, process_error>(unexpect, process_error(error_code::invalid_argument));
    }

    int   status    = 0;
    int   wait_opts = no_hang ? WNOHANG : 0;
    pid_t result    = waitpid(handle.pid, &status, wait_opts);

    if(result < 0) { return expected<int, process_error>(unexpect, make_system_error(errno)); }

    if(result == 0) {
        // Process still running (WNOHANG)
        return expected<int, process_error>(unexpect, process_error(error_code::would_block));
    }

    // Process terminated
    int exit_code = -1;
    if(WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if(WIFSIGNALED(status)) {
        exit_code = 128 + WTERMSIG(status);
    }

    handle.exit_code = exit_code;
    handle.state     = 1;  // Mark as terminated

    return expected<int, process_error>(exit_code);
}

expected<void, process_error> kill_process_impl(const process_handle& handle, int signal) noexcept {
    if(!handle.is_valid()) { return expected<void, process_error>(unexpect, process_error(error_code::invalid_argument)); }

    if(::kill(handle.pid, signal) != 0) { return expected<void, process_error>(unexpect, make_system_error(errno)); }

    return expected<void, process_error>();
}

expected<void, process_error> terminate_process_impl(const process_handle& handle) noexcept {
    return kill_process_impl(handle, SIGTERM);
}

expected<io_result, process_error> read_pipe_impl(const pipe_handle& pipe, const io_request& request) noexcept {
    if(!pipe.is_valid() || !request.buffer || request.length == 0) {
        return expected<io_result, process_error>(unexpect, process_error(error_code::invalid_argument));
    }

    ssize_t result = read(pipe.fd, request.buffer, request.length);

    if(result < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return expected<io_result, process_error>(unexpect, process_error(error_code::would_block));
        }
        return expected<io_result, process_error>(unexpect, make_system_error(errno));
    }

    io_result res;
    res.bytes_transferred = static_cast<size_t>(result);
    return expected<io_result, process_error>(res);
}

expected<io_result, process_error> write_pipe_impl(const pipe_handle& pipe, const io_request& request) noexcept {
    if(!pipe.is_valid() || !request.buffer || request.length == 0) {
        return expected<io_result, process_error>(unexpect, process_error(error_code::invalid_argument));
    }

    ssize_t result = write(pipe.fd, request.buffer, request.length);

    if(result < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return expected<io_result, process_error>(unexpect, process_error(error_code::would_block));
        }
        return expected<io_result, process_error>(unexpect, make_system_error(errno));
    }

    io_result res;
    res.bytes_transferred = static_cast<size_t>(result);
    return expected<io_result, process_error>(res);
}

expected<void, process_error> close_pipe_impl(pipe_handle& pipe) noexcept {
    if(!pipe.is_valid()) { return expected<void, process_error>(); }

    if(::close(pipe.fd) != 0) { return expected<void, process_error>(unexpect, make_system_error(errno)); }

    pipe.fd = -1;
    return expected<void, process_error>();
}

process_caps query_process_caps_impl() noexcept {
    process_caps caps;
    caps.supports_pipes          = true;
    caps.supports_detach         = true;
    caps.supports_process_groups = true;
    caps.supports_search_path    = false;  // execv doesn't search PATH, use execvp for that
    return caps;
}

}  // namespace platform::process::linux_impl

#endif  // __linux__
