/**
 * @file window_manager.cpp
 * @brief Cross-platform window management implementation
 */

#include "window_manager.hpp"
#include <chrono>
#include <algorithm>
#include <cassert>
#include <iostream>

// Platform-specific includes
#ifdef PLATFORM_WINDOWS
    #include <dwmapi.h>
    #include <shellscalingapi.h>
    #pragma comment(lib, "dwmapi.lib")
    #pragma comment(lib, "shcore.lib")
#endif

namespace qcs::ui::window {

// ============================================================================
// WINDOWS IMPLEMENTATION
// ============================================================================

#ifdef PLATFORM_WINDOWS

namespace {
    // Global window class registration
    const wchar_t* WINDOW_CLASS_NAME = L"QuantumCanvasWindow";
    bool g_window_class_registered = false;
    std::unordered_map<HWND, Win32Window*> g_hwnd_to_window;
    
    bool register_window_class() {
        if (g_window_class_registered) return true;
        
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = Win32Window::window_proc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr; // We'll handle drawing ourselves
        wc.lpszClassName = WINDOW_CLASS_NAME;
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
        
        if (!RegisterClassExW(&wc)) {
            return false;
        }
        
        g_window_class_registered = true;
        return true;
    }
}

Win32Window::Win32Window(const WindowDesc& desc) 
    : m_id(0), m_hwnd(nullptr), m_hdc(nullptr) {
    
    if (!register_window_class()) {
        std::cerr << "Failed to register window class" << std::endl;
        return;
    }
    
    if (!initialize_window(desc)) {
        std::cerr << "Failed to initialize window" << std::endl;
        return;
    }
}

Win32Window::~Win32Window() {
    if (m_hwnd) {
        g_hwnd_to_window.erase(m_hwnd);
        
        if (m_hdc) {
            ReleaseDC(m_hwnd, m_hdc);
            m_hdc = nullptr;
        }
        
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool Win32Window::initialize_window(const WindowDesc& desc) {
    // Enable per-monitor DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    // Calculate window size including borders
    RECT rect = { 0, 0, desc.width, desc.height };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD ex_style = WS_EX_APPWINDOW;
    
    if (!desc.resizable) {
        style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    }
    
    if (!desc.decorated) {
        style = WS_POPUP;
    }
    
    AdjustWindowRectEx(&rect, style, FALSE, ex_style);
    
    // Calculate position (center if requested)
    int x = desc.x;
    int y = desc.y;
    if (x == -1 || y == -1) {
        int screen_width = GetSystemMetrics(SM_CXSCREEN);
        int screen_height = GetSystemMetrics(SM_CYSCREEN);
        x = (screen_width - (rect.right - rect.left)) / 2;
        y = (screen_height - (rect.bottom - rect.top)) / 2;
    }
    
    // Convert title to wide string
    std::wstring wide_title(desc.title.begin(), desc.title.end());
    
    // Create window
    m_hwnd = CreateWindowExW(
        ex_style,
        WINDOW_CLASS_NAME,
        wide_title.c_str(),
        style,
        x, y,
        rect.right - rect.left,
        rect.bottom - rect.top,
        desc.parent_id != INVALID_WINDOW_ID ? 
            reinterpret_cast<HWND>(desc.parent_id) : nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );
    
    if (!m_hwnd) {
        std::cerr << "CreateWindowEx failed: " << GetLastError() << std::endl;
        return false;
    }
    
    // Get device context
    m_hdc = GetDC(m_hwnd);
    if (!m_hdc) {
        std::cerr << "GetDC failed" << std::endl;
        return false;
    }
    
    // Store mapping for window proc
    g_hwnd_to_window[m_hwnd] = this;
    
    // Initialize state
    update_state();
    
    // Apply additional properties
    if (desc.always_on_top) {
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                     SWP_NOMOVE | SWP_NOSIZE);
    }
    
    if (desc.opacity < 1.0f) {
        SetWindowLong(m_hwnd, GWL_EXSTYLE, 
                      GetWindowLong(m_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(m_hwnd, 0, 
                                   static_cast<BYTE>(desc.opacity * 255), LWA_ALPHA);
    }
    
    // Show window if requested
    if (desc.maximized) {
        ShowWindow(m_hwnd, SW_MAXIMIZE);
    } else {
        ShowWindow(m_hwnd, SW_SHOW);
    }
    
    UpdateWindow(m_hwnd);
    
    return true;
}

void Win32Window::show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        update_state();
    }
}

void Win32Window::hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
        update_state();
    }
}

void Win32Window::close() {
    if (m_hwnd) {
        PostMessage(m_hwnd, WM_CLOSE, 0, 0);
    }
}

void Win32Window::focus() {
    if (m_hwnd) {
        SetForegroundWindow(m_hwnd);
        SetFocus(m_hwnd);
    }
}

void Win32Window::set_title(const std::string& title) {
    if (m_hwnd) {
        std::wstring wide_title(title.begin(), title.end());
        SetWindowTextW(m_hwnd, wide_title.c_str());
    }
}

std::string Win32Window::get_title() const {
    if (!m_hwnd) return "";
    
    wchar_t buffer[256];
    int length = GetWindowTextW(m_hwnd, buffer, 256);
    std::wstring wide_title(buffer, length);
    return std::string(wide_title.begin(), wide_title.end());
}

void Win32Window::set_size(int32_t width, int32_t height) {
    if (m_hwnd) {
        RECT rect = { 0, 0, width, height };
        DWORD style = GetWindowLong(m_hwnd, GWL_STYLE);
        DWORD ex_style = GetWindowLong(m_hwnd, GWL_EXSTYLE);
        AdjustWindowRectEx(&rect, style, FALSE, ex_style);
        
        SetWindowPos(m_hwnd, nullptr, 0, 0, 
                     rect.right - rect.left, rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOZORDER);
        update_state();
    }
}

void Win32Window::get_size(int32_t& width, int32_t& height) const {
    if (m_hwnd) {
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    } else {
        width = height = 0;
    }
}

void Win32Window::set_position(int32_t x, int32_t y) {
    if (m_hwnd) {
        SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, 
                     SWP_NOSIZE | SWP_NOZORDER);
        update_state();
    }
}

void Win32Window::get_position(int32_t& x, int32_t& y) const {
    if (m_hwnd) {
        RECT rect;
        GetWindowRect(m_hwnd, &rect);
        x = rect.left;
        y = rect.top;
    } else {
        x = y = 0;
    }
}

void Win32Window::minimize() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_MINIMIZE);
        update_state();
    }
}

void Win32Window::maximize() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_MAXIMIZE);
        update_state();
    }
}

void Win32Window::restore() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_RESTORE);
        update_state();
    }
}

void Win32Window::set_fullscreen(bool fullscreen) {
    if (!m_hwnd) return;
    
    static RECT s_windowed_rect;
    static DWORD s_windowed_style;
    
    if (fullscreen) {
        // Store current window rect and style
        GetWindowRect(m_hwnd, &s_windowed_rect);
        s_windowed_style = GetWindowLong(m_hwnd, GWL_STYLE);
        
        // Remove window decoration
        SetWindowLong(m_hwnd, GWL_STYLE, s_windowed_style & ~WS_OVERLAPPEDWINDOW);
        
        // Get monitor info
        HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitor_info = { sizeof(monitor_info) };
        GetMonitorInfo(monitor, &monitor_info);
        
        // Set fullscreen
        SetWindowPos(m_hwnd, HWND_TOP,
                     monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.top,
                     monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        // Restore windowed mode
        SetWindowLong(m_hwnd, GWL_STYLE, s_windowed_style);
        SetWindowPos(m_hwnd, nullptr,
                     s_windowed_rect.left,
                     s_windowed_rect.top,
                     s_windowed_rect.right - s_windowed_rect.left,
                     s_windowed_rect.bottom - s_windowed_rect.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
    
    update_state();
}

bool Win32Window::is_fullscreen() const {
    return m_state.fullscreen;
}

float Win32Window::get_dpi_scale() const {
    if (!m_hwnd) return 1.0f;
    
    UINT dpi = GetDpiForWindow(m_hwnd);
    return static_cast<float>(dpi) / 96.0f; // 96 DPI is 100% scaling
}

void Win32Window::get_framebuffer_size(int32_t& width, int32_t& height) const {
    get_size(width, height);
    float scale = get_dpi_scale();
    width = static_cast<int32_t>(width * scale);
    height = static_cast<int32_t>(height * scale);
}

bool Win32Window::create_render_surface(qcs::core::rendering::RenderingEngine* engine) {
    // TODO: Integrate with WebGPU surface creation
    // This would create a WebGPU surface using the HWND
    return false; // Placeholder
}

qcs::core::rendering::RenderSurface* Win32Window::get_render_surface() const {
    return m_render_surface.get();
}

bool Win32Window::pump_events() {
    MSG msg;
    bool had_events = false;
    
    while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        had_events = true;
    }
    
    return had_events;
}

void Win32Window::update_state() {
    if (!m_hwnd) return;
    
    // Update position and size
    RECT window_rect, client_rect;
    GetWindowRect(m_hwnd, &window_rect);
    GetClientRect(m_hwnd, &client_rect);
    
    m_state.x = window_rect.left;
    m_state.y = window_rect.top;
    m_state.width = client_rect.right - client_rect.left;
    m_state.height = client_rect.bottom - client_rect.top;
    
    // Update DPI scale
    m_state.dpi_scale = get_dpi_scale();
    
    // Update window state flags
    WINDOWPLACEMENT placement = { sizeof(placement) };
    GetWindowPlacement(m_hwnd, &placement);
    
    m_state.minimized = placement.showCmd == SW_MINIMIZE;
    m_state.maximized = placement.showCmd == SW_MAXIMIZE;
    m_state.focused = GetForegroundWindow() == m_hwnd;
    m_state.visible = IsWindowVisible(m_hwnd);
    
    // Check if fullscreen (simplified check)
    HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info = { sizeof(monitor_info) };
    GetMonitorInfo(monitor, &monitor_info);
    
    m_state.fullscreen = (window_rect.left == monitor_info.rcMonitor.left &&
                         window_rect.top == monitor_info.rcMonitor.top &&
                         window_rect.right == monitor_info.rcMonitor.right &&
                         window_rect.bottom == monitor_info.rcMonitor.bottom);
    
    m_state.last_update_time = std::chrono::steady_clock::now().time_since_epoch().count();
}

LRESULT CALLBACK Win32Window::window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // Find the window instance
    auto it = g_hwnd_to_window.find(hwnd);
    if (it != g_hwnd_to_window.end()) {
        it->second->handle_message(msg, wparam, lparam);
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Win32Window::handle_message(UINT msg, WPARAM wparam, LPARAM lparam) {
    if (!m_event_callback) return;
    
    WindowEvent event;
    event.window_id = m_id;
    event.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    
    switch (msg) {
        case WM_SIZE:
            event.type = WindowEventType::Resize;
            event.param1 = LOWORD(lparam); // width
            event.param2 = HIWORD(lparam); // height
            update_state();
            break;
            
        case WM_MOVE:
            event.type = WindowEventType::Move;
            event.param1 = LOWORD(lparam); // x
            event.param2 = HIWORD(lparam); // y
            update_state();
            break;
            
        case WM_CLOSE:
            event.type = WindowEventType::Close;
            break;
            
        case WM_SETFOCUS:
            event.type = WindowEventType::Focus;
            update_state();
            break;
            
        case WM_KILLFOCUS:
            event.type = WindowEventType::LostFocus;
            update_state();
            break;
            
        case WM_DPICHANGED:
            event.type = WindowEventType::DPIChange;
            event.float_param = get_dpi_scale();
            update_state();
            break;
            
        default:
            return; // Don't dispatch unhandled events
    }
    
    m_event_callback(event);
}

#endif // PLATFORM_WINDOWS

// ============================================================================
// WINDOW MANAGER IMPLEMENTATION
// ============================================================================

WindowManager::WindowManager() {
    // Initialize performance tracking
    m_stats = {};
}

WindowManager::~WindowManager() {
    shutdown();
}

bool WindowManager::initialize(qcs::core::rendering::RenderingEngine* rendering_engine) {
    m_rendering_engine = rendering_engine;
    
    // Platform-specific initialization
#ifdef PLATFORM_WINDOWS
    // Enable high-DPI support
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif
    
    return true;
}

void WindowManager::shutdown() {
    close_all_windows();
    
    std::lock_guard<std::mutex> lock(m_windows_mutex);
    m_windows.clear();
    
    m_next_window_id = 1;
    m_main_window_id = INVALID_WINDOW_ID;
    m_focused_window_id = INVALID_WINDOW_ID;
}

WindowId WindowManager::generate_window_id() {
    return m_next_window_id++;
}

WindowId WindowManager::create_window(const WindowDesc& desc) {
    WindowId window_id = generate_window_id();
    
    // Create platform-specific window
    std::unique_ptr<IWindow> window;
    
#ifdef PLATFORM_WINDOWS
    window = std::make_unique<Win32Window>(desc);
#else
    std::cerr << "Platform not supported for window creation" << std::endl;
    return INVALID_WINDOW_ID;
#endif
    
    if (!window || !window->is_valid()) {
        std::cerr << "Failed to create window" << std::endl;
        return INVALID_WINDOW_ID;
    }
    
    // Set window ID (platform implementations need this)
    // Note: This is a simplified approach - real implementation would set ID during construction
    
    // Set up event callback
    window->set_event_callback([this](const WindowEvent& event) {
        dispatch_event(event);
        return true; // Event handled
    });
    
    // Create render surface if rendering engine is available
    if (m_rendering_engine) {
        window->create_render_surface(m_rendering_engine);
    }
    
    // Store window
    {
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        m_windows[window_id] = std::move(window);
    }
    
    // Set as main window if it's the first one
    if (m_main_window_id == INVALID_WINDOW_ID) {
        m_main_window_id = window_id;
    }
    
    // Update stats
    m_stats.windows_created++;
    
    return window_id;
}

bool WindowManager::destroy_window(WindowId window_id) {
    std::lock_guard<std::mutex> lock(m_windows_mutex);
    
    auto it = m_windows.find(window_id);
    if (it == m_windows.end()) {
        return false;
    }
    
    // Update tracking
    if (m_main_window_id == window_id) {
        m_main_window_id = INVALID_WINDOW_ID;
        // Find new main window
        for (const auto& pair : m_windows) {
            if (pair.first != window_id) {
                m_main_window_id = pair.first;
                break;
            }
        }
    }
    
    if (m_focused_window_id == window_id) {
        m_focused_window_id = INVALID_WINDOW_ID;
    }
    
    // Remove window
    m_windows.erase(it);
    m_stats.windows_destroyed++;
    
    return true;
}

IWindow* WindowManager::get_window(WindowId window_id) {
    std::lock_guard<std::mutex> lock(m_windows_mutex);
    
    auto it = m_windows.find(window_id);
    return (it != m_windows.end()) ? it->second.get() : nullptr;
}

const IWindow* WindowManager::get_window(WindowId window_id) const {
    std::lock_guard<std::mutex> lock(m_windows_mutex);
    
    auto it = m_windows.find(window_id);
    return (it != m_windows.end()) ? it->second.get() : nullptr;
}

std::vector<WindowId> WindowManager::get_all_window_ids() const {
    std::lock_guard<std::mutex> lock(m_windows_mutex);
    
    std::vector<WindowId> ids;
    ids.reserve(m_windows.size());
    
    for (const auto& pair : m_windows) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

size_t WindowManager::get_window_count() const {
    std::lock_guard<std::mutex> lock(m_windows_mutex);
    return m_windows.size();
}

void WindowManager::update_all_windows() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(m_windows_mutex);
    
    for (auto& pair : m_windows) {
        pair.second->pump_events();
    }
    
    cleanup_destroyed_windows();
    
    // Update performance stats
    auto end_time = std::chrono::high_resolution_clock::now();
    double process_time = std::chrono::duration<double, std::micro>(end_time - start_time).count();
    
    m_stats.avg_event_process_time = (m_stats.avg_event_process_time * 0.9) + (process_time * 0.1);
}

bool WindowManager::handle_system_events() {
    bool had_events = false;
    
    // Process platform-specific global events
#ifdef PLATFORM_WINDOWS
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false; // Application should quit
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        had_events = true;
    }
#endif
    
    return had_events;
}

void WindowManager::dispatch_event(const WindowEvent& event) {
    // Update focus tracking
    if (event.type == WindowEventType::Focus) {
        update_focus_tracking(event.window_id);
    } else if (event.type == WindowEventType::LostFocus && 
               m_focused_window_id == event.window_id) {
        m_focused_window_id = INVALID_WINDOW_ID;
    }
    
    // Dispatch to global callback
    if (m_global_event_callback) {
        m_global_event_callback(event);
    }
    
    // Handle window close events
    if (event.type == WindowEventType::Close) {
        // Schedule window for destruction
        // Note: We don't destroy immediately to avoid iterator invalidation
    }
    
    m_stats.events_processed++;
}

void WindowManager::update_focus_tracking(WindowId new_focus) {
    m_focused_window_id = new_focus;
}

void WindowManager::cleanup_destroyed_windows() {
    // Remove any windows that have been marked for destruction
    // This is a simplified implementation - real version would handle cleanup properly
}

bool WindowManager::should_quit() const {
    return get_window_count() == 0;
}

void WindowManager::close_all_windows() {
    auto window_ids = get_all_window_ids();
    for (WindowId id : window_ids) {
        if (auto* window = get_window(id)) {
            window->close();
        }
    }
}

void WindowManager::minimize_all_windows() {
    auto window_ids = get_all_window_ids();
    for (WindowId id : window_ids) {
        if (auto* window = get_window(id)) {
            window->minimize();
        }
    }
}

void WindowManager::restore_all_windows() {
    auto window_ids = get_all_window_ids();
    for (WindowId id : window_ids) {
        if (auto* window = get_window(id)) {
            window->restore();
        }
    }
}

void WindowManager::set_global_event_callback(WindowEventCallback callback) {
    m_global_event_callback = std::move(callback);
}

void WindowManager::set_rendering_engine(qcs::core::rendering::RenderingEngine* engine) {
    m_rendering_engine = engine;
    
    // Update existing windows
    auto window_ids = get_all_window_ids();
    for (WindowId id : window_ids) {
        if (auto* window = get_window(id)) {
            window->create_render_surface(engine);
        }
    }
}

qcs::core::rendering::RenderSurface* WindowManager::get_render_surface_for_window(WindowId window_id) {
    if (auto* window = get_window(window_id)) {
        return window->get_render_surface();
    }
    return nullptr;
}

void WindowManager::reset_performance_stats() {
    m_stats = {};
}

bool WindowManager::is_platform_supported() {
#ifdef PLATFORM_WINDOWS
    return true;
#else
    return false;
#endif
}

std::string WindowManager::get_platform_name() {
#ifdef PLATFORM_WINDOWS
    return "Windows";
#elif defined(PLATFORM_MACOS)
    return "macOS";
#elif defined(PLATFORM_LINUX)
    return "Linux";
#else
    return "Unknown";
#endif
}

std::vector<std::string> WindowManager::get_supported_features() {
    std::vector<std::string> features;
    
    features.push_back("Multi-window support");
    features.push_back("High-DPI awareness");
    features.push_back("WebGPU integration");
    features.push_back("Event system");
    
#ifdef PLATFORM_WINDOWS
    features.push_back("Windows native");
    features.push_back("DWM integration");
#endif
    
    return features;
}

// ============================================================================
// FACTORY FUNCTIONS
// ============================================================================

std::unique_ptr<WindowManager> create_window_manager() {
    return std::make_unique<WindowManager>();
}

WindowManager& get_global_window_manager() {
    static WindowManager instance;
    return instance;
}

} // namespace qcs::ui::window