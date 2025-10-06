#ifndef ASYNCLE_HARDWARE_MEMORY_HPP
#define ASYNCLE_HARDWARE_MEMORY_HPP

#include "arch/current.hpp"
#include "platform/cache_detection.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

// Platform-specific includes for intrinsics
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <emmintrin.h>  // SSE2 for _mm_prefetch, _mm_clflush
#if defined(__SSE4_2__)
#include <smmintrin.h>
#endif
#endif

namespace asyncle::hardware {

// ============================================================================
// Cache Line Constants (Architecture-specific)
// ============================================================================

// Cache line sizes are architecture-specific and imported from arch/current.hpp:
// - x86-64: 64 bytes
// - ARM Cortex-A: 64 bytes
// - Apple Silicon (M1/M2/M3): 128 bytes
// - Generic fallback: 64 bytes
// These constants are already defined in arch/current.hpp

// ============================================================================
// Runtime Cache Detection (Platform-specific)
// ============================================================================

// cache_info structure is defined in platform/cache_detection.hpp
using platform::cache_info;

// Detect cache information at runtime (implemented in platform-specific source files)
using platform::detect_cache_info;

// Detect cache line size at runtime
inline size_t detect_cache_line_size() noexcept { return detect_cache_info().l1_line_size; }

// ============================================================================
// Cache-Aligned Storage
// ============================================================================

// Cache-aligned storage - ensures value starts at cache line boundary
// and occupies exactly one cache line (with padding)
template <typename T>
struct alignas(cache_line_size) cache_aligned {
    T value;

    // Ensure type fits in a cache line
    static_assert(sizeof(T) <= cache_line_size, "Type too large for single cache line");

    // Padding to fill cache line
    char padding[cache_line_size - sizeof(T)];

    // Constructors
    constexpr cache_aligned() noexcept(std::is_nothrow_default_constructible_v<T>): value(), padding {} {}

    // Constructor from value (works for both copyable and non-copyable types like atomic)
    template <typename U>
    requires std::is_constructible_v<T, U>
    constexpr cache_aligned(U&& v) noexcept(std::is_nothrow_constructible_v<T, U>):
        value(std::forward<U>(v)),
        padding {} {}

    // Transparent access
    T& get() noexcept { return value; }

    const T& get() const noexcept { return value; }

    operator T&() noexcept { return value; }

    operator const T&() const noexcept { return value; }

    T* operator->() noexcept { return &value; }

    const T* operator->() const noexcept { return &value; }

    T& operator*() noexcept { return value; }

    const T& operator*() const noexcept { return value; }
};

// ============================================================================
// Cache-Padded Storage
// ============================================================================

// Cache-padded storage - prevents false sharing by padding to next cache line boundary
template <typename T>
struct cache_padded {
    alignas(cache_line_size) T value;

    // Calculate padding needed to reach next cache line boundary
    static constexpr size_t padding_size =
      (sizeof(T) % cache_line_size == 0) ? 0 : (cache_line_size - (sizeof(T) % cache_line_size));

    // Padding bytes (empty if already aligned)
    [[no_unique_address]] char padding[padding_size];

    // Constructors
    constexpr cache_padded() noexcept(std::is_nothrow_default_constructible_v<T>): value(), padding {} {}

    // Constructor from value (works for both copyable and non-copyable types like atomic)
    template <typename U>
    requires std::is_constructible_v<T, U>
    constexpr cache_padded(U&& v) noexcept(std::is_nothrow_constructible_v<T, U>):
        value(std::forward<U>(v)),
        padding {} {}

    // Transparent access
    T& get() noexcept { return value; }

    const T& get() const noexcept { return value; }

    operator T&() noexcept { return value; }

    operator const T&() const noexcept { return value; }

    T* operator->() noexcept { return &value; }

    const T* operator->() const noexcept { return &value; }

    T& operator*() noexcept { return value; }

    const T& operator*() const noexcept { return value; }
};

// ============================================================================
// Cache Line Utilities
// ============================================================================

// Check if pointer is cache-aligned
constexpr bool is_cache_aligned(const void* ptr) noexcept {
    return (reinterpret_cast<uintptr_t>(ptr) % cache_line_size) == 0;
}

// Align pointer up to next cache line boundary
constexpr void* align_to_cache_line(void* ptr) noexcept {
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<void*>((addr + cache_line_size - 1) & ~(cache_line_size - 1));
}

// Align size up to next cache line boundary
constexpr size_t align_size_to_cache_line(size_t size) noexcept {
    return (size + cache_line_size - 1) & ~(cache_line_size - 1);
}

// ============================================================================
// Prefetch Hints
// ============================================================================

// Prefetch locality hints
enum class prefetch_locality {
    none     = 0,  // Non-temporal (no cache pollution)
    low      = 1,  // Low temporal locality (evict soon)
    moderate = 2,  // Moderate temporal locality
    high     = 3   // High temporal locality (keep in cache)
};

// Prefetch for reading
template <prefetch_locality Locality = prefetch_locality::moderate>
inline void prefetch_read(const void* addr) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(addr, 0, static_cast<int>(Locality));
#elif defined(_MSC_VER) && (defined(__x86_64__) || defined(_M_X64))
    switch(Locality) {
    case prefetch_locality::none    : _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_NTA); break;
    case prefetch_locality::low     : _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_T2); break;
    case prefetch_locality::moderate: _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_T1); break;
    case prefetch_locality::high    : _mm_prefetch(static_cast<const char*>(addr), _MM_HINT_T0); break;
    }
#else
    (void)addr;  // No-op on unsupported platforms
#endif
}

// Prefetch for writing
template <prefetch_locality Locality = prefetch_locality::moderate>
inline void prefetch_write(const void* addr) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(addr, 1, static_cast<int>(Locality));
#elif defined(_MSC_VER) && (defined(__x86_64__) || defined(_M_X64))
    // MSVC doesn't have write prefetch, use read prefetch as fallback
    prefetch_read<Locality>(addr);
#else
    (void)addr;  // No-op on unsupported platforms
#endif
}

// Prefetch range of memory
template <prefetch_locality Locality = prefetch_locality::moderate>
inline void prefetch_range(const void* addr, size_t size) noexcept {
    const char* ptr = static_cast<const char*>(addr);
    const char* end = ptr + size;

    for(; ptr < end; ptr += cache_line_size) { prefetch_read<Locality>(ptr); }
}

// ============================================================================
// Memory Barriers and Fences
// ============================================================================

// Compiler barrier - prevents compiler reordering
inline void compiler_barrier() noexcept {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" ::: "memory");
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
#else
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

// Hardware memory barriers - prevent CPU reordering

inline void memory_barrier_acquire() noexcept { std::atomic_thread_fence(std::memory_order_acquire); }

inline void memory_barrier_release() noexcept { std::atomic_thread_fence(std::memory_order_release); }

inline void memory_barrier_seq_cst() noexcept { std::atomic_thread_fence(std::memory_order_seq_cst); }

inline void memory_barrier_acq_rel() noexcept { std::atomic_thread_fence(std::memory_order_acq_rel); }

// Full memory fence (both compiler and hardware)
inline void full_barrier() noexcept { std::atomic_thread_fence(std::memory_order_seq_cst); }

// ============================================================================
// Cache Control
// ============================================================================

// Flush cache line containing address (writeback to memory)
inline void cache_flush(const void* addr) noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    _mm_clflush(addr);
#elif defined(__aarch64__) || defined(_M_ARM64)
    asm volatile("dc cvac, %0" : : "r"(addr) : "memory");
#else
    (void)addr;  // No-op on unsupported platforms
#endif
}

// Flush and invalidate cache line (optimized flush on x86)
inline void cache_flush_invalidate(const void* addr) noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(__SSE4_2__)
    // Use clflushopt if available (requires SSE4.2+)
    // Note: Actually requires CLFLUSHOPT CPUID bit, but SSE4.2 is a proxy
    _mm_clflush(addr);  // Fallback to clflush for now
#else
    _mm_clflush(addr);
#endif
#elif defined(__aarch64__) || defined(_M_ARM64)
    asm volatile("dc civac, %0" : : "r"(addr) : "memory");
#else
    (void)addr;  // No-op on unsupported platforms
#endif
}

// Flush range of cache lines
inline void cache_flush_range(const void* addr, size_t size) noexcept {
    const char* ptr = static_cast<const char*>(addr);
    const char* end = ptr + size;

    for(; ptr < end; ptr += cache_line_size) { cache_flush(ptr); }
}

// Invalidate range of cache lines
inline void cache_invalidate_range(const void* addr, size_t size) noexcept {
    const char* ptr = static_cast<const char*>(addr);
    const char* end = ptr + size;

    for(; ptr < end; ptr += cache_line_size) { cache_flush_invalidate(ptr); }
}

}  // namespace asyncle::hardware

#endif  // ASYNCLE_HARDWARE_MEMORY_HPP
