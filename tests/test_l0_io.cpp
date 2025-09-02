#include <asyncle/io/l0_interface.hpp>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>

using namespace asyncle::io;

int main() {
    std::cout << "Testing L0 I/O interface...\n";

    // Test capability query
    auto caps = l0_query_caps();
    std::cout << "System page size: " << caps.system_page_size << " bytes\n";
    std::cout << "Large pages supported: " << (caps.supports_large_pages ? "yes" : "no") << "\n";
    std::cout << "Memory locking supported: " << (caps.supports_memory_lock ? "yes" : "no") << "\n";
    std::cout << "Anonymous mapping supported: " << (caps.supports_anonymous ? "yes" : "no") << "\n";

    // Test anonymous mapping
    {
        io_request request;
        request.length  = 4096;  // One page
        request.backing = backing_type::anonymous;
        request.access  = access_mode::read_write;
        request.sharing = sharing_mode::private_cow;

        auto result = l0_map(-1, request);
        if(result) {
            const auto& region = *result;
            std::cout << "Anonymous mapping successful at: " << region.address << " (size: " << region.length << ")\n";

            // Test writing and reading
            char* data = static_cast<char*>(region.address);
            strcpy(data, "Hello L0 Interface!");

            std::cout << "Written and read back: " << data << "\n";

            // Test memory advice
            auto advice_result = l0_advise(region, access_pattern::random_access);
            if(advice_result) { std::cout << "Memory advice applied successfully\n"; }

            // Unmap the region
            l0_unmap(region);
            std::cout << "Anonymous mapping unmapped successfully\n";
        } else {
            std::cout << "Anonymous mapping failed: " << result.error().message() << "\n";
        }
    }

    // Test file-backed mapping
    {
        // Create a temporary file
        const char* filename = "/tmp/test_l0_io.dat";
        FILE*       file     = fopen(filename, "w+");
        if(!file) {
            std::cout << "Failed to create test file\n";
            return 1;
        }

        // Write some test data
        const char* test_data = "This is test data for file-backed mapping!";
        fwrite(test_data, 1, strlen(test_data), file);
        fflush(file);

        int fd = fileno(file);

        io_request request;
        request.length  = 4096;  // One page (larger than file for testing)
        request.offset  = 0;
        request.backing = backing_type::file_backed;
        request.access  = access_mode::read;
        request.sharing = sharing_mode::shared;

        auto result = l0_map(fd, request);
        if(result) {
            const auto& region = *result;
            std::cout << "File-backed mapping successful at: " << region.address << " (size: " << region.length << ")\n";

            // Read the mapped data
            const char* mapped_data = static_cast<const char*>(region.address);
            std::cout << "File content: " << std::string(mapped_data, strlen(test_data)) << "\n";

            // Test sync operation
            auto sync_result = l0_sync(region, false);
            if(sync_result) {
                std::cout << "File sync successful\n";
            } else {
                std::cout << "File sync failed: " << sync_result.error().message() << "\n";
            }

            // Unmap the region
            l0_unmap(region);
            std::cout << "File-backed mapping unmapped successfully\n";
        } else {
            std::cout << "File-backed mapping failed: " << result.error().message() << "\n";
        }

        fclose(file);
        unlink(filename);
    }

    // Test capability features if available
    if(caps.supports_memory_lock) {
        std::cout << "\nTesting memory locking...\n";

        io_request request;
        request.length  = 4096;
        request.backing = backing_type::anonymous;
        request.access  = access_mode::read_write;
        request.locking = locking_strategy::lock_resident;

        auto result = l0_map(-1, request);
        if(result) {
            const auto& region = *result;
            std::cout << "Memory-locked mapping created, locked: " << (region.is_locked ? "yes" : "no") << "\n";

            l0_unmap(region);
        }
    }

    std::cout << "\nAll L0 I/O interface tests completed!\n";
    return 0;
}
