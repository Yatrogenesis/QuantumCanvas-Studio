/*
 * Copyright (c) 2024 Francisco Molina (QuantumCanvas Studio)
 * Licensed under Dual License Agreement - See LICENSE file for details
 * 
 * ATTRIBUTION REQUIRED: This software must include attribution to Francisco Molina
 * COMMERCIAL USE: Requires separate license and royalties - contact pako.molina@gmail.com
 * 
 * Project: https://github.com/Yatrogenesis/QuantumCanvas-Studio
 * Author: Francisco Molina <pako.molina@gmail.com>
 */

#include "kernel_manager.hpp"
#include "../memory/memory_manager.hpp"
#include "../resources/resource_manager.hpp"
#include "../events/event.hpp"
#include <filesystem>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#elif __linux__
    #include <sys/resource.h>
    #include <unistd.h>
#elif __APPLE__
    #include <mach/mach.h>
#endif

namespace QuantumCanvas::Core {

// Singleton instance
KernelManager& KernelManager::instance() {
    static KernelManager instance;
    return instance;
}

KernelManager::KernelManager() 
    : start_time_(std::chrono::system_clock::now()) {
}

KernelManager::~KernelManager() {
    if (is_running_) {
        shutdown();
    }
}

bool KernelManager::initialize() {
    if (is_initialized_) {
        return true;
    }
    
    try {
        // Initialize core managers
        memory_manager_ = std::make_unique<MemoryManager>();
        resource_manager_ = std::make_unique<ResourceManager>();
        
        // Initialize core services
        initialize_core_services();
        
        is_initialized_ = true;
        is_running_ = true;
        
        // Publish initialization event
        auto event = std::make_unique<SystemEvent>(
            CoreEventType::ServiceRegistered,
            "KernelManager initialized"
        );
        publish_event(std::move(event));
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Kernel initialization failed: " << e.what() << std::endl;
        cleanup_resources();
        return false;
    }
}

void KernelManager::shutdown() {
    if (!is_running_) {
        return;
    }
    
    is_running_ = false;
    
    // Notify shutdown
    auto event = std::make_unique<SystemEvent>(
        CoreEventType::ShutdownRequested,
        "Kernel shutting down"
    );
    publish_event(std::move(event));
    
    // Process remaining events
    process_events();
    
    // Shutdown all services
    shutdown_all_services();
    
    // Unload all plugins
    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        plugins_.clear();
    }
    
    // Clean up resources
    cleanup_resources();
    
    is_initialized_ = false;
}

bool KernelManager::is_running() const {
    return is_running_;
}

void KernelManager::initialize_core_services() {
    // Core services initialization happens here
    // This will be expanded as services are implemented
}

void KernelManager::shutdown_all_services() {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    // Shutdown services in reverse order of registration
    std::vector<std::pair<ServiceId, std::shared_ptr<IService>>> services_to_shutdown;
    for (const auto& [id, service] : services_) {
        services_to_shutdown.push_back({id, service});
    }
    
    std::reverse(services_to_shutdown.begin(), services_to_shutdown.end());
    
    for (auto& [id, service] : services_to_shutdown) {
        if (service && service->is_initialized()) {
            service->shutdown();
        }
    }
    
    services_.clear();
}

void KernelManager::cleanup_resources() {
    // Clean up event handlers
    {
        std::lock_guard<std::mutex> lock(events_mutex_);
        event_handlers_.clear();
        event_queue_.clear();
    }
    
    // Clean up managers
    resource_manager_.reset();
    memory_manager_.reset();
}

void KernelManager::unregister_service(const ServiceId& id) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    auto it = services_.find(id);
    if (it != services_.end()) {
        if (it->second && it->second->is_initialized()) {
            it->second->shutdown();
        }
        services_.erase(it);
        notify_service_unregistered(id);
    }
}

std::vector<std::string> KernelManager::get_registered_services() const {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    std::vector<std::string> service_names;
    service_names.reserve(services_.size());
    
    for (const auto& [id, service] : services_) {
        if (service) {
            service_names.push_back(service->name());
        }
    }
    
    return service_names;
}

bool KernelManager::load_plugin(const std::filesystem::path& plugin_path) {
    if (!std::filesystem::exists(plugin_path)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    try {
        // Plugin loading implementation would go here
        // This involves dynamic library loading which is platform-specific
        
        PluginId id = next_plugin_id_++;
        
        // Notify plugin loaded
        auto event = std::make_unique<SystemEvent>(
            CoreEventType::PluginLoaded,
            "Plugin loaded: " + plugin_path.string()
        );
        publish_event(std::move(event));
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load plugin: " << e.what() << std::endl;
        return false;
    }
}

bool KernelManager::unload_plugin(PluginId plugin_id) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = plugins_.find(plugin_id);
    if (it != plugins_.end()) {
        // Notify plugin unloading
        auto event = std::make_unique<SystemEvent>(
            CoreEventType::PluginUnloaded,
            "Plugin unloaded: " + std::to_string(plugin_id)
        );
        publish_event(std::move(event));
        
        plugins_.erase(it);
        return true;
    }
    
    return false;
}

std::vector<PluginId> KernelManager::get_loaded_plugins() const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    std::vector<PluginId> plugin_ids;
    plugin_ids.reserve(plugins_.size());
    
    for (const auto& [id, plugin] : plugins_) {
        plugin_ids.push_back(id);
    }
    
    return plugin_ids;
}

IPlugin* KernelManager::get_plugin(PluginId plugin_id) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = plugins_.find(plugin_id);
    if (it != plugins_.end()) {
        return it->second.get();
    }
    
    return nullptr;
}

void KernelManager::publish_event(std::unique_ptr<IEvent> event) {
    if (!event) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(events_mutex_);
    event_queue_.push_back(std::move(event));
}

void KernelManager::subscribe(EventType type, IEventHandler* handler) {
    if (!handler) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(events_mutex_);
    
    EventSubscription subscription{handler, handler->priority()};
    event_handlers_[type].push_back(subscription);
    
    // Sort by priority
    std::sort(event_handlers_[type].begin(), event_handlers_[type].end(),
              [](const EventSubscription& a, const EventSubscription& b) {
                  return a.priority > b.priority;
              });
}

void KernelManager::unsubscribe(EventType type, IEventHandler* handler) {
    if (!handler) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(events_mutex_);
    
    auto it = event_handlers_.find(type);
    if (it != event_handlers_.end()) {
        auto& handlers = it->second;
        handlers.erase(
            std::remove_if(handlers.begin(), handlers.end(),
                          [handler](const EventSubscription& sub) {
                              return sub.handler == handler;
                          }),
            handlers.end()
        );
    }
}

void KernelManager::process_events() {
    std::vector<std::unique_ptr<IEvent>> events_to_process;
    
    {
        std::lock_guard<std::mutex> lock(events_mutex_);
        events_to_process = std::move(event_queue_);
        event_queue_.clear();
    }
    
    for (const auto& event : events_to_process) {
        if (!event) continue;
        
        EventType type = event->type();
        
        std::lock_guard<std::mutex> lock(events_mutex_);
        auto it = event_handlers_.find(type);
        if (it != event_handlers_.end()) {
            for (const auto& subscription : it->second) {
                if (subscription.handler && subscription.handler->can_handle(type)) {
                    subscription.handler->handle_event(*event);
                }
            }
        }
    }
}

IMemoryManager& KernelManager::memory_manager() {
    if (!memory_manager_) {
        throw std::runtime_error("Memory manager not initialized");
    }
    return *memory_manager_;
}

IResourceManager& KernelManager::resource_manager() {
    if (!resource_manager_) {
        throw std::runtime_error("Resource manager not initialized");
    }
    return *resource_manager_;
}

KernelManager::PerformanceStats KernelManager::get_performance_stats() const {
    PerformanceStats stats{};
    
    // Calculate memory usage
    stats.total_memory_usage = 0;
    if (memory_manager_) {
        stats.total_memory_usage = memory_manager_->get_total_allocated();
    }
    
    // Service and plugin counts
    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        stats.service_count = services_.size();
    }
    
    {
        std::lock_guard<std::mutex> lock(plugins_mutex_);
        stats.plugin_count = plugins_.size();
    }
    
    // Event queue size
    {
        std::lock_guard<std::mutex> lock(events_mutex_);
        stats.event_queue_size = event_queue_.size();
    }
    
    // Calculate uptime
    auto now = std::chrono::system_clock::now();
    stats.uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time_
    );
    
    // Platform-specific CPU usage
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        stats.total_memory_usage = pmc.WorkingSetSize;
    }
#elif __linux__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        stats.total_memory_usage = usage.ru_maxrss * 1024;
    }
#elif __APPLE__
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
        stats.total_memory_usage = info.resident_size;
    }
#endif
    
    return stats;
}

void KernelManager::notify_service_registered(const ServiceId& id) {
    auto event = std::make_unique<SystemEvent>(
        CoreEventType::ServiceRegistered,
        "Service registered"
    );
    publish_event(std::move(event));
}

void KernelManager::notify_service_unregistered(const ServiceId& id) {
    auto event = std::make_unique<SystemEvent>(
        CoreEventType::ServiceUnregistered,
        "Service unregistered"
    );
    publish_event(std::move(event));
}

} // namespace QuantumCanvas::Core