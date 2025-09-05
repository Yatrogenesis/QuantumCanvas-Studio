#include "memory_manager.hpp"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <chrono>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <malloc.h>
#elif defined(__linux__)
    #include <sys/mman.h>
    #include <unistd.h>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <sys/mman.h>
#endif

namespace QuantumCanvas::Core {

// Implementation of MemoryManager
MemoryManager::MemoryManager(size_t initial_heap_size) 
    : memory_limit_(initial_heap_size * 2) { // Set limit to 2x initial heap size
    
    // Initialize memory pools with default configurations
    for (size_t i = 0; i < NUM_POOLS; ++i) {
        initialize_pool(i, POOL_SIZES[i], DEFAULT_BLOCKS_PER_POOL);
    }
    
    initialized_ = true;
    
    // Log initialization
    std::cout << "[MemoryManager] Initialized with " << NUM_POOLS << " pools, " 
              << "initial heap size: " << (initial_heap_size / 1024 / 1024) << "MB" << std::endl;
}

MemoryManager::~MemoryManager() {
    if (tracking_enabled_) {
        check_leaks();
    }
    
    // Clean up large allocations
    {
        std::lock_guard<std::mutex> lock(large_alloc_mutex_);
        for (auto& [ptr, info] : large_allocations_) {
            #ifdef _WIN32
                _aligned_free(ptr);
            #else
                free(ptr);
            #endif
        }
    }
    
    // Pools are automatically cleaned up via RAII
    std::cout << "[MemoryManager] Shutdown complete" << std::endl;
}

void* MemoryManager::allocate(size_t size, size_t alignment) {
    if (size == 0) {
        return nullptr;
    }
    
    // Ensure alignment is power of 2
    if (!is_power_of_two(alignment)) {
        alignment = alignof(std::max_align_t);
    }
    
    // Check memory limit
    if (memory_limit_ > 0 && stats_.current_usage + size > memory_limit_) {
        throw std::bad_alloc();
    }
    
    void* ptr = nullptr;
    bool from_pool = false;
    
    // Try to allocate from pools first for small allocations
    size_t pool_index = find_pool_index(size);
    if (pool_index < NUM_POOLS && alignment <= alignof(std::max_align_t)) {
        ptr = allocate_from_pool(pool_index);
        from_pool = true;
    }
    
    // Fall back to large allocation
    if (!ptr) {
        ptr = allocate_large(size, alignment);
    }
    
    if (ptr) {
        update_stats_allocation(size, from_pool);
        if (tracking_enabled_) {
            track_allocation(ptr, size);
        }
    }
    
    return ptr;
}

void MemoryManager::deallocate(void* ptr, size_t size) {
    if (!ptr) {
        return;
    }
    
    bool deallocated = false;
    
    // Try pool deallocation first
    size_t pool_index = find_pool_index(size);
    if (pool_index < NUM_POOLS) {
        // Check if this pointer belongs to the pool
        auto& pool = pools_[pool_index];
        std::lock_guard<std::mutex> lock(pool.mutex);
        
        uint8_t* pool_start = pool.memory.get();
        uint8_t* pool_end = pool_start + (pool.block_count * pool.block_size);
        
        if (ptr >= pool_start && ptr < pool_end) {
            deallocate_to_pool(ptr, pool_index);
            deallocated = true;
        }
    }
    
    // Large allocation deallocation
    if (!deallocated) {
        deallocate_large(ptr);
    }
    
    update_stats_deallocation(size);
    if (tracking_enabled_) {
        track_deallocation(ptr);
    }
}

void* MemoryManager::allocate_aligned(size_t size, size_t alignment) {
    if (alignment <= alignof(std::max_align_t)) {
        return allocate(size, alignment);
    }
    
    // For large alignments, use system-specific aligned allocation
    void* ptr = nullptr;
    
    #ifdef _WIN32
        ptr = _aligned_malloc(size, alignment);
    #else
        if (posix_memalign(&ptr, alignment, size) != 0) {
            ptr = nullptr;
        }
    #endif
    
    if (ptr) {
        // Track as large allocation
        std::lock_guard<std::mutex> lock(large_alloc_mutex_);
        large_allocations_[ptr] = {ptr, size, alignment};
        update_stats_allocation(size, false);
        if (tracking_enabled_) {
            track_allocation(ptr, size);
        }
    }
    
    return ptr;
}

void MemoryManager::deallocate_aligned(void* ptr, size_t size, size_t alignment) {
    if (!ptr) {
        return;
    }
    
    // Remove from large allocations tracking
    {
        std::lock_guard<std::mutex> lock(large_alloc_mutex_);
        auto it = large_allocations_.find(ptr);
        if (it != large_allocations_.end()) {
            #ifdef _WIN32
                _aligned_free(ptr);
            #else
                free(ptr);
            #endif
            large_allocations_.erase(it);
        }
    }
    
    update_stats_deallocation(size);
    if (tracking_enabled_) {
        track_deallocation(ptr);
    }
}

MemoryStats MemoryManager::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void MemoryManager::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = MemoryStats{};
}

void MemoryManager::check_leaks() const {
    if (!tracking_enabled_) {
        std::cout << "[MemoryManager] Memory tracking is disabled" << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    if (tracked_allocations_.empty()) {
        std::cout << "[MemoryManager] No memory leaks detected" << std::endl;
        return;
    }
    
    std::cout << "[MemoryManager] Memory leaks detected: " << tracked_allocations_.size() << " allocations" << std::endl;
    
    size_t total_leaked = 0;
    for (const auto& [ptr, info] : tracked_allocations_) {
        total_leaked += info.size;
        std::cout << "  Leak: " << info.size << " bytes at " << ptr;
        if (!info.file.empty()) {
            std::cout << " (" << info.file << ":" << info.line << ")";
        }
        std::cout << std::endl;
    }
    
    std::cout << "Total leaked: " << total_leaked << " bytes" << std::endl;
}

void MemoryManager::dump_allocations() const {
    if (!tracking_enabled_) {
        std::cout << "[MemoryManager] Memory tracking is disabled" << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    std::cout << "[MemoryManager] Current allocations: " << tracked_allocations_.size() << std::endl;
    
    for (const auto& [ptr, info] : tracked_allocations_) {
        std::cout << "  " << info.size << " bytes at " << ptr;
        if (!info.file.empty()) {
            std::cout << " (" << info.file << ":" << info.line << ")";
        }
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - info.timestamp);
        std::cout << " [" << duration.count() << "s ago]" << std::endl;
    }
}

void MemoryManager::configure_pool(size_t block_size, size_t block_count) {
    // Find existing pool or create new configuration
    for (size_t i = 0; i < NUM_POOLS; ++i) {
        if (pools_[i].block_size == block_size) {
            resize_pool(i, block_count);
            return;
        }
    }
    
    // Configuration not found - this would require dynamic pool management
    // For now, log the request
    std::cout << "[MemoryManager] Pool configuration requested: " 
              << block_size << " byte blocks, " << block_count << " count" << std::endl;
}

void MemoryManager::resize_pool(size_t pool_index, size_t new_block_count) {
    if (pool_index >= NUM_POOLS) {
        return;
    }
    
    auto& pool = pools_[pool_index];
    std::lock_guard<std::mutex> lock(pool.mutex);
    
    if (pool.allocated_count > 0) {
        std::cout << "[MemoryManager] Cannot resize pool " << pool_index 
                  << " - " << pool.allocated_count << " blocks in use" << std::endl;
        return;
    }
    
    // Reinitialize the pool with new size
    initialize_pool(pool_index, pool.block_size, new_block_count);
    
    std::cout << "[MemoryManager] Pool " << pool_index << " resized to " 
              << new_block_count << " blocks" << std::endl;
}

void MemoryManager::defragment_pools() {
    // Defragmentation would require moving allocated blocks
    // This is complex and typically not needed with pool allocators
    // For now, just log statistics
    
    std::cout << "[MemoryManager] Pool statistics:" << std::endl;
    for (size_t i = 0; i < NUM_POOLS; ++i) {
        const auto& pool = pools_[i];
        std::lock_guard<std::mutex> lock(pool.mutex);
        
        double usage = (double)pool.allocated_count / pool.block_count * 100.0;
        std::cout << "  Pool " << i << " (" << pool.block_size << " bytes): " 
                  << pool.allocated_count << "/" << pool.block_count 
                  << " (" << usage << "% used)" << std::endl;
    }
}

// Private implementation methods

size_t MemoryManager::find_pool_index(size_t size) const {
    // Binary search for the appropriate pool
    auto it = std::lower_bound(POOL_SIZES, POOL_SIZES + NUM_POOLS, size);
    if (it != POOL_SIZES + NUM_POOLS) {
        return it - POOL_SIZES;
    }
    return NUM_POOLS; // Size too large for pools
}

void* MemoryManager::allocate_from_pool(size_t pool_index) {
    if (pool_index >= NUM_POOLS) {
        return nullptr;
    }
    
    auto& pool = pools_[pool_index];
    std::lock_guard<std::mutex> lock(pool.mutex);
    
    if (pool.free_blocks.empty()) {
        // Pool is exhausted
        stats_.pool_misses++;
        return nullptr;
    }
    
    void* ptr = pool.free_blocks.top();
    pool.free_blocks.pop();
    pool.allocated_count++;
    stats_.pool_hits++;
    
    return ptr;
}

void MemoryManager::deallocate_to_pool(void* ptr, size_t pool_index) {
    if (pool_index >= NUM_POOLS || !ptr) {
        return;
    }
    
    auto& pool = pools_[pool_index];
    // Note: mutex should already be locked by caller
    
    pool.free_blocks.push(ptr);
    pool.allocated_count--;
    
    // Optional: clear memory for security/debugging
    #ifdef DEBUG
        std::memset(ptr, 0xDE, pool.block_size); // "Dead" pattern
    #endif
}

void* MemoryManager::allocate_large(size_t size, size_t alignment) {
    void* ptr = nullptr;
    
    if (alignment <= alignof(std::max_align_t)) {
        ptr = std::malloc(size);
    } else {
        #ifdef _WIN32
            ptr = _aligned_malloc(size, alignment);
        #else
            if (posix_memalign(&ptr, alignment, size) != 0) {
                ptr = nullptr;
            }
        #endif
    }
    
    if (ptr) {
        std::lock_guard<std::mutex> lock(large_alloc_mutex_);
        large_allocations_[ptr] = {ptr, size, alignment};
    }
    
    return ptr;
}

void MemoryManager::deallocate_large(void* ptr) {
    if (!ptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(large_alloc_mutex_);
    auto it = large_allocations_.find(ptr);
    if (it != large_allocations_.end()) {
        if (it->second.alignment <= alignof(std::max_align_t)) {
            std::free(ptr);
        } else {
            #ifdef _WIN32
                _aligned_free(ptr);
            #else
                free(ptr);
            #endif
        }
        large_allocations_.erase(it);
    }
}

void MemoryManager::initialize_pool(size_t index, size_t block_size, size_t block_count) {
    if (index >= NUM_POOLS) {
        return;
    }
    
    auto& pool = pools_[index];
    
    // Clear existing pool
    pool.free_blocks = std::stack<void*>();
    pool.allocated_count = 0;
    
    // Set pool properties
    pool.block_size = block_size;
    pool.block_count = block_count;
    pool.blocks_in_use = 0;
    pool.total_memory = block_size * block_count;
    
    // Allocate memory for the pool
    pool.memory = std::make_unique<uint8_t[]>(pool.total_memory);
    
    // Initialize free block list
    uint8_t* current = pool.memory.get();
    for (size_t i = 0; i < block_count; ++i) {
        pool.free_blocks.push(current);
        current += block_size;
    }
    
    std::cout << "[MemoryManager] Pool " << index << " initialized: " 
              << block_count << " blocks of " << block_size << " bytes each" << std::endl;
}

void MemoryManager::update_stats_allocation(size_t size, bool from_pool) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_allocated += size;
    stats_.current_usage += size;
    stats_.allocation_count++;
    
    if (stats_.current_usage > stats_.peak_usage) {
        stats_.peak_usage = stats_.current_usage;
    }
    
    if (from_pool) {
        stats_.pool_hits++;
    } else {
        stats_.pool_misses++;
    }
}

void MemoryManager::update_stats_deallocation(size_t size) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_deallocated += size;
    stats_.current_usage -= size;
    stats_.deallocation_count++;
}

void MemoryManager::track_allocation(void* ptr, size_t size) {
    if (!tracking_enabled_ || !ptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    tracked_allocations_[ptr] = {
        ptr, 
        size, 
        "",  // file - would be filled by macro in debug builds
        0,   // line - would be filled by macro in debug builds
        std::chrono::system_clock::now()
    };
}

void MemoryManager::track_deallocation(void* ptr) {
    if (!tracking_enabled_ || !ptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(tracking_mutex_);
    tracked_allocations_.erase(ptr);
}

} // namespace QuantumCanvas::Core

// Debug macros for tracking allocations with file/line info
#ifdef DEBUG
    #define QC_MALLOC(size) QuantumCanvas::Core::MemoryManager::instance().allocate_tracked(size, __FILE__, __LINE__)
    #define QC_FREE(ptr, size) QuantumCanvas::Core::MemoryManager::instance().deallocate_tracked(ptr, size, __FILE__, __LINE__)
#else
    #define QC_MALLOC(size) QuantumCanvas::Core::MemoryManager::instance().allocate(size)
    #define QC_FREE(ptr, size) QuantumCanvas::Core::MemoryManager::instance().deallocate(ptr, size)
#endif