#pragma once

#include <memory>
#include <vector>
#include <stack>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <array>
#include <unordered_map>

namespace QuantumCanvas::Core {

// Memory statistics
struct MemoryStats {
    size_t total_allocated = 0;
    size_t total_deallocated = 0;
    size_t current_usage = 0;
    size_t peak_usage = 0;
    size_t allocation_count = 0;
    size_t deallocation_count = 0;
    size_t pool_hits = 0;
    size_t pool_misses = 0;
};

// Memory pool info
struct PoolInfo {
    size_t block_size;
    size_t block_count;
    size_t blocks_in_use;
    size_t total_memory;
    std::unique_ptr<uint8_t[]> memory;
    std::stack<void*> free_blocks;
    std::atomic<size_t> allocated_count{0};
    std::mutex mutex;
};

// Base interface for memory management
class IMemoryManager {
public:
    virtual ~IMemoryManager() = default;
    
    // Basic allocation/deallocation
    virtual void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
    virtual void deallocate(void* ptr, size_t size) = 0;
    
    // Statistics
    virtual MemoryStats get_stats() const = 0;
    virtual size_t get_total_allocated() const = 0;
    virtual void reset_stats() = 0;
    
    // Memory debugging
    virtual void enable_tracking(bool enable) = 0;
    virtual void check_leaks() const = 0;
    virtual void dump_allocations() const = 0;
};

// High-performance memory manager with pooling
class MemoryManager final : public IMemoryManager {
public:
    // Pool configurations
    static constexpr size_t POOL_SIZES[] = {
        16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
    };
    static constexpr size_t NUM_POOLS = sizeof(POOL_SIZES) / sizeof(POOL_SIZES[0]);
    static constexpr size_t DEFAULT_BLOCKS_PER_POOL = 256;
    
    explicit MemoryManager(size_t initial_heap_size = 64 * 1024 * 1024); // 64MB default
    ~MemoryManager() override;
    
    // IMemoryManager implementation
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    void deallocate(void* ptr, size_t size) override;
    MemoryStats get_stats() const override;
    size_t get_total_allocated() const override { return stats_.current_usage; }
    void reset_stats() override;
    
    // Memory tracking and debugging
    void enable_tracking(bool enable) override { tracking_enabled_ = enable; }
    void check_leaks() const override;
    void dump_allocations() const override;
    
    // Pool management
    void configure_pool(size_t block_size, size_t block_count);
    void resize_pool(size_t pool_index, size_t new_block_count);
    void defragment_pools();
    
    // Memory limits
    void set_memory_limit(size_t limit_bytes) { memory_limit_ = limit_bytes; }
    size_t get_memory_limit() const { return memory_limit_; }
    bool is_memory_limit_exceeded() const { return stats_.current_usage > memory_limit_; }
    
    // Custom allocators for specific types
    template<typename T>
    T* allocate_object() {
        void* memory = allocate(sizeof(T), alignof(T));
        return new(memory) T();
    }
    
    template<typename T, typename... Args>
    T* allocate_object(Args&&... args) {
        void* memory = allocate(sizeof(T), alignof(T));
        return new(memory) T(std::forward<Args>(args)...);
    }
    
    template<typename T>
    void deallocate_object(T* obj) {
        if (obj) {
            obj->~T();
            deallocate(obj, sizeof(T));
        }
    }
    
    // Array allocation
    template<typename T>
    T* allocate_array(size_t count) {
        size_t total_size = sizeof(T) * count;
        void* memory = allocate(total_size, alignof(T));
        return static_cast<T*>(memory);
    }
    
    template<typename T>
    void deallocate_array(T* array, size_t count) {
        if (array) {
            deallocate(array, sizeof(T) * count);
        }
    }
    
    // Aligned allocation
    void* allocate_aligned(size_t size, size_t alignment);
    void deallocate_aligned(void* ptr, size_t size, size_t alignment);
    
private:
    // Memory pools for different sizes
    std::array<PoolInfo, NUM_POOLS> pools_;
    
    // Large allocation tracking
    struct LargeAllocation {
        void* ptr;
        size_t size;
        size_t alignment;
    };
    std::unordered_map<void*, LargeAllocation> large_allocations_;
    mutable std::mutex large_alloc_mutex_;
    
    // Memory tracking
    struct AllocationInfo {
        void* ptr;
        size_t size;
        std::string file;
        int line;
        std::chrono::system_clock::time_point timestamp;
    };
    std::unordered_map<void*, AllocationInfo> tracked_allocations_;
    mutable std::mutex tracking_mutex_;
    std::atomic<bool> tracking_enabled_{false};
    
    // Statistics
    mutable std::mutex stats_mutex_;
    MemoryStats stats_;
    
    // Configuration
    size_t memory_limit_ = 0; // 0 means no limit
    std::atomic<bool> initialized_{false};
    
    // Internal methods
    size_t find_pool_index(size_t size) const;
    void* allocate_from_pool(size_t pool_index);
    void deallocate_to_pool(void* ptr, size_t pool_index);
    void* allocate_large(size_t size, size_t alignment);
    void deallocate_large(void* ptr);
    void initialize_pool(size_t index, size_t block_size, size_t block_count);
    void update_stats_allocation(size_t size, bool from_pool);
    void update_stats_deallocation(size_t size);
    void track_allocation(void* ptr, size_t size);
    void track_deallocation(void* ptr);
    
    // Alignment helpers
    static size_t align_up(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
    
    static bool is_power_of_two(size_t value) {
        return value && !(value & (value - 1));
    }
};

// RAII memory scope for automatic cleanup
class MemoryScope {
public:
    explicit MemoryScope(MemoryManager& manager) : manager_(manager) {
        start_usage_ = manager_.get_total_allocated();
    }
    
    ~MemoryScope() {
        size_t end_usage = manager_.get_total_allocated();
        if (end_usage > start_usage_) {
            // Memory leak detected in scope
            size_t leaked = end_usage - start_usage_;
            // Log warning
        }
    }
    
private:
    MemoryManager& manager_;
    size_t start_usage_;
};

// Memory allocator adapter for STL containers
template<typename T>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };
    
    explicit PoolAllocator(MemoryManager* manager) noexcept : manager_(manager) {}
    
    template<typename U>
    PoolAllocator(const PoolAllocator<U>& other) noexcept : manager_(other.manager_) {}
    
    T* allocate(size_t n) {
        if (manager_) {
            return static_cast<T*>(manager_->allocate(n * sizeof(T), alignof(T)));
        }
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }
    
    void deallocate(T* ptr, size_t n) noexcept {
        if (manager_) {
            manager_->deallocate(ptr, n * sizeof(T));
        } else {
            ::operator delete(ptr);
        }
    }
    
    template<typename U, typename... Args>
    void construct(U* ptr, Args&&... args) {
        new(ptr) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* ptr) {
        ptr->~U();
    }
    
    bool operator==(const PoolAllocator& other) const noexcept {
        return manager_ == other.manager_;
    }
    
    bool operator!=(const PoolAllocator& other) const noexcept {
        return manager_ != other.manager_;
    }
    
private:
    MemoryManager* manager_;
    
    template<typename U>
    friend class PoolAllocator;
};

} // namespace QuantumCanvas::Core