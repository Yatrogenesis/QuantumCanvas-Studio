#include <gtest/gtest.h>
#include "../../src/core/memory/memory_manager.hpp"
#include <vector>
#include <thread>
#include <chrono>
#include <random>

using namespace QuantumCanvas::Core;

class MemoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<MemoryManager>(64 * 1024 * 1024); // 64MB
    }

    void TearDown() override {
        manager.reset();
    }

    std::unique_ptr<MemoryManager> manager;
};

TEST_F(MemoryManagerTest, BasicAllocationDeallocation) {
    // Test basic allocation
    void* ptr1 = manager->allocate(1024);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_TRUE(manager->get_total_allocated() >= 1024);
    
    // Test deallocation
    manager->deallocate(ptr1, 1024);
    
    // Test multiple allocations
    std::vector<void*> ptrs;
    for (int i = 0; i < 100; ++i) {
        void* ptr = manager->allocate(512);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    // Clean up
    for (size_t i = 0; i < ptrs.size(); ++i) {
        manager->deallocate(ptrs[i], 512);
    }
}

TEST_F(MemoryManagerTest, PoolAllocation) {
    // Test pool allocation for different sizes
    for (size_t i = 0; i < MemoryManager::NUM_POOLS; ++i) {
        size_t size = MemoryManager::POOL_SIZES[i];
        void* ptr = manager->allocate(size);
        ASSERT_NE(ptr, nullptr) << "Failed to allocate " << size << " bytes from pool " << i;
        manager->deallocate(ptr, size);
    }
}

TEST_F(MemoryManagerTest, LargeAllocation) {
    // Test large allocation that exceeds pool sizes
    size_t large_size = 1024 * 1024; // 1MB
    void* ptr = manager->allocate(large_size);
    ASSERT_NE(ptr, nullptr);
    
    // Write to memory to ensure it's accessible
    std::memset(ptr, 0xAA, large_size);
    
    manager->deallocate(ptr, large_size);
}

TEST_F(MemoryManagerTest, AlignedAllocation) {
    // Test aligned allocation
    void* ptr1 = manager->allocate_aligned(1024, 64);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 64, 0);
    manager->deallocate_aligned(ptr1, 1024, 64);
    
    // Test large alignment
    void* ptr2 = manager->allocate_aligned(2048, 4096);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 4096, 0);
    manager->deallocate_aligned(ptr2, 2048, 4096);
}

TEST_F(MemoryManagerTest, ObjectAllocation) {
    // Test object allocation
    struct TestObject {
        int value;
        float data[10];
        TestObject(int v = 42) : value(v) {}
    };
    
    auto* obj = manager->allocate_object<TestObject>(123);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->value, 123);
    manager->deallocate_object(obj);
}

TEST_F(MemoryManagerTest, ArrayAllocation) {
    // Test array allocation
    int* arr = manager->allocate_array<int>(1000);
    ASSERT_NE(arr, nullptr);
    
    // Initialize and test array
    for (int i = 0; i < 1000; ++i) {
        arr[i] = i;
    }
    
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(arr[i], i);
    }
    
    manager->deallocate_array(arr, 1000);
}

TEST_F(MemoryManagerTest, Statistics) {
    auto initial_stats = manager->get_stats();
    
    void* ptr1 = manager->allocate(1024);
    void* ptr2 = manager->allocate(2048);
    
    auto stats_after_alloc = manager->get_stats();
    EXPECT_GT(stats_after_alloc.current_usage, initial_stats.current_usage);
    EXPECT_GT(stats_after_alloc.allocation_count, initial_stats.allocation_count);
    
    manager->deallocate(ptr1, 1024);
    manager->deallocate(ptr2, 2048);
    
    auto stats_after_dealloc = manager->get_stats();
    EXPECT_EQ(stats_after_dealloc.current_usage, initial_stats.current_usage);
}

TEST_F(MemoryManagerTest, MemoryTracking) {
    manager->enable_tracking(true);
    
    void* ptr = manager->allocate(1024);
    ASSERT_NE(ptr, nullptr);
    
    // Memory should be tracked
    // Note: In a real test, we'd expose tracking information
    
    manager->deallocate(ptr, 1024);
    
    // Check for leaks - should be none
    manager->check_leaks();
}

TEST_F(MemoryManagerTest, ThreadSafety) {
    const int num_threads = 8;
    const int allocs_per_thread = 100;
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> thread_ptrs(num_threads);
    
    // Launch threads for allocation
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, allocs_per_thread, &thread_ptrs]() {
            std::mt19937 rng(t);
            std::uniform_int_distribution<size_t> size_dist(16, 2048);
            
            for (int i = 0; i < allocs_per_thread; ++i) {
                size_t size = size_dist(rng);
                void* ptr = manager->allocate(size);
                ASSERT_NE(ptr, nullptr);
                thread_ptrs[t].push_back(ptr);
                
                // Small delay to increase contention
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    // Wait for all allocations
    for (auto& thread : threads) {
        thread.join();
    }
    threads.clear();
    
    // Launch threads for deallocation
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, &thread_ptrs]() {
            std::mt19937 rng(t + 1000);
            std::uniform_int_distribution<size_t> size_dist(16, 2048);
            
            for (void* ptr : thread_ptrs[t]) {
                size_t size = size_dist(rng);
                manager->deallocate(ptr, size);
                
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    // Wait for all deallocations
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST_F(MemoryManagerTest, MemoryLimit) {
    // Set a small memory limit for testing
    manager->set_memory_limit(1024);
    
    // Should be able to allocate within limit
    void* ptr1 = manager->allocate(512);
    ASSERT_NE(ptr1, nullptr);
    
    // Should fail to allocate beyond limit
    EXPECT_THROW(manager->allocate(1024), std::bad_alloc);
    
    manager->deallocate(ptr1, 512);
}

TEST_F(MemoryManagerTest, STLAllocator) {
    // Test the STL allocator adapter
    PoolAllocator<int> alloc(manager.get());
    
    std::vector<int, PoolAllocator<int>> vec(alloc);
    vec.reserve(1000);
    
    for (int i = 0; i < 1000; ++i) {
        vec.push_back(i);
    }
    
    EXPECT_EQ(vec.size(), 1000);
    for (size_t i = 0; i < vec.size(); ++i) {
        EXPECT_EQ(vec[i], static_cast<int>(i));
    }
}

TEST_F(MemoryManagerTest, Performance) {
    const int num_allocs = 10000;
    std::vector<void*> ptrs;
    ptrs.reserve(num_allocs);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Allocate
    for (int i = 0; i < num_allocs; ++i) {
        void* ptr = manager->allocate(64); // Use pool allocation
        ptrs.push_back(ptr);
    }
    
    // Deallocate
    for (int i = 0; i < num_allocs; ++i) {
        manager->deallocate(ptrs[i], 64);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should be very fast - target < 1μs per allocation
    double avg_time = static_cast<double>(duration.count()) / (2 * num_allocs); // 2 operations per iteration
    EXPECT_LT(avg_time, 1.0) << "Average allocation time: " << avg_time << "μs";
    
    std::cout << "Average allocation time: " << avg_time << "μs" << std::endl;
}

// Test fixture for stress testing
class MemoryManagerStressTest : public MemoryManagerTest {
protected:
    static constexpr int STRESS_ITERATIONS = 1000;
    static constexpr int STRESS_THREADS = 4;
};

TEST_F(MemoryManagerStressTest, DISABLED_LongRunningStress) {
    // This test is disabled by default as it takes a long time
    const int iterations = 10000;
    std::mt19937 rng;
    std::uniform_int_distribution<size_t> size_dist(1, 4096);
    std::vector<std::pair<void*, size_t>> allocations;
    
    for (int i = 0; i < iterations; ++i) {
        if (allocations.empty() || (rng() % 3 == 0 && allocations.size() < 1000)) {
            // Allocate
            size_t size = size_dist(rng);
            void* ptr = manager->allocate(size);
            if (ptr) {
                allocations.emplace_back(ptr, size);
            }
        } else {
            // Deallocate random allocation
            size_t index = rng() % allocations.size();
            manager->deallocate(allocations[index].first, allocations[index].second);
            allocations.erase(allocations.begin() + index);
        }
        
        if (i % 1000 == 0) {
            auto stats = manager->get_stats();
            std::cout << "Iteration " << i << ": " << allocations.size() 
                      << " active allocations, " << stats.current_usage << " bytes used" << std::endl;
        }
    }
    
    // Clean up remaining allocations
    for (const auto& [ptr, size] : allocations) {
        manager->deallocate(ptr, size);
    }
}