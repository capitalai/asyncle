#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <platform/mmap.hpp>
#ifdef __unix__
#include <unistd.h>
#endif

using namespace platform::mmap;

// Helper function to describe error
const char* error_to_string(error_code code) {
    switch(code) {
    case error_code::success                  : return "success";
    case error_code::invalid_argument         : return "invalid argument";
    case error_code::no_memory                : return "no memory";
    case error_code::permission_denied        : return "permission denied";
    case error_code::file_not_found           : return "file not found";
    case error_code::device_busy              : return "device busy";
    case error_code::io_error                 : return "I/O error";
    case error_code::no_such_device           : return "no such device";
    case error_code::address_in_use           : return "address in use";
    case error_code::bad_address              : return "bad address";
    case error_code::not_supported            : return "not supported";
    case error_code::large_pages_unavailable  : return "large pages unavailable";
    case error_code::sync_not_supported       : return "sync not supported";
    case error_code::lock_on_fault_unavailable: return "lock on fault unavailable";
    case error_code::fixed_address_unavailable: return "fixed address unavailable";
    default                                   : return "unknown error";
    }
}

int main() {
    std::cout << "Testing cross-platform mmap library...\n";

    // Test capability query
    auto caps = query_capabilities();
    std::cout << "System page size: " << caps.system_page_size << " bytes\n";
    std::cout << "Large pages supported: " << (caps.supports_large_pages ? "yes" : "no") << "\n";
    std::cout << "Memory locking supported: " << (caps.supports_memory_lock ? "yes" : "no") << "\n";
    std::cout << "Anonymous mapping supported: " << (caps.supports_anonymous ? "yes" : "no") << "\n";
    std::cout << "Fixed no-replace supported: " << (caps.supports_fixed_no_replace ? "yes" : "no") << "\n";
    std::cout << "Durable sync supported: " << (caps.supports_durable_sync ? "yes" : "no") << "\n";

    // Test flattened error system
    {
        memory_error err1(error_code::invalid_argument);
        memory_error err2(error_domain::feature, error_code::large_pages_unavailable, 22);

        std::cout << "\nFlattened error system test:\n";
        std::cout
          << "Error 1: domain=" << static_cast<int>(err1.domain) << ", code=" << static_cast<int>(err1.code)
          << ", errno=" << static_cast<int>(err1.platform_errno) << "\n";
        std::cout
          << "Error 2: domain=" << static_cast<int>(err2.domain) << ", code=" << static_cast<int>(err2.code)
          << ", errno=" << static_cast<int>(err2.platform_errno) << "\n";
        std::cout << "Error size: " << sizeof(memory_error) << " bytes (should be 4)\n";
    }

    // Test anonymous mapping
    {
        std::cout << "\nTesting anonymous mapping...\n";

        memory_request request;
        request.length  = 4096;  // One page
        request.backing = backing_type::anonymous;
        request.access  = access_mode::read_write;
        request.sharing = sharing_mode::private_cow;

        auto result = map_memory(-1, request);
        if(result) {
            const auto& region = *result;
            std::cout << "Anonymous mapping successful at: " << region.address << " (size: " << region.length << ")\n";

            // Test writing and reading
            char* data = static_cast<char*>(region.address);
            strcpy(data, "Hello Platform Mmap!");

            std::cout << "Written and read back: " << data << "\n";

            // Test memory advice
            auto advice_result = advise_memory(region, access_pattern::random_access);
            if(advice_result) {
                std::cout << "Memory advice applied successfully\n";
            } else {
                std::cout << "Memory advice failed: " << error_to_string(advice_result.error().code) << "\n";
            }

            // Unmap the region
            unmap_memory(region);
            std::cout << "Anonymous mapping unmapped successfully\n";
        } else {
            std::cout << "Anonymous mapping failed: " << error_to_string(result.error().code) << "\n";
        }
    }

    // Test file-backed mapping
    {
        std::cout << "\nTesting file-backed mapping...\n";

        // Create a temporary file using a cross-platform approach
#ifdef _WIN32
        const char* filename = "test_platform_mmap.dat";
#else
        const char* filename = "/tmp/test_platform_mmap.dat";
#endif
        FILE* file = fopen(filename, "w+b");
        if(!file) {
            std::cout << "Failed to create test file\n";
            return 1;
        }

        // Write some test data
        const char* test_data = "This is test data for cross-platform mmap!";
        fwrite(test_data, 1, strlen(test_data), file);
        fflush(file);

#ifdef _WIN32
        int fd = _fileno(file);
#else
        int fd = fileno(file);
#endif

        memory_request request;
        request.length  = 4096;  // One page (larger than file for testing)
        request.offset  = 0;
        request.backing = backing_type::file_backed;
        request.access  = access_mode::read;
        request.sharing = sharing_mode::shared;

        auto result = map_memory(fd, request);
        if(result) {
            const auto& region = *result;
            std::cout << "File-backed mapping successful at: " << region.address << " (size: " << region.length << ")\n";

            // Read the mapped data
            const char* mapped_data = static_cast<const char*>(region.address);
            std::cout << "File content: " << std::string(mapped_data, strlen(test_data)) << "\n";

            // Test sync operation
            auto sync_result = sync_memory(region, false);
            if(sync_result) {
                std::cout << "File sync successful\n";
            } else {
                std::cout << "File sync failed: " << error_to_string(sync_result.error().code) << "\n";
            }

            // Test prefetch operation
            auto prefetch_result = prefetch_memory(region, 0, region.length);
            if(prefetch_result) {
                std::cout << "Memory prefetch successful\n";
            } else {
                std::cout << "Memory prefetch failed: " << error_to_string(prefetch_result.error().code) << "\n";
            }

            // Unmap the region
            unmap_memory(region);
            std::cout << "File-backed mapping unmapped successfully\n";
        } else {
            std::cout << "File-backed mapping failed: " << error_to_string(result.error().code) << "\n";
        }

        fclose(file);
#ifdef __unix__
        unlink(filename);
#else
        std::remove(filename);
#endif
    }

    // Test memory locking if supported
    if(caps.supports_memory_lock) {
        std::cout << "\nTesting memory locking...\n";

        memory_request request;
        request.length  = 4096;
        request.backing = backing_type::anonymous;
        request.access  = access_mode::read_write;
        request.locking = locking_strategy::lock_resident;

        auto result = map_memory(-1, request);
        if(result) {
            const auto& region = *result;
            std::cout << "Memory-locked mapping created, locked: " << (region.is_locked ? "yes" : "no") << "\n";

            // Test manual locking
            auto lock_result = lock_memory(region, locking_strategy::lock_resident);
            if(lock_result) {
                std::cout << "Manual memory locking successful\n";

                // Test unlocking
                auto unlock_result = unlock_memory(region);
                if(unlock_result) {
                    std::cout << "Memory unlocking successful\n";
                } else {
                    std::cout << "Memory unlocking failed: " << error_to_string(unlock_result.error().code) << "\n";
                }
            } else {
                std::cout << "Manual memory locking failed: " << error_to_string(lock_result.error().code) << "\n";
            }

            unmap_memory(region);
        }
    }

    // Test large pages if supported
    if(caps.supports_large_pages) {
        std::cout << "\nTesting large page support...\n";
        std::cout << "Supported large page sizes: ";
        for(std::size_t i = 0; i < 8 && caps.large_page_sizes[i] != 0; ++i) {
            if(i > 0) std::cout << ", ";
            std::cout << (caps.large_page_sizes[i] / (1024 * 1024)) << "MB";
        }
        std::cout << "\n";

        memory_request request;
        request.length          = 2 * 1024 * 1024;  // 2MB
        request.backing         = backing_type::anonymous;
        request.access          = access_mode::read_write;
        request.page_pref       = page_preference::prefer_large;
        request.large_page_size = 2 * 1024 * 1024;

        auto result = map_memory(-1, request);
        if(result) {
            const auto& region = *result;
            std::cout << "Large page mapping attempt successful, actual pages: ";
            switch(region.actual_pages) {
            case page_preference::system_default: std::cout << "system default\n"; break;
            case page_preference::prefer_large  : std::cout << "large pages (preferred)\n"; break;
            case page_preference::require_large : std::cout << "large pages (required)\n"; break;
            }

            unmap_memory(region);
        } else {
            std::cout << "Large page mapping failed: " << error_to_string(result.error().code) << "\n";
        }
    }

    std::cout << "\nAll cross-platform mmap tests completed!\n";
    return 0;
}
