#include <asyncle/hardware/memory.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace asyncle::hardware;

// Test cache alignment
void test_cache_alignment() {
    std::cout << "Testing cache alignment...\n";

    // Test cache_aligned
    cache_aligned<int> aligned_int;
    assert(is_cache_aligned(&aligned_int));
    assert(sizeof(aligned_int) == cache_line_size);
    std::cout << "  ✓ cache_aligned<int> is cache-aligned\n";

    // Test cache_padded
    cache_padded<int> padded_int;
    assert(is_cache_aligned(&padded_int));
    std::cout << "  ✓ cache_padded<int> is cache-aligned\n";

    // Test with larger type
    struct LargeType {
        int    data[10];
        double value;
    };

    cache_padded<LargeType> padded_large;
    assert(is_cache_aligned(&padded_large));
    std::cout << "  ✓ cache_padded<LargeType> is cache-aligned\n";

    // Test array of cache_padded (each element should be on separate cache line)
    cache_padded<std::atomic<int>> counters[4];
    for(size_t i = 0; i < 4; ++i) { assert(is_cache_aligned(&counters[i])); }
    std::cout << "  ✓ Array of cache_padded elements are all cache-aligned\n";
}

// Test transparent access
void test_transparent_access() {
    std::cout << "Testing transparent access...\n";

    // Test cache_aligned access
    cache_aligned<int> x(42);
    assert(x.get() == 42);
    assert(*x == 42);
    assert(static_cast<int>(x) == 42);
    x.get() = 100;
    assert(x.get() == 100);
    std::cout << "  ✓ cache_aligned transparent access works\n";

    // Test cache_padded access
    cache_padded<double> y(3.14);
    assert(y.get() == 3.14);
    assert(*y == 3.14);
    assert(static_cast<double>(y) == 3.14);
    y.get() = 2.71;
    assert(y.get() == 2.71);
    std::cout << "  ✓ cache_padded transparent access works\n";
}

// Test false sharing prevention
void test_false_sharing_prevention() {
    std::cout << "Testing false sharing prevention...\n";

    constexpr size_t iterations = 10000000;

    // Bad design: counters on same cache line
    struct BadCounters {
        std::atomic<uint64_t> a { 0 };
        std::atomic<uint64_t> b { 0 };
    };

    // Good design: counters on separate cache lines
    struct GoodCounters {
        cache_padded<std::atomic<uint64_t>> a { 0 };
        cache_padded<std::atomic<uint64_t>> b { 0 };
    };

    // Benchmark bad design
    BadCounters bad;
    auto        bad_start = std::chrono::high_resolution_clock::now();
    std::thread bad_t1([&]() {
        for(size_t i = 0; i < iterations; ++i) { bad.a.fetch_add(1, std::memory_order_relaxed); }
    });
    std::thread bad_t2([&]() {
        for(size_t i = 0; i < iterations; ++i) { bad.b.fetch_add(1, std::memory_order_relaxed); }
    });
    bad_t1.join();
    bad_t2.join();
    auto bad_end  = std::chrono::high_resolution_clock::now();
    auto bad_time = std::chrono::duration_cast<std::chrono::milliseconds>(bad_end - bad_start).count();

    // Benchmark good design
    GoodCounters good;
    auto         good_start = std::chrono::high_resolution_clock::now();
    std::thread  good_t1([&]() {
        for(size_t i = 0; i < iterations; ++i) { good.a.get().fetch_add(1, std::memory_order_relaxed); }
    });
    std::thread  good_t2([&]() {
        for(size_t i = 0; i < iterations; ++i) { good.b.get().fetch_add(1, std::memory_order_relaxed); }
    });
    good_t1.join();
    good_t2.join();
    auto good_end  = std::chrono::high_resolution_clock::now();
    auto good_time = std::chrono::duration_cast<std::chrono::milliseconds>(good_end - good_start).count();

    std::cout << "  Bad design (false sharing):  " << bad_time << "ms\n";
    std::cout << "  Good design (cache-padded):  " << good_time << "ms\n";

    double speedup = static_cast<double>(bad_time) / static_cast<double>(good_time);
    std::cout << "  Speedup: " << speedup << "x\n";

    // Good design should be faster (though not guaranteed on all systems)
    // We just check that it works correctly
    assert(bad.a.load() == iterations);
    assert(bad.b.load() == iterations);
    assert(good.a.get().load() == iterations);
    assert(good.b.get().load() == iterations);
    std::cout << "  ✓ False sharing prevention test passed\n";
}

// Test prefetch (smoke test - hard to verify correctness)
void test_prefetch() {
    std::cout << "Testing prefetch...\n";

    constexpr size_t size = 1000;
    int              data[size];
    for(size_t i = 0; i < size; ++i) { data[i] = static_cast<int>(i); }

    // Test prefetch_read
    for(size_t i = 0; i < size - 10; ++i) {
        prefetch_read(&data[i + 10]);
        volatile int x = data[i];  // Access current element
        (void)x;
    }
    std::cout << "  ✓ prefetch_read executed without crash\n";

    // Test prefetch_write
    for(size_t i = 0; i < size - 10; ++i) {
        prefetch_write(&data[i + 10]);
        data[i] = i * 2;
    }
    std::cout << "  ✓ prefetch_write executed without crash\n";

    // Test prefetch_range
    prefetch_range(data, sizeof(data));
    std::cout << "  ✓ prefetch_range executed without crash\n";

    // Test different localities
    prefetch_read<prefetch_locality::none>(&data[0]);
    prefetch_read<prefetch_locality::low>(&data[0]);
    prefetch_read<prefetch_locality::moderate>(&data[0]);
    prefetch_read<prefetch_locality::high>(&data[0]);
    std::cout << "  ✓ All prefetch localities work\n";
}

// Test memory barriers (smoke test)
void test_memory_barriers() {
    std::cout << "Testing memory barriers...\n";

    // These are mostly smoke tests - hard to verify correctness
    compiler_barrier();
    memory_barrier_acquire();
    memory_barrier_release();
    memory_barrier_seq_cst();
    memory_barrier_acq_rel();
    full_barrier();

    std::cout << "  ✓ All memory barriers executed without crash\n";
}

// Test cache control (smoke test)
void test_cache_control() {
    std::cout << "Testing cache control...\n";

    int data[100];
    for(size_t i = 0; i < 100; ++i) { data[i] = static_cast<int>(i); }

    // Test cache_flush
    cache_flush(&data[0]);
    std::cout << "  ✓ cache_flush executed without crash\n";

    // Test cache_flush_invalidate
    cache_flush_invalidate(&data[0]);
    std::cout << "  ✓ cache_flush_invalidate executed without crash\n";

    // Test cache_flush_range
    cache_flush_range(data, sizeof(data));
    std::cout << "  ✓ cache_flush_range executed without crash\n";

    // Test cache_invalidate_range
    cache_invalidate_range(data, sizeof(data));
    std::cout << "  ✓ cache_invalidate_range executed without crash\n";
}

// Test cache info detection
void test_cache_info_detection() {
    std::cout << "Testing cache info detection...\n";

    cache_info info = detect_cache_info();

    std::cout << "  Detected cache configuration:\n";
    std::cout << "    L1 line size: " << info.l1_line_size << " bytes\n";
    std::cout << "    L2 line size: " << info.l2_line_size << " bytes\n";
    std::cout << "    L3 line size: " << info.l3_line_size << " bytes\n";
    std::cout << "    L1 cache size: " << info.l1_cache_size / 1024 << " KB\n";
    std::cout << "    L2 cache size: " << info.l2_cache_size / 1024 << " KB\n";
    std::cout << "    L3 cache size: " << info.l3_cache_size / 1024 << " KB\n";

    assert(info.l1_line_size > 0);
    assert(info.l1_cache_size > 0);
    std::cout << "  ✓ Cache info detection works\n";
}

// Test utilities
void test_utilities() {
    std::cout << "Testing utility functions...\n";

    // Test align_to_cache_line
    char  buffer[200];
    void* aligned = align_to_cache_line(buffer);
    assert(is_cache_aligned(aligned));
    std::cout << "  ✓ align_to_cache_line works\n";

    // Test align_size_to_cache_line
    assert(align_size_to_cache_line(1) == cache_line_size);
    assert(align_size_to_cache_line(cache_line_size) == cache_line_size);
    assert(align_size_to_cache_line(cache_line_size + 1) == cache_line_size * 2);
    std::cout << "  ✓ align_size_to_cache_line works\n";
}

int main() {
    std::cout << "Running hardware::memory tests...\n\n";
    std::cout << "Compile-time cache line size: " << cache_line_size << " bytes\n\n";

    test_cache_alignment();
    test_transparent_access();
    test_false_sharing_prevention();
    test_prefetch();
    test_memory_barriers();
    test_cache_control();
    test_cache_info_detection();
    test_utilities();

    std::cout << "\n✓ All tests passed!\n";
    return 0;
}
