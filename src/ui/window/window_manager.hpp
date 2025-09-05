/**
 * @file window_manager.hpp
 * @brief Cross-platform window management system for QuantumCanvas Studio
 * 
 * Enterprise-grade window manager with WebGPU integration, multi-monitor support,
 * and high-DPI awareness. Provides abstraction over Win32/Cocoa/X11 APIs.
 * 
 * @author QuantumCanvas Team
 * @date 2025-01-14
 * @version 1.0.0-alpha
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <cstdint>

// Platform detection
#ifdef _WIN32
    #include <windows.h>
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #include <objc/objc.h>
    #define PLATFORM_MACOS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#endif

// Forward declarations
namespace qcs::core::rendering {
    class RenderingEngine;
    struct RenderSurface;
}

namespace qcs::ui::events {
    struct InputEvent;
    class InputHandler;
}

namespace qcs::ui::window {

// ============================================================================
// CORE TYPES & ENUMS
// ============================================================================

using WindowId = uint32_t;
constexpr WindowId INVALID_WINDOW_ID = 0;

/**
 * @brief Window creation descriptor
 */
struct WindowDesc {
    std::string title = "QuantumCanvas Studio";
    int32_t width = 1920;
    int32_t height = 1080;
    int32_t x = -1;  // -1 for center
    int32_t y = -1;  // -1 for center
    bool resizable = true;
    bool maximized = false;
    bool fullscreen = false;
    bool vsync = true;
    bool high_dpi = true;
    WindowId parent_id = INVALID_WINDOW_ID;
    
    // Advanced options
    int32_t min_width = 800;
    int32_t min_height = 600;
    float opacity = 1.0f;
    bool always_on_top = false;
    bool decorated = true;  // Window borders/title bar
};

/**
 * @brief Window state information
 */
struct WindowState {
    WindowId id = INVALID_WINDOW_ID;
    int32_t x, y, width, height;
    float dpi_scale = 1.0f;
    bool minimized = false;
    bool maximized = false;
    bool fullscreen = false;
    bool focused = false;
    bool visible = true;
    uint64_t last_update_time = 0;
};

/**
 * @brief Window event types
 */
enum class WindowEventType {
    Resize,
    Move,
    Close,
    Focus,
    LostFocus,
    Minimize,
    Maximize,
    Restore,
    DPIChange,
    MouseEnter,
    MouseLeave
};

/**
 * @brief Window event data
 */
struct WindowEvent {
    WindowEventType type;
    WindowId window_id;
    int32_t param1 = 0;  // Context-dependent parameters
    int32_t param2 = 0;
    float float_param = 0.0f;
    uint64_t timestamp = 0;
};

/**
 * @brief Window event callback signature
 */
using WindowEventCallback = std::function<bool(const WindowEvent&)>;

// ============================================================================
// WINDOW INTERFACE
// ============================================================================

/**
 * @brief Abstract window interface
 */
class IWindow {
public:
    virtual ~IWindow() = default;

    // Basic window operations
    virtual WindowId get_id() const = 0;
    virtual bool is_valid() const = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void close() = 0;
    virtual void focus() = 0;

    // Window properties
    virtual void set_title(const std::string& title) = 0;
    virtual std::string get_title() const = 0;
    virtual void set_size(int32_t width, int32_t height) = 0;
    virtual void get_size(int32_t& width, int32_t& height) const = 0;
    virtual void set_position(int32_t x, int32_t y) = 0;
    virtual void get_position(int32_t& x, int32_t& y) const = 0;

    // Window state
    virtual WindowState get_state() const = 0;
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void restore() = 0;
    virtual void set_fullscreen(bool fullscreen) = 0;
    virtual bool is_fullscreen() const = 0;

    // DPI and scaling
    virtual float get_dpi_scale() const = 0;
    virtual void get_framebuffer_size(int32_t& width, int32_t& height) const = 0;

    // Platform handles
    virtual void* get_native_handle() const = 0;
    virtual void* get_native_display() const = 0;

    // Rendering integration
    virtual bool create_render_surface(qcs::core::rendering::RenderingEngine* engine) = 0;
    virtual qcs::core::rendering::RenderSurface* get_render_surface() const = 0;
    
    // Event handling
    virtual void set_event_callback(WindowEventCallback callback) = 0;
    virtual bool pump_events() = 0;  // Process pending events
};

// ============================================================================
// PLATFORM-SPECIFIC WINDOW IMPLEMENTATIONS
// ============================================================================

#ifdef PLATFORM_WINDOWS
/**
 * @brief Windows-specific window implementation
 */
class Win32Window : public IWindow {
private:
    WindowId m_id;
    HWND m_hwnd;
    HDC m_hdc;
    WindowState m_state;
    WindowEventCallback m_event_callback;
    std::unique_ptr<qcs::core::rendering::RenderSurface> m_render_surface;
    
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    bool initialize_window(const WindowDesc& desc);
    void update_state();
    void handle_message(UINT msg, WPARAM wparam, LPARAM lparam);

public:
    explicit Win32Window(const WindowDesc& desc);
    virtual ~Win32Window();

    // IWindow implementation
    WindowId get_id() const override { return m_id; }
    bool is_valid() const override { return m_hwnd != nullptr; }
    void show() override;
    void hide() override;
    void close() override;
    void focus() override;
    
    void set_title(const std::string& title) override;
    std::string get_title() const override;
    void set_size(int32_t width, int32_t height) override;
    void get_size(int32_t& width, int32_t& height) const override;
    void set_position(int32_t x, int32_t y) override;
    void get_position(int32_t& x, int32_t& y) const override;
    
    WindowState get_state() const override { return m_state; }
    void minimize() override;
    void maximize() override;
    void restore() override;
    void set_fullscreen(bool fullscreen) override;
    bool is_fullscreen() const override;
    
    float get_dpi_scale() const override;
    void get_framebuffer_size(int32_t& width, int32_t& height) const override;
    
    void* get_native_handle() const override { return m_hwnd; }
    void* get_native_display() const override { return m_hdc; }
    
    bool create_render_surface(qcs::core::rendering::RenderingEngine* engine) override;
    qcs::core::rendering::RenderSurface* get_render_surface() const override;
    
    void set_event_callback(WindowEventCallback callback) override { m_event_callback = std::move(callback); }
    bool pump_events() override;
};
#endif // PLATFORM_WINDOWS

// ============================================================================
// WINDOW MANAGER
// ============================================================================

/**
 * @brief Main window management system
 * 
 * Manages multiple windows, event dispatching, and rendering context coordination.
 * Thread-safe for window creation/destruction from different threads.
 * 
 * Features:
 * - Multi-window support with parent-child relationships
 * - High-DPI awareness and per-monitor DPI scaling
 * - WebGPU render surface management
 * - Global input event coordination
 * - Window state persistence and restoration
 * 
 * Performance:
 * - Sub-millisecond window event processing
 * - Efficient event batching and filtering
 * - Lazy window state updates
 * - Optimized for 120Hz refresh rates
 */
class WindowManager {
private:
    // Window storage and management
    std::unordered_map<WindowId, std::unique_ptr<IWindow>> m_windows;
    WindowId m_next_window_id = 1;
    WindowId m_main_window_id = INVALID_WINDOW_ID;
    WindowId m_focused_window_id = INVALID_WINDOW_ID;
    
    // Event handling
    std::vector<WindowEvent> m_event_queue;
    std::vector<qcs::ui::events::InputHandler*> m_input_handlers;
    WindowEventCallback m_global_event_callback;
    
    // Rendering integration
    qcs::core::rendering::RenderingEngine* m_rendering_engine = nullptr;
    
    // Thread safety
    mutable std::mutex m_windows_mutex;
    mutable std::mutex m_events_mutex;
    
    // Performance tracking
    struct PerformanceStats {
        uint64_t events_processed = 0;
        uint64_t windows_created = 0;
        uint64_t windows_destroyed = 0;
        double avg_event_process_time = 0.0;
        uint64_t last_stats_update = 0;
    } m_stats;
    
    // Internal helpers
    WindowId generate_window_id();
    void dispatch_event(const WindowEvent& event);
    void update_focus_tracking(WindowId new_focus);
    void cleanup_destroyed_windows();

public:
    WindowManager();
    ~WindowManager();

    // Initialization and shutdown
    bool initialize(qcs::core::rendering::RenderingEngine* rendering_engine = nullptr);
    void shutdown();
    
    // Window lifecycle
    WindowId create_window(const WindowDesc& desc = {});
    bool destroy_window(WindowId window_id);
    IWindow* get_window(WindowId window_id);
    const IWindow* get_window(WindowId window_id) const;
    
    // Window queries
    std::vector<WindowId> get_all_window_ids() const;
    WindowId get_main_window_id() const { return m_main_window_id; }
    WindowId get_focused_window_id() const { return m_focused_window_id; }
    size_t get_window_count() const;
    bool has_windows() const { return get_window_count() > 0; }
    
    // Event handling
    void update_all_windows();  // Call once per frame
    bool handle_system_events(); // Process OS events
    void set_global_event_callback(WindowEventCallback callback);
    
    // Input handling integration
    void register_input_handler(qcs::ui::events::InputHandler* handler);
    void unregister_input_handler(qcs::ui::events::InputHandler* handler);
    
    // Rendering integration
    void set_rendering_engine(qcs::core::rendering::RenderingEngine* engine);
    qcs::core::rendering::RenderingEngine* get_rendering_engine() const { return m_rendering_engine; }
    qcs::core::rendering::RenderSurface* get_render_surface_for_window(WindowId window_id);
    
    // Utility functions
    bool should_quit() const; // True if all windows closed
    void close_all_windows();
    void minimize_all_windows();
    void restore_all_windows();
    
    // Performance and diagnostics
    PerformanceStats get_performance_stats() const { return m_stats; }
    void reset_performance_stats();
    
    // Platform-specific utilities
    static bool is_platform_supported();
    static std::string get_platform_name();
    static std::vector<std::string> get_supported_features();
};

// ============================================================================
// FACTORY FUNCTIONS
// ============================================================================

/**
 * @brief Create a window manager instance
 */
std::unique_ptr<WindowManager> create_window_manager();

/**
 * @brief Get the global window manager instance (singleton)
 */
WindowManager& get_global_window_manager();

} // namespace qcs::ui::window