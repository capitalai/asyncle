# Hardware Module Design Document

## Overview

The `asyncle::hardware` module provides low-level hardware abstractions for high-performance computing, focusing on CPU cache optimization, memory access patterns, and hardware-specific instructions.

## Design Philosophy

### Core Principles

1. **Header-only** - Maintain asyncle's zero-dependency philosophy
2. **Compile-time optimization** - Prefer `constexpr` and templates where possible
3. **Platform abstraction** - Unified interface with platform-specific implementations
4. **Graceful degradation** - Fall back to standard implementations when hardware features unavailable
5. **Zero overhead** - Abstractions should compile to optimal machine code

### Non-Goals

- Not a full SIMD library (use dedicated libraries for complex SIMD)
- Not a replacement for compiler intrinsics (provide convenient wrappers)
- Not runtime CPU dispatching for multiple architectures (single compile target)

## Module Structure

```
asyncle/
├── include/
│   └── asyncle/
│       └── hardware/
│           ├── memory.hpp          # Cache alignment, prefetch, barriers (Phase 1)
│           ├── capabilities.hpp    # Runtime feature detection (Phase 2)
│           ├── simd.hpp           # SIMD abstractions (Phase 3)
│           └── wait.hpp           # umonitor/umwait (Phase 3)
├── src/
│   └── hardware/
│       ├── memory_linux.cpp       # Platform-specific implementations
│       ├── memory_windows.cpp
│       └── memory_macos.cpp
└── tests/
    └── test_hardware_memory.cpp
```

## Phase 1: Memory Module (Current Priority)

### 1.1 Cache Line Abstraction

#### Requirements

- Provide compile-time cache line size constant
- Support runtime cache line detection
- Offer alignment and padding utilities
- Prevent false sharing in concurrent data structures

#### API Design

```cpp
namespace asyncle::hardware {

// Compile-time constants (conservative defaults)
inline constexpr size_t cache_line_size = 64;
inline constexpr size_t l1_cache_line_size = 64;
inline constexpr size_t l2_cache_line_size = 64;
inline constexpr size_t l3_cache_line_size = 64;

// Runtime detection
struct cache_info {
    size_t l1_line_size;
    size_t l2_line_size;
    size_t l3_line_size;
    size_t l1_cache_size;
    size_t l2_cache_size;
    size_t l3_cache_size;
};

cache_info detect_cache_info() noexcept;
size_t detect_cache_line_size() noexcept;

// Cache-aligned storage (exactly one cache line)
template <typename T>
struct alignas(cache_line_size) cache_aligned {
    T value;

    // Ensure size is exactly cache_line_size
    static_assert(sizeof(T) <= cache_line_size,
                  "Type too large for single cache line");

    char padding[cache_line_size - sizeof(T)];

    // Transparent access
    T& get() noexcept { return value; }
    const T& get() const noexcept { return value; }
    operator T&() noexcept { return value; }
    operator const T&() const noexcept { return value; }
};

// Cache-padded storage (prevents false sharing)
template <typename T>
struct cache_padded {
    alignas(cache_line_size) T value;

    // Pad to next cache line boundary
    static constexpr size_t padding_size =
        (sizeof(T) % cache_line_size == 0) ? 0
        : (cache_line_size - (sizeof(T) % cache_line_size));

    char padding[padding_size];

    // Transparent access
    T& get() noexcept { return value; }
    const T& get() const noexcept { return value; }
    operator T&() noexcept { return value; }
    operator const T&() const noexcept { return value; }
};

// Helper to check if pointer is cache-aligned
constexpr bool is_cache_aligned(const void* ptr) noexcept {
    return (reinterpret_cast<uintptr_t>(ptr) % cache_line_size) == 0;
}

// Align pointer to next cache line boundary
constexpr void* align_to_cache_line(void* ptr) noexcept {
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<void*>(
        (addr + cache_line_size - 1) & ~(cache_line_size - 1)
    );
}

}
```

#### Usage Examples

```cpp
// Example 1: Lock-free queue (prevent false sharing)
struct alignas(asyncle::hardware::cache_line_size) ProducerState {
    std::atomic<size_t> head;
};

struct alignas(asyncle::hardware::cache_line_size) ConsumerState {
    std::atomic<size_t> tail;
};

// Example 2: Per-thread counters
using asyncle::hardware::cache_padded;
std::array<cache_padded<std::atomic<uint64_t>>, 16> per_thread_counters;

// Example 3: Cache-friendly data structure
struct OrderBookLevel {
    double price;
    uint64_t volume;
    uint32_t order_count;
};
using Level = asyncle::hardware::cache_aligned<OrderBookLevel>;
```

### 1.2 Prefetch Hints

#### Requirements

- Provide cross-platform prefetch interface
- Support different temporal localities
- Allow prefetch for read/write operations
- Compile to no-op when unavailable

#### API Design

```cpp
namespace asyncle::hardware {

// Prefetch locality hints
enum class prefetch_locality {
    none = 0,        // Non-temporal (no cache pollution)
    low = 1,         // Low temporal locality (evict soon)
    moderate = 2,    // Moderate temporal locality
    high = 3         // High temporal locality (keep in cache)
};

// Prefetch for reading
template <prefetch_locality Locality = prefetch_locality::moderate>
inline void prefetch_read(const void* addr) noexcept {
    #if defined(__GNUC__) || defined(__clang__)
        __builtin_prefetch(addr, 0, static_cast<int>(Locality));
    #elif defined(_MSC_VER)
        _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_T0);
    #else
        (void)addr; // No-op
    #endif
}

// Prefetch for writing
template <prefetch_locality Locality = prefetch_locality::moderate>
inline void prefetch_write(const void* addr) noexcept {
    #if defined(__GNUC__) || defined(__clang__)
        __builtin_prefetch(addr, 1, static_cast<int>(Locality));
    #elif defined(_MSC_VER)
        _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_T0);
    #else
        (void)addr; // No-op
    #endif
}

// Prefetch range
template <prefetch_locality Locality = prefetch_locality::moderate>
inline void prefetch_range(const void* addr, size_t size) noexcept {
    const char* ptr = static_cast<const char*>(addr);
    const char* end = ptr + size;

    for (; ptr < end; ptr += cache_line_size) {
        prefetch_read<Locality>(ptr);
    }
}

}
```

#### Usage Examples

```cpp
// Example 1: Prefetch next element in linked list
void process_list(Node* head) {
    Node* current = head;
    while (current) {
        if (current->next) {
            asyncle::hardware::prefetch_read(current->next);
        }
        process(current->data);
        current = current->next;
    }
}

// Example 2: Stream processing with prefetch
void process_orders(const Order* orders, size_t count) {
    constexpr size_t prefetch_distance = 8;

    for (size_t i = 0; i < count; ++i) {
        if (i + prefetch_distance < count) {
            asyncle::hardware::prefetch_read(&orders[i + prefetch_distance]);
        }
        process_order(orders[i]);
    }
}
```

### 1.3 Memory Barriers and Fences

#### Requirements

- Provide semantic memory ordering abstractions
- Support acquire/release/seq_cst semantics
- Cross-platform compiler fence and hardware fence
- Clear distinction between compiler and hardware barriers

#### API Design

```cpp
namespace asyncle::hardware {

// Compiler barriers (prevent compiler reordering)
inline void compiler_barrier() noexcept {
    #if defined(__GNUC__) || defined(__clang__)
        asm volatile("" ::: "memory");
    #elif defined(_MSC_VER)
        _ReadWriteBarrier();
    #else
        std::atomic_signal_fence(std::memory_order_seq_cst);
    #endif
}

// Hardware memory barriers (prevent CPU reordering)
inline void memory_barrier_acquire() noexcept {
    std::atomic_thread_fence(std::memory_order_acquire);
}

inline void memory_barrier_release() noexcept {
    std::atomic_thread_fence(std::memory_order_release);
}

inline void memory_barrier_seq_cst() noexcept {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

inline void memory_barrier_acq_rel() noexcept {
    std::atomic_thread_fence(std::memory_order_acq_rel);
}

// Full memory fence (both compiler and hardware)
inline void full_barrier() noexcept {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

}
```

### 1.4 Cache Control

#### Requirements

- Flush specific cache lines
- Invalidate cache lines
- Non-temporal stores (bypass cache)
- Clear documentation on use cases

#### API Design

```cpp
namespace asyncle::hardware {

// Flush cache line containing address (writeback to memory)
inline void cache_flush(const void* addr) noexcept {
    #if defined(__x86_64__) || defined(_M_X64)
        _mm_clflush(addr);
    #elif defined(__aarch64__)
        asm volatile("dc cvac, %0" : : "r"(addr) : "memory");
    #else
        (void)addr; // No-op
    #endif
}

// Flush and invalidate cache line
inline void cache_flush_invalidate(const void* addr) noexcept {
    #if defined(__x86_64__) || defined(_M_X64)
        _mm_clflushopt(addr);
    #elif defined(__aarch64__)
        asm volatile("dc civac, %0" : : "r"(addr) : "memory");
    #else
        (void)addr; // No-op
    #endif
}

// Flush range
inline void cache_flush_range(const void* addr, size_t size) noexcept {
    const char* ptr = static_cast<const char*>(addr);
    const char* end = ptr + size;

    for (; ptr < end; ptr += cache_line_size) {
        cache_flush(ptr);
    }
}

}
```

## Phase 2: Capabilities Module (Future)

Runtime CPU feature detection:
- Cache sizes and topology
- SIMD instruction sets (SSE, AVX, AVX-512, NEON)
- Hardware features (TSX, AES-NI, CRC32, RDRAND)
- CPU wait instructions (umonitor/umwait)

## Phase 3: SIMD and Wait Modules (Future)

### SIMD Module
- Portable SIMD types (vector<T, N>)
- Common operations (load, store, arithmetic)
- SWAR implementations for non-SIMD platforms

### Wait Module
- umonitor/umwait abstractions
- Efficient spin-wait with power saving
- Cross-platform wait primitives

## Implementation Strategy

### Phase 1 Implementation Steps

1. ✅ **Create design document** (this file)
2. ⏳ **Implement `asyncle/hardware/memory.hpp`**
   - Cache line constants and types
   - cache_aligned<T> and cache_padded<T>
   - Prefetch functions
   - Memory barriers
   - Cache control functions
3. ⏳ **Platform-specific runtime detection**
   - Linux: sysconf, /sys/devices/system/cpu
   - Windows: GetLogicalProcessorInformation
   - macOS: sysctl
4. ⏳ **Write comprehensive tests**
   - Unit tests for all functions
   - False sharing prevention tests
   - Alignment verification tests
   - Benchmark comparisons
5. ⏳ **Update documentation**
   - Add to main README
   - Create usage examples
   - Document performance characteristics

### Testing Strategy

```cpp
// Test false sharing prevention
void test_false_sharing() {
    struct BadCounter {
        std::atomic<int> a;
        std::atomic<int> b;
    };

    struct GoodCounter {
        asyncle::hardware::cache_padded<std::atomic<int>> a;
        asyncle::hardware::cache_padded<std::atomic<int>> b;
    };

    // Benchmark: GoodCounter should be significantly faster
    // when a and b are updated by different threads
}

// Test alignment
void test_alignment() {
    asyncle::hardware::cache_aligned<int> x;
    assert(asyncle::hardware::is_cache_aligned(&x));
}

// Test prefetch (smoke test - hard to verify correctness)
void test_prefetch() {
    int data[1000];
    asyncle::hardware::prefetch_range(data, sizeof(data));
    // Should compile and run without crash
}
```

## Use Cases

### Trading System (noemix-trader)

```cpp
// Order book with cache-optimized levels
struct OrderBook {
    asyncle::hardware::cache_aligned<std::atomic<uint64_t>> sequence;
    asyncle::hardware::cache_padded<Price> bid_levels[10];
    asyncle::hardware::cache_padded<Price> ask_levels[10];
};

// Per-thread state without false sharing
struct ThreadState {
    asyncle::hardware::cache_padded<std::atomic<uint64_t>> messages_processed;
    asyncle::hardware::cache_padded<std::atomic<uint64_t>> orders_matched;
};
```

### Lock-Free Data Structures

```cpp
// SPSC queue with separated producer/consumer state
template <typename T, size_t Size>
class SPSCQueue {
    alignas(asyncle::hardware::cache_line_size)
    std::atomic<size_t> head_;

    alignas(asyncle::hardware::cache_line_size)
    std::atomic<size_t> tail_;

    std::array<T, Size> buffer_;
};
```

## Future Considerations

1. **NUMA awareness** - Detect and optimize for NUMA topology
2. **Huge pages** - Support for transparent huge pages
3. **Cache partitioning** - Intel CAT (Cache Allocation Technology)
4. **Memory bandwidth optimization** - Stream operations, non-temporal stores
5. **Hardware transactional memory** - TSX abstractions

## References

- Intel 64 and IA-32 Architectures Optimization Reference Manual
- ARM Cortex-A Series Programmer's Guide
- Linux kernel Documentation/memory-barriers.txt
- C++ memory model (cppreference.com)
- LLVM libc++ implementation of std::hardware_destructive_interference_size

## Open Questions

1. Should we provide std::hardware_constructive_interference_size equivalent?
2. Do we need page_aligned<T> for mmap operations?
3. Should cache_aligned<T> enforce size <= cache_line_size or allow larger types?
4. Need architecture-specific cache line sizes (e.g., Apple M1 has 128-byte lines)?

## Approval Checklist

- [ ] Design reviewed and approved
- [ ] Implementation matches design document
- [ ] All tests pass (including false sharing prevention)
- [ ] Documentation complete
- [ ] Examples provided for common use cases
- [ ] Performance verified (benchmarks show expected improvements)
