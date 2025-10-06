#ifndef ASYNCLE_IO_PROCESS_HPP
#define ASYNCLE_IO_PROCESS_HPP

#include "../../platform/process.hpp"
#include "result.hpp"
#include <memory>
#include <span>
#include <string_view>
#include <utility>

namespace asyncle::io {

// Import platform types
using platform::process::error_code;
using platform::process::error_domain;
using platform::process::expected;
using platform::process::io_request;
using platform::process::io_result;
using platform::process::pipe_handle;
using platform::process::pipe_mode;
using platform::process::process_caps;
using platform::process::process_error;
using platform::process::process_handle;
using platform::process::spawn_flags;
using platform::process::spawn_request;
using platform::process::unexpect;

// Standardized result types for process operations
template <typename T>
using process_result = result<T, process_error>;

using process_void_result = void_result<process_error>;

// Single RAII process class with full capabilities
class process {
    public:
    // Type aliases for result types and error handling
    using error_type = process_error;
    template <typename T>
    using result_type = process_result<T>;
    using void_result_type = process_void_result;

    private:
    process_handle handle_;
    pipe_handle    stdin_;
    pipe_handle    stdout_;
    pipe_handle    stderr_;

    public:
    // Constructors
    process() noexcept = default;

    // Spawn process with full request
    explicit process(const spawn_request& request) noexcept {
        pipe_handle stdin_pipe, stdout_pipe, stderr_pipe;
        auto        result = platform::process::spawn_process(request, &stdin_pipe, &stdout_pipe, &stderr_pipe);
        if(result) {
            handle_ = result.value();
            stdin_  = stdin_pipe;
            stdout_ = stdout_pipe;
            stderr_ = stderr_pipe;
        }
    }

    // Simple spawn with executable path and args
    process(const char* executable, const char* const* args, pipe_mode stdin_mode = pipe_mode::pipe,
            pipe_mode stdout_mode = pipe_mode::pipe, pipe_mode stderr_mode = pipe_mode::pipe) noexcept {
        spawn_request req {};
        req.executable  = executable;
        req.args        = args;
        req.stdin_mode  = stdin_mode;
        req.stdout_mode = stdout_mode;
        req.stderr_mode = stderr_mode;

        pipe_handle stdin_pipe, stdout_pipe, stderr_pipe;
        auto        result = platform::process::spawn_process(req, &stdin_pipe, &stdout_pipe, &stderr_pipe);
        if(result) {
            handle_ = result.value();
            stdin_  = stdin_pipe;
            stdout_ = stdout_pipe;
            stderr_ = stderr_pipe;
        }
    }

    // Create from existing handles
    process(process_handle h, pipe_handle in, pipe_handle out, pipe_handle err) noexcept
        : handle_(h), stdin_(in), stdout_(out), stderr_(err) {}

    // Move semantics
    process(process&& other) noexcept
        : handle_(std::exchange(other.handle_, process_handle {})), stdin_(std::exchange(other.stdin_, pipe_handle {})),
          stdout_(std::exchange(other.stdout_, pipe_handle {})), stderr_(std::exchange(other.stderr_, pipe_handle {})) {}

    process& operator=(process&& other) noexcept {
        if(this != &other) {
            close_pipes();
            handle_ = std::exchange(other.handle_, process_handle {});
            stdin_  = std::exchange(other.stdin_, pipe_handle {});
            stdout_ = std::exchange(other.stdout_, pipe_handle {});
            stderr_ = std::exchange(other.stderr_, pipe_handle {});
        }
        return *this;
    }

    // No copy
    process(const process&)            = delete;
    process& operator=(const process&) = delete;

    // Destructor
    ~process() { close_pipes(); }

    // Core operations
    process_result<process_handle> spawn(const spawn_request& request) noexcept {
        close_pipes();
        pipe_handle stdin_pipe, stdout_pipe, stderr_pipe;
        auto        result = platform::process::spawn_process(request, &stdin_pipe, &stdout_pipe, &stderr_pipe);
        if(result) {
            handle_ = result.value();
            stdin_  = stdin_pipe;
            stdout_ = stdout_pipe;
            stderr_ = stderr_pipe;
        }
        return result;
    }

    process_result<process_handle> spawn(const char* executable, const char* const* args,
                                                    pipe_mode stdin_mode  = pipe_mode::pipe,
                                                    pipe_mode stdout_mode = pipe_mode::pipe,
                                                    pipe_mode stderr_mode = pipe_mode::pipe) noexcept {
        spawn_request req {};
        req.executable  = executable;
        req.args        = args;
        req.stdin_mode  = stdin_mode;
        req.stdout_mode = stdout_mode;
        req.stderr_mode = stderr_mode;
        return spawn(req);
    }

    process_result<int> wait(bool no_hang = false) noexcept {
        if(!is_running()) { return process_result<int>(unexpect, process_error(error_code::invalid_argument)); }
        return platform::process::wait_process(handle_, no_hang);
    }

    process_void_result kill(int signal) noexcept {
        if(!is_running()) { return process_void_result(unexpect, process_error(error_code::invalid_argument)); }
        return platform::process::kill_process(handle_, signal);
    }

    process_void_result terminate() noexcept {
        if(!is_running()) { return process_void_result(unexpect, process_error(error_code::invalid_argument)); }
        return platform::process::terminate_process(handle_);
    }

    void close_pipes() noexcept {
        if(stdin_.is_valid()) {
            platform::process::close_pipe(stdin_);
            stdin_ = pipe_handle {};
        }
        if(stdout_.is_valid()) {
            platform::process::close_pipe(stdout_);
            stdout_ = pipe_handle {};
        }
        if(stderr_.is_valid()) {
            platform::process::close_pipe(stderr_);
            stderr_ = pipe_handle {};
        }
    }

    // I/O operations
    process_result<io_result> write_stdin(const io_request& request) noexcept {
        if(!stdin_.is_valid()) {
            return process_result<io_result>(unexpect, process_error(error_code::invalid_argument));
        }
        return platform::process::write_pipe(stdin_, request);
    }

    process_result<size_t> write_stdin(const void* buffer, size_t length) noexcept {
        io_request req {};
        req.buffer = const_cast<void*>(buffer);
        req.length = length;
        auto result = write_stdin(req);
        if(result) { return process_result<size_t>(result.value().bytes_transferred); }
        return process_result<size_t>(unexpect, result.error());
    }

    process_result<io_result> read_stdout(const io_request& request) noexcept {
        if(!stdout_.is_valid()) {
            return process_result<io_result>(unexpect, process_error(error_code::invalid_argument));
        }
        return platform::process::read_pipe(stdout_, request);
    }

    process_result<size_t> read_stdout(void* buffer, size_t length) noexcept {
        io_request req {};
        req.buffer = buffer;
        req.length = length;
        auto result = read_stdout(req);
        if(result) { return process_result<size_t>(result.value().bytes_transferred); }
        return process_result<size_t>(unexpect, result.error());
    }

    process_result<io_result> read_stderr(const io_request& request) noexcept {
        if(!stderr_.is_valid()) {
            return process_result<io_result>(unexpect, process_error(error_code::invalid_argument));
        }
        return platform::process::read_pipe(stderr_, request);
    }

    process_result<size_t> read_stderr(void* buffer, size_t length) noexcept {
        io_request req {};
        req.buffer = buffer;
        req.length = length;
        auto result = read_stderr(req);
        if(result) { return process_result<size_t>(result.value().bytes_transferred); }
        return process_result<size_t>(unexpect, result.error());
    }

    // Pipe management
    process_void_result close_stdin() noexcept {
        if(!stdin_.is_valid()) { return process_void_result(); }
        auto result = platform::process::close_pipe(stdin_);
        if(result) { stdin_ = pipe_handle {}; }
        return result;
    }

    process_void_result close_stdout() noexcept {
        if(!stdout_.is_valid()) { return process_void_result(); }
        auto result = platform::process::close_pipe(stdout_);
        if(result) { stdout_ = pipe_handle {}; }
        return result;
    }

    process_void_result close_stderr() noexcept {
        if(!stderr_.is_valid()) { return process_void_result(); }
        auto result = platform::process::close_pipe(stderr_);
        if(result) { stderr_ = pipe_handle {}; }
        return result;
    }

    // Static utilities
    static process_caps capabilities() noexcept { return platform::process::query_process_caps(); }

    // Accessors
    const process_handle& handle() const noexcept { return handle_; }
    process_handle&       handle() noexcept { return handle_; }
    bool                  is_running() const noexcept { return handle_.is_valid(); }
    explicit operator bool() const noexcept { return is_running(); }
    int pid() const noexcept { return handle_.pid; }
    int exit_code() const noexcept { return handle_.exit_code; }

    const pipe_handle& stdin_pipe() const noexcept { return stdin_; }
    const pipe_handle& stdout_pipe() const noexcept { return stdout_; }
    const pipe_handle& stderr_pipe() const noexcept { return stderr_; }

    bool has_stdin() const noexcept { return stdin_.is_valid(); }
    bool has_stdout() const noexcept { return stdout_.is_valid(); }
    bool has_stderr() const noexcept { return stderr_.is_valid(); }
};

}  // namespace asyncle::io

#endif  // ASYNCLE_IO_PROCESS_HPP
