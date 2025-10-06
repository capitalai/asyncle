# Asyncle I/O Modules

Asyncle provides cross-platform I/O modules with a two-layer architecture: a thin platform abstraction layer and RAII wrapper classes that provide a clean, modern C++ interface.

## Design Philosophy

The I/O modules follow these principles:

1. **Thin platform abstraction** - Direct mapping to OS primitives (no heavy abstractions)
2. **RAII resource management** - Automatic cleanup, move-only semantics
3. **No async runtime dependency** - Synchronous operations that can be wrapped by any async framework
4. **Consistent error handling** - All modules use `expected<T, error>` for error reporting
5. **Cross-platform** - Clean separation between interface and platform-specific implementation

## Architecture

```
include/
├── platform/              # Platform abstraction layer
│   ├── file.hpp           # File I/O interface
│   ├── file_linux.hpp     # Linux implementation
│   ├── mmap.hpp           # Memory mapping interface
│   ├── mmap_linux.hpp     # Linux implementation
│   ├── process.hpp        # Process management interface
│   └── process_linux.hpp  # Linux implementation
│
├── asyncle/io/            # RAII wrapper layer
│   ├── file.hpp           # File RAII class
│   ├── mmap.hpp           # Memory mapping RAII class
│   └── process.hpp        # Process management RAII class
│
└── src/platform/          # Compiled implementations
    ├── file_linux.cpp
    ├── mmap_linux.cpp
    └── process_linux.cpp
```

## Module Overview

### File I/O (`asyncle::io::file`)

Cross-platform file operations with support for:
- Basic I/O: read, write, seek, truncate
- Vectored I/O: readv, writev
- Zero-copy: splice, sendfile
- File locking: advisory locks
- Synchronization: fsync, fdatasync
- Advice hints: fadvise, fallocate

**Quick Example:**
```cpp
#include <asyncle/io/file.hpp>

using namespace asyncle::io;

// Open file for reading
file f("data.txt", access_mode::read_only);
if (!f) {
    // Handle error
    return;
}

// Read data
char buffer[4096];
auto result = f.read(buffer, sizeof(buffer));
if (result) {
    size_t bytes_read = result.value();
    // Process data...
}

// File automatically closed on destruction
```

### Memory Mapping (`asyncle::io::mmap`)

Memory-mapped file and anonymous memory with:
- File-backed and anonymous mappings
- Shared and private (copy-on-write) modes
- Memory locking and prefetching
- Access pattern hints (sequential, random)
- Synchronization control

**Quick Example:**
```cpp
#include <asyncle/io/file.hpp>
#include <asyncle/io/mmap.hpp>

using namespace asyncle::io;

// Open file
file f("large_data.bin", access_mode::read_only);

// Map entire file into memory
auto size_result = f.size();
if (!size_result) return;

mmap mapping(f, size_result.value());
if (!mapping) return;

// Access mapped data
const auto* data = mapping.as<uint8_t>();
// Process data directly from memory...

// Hint access pattern for better performance
mapping.advise(access_pattern::sequential);

// Mapping automatically unmapped on destruction
```

### Process Management (`asyncle::io::process`)

Subprocess spawning and bidirectional communication with:
- Full stdin/stdout/stderr pipe control
- Environment variable customization
- Working directory control
- Process groups and signals
- Non-blocking I/O on all pipes

**Quick Example:**
```cpp
#include <asyncle/io/process.hpp>

using namespace asyncle::io;

// Spawn process with pipes
const char* args[] = {"/bin/cat", nullptr};
process proc("/bin/cat", args);

if (!proc) {
    // Handle spawn error
    return;
}

// Write to stdin
const char* input = "Hello, process!\n";
auto write_result = proc.write_stdin(input, strlen(input));

// Close stdin to signal EOF
proc.close_stdin();

// Read from stdout
char buffer[1024];
auto read_result = proc.read_stdout(buffer, sizeof(buffer));
if (read_result) {
    size_t bytes_read = read_result.value();
    // Process output...
}

// Wait for process to exit
auto exit_code = proc.wait();
```

## Error Handling

All I/O operations return `expected<T, error_type>` where:
- **Success**: Contains the result value (e.g., `size_t` bytes transferred)
- **Failure**: Contains structured error with domain, code, and platform errno

```cpp
auto result = file.read(buffer, size);

if (!result) {
    auto err = result.error();

    // Check error domain
    if (err.domain == error_domain::system) {
        // System error - check code
        if (err.code == error_code::would_block) {
            // Handle non-blocking case
        }
        // Platform errno available in err.platform_errno
    }
}
```

## Platform Layer

The `platform::*` namespace provides low-level functions that map directly to OS primitives:

```cpp
#include <platform/file.hpp>

using namespace platform::file;

// Low-level file operations
file_request req{};
req.access = access_mode::read_write;
req.permissions = 0644;

auto result = open_file("test.txt", req);
if (result) {
    file_handle handle = result.value();

    io_request io_req{};
    io_req.buffer = buffer;
    io_req.length = size;

    auto read_result = read_file(handle, io_req);

    close_file(handle);
}
```

Most users should use the RAII wrappers (`asyncle::io::*`) instead.

## Advanced Usage

### File with Custom Options

```cpp
using namespace asyncle::io;

file_request req{};
req.access = access_mode::read_write;
req.create_mode = create_mode::create_or_open;
req.permissions = 0644;
req.flags = file_flags::direct_io;  // O_DIRECT on Linux

file f("data.bin", req);
```

### Memory Mapping with Control

```cpp
using namespace asyncle::io;

memory_request req{};
req.length = 1024 * 1024;  // 1MB
req.backing = backing_type::anonymous;
req.access = mmap_access::access_mode::read_write;
req.sharing = sharing_mode::private_cow;
req.placement = placement_strategy::anywhere;

mmap mapping(req);
```

### Process with Environment

```cpp
using namespace asyncle::io;

const char* args[] = {"/usr/bin/env", nullptr};
const char* env[] = {
    "PATH=/usr/bin:/bin",
    "MY_VAR=value",
    nullptr
};

spawn_request req{};
req.executable = "/usr/bin/env";
req.args = args;
req.env = env;
req.working_dir = "/tmp";
req.stdin_mode = pipe_mode::pipe;
req.stdout_mode = pipe_mode::pipe;
req.stderr_mode = pipe_mode::pipe;

process proc(req);
```

## Capability Queries

Each module provides capability queries to detect platform features:

```cpp
// File capabilities
auto file_caps = file::capabilities();
if (file_caps.supports_splice) {
    // Use zero-copy splice
}

// Memory mapping capabilities
auto mmap_caps = mmap::capabilities();
std::cout << "Page size: " << mmap_caps.system_page_size << "\n";

// Process capabilities
auto proc_caps = process::capabilities();
if (proc_caps.supports_process_groups) {
    // Use process groups
}
```

## Integration with Async Frameworks

These modules are designed to be wrapped by async frameworks:

```cpp
// Pseudo-code for async wrapper
template<typename Executor>
class async_file {
    asyncle::io::file file_;
    Executor executor_;

public:
    auto async_read(char* buf, size_t len) {
        // Schedule read on executor
        return executor_.schedule([&]() {
            return file_.read(buf, len);
        });
    }
};
```

The synchronous nature allows any async runtime to integrate without coupling.

## Testing

Each module has comprehensive platform-level tests:

```bash
# Build and run tests
cmake -B build
cmake --build build

# Run I/O tests
./build/test_platform_file
./build/test_platform_mmap
./build/test_platform_process
./build/test_asyncle_io

# Or via ctest
ctest --test-dir build -R "platform\.(file|mmap|process)"
ctest --test-dir build -R "asyncle\.io"
```

## Platform Support

Currently implemented:
- ✅ Linux (file, mmap, process)

Planned:
- 🔄 Windows (file, mmap, process)
- 🔄 macOS (file, mmap, process)

## Performance Considerations

### File I/O
- Use vectored I/O (`readv`/`writev`) for scattered data
- Use `splice`/`sendfile` for zero-copy transfers
- Set `O_DIRECT` flag to bypass page cache for large sequential I/O
- Use `fadvise` hints to optimize kernel behavior

### Memory Mapping
- Align sizes to page boundaries for efficiency
- Use `prefetch` for predictable access patterns
- Lock pages with `mlock` for latency-sensitive code
- Choose appropriate sharing mode (shared vs private)

### Process Management
- Pipes are set to non-blocking mode automatically
- Close unused pipe ends to prevent deadlocks
- Use `waitpid` with `WNOHANG` for non-blocking status checks

## Related Modules

- **Core async framework**: See [README.md](../README.md) for the async-ready data-flow system
- **C++23 features**: Uses `std::expected`-compatible types
- **Platform abstractions**: All platform code isolated in `platform/` namespace

## License

Same as main Asyncle project - see [LICENSE](../LICENSE)
