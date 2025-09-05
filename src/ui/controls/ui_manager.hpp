/**
 * @file ui_manager.hpp
 * @brief Main UI management system for QuantumCanvas Studio
 * 
 * Provides immediate-mode UI system with professional controls optimized for
 * creative workflows. Integrates with Dear ImGui for rapid development while
 * providing custom controls for specialized creative tools.
 * 
 * @author QuantumCanvas Team
 * @date 2025-01-14
 * @version 1.0.0-alpha
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

// ImGui integration
struct ImGuiContext;
struct ImDrawData;

// Forward declarations
namespace qcs::ui::window {
    class WindowManager;
    class IWindow;
    using WindowId = uint32_t;
}

namespace qcs::ui::events {
    struct InputEvent;
    struct MouseEvent;
    struct KeyboardEvent;
    class InputHandler;
}

namespace qcs::core::rendering {
    class RenderingEngine;
}

namespace qcs::ui::controls {

// ============================================================================
// UI THEME AND STYLING
// ============================================================================

/**
 * @brief UI color scheme
 */
struct UIColors {
    // Base colors
    uint32_t background = 0xFF2D2D30;       // Dark gray background
    uint32_t surface = 0xFF3E3E42;          // Surface color
    uint32_t primary = 0xFF0078D4;          // Primary accent blue
    uint32_t secondary = 0xFF6CB4EE;        // Secondary blue
    uint32_t success = 0xFF16C60C;          // Success green
    uint32_t warning = 0xFFFFB900;          // Warning orange
    uint32_t error = 0xFFD13438;            // Error red
    
    // Text colors
    uint32_t text_primary = 0xFFFFFFFF;     // White text
    uint32_t text_secondary = 0xFFB3B3B3;   // Light gray text
    uint32_t text_disabled = 0xFF666666;    // Disabled text
    
    // Interactive colors
    uint32_t button_normal = 0xFF404040;    // Normal button
    uint32_t button_hovered = 0xFF4A4A4A;   // Hovered button
    uint32_t button_active = 0xFF555555;    // Active button
    uint32_t button_disabled = 0xFF2A2A2A;  // Disabled button
    
    // Border and separator colors
    uint32_t border = 0xFF555555;           // Border color
    uint32_t separator = 0xFF3A3A3A;        // Separator lines
    
    // Special creative tool colors
    uint32_t canvas_bg = 0xFF1E1E1E;        // Canvas background
    uint32_t grid_line = 0xFF333333;        // Grid lines
    uint32_t selection = 0x4000AAFF;        // Selection overlay
    uint32_t guide_line = 0xFF00FF00;       // Guide lines
};

/**
 * @brief UI layout metrics
 */
struct UIMetrics {
    float window_rounding = 4.0f;
    float frame_rounding = 2.0f;
    float item_spacing_x = 8.0f;
    float item_spacing_y = 4.0f;
    float indent_spacing = 21.0f;
    float scrollbar_size = 14.0f;
    float grab_min_size = 10.0f;
    
    // Font sizes
    float font_size_small = 12.0f;
    float font_size_normal = 14.0f;
    float font_size_large = 18.0f;
    float font_size_title = 24.0f;
    
    // Panel dimensions
    float toolbar_height = 40.0f;
    float property_panel_width = 280.0f;
    float status_bar_height = 24.0f;
    
    // Touch-friendly sizes
    float touch_button_size = 44.0f;
    float touch_spacing = 12.0f;
};

/**
 * @brief UI theme configuration
 */
struct UITheme {
    UIColors colors;
    UIMetrics metrics;
    std::string name = "Dark Professional";
    
    // Theme presets
    static UITheme create_dark_theme();
    static UITheme create_light_theme();
    static UITheme create_high_contrast_theme();
};

// ============================================================================
// UI LAYOUT SYSTEM
// ============================================================================

/**
 * @brief Layout direction
 */
enum class LayoutDirection {
    Horizontal,
    Vertical
};

/**
 * @brief Panel docking positions
 */
enum class DockPosition {
    None,
    Left,
    Right, 
    Top,
    Bottom,
    Center
};

/**
 * @brief UI panel base class
 */
class UIPanel {
protected:
    std::string m_title;
    bool m_visible = true;
    bool m_closeable = true;
    DockPosition m_dock_position = DockPosition::None;
    float m_width = 280.0f;
    float m_height = 400.0f;
    
public:
    UIPanel(const std::string& title) : m_title(title) {}
    virtual ~UIPanel() = default;
    
    // Panel properties
    const std::string& get_title() const { return m_title; }
    void set_title(const std::string& title) { m_title = title; }
    
    bool is_visible() const { return m_visible; }
    void set_visible(bool visible) { m_visible = visible; }
    
    DockPosition get_dock_position() const { return m_dock_position; }
    void set_dock_position(DockPosition position) { m_dock_position = position; }
    
    void get_size(float& width, float& height) const { width = m_width; height = m_height; }
    void set_size(float width, float height) { m_width = width; m_height = height; }
    
    // Panel lifecycle
    virtual void on_show() {}
    virtual void on_hide() {}
    virtual void on_resize(float width, float height) { m_width = width; m_height = height; }
    
    // Rendering
    virtual void render() = 0;  // Must be implemented by derived classes
};

/**
 * @brief Property panel for tool settings
 */
class PropertyPanel : public UIPanel {
private:
    struct Property {
        std::string name;
        std::string category;
        std::function<void()> render_func;
    };
    
    std::vector<Property> m_properties;
    std::string m_active_category = "General";
    
public:
    PropertyPanel();
    
    // Property management
    void add_property(const std::string& name, const std::string& category,
                     std::function<void()> render_func);
    void remove_property(const std::string& name);
    void clear_properties();
    
    void set_active_category(const std::string& category) { m_active_category = category; }
    
    void render() override;
};

/**
 * @brief Toolbar with tool buttons
 */
class Toolbar : public UIPanel {
public:
    struct ToolButton {
        std::string id;
        std::string tooltip;
        std::function<void()> callback;
        bool enabled = true;
        bool selected = false;
        void* icon_texture = nullptr;  // ImTextureID
    };
    
private:
    std::vector<ToolButton> m_buttons;
    std::string m_active_tool_id;
    LayoutDirection m_layout = LayoutDirection::Horizontal;
    
public:
    Toolbar();
    
    // Button management
    void add_button(const ToolButton& button);
    void remove_button(const std::string& id);
    void set_button_enabled(const std::string& id, bool enabled);
    void set_active_tool(const std::string& id);
    std::string get_active_tool() const { return m_active_tool_id; }
    
    void set_layout_direction(LayoutDirection direction) { m_layout = direction; }
    
    void render() override;
};

/**
 * @brief Canvas viewport panel
 */
class CanvasPanel : public UIPanel {
private:
    float m_zoom = 1.0f;
    float m_pan_x = 0.0f;
    float m_pan_y = 0.0f;
    bool m_show_grid = true;
    bool m_show_rulers = true;
    float m_grid_size = 20.0f;
    
    // Canvas interaction
    bool m_panning = false;
    bool m_zooming = false;
    
public:
    CanvasPanel();
    
    // View transformation
    float get_zoom() const { return m_zoom; }
    void set_zoom(float zoom) { m_zoom = std::max(0.1f, std::min(10.0f, zoom)); }
    void zoom_in() { set_zoom(m_zoom * 1.2f); }
    void zoom_out() { set_zoom(m_zoom / 1.2f); }
    void zoom_to_fit();
    void zoom_to_selection();
    
    void get_pan(float& x, float& y) const { x = m_pan_x; y = m_pan_y; }
    void set_pan(float x, float y) { m_pan_x = x; m_pan_y = y; }
    
    // Grid and rulers
    bool is_grid_visible() const { return m_show_grid; }
    void set_grid_visible(bool visible) { m_show_grid = visible; }
    
    bool are_rulers_visible() const { return m_show_rulers; }
    void set_rulers_visible(bool visible) { m_show_rulers = visible; }
    
    float get_grid_size() const { return m_grid_size; }
    void set_grid_size(float size) { m_grid_size = size; }
    
    // Coordinate conversion
    void screen_to_canvas(float screen_x, float screen_y, float& canvas_x, float& canvas_y) const;
    void canvas_to_screen(float canvas_x, float canvas_y, float& screen_x, float& screen_y) const;
    
    void render() override;
    
protected:
    void render_grid();
    void render_rulers();
    void handle_canvas_input();
};

// ============================================================================
// UI MANAGER
// ============================================================================

/**
 * @brief Main UI management system
 * 
 * Manages the entire UI system including panels, themes, input handling,
 * and integration with the rendering engine. Provides immediate-mode UI
 * with professional layout and theming.
 */
class UIManager : public qcs::ui::events::InputHandler {
private:
    // Core systems
    qcs::ui::window::WindowManager* m_window_manager = nullptr;
    qcs::core::rendering::RenderingEngine* m_rendering_engine = nullptr;
    
    // ImGui integration
    ImGuiContext* m_imgui_context = nullptr;
    bool m_imgui_initialized = false;
    
    // UI state
    UITheme m_theme;
    std::vector<std::unique_ptr<UIPanel>> m_panels;
    std::unique_ptr<PropertyPanel> m_property_panel;
    std::unique_ptr<Toolbar> m_main_toolbar;
    std::unique_ptr<CanvasPanel> m_canvas_panel;
    
    // Layout state
    bool m_show_demo_window = false;
    bool m_show_metrics = false;
    float m_main_menu_height = 0.0f;
    
    // Performance tracking
    struct UIPerformanceStats {
        float frame_time = 0.0f;
        float render_time = 0.0f;
        int32_t draw_calls = 0;
        int32_t vertices = 0;
        size_t memory_usage = 0;
    } m_stats;

public:
    UIManager();
    ~UIManager();
    
    // Initialization
    bool initialize(qcs::ui::window::WindowManager* window_manager,
                   qcs::core::rendering::RenderingEngine* rendering_engine);
    void shutdown();
    bool is_initialized() const { return m_imgui_initialized; }
    
    // Frame lifecycle
    void begin_frame();  // Call at start of frame
    void end_frame();    // Call at end of frame
    void render();       // Render all UI elements
    
    // Theme management
    void set_theme(const UITheme& theme);
    const UITheme& get_theme() const { return m_theme; }
    void apply_theme_colors();
    
    // Panel management
    void add_panel(std::unique_ptr<UIPanel> panel);
    void remove_panel(const std::string& title);
    UIPanel* get_panel(const std::string& title);
    void show_panel(const std::string& title);
    void hide_panel(const std::string& title);
    
    // Built-in panels
    PropertyPanel* get_property_panel() { return m_property_panel.get(); }
    Toolbar* get_main_toolbar() { return m_main_toolbar.get(); }
    CanvasPanel* get_canvas_panel() { return m_canvas_panel.get(); }
    
    // Main menu and dialogs
    void render_main_menu();
    void show_about_dialog();
    void show_preferences_dialog();
    void show_demo_window(bool show = true) { m_show_demo_window = show; }
    
    // Input handling (from InputHandler interface)
    bool handle_mouse_event(const qcs::ui::events::MouseEvent& event) override;
    bool handle_keyboard_event(const qcs::ui::events::KeyboardEvent& event) override;
    int32_t get_priority() const override { return 1000; } // High priority
    
    // Utility functions
    bool wants_capture_mouse() const;
    bool wants_capture_keyboard() const;
    
    // Performance and diagnostics
    UIPerformanceStats get_performance_stats() const { return m_stats; }
    void show_metrics(bool show = true) { m_show_metrics = show; }
    
    // Immediate-mode UI helpers
    bool button(const std::string& label, float width = 0.0f, float height = 0.0f);
    bool image_button(void* texture_id, float width, float height, const std::string& tooltip = "");
    bool color_picker(const std::string& label, float* color);
    bool slider_float(const std::string& label, float* value, float min_val, float max_val);
    bool drag_float(const std::string& label, float* value, float speed = 1.0f);
    bool checkbox(const std::string& label, bool* value);
    bool combo(const std::string& label, int* current_item, const std::vector<std::string>& items);
    
    // Layout helpers
    void same_line();
    void separator();
    void spacing();
    void indent(float indent_width = 0.0f);
    void unindent(float indent_width = 0.0f);
    void begin_group();
    void end_group();
    
    // Text rendering
    void text(const std::string& text);
    void text_colored(uint32_t color, const std::string& text);
    void text_disabled(const std::string& text);
    void text_wrapped(const std::string& text);

private:
    // Internal initialization
    bool setup_imgui();
    void setup_fonts();
    void setup_style();
    
    // ImGui platform integration
    void setup_platform_io();
    void update_mouse_data();
    void update_keyboard_data();
    
    // Rendering helpers
    void render_dockspace();
    void render_panels();
    void render_debug_info();
    
    // Event conversion
    void convert_mouse_event(const qcs::ui::events::MouseEvent& event);
    void convert_keyboard_event(const qcs::ui::events::KeyboardEvent& event);
};

// ============================================================================
// GLOBAL UI ACCESS
// ============================================================================

/**
 * @brief Get global UI manager instance
 */
UIManager& get_global_ui_manager();

/**
 * @brief Initialize global UI system
 */
bool initialize_global_ui(qcs::ui::window::WindowManager* window_manager,
                         qcs::core::rendering::RenderingEngine* rendering_engine);

/**
 * @brief Shutdown global UI system
 */
void shutdown_global_ui();

} // namespace qcs::ui::controls