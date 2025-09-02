#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <platform/file.hpp>
#include <string>
#include <vector>

using namespace platform::file;

// Helper function to describe error
const char* error_to_string(error_code code) {
    switch(code) {
    case error_code::success: return "success";
    case error_code::io_error: return "io error";
    case error_code::invalid_argument: return "invalid argument";
    case error_code::no_memory: return "no memory";
    case error_code::permission_denied: return "permission denied";
    case error_code::file_not_found: return "file not found";
    case error_code::file_exists: return "file exists";
    case error_code::is_directory: return "is directory";
    case error_code::not_directory: return "not directory";
    case error_code::too_many_files: return "too many files";
    case error_code::file_too_large: return "file too large";
    case error_code::no_space: return "no space";
    case error_code::invalid_seek: return "invalid seek";
    case error_code::read_only_fs: return "read only filesystem";
    case error_code::broken_pipe: return "broken pipe";
    case error_code::would_block: return "would block";
    case error_code::interrupted: return "interrupted";
    case error_code::not_supported: return "not supported";
    case error_code::platform_specific: return "platform specific";
    default: return "unknown error";
    }
}

int main() {
    std::cout << "Testing cross-platform file module...\n\n";
    
    // Query capabilities
    auto caps = query_file_caps();
    std::cout << "Platform file capabilities:\n";
    std::cout << "Direct I/O supported: " << (caps.supports_direct_io ? "yes" : "no") << "\n";
    std::cout << "Async I/O supported: " << (caps.supports_async_io ? "yes" : "no") << "\n";
    std::cout << "Splice supported: " << (caps.supports_splice ? "yes" : "no") << "\n";
    std::cout << "Fallocate supported: " << (caps.supports_fallocate ? "yes" : "no") << "\n";
    std::cout << "Fadvise supported: " << (caps.supports_fadvise ? "yes" : "no") << "\n";
    std::cout << "Mmap supported: " << (caps.supports_mmap ? "yes" : "no") << "\n";
    std::cout << "File locking supported: " << (caps.supports_lock ? "yes" : "no") << "\n";
    std::cout << "Extended seek supported: " << (caps.supports_extended_seek ? "yes" : "no") << "\n";
    std::cout << "Max open files: " << caps.max_open_files << "\n";
    std::cout << "\n";
    
    // Test error system
    std::cout << "Flattened error system test:\n";
    file_error err1(error_code::invalid_argument);
    file_error err2(error_domain::platform, error_code::platform_specific, 22);
    std::cout << "Error 1: domain=" << static_cast<int>(err1.domain) 
              << ", code=" << static_cast<int>(err1.code) 
              << ", errno=" << static_cast<int>(err1.platform_errno) << "\n";
    std::cout << "Error 2: domain=" << static_cast<int>(err2.domain) 
              << ", code=" << static_cast<int>(err2.code) 
              << ", errno=" << static_cast<int>(err2.platform_errno) << "\n";
    std::cout << "Error size: " << sizeof(file_error) << " bytes (should be 4)\n";
    assert(sizeof(file_error) == 4);
    std::cout << "\n";
    
    // Test file creation and writing
    std::cout << "Testing file creation and I/O...\n";
    {
        const char* test_filename = "/tmp/test_file.dat";
        
        // Open file for writing (create if not exists)
        file_request req;
        req.access = static_cast<access_mode>(
            static_cast<uint8_t>(access_mode::write_only) | 
            static_cast<uint8_t>(access_mode::create) |
            static_cast<uint8_t>(access_mode::truncate));
        req.permissions = 0644;
        
        auto open_result = open_file(test_filename, req);
        if(open_result) {
            file_handle handle = open_result.value();
            std::cout << "File opened for writing: fd=" << handle.fd << "\n";
            
            // Write data
            const char* test_data = "Hello Platform File Module!";
            io_request write_req;
            write_req.buffer = const_cast<char*>(test_data);
            write_req.length = strlen(test_data);
            write_req.offset = static_cast<uint64_t>(-1); // Current position
            
            auto write_result = write_file(handle, write_req);
            if(write_result) {
                std::cout << "Written " << write_result.value().bytes_transferred << " bytes\n";
                
                // Sync file
                auto sync_result = sync_file(handle, sync_flags::full_sync);
                if(sync_result) {
                    std::cout << "File synced successfully\n";
                } else {
                    std::cout << "Sync failed: " << error_to_string(sync_result.error().code) << "\n";
                }
            } else {
                std::cout << "Write failed: " << error_to_string(write_result.error().code) << "\n";
            }
            
            // Get file info
            auto stat_result = stat_file(handle);
            if(stat_result) {
                file_info info = stat_result.value();
                std::cout << "File size: " << info.size << " bytes\n";
                std::cout << "File type: " << static_cast<int>(info.type) << "\n";
                std::cout << "Permissions: " << std::oct << (info.mode & 0777) << std::dec << "\n";
            }
            
            // Close file
            close_file(handle);
            std::cout << "File closed\n";
            
            // Now open for reading
            req.access = access_mode::read_only;
            auto read_open = open_file(test_filename, req);
            if(read_open) {
                handle = read_open.value();
                std::cout << "\nFile opened for reading: fd=" << handle.fd << "\n";
                
                // Read data
                char read_buffer[100] = {0};
                io_request read_req;
                read_req.buffer = read_buffer;
                read_req.length = sizeof(read_buffer) - 1;
                read_req.offset = 0; // Read from beginning
                
                auto read_result = read_file(handle, read_req);
                if(read_result) {
                    std::cout << "Read " << read_result.value().bytes_transferred << " bytes: '" 
                              << read_buffer << "'\n";
                    assert(strcmp(read_buffer, test_data) == 0);
                } else {
                    std::cout << "Read failed: " << error_to_string(read_result.error().code) << "\n";
                }
                
                // Test seek and tell
                auto seek_result = seek_file(handle, 6, seek_origin::begin);
                if(seek_result) {
                    std::cout << "Seeked to position: " << seek_result.value() << "\n";
                    
                    auto tell_result = tell_file(handle);
                    if(tell_result) {
                        std::cout << "Current position: " << tell_result.value() << "\n";
                        assert(tell_result.value() == 6);
                    }
                }
                
                close_file(handle);
            }
            
            // Clean up test file
            std::remove(test_filename);
        } else {
            std::cout << "Failed to open file for writing: " 
                      << error_to_string(open_result.error().code) << "\n";
        }
    }
    
    // Test temporary file creation
    std::cout << "\nTesting temporary file creation...\n";
    {
        file_request req;
        req.access = access_mode::read_write;
        
        auto temp_result = create_temp(nullptr, req);
        if(temp_result) {
            file_handle handle = temp_result.value();
            std::cout << "Temporary file created: fd=" << handle.fd << "\n";
            
            // Write and read from temp file
            const char* temp_data = "Temporary data";
            io_request write_req;
            write_req.buffer = const_cast<char*>(temp_data);
            write_req.length = strlen(temp_data);
            write_req.offset = 0;
            
            auto write_result = write_file(handle, write_req);
            if(write_result) {
                std::cout << "Written to temp file: " << write_result.value().bytes_transferred << " bytes\n";
                
                // Read back
                char temp_buffer[100] = {0};
                io_request read_req;
                read_req.buffer = temp_buffer;
                read_req.length = sizeof(temp_buffer) - 1;
                read_req.offset = 0;
                
                auto read_result = read_file(handle, read_req);
                if(read_result) {
                    std::cout << "Read from temp file: '" << temp_buffer << "'\n";
                    assert(strcmp(temp_buffer, temp_data) == 0);
                }
            }
            
            close_file(handle);
            std::cout << "Temporary file closed\n";
        } else {
            std::cout << "Failed to create temporary file: " 
                      << error_to_string(temp_result.error().code) << "\n";
        }
    }
    
    // Test vectored I/O
    std::cout << "\nTesting vectored I/O...\n";
    {
        const char* vec_filename = "/tmp/test_vectored.dat";
        
        file_request req;
        req.access = static_cast<access_mode>(
            static_cast<uint8_t>(access_mode::read_write) | 
            static_cast<uint8_t>(access_mode::create) |
            static_cast<uint8_t>(access_mode::truncate));
        req.permissions = 0644;
        
        auto open_result = open_file(vec_filename, req);
        if(open_result) {
            file_handle handle = open_result.value();
            
            // Prepare multiple buffers for vectored write
            const char* buf1 = "First ";
            const char* buf2 = "Second ";
            const char* buf3 = "Third";
            
            io_request requests[3];
            requests[0].buffer = const_cast<char*>(buf1);
            requests[0].length = strlen(buf1);
            requests[1].buffer = const_cast<char*>(buf2);
            requests[1].length = strlen(buf2);
            requests[2].buffer = const_cast<char*>(buf3);
            requests[2].length = strlen(buf3);
            
            auto writev_result = write_vectored(handle, requests, 3);
            if(writev_result) {
                std::cout << "Vectored write: " << writev_result.value().bytes_transferred << " bytes\n";
                
                // Read back with single read
                char read_buffer[100] = {0};
                io_request read_req;
                read_req.buffer = read_buffer;
                read_req.length = sizeof(read_buffer) - 1;
                read_req.offset = 0;
                
                auto read_result = read_file(handle, read_req);
                if(read_result) {
                    std::cout << "Read back: '" << read_buffer << "'\n";
                    assert(strcmp(read_buffer, "First Second Third") == 0);
                }
            } else {
                std::cout << "Vectored write failed: " << error_to_string(writev_result.error().code) << "\n";
            }
            
            close_file(handle);
            std::remove(vec_filename);
        }
    }
    
    // Test file truncation
    std::cout << "\nTesting file truncation...\n";
    {
        const char* trunc_filename = "/tmp/test_truncate.dat";
        
        file_request req;
        req.access = static_cast<access_mode>(
            static_cast<uint8_t>(access_mode::read_write) | 
            static_cast<uint8_t>(access_mode::create) |
            static_cast<uint8_t>(access_mode::truncate));
        
        auto open_result = open_file(trunc_filename, req);
        if(open_result) {
            file_handle handle = open_result.value();
            
            // Write initial data
            const char* initial_data = "This is a longer string that will be truncated";
            io_request write_req;
            write_req.buffer = const_cast<char*>(initial_data);
            write_req.length = strlen(initial_data);
            write_req.offset = 0;
            
            write_file(handle, write_req);
            
            // Get initial size
            auto size_result = get_file_size(handle);
            if(size_result) {
                std::cout << "Initial file size: " << size_result.value() << " bytes\n";
                
                // Truncate to 10 bytes
                auto trunc_result = truncate_file(handle, 10);
                if(trunc_result) {
                    std::cout << "File truncated to 10 bytes\n";
                    
                    auto new_size = get_file_size(handle);
                    if(new_size) {
                        std::cout << "New file size: " << new_size.value() << " bytes\n";
                        assert(new_size.value() == 10);
                    }
                } else {
                    std::cout << "Truncation failed: " << error_to_string(trunc_result.error().code) << "\n";
                }
            }
            
            close_file(handle);
            std::remove(trunc_filename);
        }
    }
    
    // Test file locking (if supported)
    if(caps.supports_lock) {
        std::cout << "\nTesting file locking...\n";
        const char* lock_filename = "/tmp/test_lock.dat";
        
        file_request req;
        req.access = static_cast<access_mode>(
            static_cast<uint8_t>(access_mode::read_write) | 
            static_cast<uint8_t>(access_mode::create));
        
        auto open_result = open_file(lock_filename, req);
        if(open_result) {
            file_handle handle = open_result.value();
            
            // Try to acquire an exclusive lock
            file_lock lock;
            lock.type = lock_type::exclusive;
            lock.command = lock_cmd::set;
            lock.start = 0;
            lock.length = 100;
            
            auto lock_result = lock_file(handle, lock);
            if(lock_result) {
                std::cout << "Exclusive lock acquired\n";
                
                // Test the lock
                file_lock test_lock_req = lock;
                test_lock_req.command = lock_cmd::get;
                auto test_result = test_lock(handle, test_lock_req);
                if(test_result) {
                    if(test_result.value().type == lock_type::unlock) {
                        std::cout << "Lock test: region is unlocked (available)\n";
                    } else {
                        std::cout << "Lock test: region is locked by PID " << test_result.value().pid << "\n";
                    }
                }
                
                // Release the lock
                lock.type = lock_type::unlock;
                lock_file(handle, lock);
                std::cout << "Lock released\n";
            } else {
                std::cout << "Failed to acquire lock: " << error_to_string(lock_result.error().code) << "\n";
            }
            
            close_file(handle);
            std::remove(lock_filename);
        }
    }
    
    std::cout << "\nAll cross-platform file tests completed!\n";
    return 0;
}