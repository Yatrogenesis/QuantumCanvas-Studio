#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <typeindex>
#include <chrono>
#include <string>

namespace QuantumCanvas::Core {

// Forward declarations
class IService;
class IEventHandler;
class IMemoryManager;
class IResourceManager;
class IPlugin;

// Service and Plugin IDs
using ServiceId = std::type_index;
using PluginId = uint64_t;
using EventType = uint32_t;

// Core event types
enum class CoreEventType : EventType {
    ServiceRegistered = 1,
    ServiceUnregistered = 2,
    PluginLoaded = 3,
    PluginUnloaded = 4,
    ShutdownRequested = 5,
    MemoryPressure = 6,
    PerformanceWarning = 7
};

// Base event interface
class IEvent {
public:
    virtual ~IEvent() = default;
    virtual EventType type() const = 0;
    virtual std::chrono::system_clock::time_point timestamp() const = 0;
    virtual std::string source() const = 0;
    virtual size_t size() const = 0;
};

// Base service interface
class IService {
public:
    virtual ~IService() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
    virtual bool is_initialized() const = 0;
    virtual size_t memory_usage() const = 0;
};

// Event handler interface
class IEventHandler {
public:
    virtual ~IEventHandler() = default;
    virtual void handle_event(const IEvent& event) = 0;
    virtual bool can_handle(EventType type) const = 0;
    virtual uint32_t priority() const { return 100; }
};

// Main Kernel Manager - Singleton pattern with thread safety
class KernelManager {
public:
    // Singleton access
    static KernelManager& instance();
    
    // Disable copy and move
    KernelManager(const KernelManager&) = delete;
    KernelManager& operator=(const KernelManager&) = delete;
    KernelManager(KernelManager&&) = delete;
    KernelManager& operator=(KernelManager&&) = delete;
    
    // Service Management
    template<typename T>
    void register_service(std::shared_ptr<T> service);
    
    template<typename T>
    std::shared_ptr<T> get_service();
    
    template<typename T>
    bool has_service() const;
    
    void unregister_service(const ServiceId& id);
    std::vector<std::string> get_registered_services() const;
    
    // Plugin Management
    bool load_plugin(const std::filesystem::path& plugin_path);
    bool unload_plugin(PluginId plugin_id);
    std::vector<PluginId> get_loaded_plugins() const;
    IPlugin* get_plugin(PluginId plugin_id);
    
    // Event System
    void publish_event(std::unique_ptr<IEvent> event);
    void subscribe(EventType type, IEventHandler* handler);
    void unsubscribe(EventType type, IEventHandler* handler);
    void process_events();
    
    // Resource Management
    IMemoryManager& memory_manager();
    IResourceManager& resource_manager();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    bool is_running() const;
    
    // Performance Monitoring
    struct PerformanceStats {
        size_t total_memory_usage;
        size_t service_count;
        size_t plugin_count;
        size_t event_queue_size;
        double cpu_usage_percent;
        std::chrono::milliseconds uptime;
    };
    
    PerformanceStats get_performance_stats() const;
    
private:
    KernelManager();
    ~KernelManager();
    
    // Service registry
    mutable std::mutex services_mutex_;
    std::unordered_map<ServiceId, std::shared_ptr<IService>> services_;
    
    // Plugin registry
    mutable std::mutex plugins_mutex_;
    std::unordered_map<PluginId, std::unique_ptr<IPlugin>> plugins_;
    PluginId next_plugin_id_ = 1;
    
    // Event system
    struct EventSubscription {
        IEventHandler* handler;
        uint32_t priority;
    };
    mutable std::mutex events_mutex_;
    std::unordered_map<EventType, std::vector<EventSubscription>> event_handlers_;
    std::vector<std::unique_ptr<IEvent>> event_queue_;
    
    // Core managers
    std::unique_ptr<IMemoryManager> memory_manager_;
    std::unique_ptr<IResourceManager> resource_manager_;
    
    // State
    std::atomic<bool> is_initialized_{false};
    std::atomic<bool> is_running_{false};
    std::chrono::system_clock::time_point start_time_;
    
    // Internal methods
    void initialize_core_services();
    void shutdown_all_services();
    void cleanup_resources();
    void notify_service_registered(const ServiceId& id);
    void notify_service_unregistered(const ServiceId& id);
};

// Template implementations
template<typename T>
void KernelManager::register_service(std::shared_ptr<T> service) {
    static_assert(std::is_base_of<IService, T>::value, 
                  "T must inherit from IService");
    
    std::lock_guard<std::mutex> lock(services_mutex_);
    ServiceId id = std::type_index(typeid(T));
    
    if (services_.find(id) != services_.end()) {
        throw std::runtime_error("Service already registered: " + std::string(typeid(T).name()));
    }
    
    services_[id] = service;
    
    if (!service->is_initialized()) {
        service->initialize();
    }
    
    notify_service_registered(id);
}

template<typename T>
std::shared_ptr<T> KernelManager::get_service() {
    static_assert(std::is_base_of<IService, T>::value, 
                  "T must inherit from IService");
    
    std::lock_guard<std::mutex> lock(services_mutex_);
    ServiceId id = std::type_index(typeid(T));
    
    auto it = services_.find(id);
    if (it != services_.end()) {
        return std::static_pointer_cast<T>(it->second);
    }
    
    return nullptr;
}

template<typename T>
bool KernelManager::has_service() const {
    static_assert(std::is_base_of<IService, T>::value, 
                  "T must inherit from IService");
    
    std::lock_guard<std::mutex> lock(services_mutex_);
    ServiceId id = std::type_index(typeid(T));
    return services_.find(id) != services_.end();
}

} // namespace QuantumCanvas::Core