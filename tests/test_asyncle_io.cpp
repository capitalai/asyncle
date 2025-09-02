#include <asyncle/io/file.hpp>
#include <asyncle/io/mmap.hpp>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

using namespace asyncle::io;

int main() {
    std::cout << "Testing redesigned asyncle I/O modules...\n\n";

    // Test file module
    std::cout << "=== Testing File Module ===\n";
    {
        // Test 1: Create file with simple constructor
        const char* test_file = "/tmp/test_asyncle_io.dat";
        {
            file_request req {};
            req.access      = static_cast<access_mode>(static_cast<uint8_t>(access_mode::write_only) | static_cast<uint8_t>(access_mode::create) | static_cast<uint8_t>(access_mode::truncate));
            req.permissions = 0644;

            file f(test_file, req);
            assert(f.is_open());
            std::cout << "File opened with request: fd=" << f.fd() << "\n";

            // Write data
            const char* data   = "Hello from redesigned file module!";
            auto        result = f.write(data, strlen(data));
            assert(result.has_value());
            std::cout << "Written " << result.value() << " bytes\n";

            // Get file size
            auto size = f.size();
            assert(size.has_value());
            assert(size.value() == strlen(data));
            std::cout << "File size: " << size.value() << " bytes\n";
        }

        // Test 2: Read file with simple constructor
        {
            file f(test_file, access_mode::read_only);
            assert(f.is_open());
            std::cout << "File opened for reading: fd=" << f.fd() << "\n";

            char buffer[100] = { 0 };
            auto result      = f.read(buffer, sizeof(buffer) - 1);
            assert(result.has_value());
            std::cout << "Read " << result.value() << " bytes: '" << buffer << "'\n";
        }

        // Test 3: Use file request for advanced options
        {
            file_request req {};
            req.access = static_cast<access_mode>(static_cast<uint8_t>(access_mode::read_write) | static_cast<uint8_t>(access_mode::direct));
            file f;
            auto result = f.open(test_file, req);
            if(!result.has_value()) {
                std::cout << "Direct I/O not supported (expected on some systems)\n";
            } else {
                std::cout << "Opened with direct I/O: fd=" << f.fd() << "\n";
            }
        }

        // Test 4: Temporary file
        {
            file f;
            auto result = f.create_temp();
            assert(result.has_value());
            std::cout << "Created temporary file: fd=" << f.fd() << "\n";

            const char* temp_data = "Temporary data";
            f.write(temp_data, strlen(temp_data));

            // Seek and tell
            auto pos = f.seek(5, seek_origin::begin);
            assert(pos.has_value());
            assert(pos.value() == 5);
            std::cout << "Seeked to position: " << pos.value() << "\n";

            auto tell_pos = f.tell();
            assert(tell_pos.has_value());
            assert(tell_pos.value() == 5);
        }

        // Test 5: File info
        {
            auto info = file::stat(test_file);
            assert(info.has_value());
            std::cout << "File info - size: " << info.value().size << ", type: " << static_cast<int>(info.value().type) << "\n";
        }

        // Test 6: Capabilities
        auto caps = file::capabilities();
        std::cout << "File capabilities - splice: " << (caps.supports_splice ? "yes" : "no")
                  << ", direct_io: " << (caps.supports_direct_io ? "yes" : "no") << "\n";

        // Cleanup
        std::remove(test_file);
    }

    // Test mmap module
    std::cout << "\n=== Testing Mmap Module ===\n";
    {
        // Test 1: Anonymous mapping with simple constructor
        {
            size_t size = 4096;
            mmap   m(size);
            assert(m.is_mapped());
            std::cout << "Anonymous mapping created: size=" << m.size() << ", addr=" << m.data() << "\n";

            // Write and read data
            char* data = m.as<char>();
            strcpy(data, "Hello from mmap!");
            std::cout << "Written and read: '" << data << "'\n";

            // Test advise
            auto advise_result = m.advise(access_pattern::sequential_access);
            if(advise_result.has_value()) {
                std::cout << "Memory advice applied\n";
            }
        }

        // Test 2: File-backed mapping
        {
            const char* mmap_file = "/tmp/test_mmap.dat";

            // Create a file with content
            {
                file_request req {};
                req.access = static_cast<access_mode>(static_cast<uint8_t>(access_mode::write_only) | static_cast<uint8_t>(access_mode::create) | static_cast<uint8_t>(access_mode::truncate));
                file f(mmap_file, req);

                const char* content = "This is mapped file content for testing!";
                f.write(content, strlen(content));
            }

            // Map the file
            {
                file f(mmap_file, access_mode::read_only);
                assert(f.is_open());

                auto size_result = f.size();
                assert(size_result.has_value());
                size_t file_size = size_result.value();

                mmap m(f, file_size);
                assert(m.is_mapped());
                assert(m.is_file_backed());
                std::cout << "File-backed mapping: size=" << m.size() << ", fd=" << m.fd() << "\n";

                const char* mapped_content = m.as<const char>();
                std::cout << "Mapped content: '" << std::string(mapped_content, file_size) << "'\n";
            }

            // Test 3: Map with custom request
            {
                file f(mmap_file, access_mode::read_write);
                memory_request req {};
                req.length     = mmap::align_to_page(100);
                req.offset     = 0;
                req.backing    = backing_type::file_backed;
                req.access     = mmap_access::access_mode::read_write;
                req.sharing    = sharing_mode::shared;
                req.placement  = placement_strategy::any_address;

                mmap m(f, req);
                assert(m.is_mapped());
                std::cout << "Mapped with custom request: size=" << m.size() << "\n";

                // Modify content
                char* data = m.as<char>();
                data[0]    = 'X';

                // Sync changes
                auto sync_result = m.sync();
                assert(sync_result.has_value());
                std::cout << "Changes synced to file\n";
            }

            // Verify changes
            {
                file f(mmap_file, access_mode::read_only);
                char buffer[10];
                f.read(buffer, sizeof(buffer));
                assert(buffer[0] == 'X');
                std::cout << "Verified file modification through mmap\n";
            }

            std::remove(mmap_file);
        }

        // Test 4: Large anonymous mapping with request
        {
            memory_request req {};
            req.length     = 1024 * 1024;  // 1MB
            req.backing    = backing_type::anonymous;
            req.access     = mmap_access::access_mode::read_write;
            req.sharing    = sharing_mode::private_cow;
            req.locking    = locking_strategy::no_lock;
            req.placement  = placement_strategy::any_address;

            mmap m(req);
            assert(m.is_mapped());
            assert(m.is_anonymous());
            std::cout << "Large anonymous mapping: size=" << m.size() << "\n";

            // Test typed access
            int* int_data = m.at<int>(100);
            if(int_data) {
                *int_data = 42;
                assert(*m.at<int>(100) == 42);
                std::cout << "Typed access successful\n";
            }
        }

        // Test 5: Capabilities
        auto caps = mmap::capabilities();
        std::cout << "Mmap capabilities - page_size: " << caps.system_page_size
                  << ", large_pages: " << (caps.supports_large_pages ? "yes" : "no")
                  << ", memory_lock: " << (caps.supports_memory_lock ? "yes" : "no") << "\n";

        // Test 6: Page alignment
        size_t unaligned = 1234;
        size_t aligned   = mmap::align_to_page(unaligned);
        std::cout << "Page alignment: " << unaligned << " -> " << aligned << "\n";
        assert(aligned >= unaligned);
        assert(aligned % mmap::page_size() == 0);
    }

    std::cout << "\n=== All Tests Passed ===\n";
    return 0;
}