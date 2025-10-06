#include <cassert>
#include <cstring>
#include <iostream>
#include <platform/process.hpp>
#include <string>
#include <vector>

using namespace platform::process;

// Helper function to describe error
const char* error_to_string(error_code code) {
    switch(code) {
    case error_code::success           : return "success";
    case error_code::io_error          : return "io error";
    case error_code::invalid_argument  : return "invalid argument";
    case error_code::no_memory         : return "no memory";
    case error_code::permission_denied : return "permission denied";
    case error_code::not_found         : return "not found";
    case error_code::already_exists    : return "already exists";
    case error_code::too_many_processes: return "too many processes";
    case error_code::would_block       : return "would block";
    case error_code::interrupted       : return "interrupted";
    case error_code::broken_pipe       : return "broken pipe";
    case error_code::process_not_found : return "process not found";
    case error_code::process_terminated: return "process terminated";
    case error_code::not_supported     : return "not supported";
    case error_code::platform_specific : return "platform specific";
    default                            : return "unknown error";
    }
}

int main() {
    std::cout << "Testing cross-platform process module...\n\n";

    // Query capabilities
    auto caps = query_process_caps();
    std::cout << "Platform process capabilities:\n";
    std::cout << "Pipes supported: " << (caps.supports_pipes ? "yes" : "no") << "\n";
    std::cout << "Detach supported: " << (caps.supports_detach ? "yes" : "no") << "\n";
    std::cout << "Process groups supported: " << (caps.supports_process_groups ? "yes" : "no") << "\n";
    std::cout << "PATH search supported: " << (caps.supports_search_path ? "yes" : "no") << "\n";
    std::cout << "\n";

    // Test 1: Simple echo command
    std::cout << "Test 1: Simple echo command\n";
    {
        const char* args[] = {"/bin/echo", "Hello from child process", nullptr};

        spawn_request req {};
        req.executable  = "/bin/echo";
        req.args        = args;
        req.stdout_mode = pipe_mode::pipe;

        pipe_handle    stdin_pipe, stdout_pipe, stderr_pipe;
        auto           result = spawn_process(req, &stdin_pipe, &stdout_pipe, &stderr_pipe);

        if(!result) {
            std::cout << "  FAILED: Could not spawn process - " << error_to_string(result.error().code) << "\n\n";
            return 1;
        }

        auto handle = result.value();
        std::cout << "  Process spawned with PID: " << handle.pid << "\n";

        // Read output
        char   buffer[256] = {};
        size_t total_read  = 0;

        while(true) {
            io_request io_req {};
            io_req.buffer = buffer + total_read;
            io_req.length = sizeof(buffer) - total_read - 1;

            auto read_result = read_pipe(stdout_pipe, io_req);
            if(!read_result) {
                if(read_result.error().code == error_code::would_block) {
                    // No data available, try again
                    continue;
                }
                break;
            }

            total_read += read_result.value().bytes_transferred;
            if(read_result.value().bytes_transferred == 0) break;  // EOF
        }

        std::cout << "  Output: " << buffer;

        // Wait for process to finish
        auto exit_result = wait_process(handle, false);
        if(exit_result) {
            std::cout << "  Exit code: " << exit_result.value() << "\n";
            std::cout << "  PASSED\n\n";
        } else {
            std::cout << "  FAILED: Could not wait for process\n\n";
            return 1;
        }

        close_pipe(stdout_pipe);
    }

    // Test 2: Bidirectional communication with cat
    std::cout << "Test 2: Bidirectional communication with cat\n";
    {
        const char* args[] = {"/bin/cat", nullptr};

        spawn_request req {};
        req.executable  = "/bin/cat";
        req.args        = args;
        req.stdin_mode  = pipe_mode::pipe;
        req.stdout_mode = pipe_mode::pipe;

        pipe_handle stdin_pipe, stdout_pipe, stderr_pipe;
        auto        result = spawn_process(req, &stdin_pipe, &stdout_pipe, &stderr_pipe);

        if(!result) {
            std::cout << "  FAILED: Could not spawn process\n\n";
            return 1;
        }

        auto handle = result.value();
        std::cout << "  Process spawned with PID: " << handle.pid << "\n";

        // Write to stdin
        const char* input   = "Hello, cat!\n";
        size_t      written = 0;
        while(written < strlen(input)) {
            io_request io_req {};
            io_req.buffer = const_cast<char*>(input + written);
            io_req.length = strlen(input) - written;

            auto write_result = write_pipe(stdin_pipe, io_req);
            if(!write_result) {
                if(write_result.error().code == error_code::would_block) { continue; }
                std::cout << "  FAILED: Could not write to stdin\n\n";
                return 1;
            }
            written += write_result.value().bytes_transferred;
        }

        // Close stdin to signal EOF
        close_pipe(stdin_pipe);

        // Read output
        char   buffer[256] = {};
        size_t total_read  = 0;

        while(true) {
            io_request io_req {};
            io_req.buffer = buffer + total_read;
            io_req.length = sizeof(buffer) - total_read - 1;

            auto read_result = read_pipe(stdout_pipe, io_req);
            if(!read_result) {
                if(read_result.error().code == error_code::would_block) { continue; }
                break;
            }

            total_read += read_result.value().bytes_transferred;
            if(read_result.value().bytes_transferred == 0) break;  // EOF
        }

        std::cout << "  Output: " << buffer;

        // Wait for process
        auto exit_result = wait_process(handle, false);
        if(exit_result) {
            std::cout << "  Exit code: " << exit_result.value() << "\n";
            if(strcmp(buffer, input) == 0) {
                std::cout << "  PASSED\n\n";
            } else {
                std::cout << "  FAILED: Output doesn't match input\n\n";
                return 1;
            }
        } else {
            std::cout << "  FAILED: Could not wait for process\n\n";
            return 1;
        }

        close_pipe(stdout_pipe);
    }

    // Test 3: Environment variables
    std::cout << "Test 3: Environment variables\n";
    {
        const char* args[] = {"/usr/bin/env", nullptr};
        const char* env[]  = {"TEST_VAR=hello", "ANOTHER_VAR=world", nullptr};

        spawn_request req {};
        req.executable  = "/usr/bin/env";
        req.args        = args;
        req.env         = env;
        req.stdout_mode = pipe_mode::pipe;

        pipe_handle stdin_pipe, stdout_pipe, stderr_pipe;
        auto        result = spawn_process(req, &stdin_pipe, &stdout_pipe, &stderr_pipe);

        if(!result) {
            std::cout << "  FAILED: Could not spawn process\n\n";
            return 1;
        }

        auto handle = result.value();
        std::cout << "  Process spawned with PID: " << handle.pid << "\n";

        // Read output
        char   buffer[1024] = {};
        size_t total_read   = 0;

        while(true) {
            io_request io_req {};
            io_req.buffer = buffer + total_read;
            io_req.length = sizeof(buffer) - total_read - 1;

            auto read_result = read_pipe(stdout_pipe, io_req);
            if(!read_result) {
                if(read_result.error().code == error_code::would_block) { continue; }
                break;
            }

            total_read += read_result.value().bytes_transferred;
            if(read_result.value().bytes_transferred == 0) break;  // EOF
        }

        std::cout << "  Environment output contains TEST_VAR: "
                  << (strstr(buffer, "TEST_VAR=hello") != nullptr ? "yes" : "no") << "\n";

        auto exit_result = wait_process(handle, false);
        if(exit_result && strstr(buffer, "TEST_VAR=hello")) {
            std::cout << "  PASSED\n\n";
        } else {
            std::cout << "  FAILED\n\n";
            return 1;
        }

        close_pipe(stdout_pipe);
    }

    // Test 4: Working directory
    std::cout << "Test 4: Working directory\n";
    {
        const char* args[] = {"/bin/pwd", nullptr};

        spawn_request req {};
        req.executable  = "/bin/pwd";
        req.args        = args;
        req.working_dir = "/tmp";
        req.stdout_mode = pipe_mode::pipe;

        pipe_handle stdin_pipe, stdout_pipe, stderr_pipe;
        auto        result = spawn_process(req, &stdin_pipe, &stdout_pipe, &stderr_pipe);

        if(!result) {
            std::cout << "  FAILED: Could not spawn process\n\n";
            return 1;
        }

        auto handle = result.value();

        // Read output
        char   buffer[256] = {};
        size_t total_read  = 0;

        while(true) {
            io_request io_req {};
            io_req.buffer = buffer + total_read;
            io_req.length = sizeof(buffer) - total_read - 1;

            auto read_result = read_pipe(stdout_pipe, io_req);
            if(!read_result) {
                if(read_result.error().code == error_code::would_block) { continue; }
                break;
            }

            total_read += read_result.value().bytes_transferred;
            if(read_result.value().bytes_transferred == 0) break;  // EOF
        }

        std::cout << "  Working directory: " << buffer;

        auto exit_result = wait_process(handle, false);
        if(exit_result && strstr(buffer, "/tmp")) {
            std::cout << "  PASSED\n\n";
        } else {
            std::cout << "  FAILED\n\n";
            return 1;
        }

        close_pipe(stdout_pipe);
    }

    std::cout << "All tests completed successfully!\n";
    return 0;
}
