/**
 * @file mobile_ui_manager.hpp
 * @brief Mobile-optimized UI management for QuantumCanvas Studio
 * 
 * Specialized UI system for mobile platforms (iOS/Android) with:
 * - Touch-optimized controls (44pt minimum touch targets)
 * - Adaptive layouts for different screen sizes
 * - Gesture-based navigation
 * - Apple Pencil/S-Pen integration
 * - Accessibility compliance (VoiceOver/TalkBack)
 * - Battery and performance optimization
 * - GDPR/EU Privacy compliance
 * 
 * @author QuantumCanvas Team
 * @date 2025-01-14
 * @version 1.0.0-Mobile
 */

#pragma once

#include "../controls/ui_manager.hpp"
#include "../events/input_events.hpp"
#include <memory>
#include <vector>
#include <unordered_map>

// Platform detection
#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
    #define QCS_PLATFORM_IOS
#endif
#endif

#ifdef __ANDROID__
    #define QCS_PLATFORM_ANDROID
#endif

namespace qcs::ui::controls::mobile {

// ============================================================================
// MOBILE DEVICE CHARACTERISTICS
// ============================================================================

/**
 * @brief Mobile device form factor detection
 */
enum class DeviceFormFactor {
    Phone,              // < 6.5" diagonal
    PhonePlus,          // 6.5" - 7" diagonal  
    Tablet,             // 7" - 11" diagonal
    TabletPro,          // 11"+ diagonal
    Foldable,           // Foldable devices
    Unknown
};

/**
 * @brief Screen orientation states
 */
enum class ScreenOrientation {
    Portrait,           // Height > Width
    Landscape,          // Width > Height
    PortraitUpsideDown, // iOS specific
    LandscapeLeft,      // Rotated left
    LandscapeRight,     // Rotated right
    FaceUp,            // Flat face up
    FaceDown,          // Flat face down
    Unknown
};

/**
 * @brief Mobile UI configuration
 */
struct MobileUIConfig {
    // Touch target specifications (iOS HIG / Android Material Design)
    float min_touch_target_size = 44.0f;        // 44pt iOS / 48dp Android minimum
    float recommended_touch_target_size = 48.0f; // Recommended size
    float touch_target_spacing = 8.0f;          // Minimum spacing between targets
    
    // Gesture thresholds
    float swipe_threshold_px = 50.0f;           // Minimum swipe distance
    float pan_threshold_px = 10.0f;             // Pan gesture threshold
    float pinch_threshold = 0.1f;               // Pinch scale threshold
    float rotation_threshold_deg = 5.0f;        // Rotation gesture threshold
    int64_t long_press_duration_ms = 500;       // Long press duration
    int64_t double_tap_duration_ms = 300;       // Double tap max interval
    
    // Stylus/Pencil settings
    bool enable_stylus_rejection = true;        // Palm rejection
    float stylus_precision_mode_scale = 2.0f;   // UI scale for precision
    bool stylus_hover_preview = true;           // Show hover preview
    
    // Accessibility compliance
    bool enable_dynamic_type = true;            // Respect system text size
    float min_font_size = 12.0f;               // Minimum readable font size
    float max_font_size = 32.0f;               // Maximum font size
    bool high_contrast_mode = false;           // High contrast for accessibility
    bool reduce_motion = false;                // Reduced animations
    
    // Performance optimization
    bool enable_view_recycling = true;         // Recycle UI elements
    int32_t max_concurrent_animations = 3;     // Limit animations
    float animation_speed_multiplier = 1.0f;   // Animation speed control
    bool enable_low_power_mode = false;        // Battery saving mode
    
    static MobileUIConfig create_ios_config();
    static MobileUIConfig create_android_config();
    static MobileUIConfig create_accessibility_optimized();
};

// ============================================================================
// MOBILE UI COMPONENTS
// ============================================================================

/**
 * @brief Mobile-optimized toolbar
 */
class MobileToolbar : public UIPanel {
private:
    enum class ToolbarLayout {
        TopHorizontal,      // Traditional top toolbar
        BottomHorizontal,   // iOS style bottom toolbar
        LeftVertical,       // Collapsed side toolbar
        FloatingCircular,   // Floating action button style
        AdaptiveOverlay     // Context-sensitive overlay
    };
    
    struct MobileToolButton {
        std::string id;
        std::string title;
        std::string icon_name;
        std::function<void()> callback;
        bool enabled = true;
        bool visible = true;
        bool primary = false;           // Primary action (highlighted)
        float touch_area_expansion = 0.0f; // Expand touch area beyond visual
        
        // Accessibility
        std::string accessibility_label;
        std::string accessibility_hint;
        bool accessibility_important = true;
    };
    
    ToolbarLayout m_layout = ToolbarLayout::BottomHorizontal;
    std::vector<MobileToolButton> m_buttons;
    MobileUIConfig m_ui_config;
    
    // Adaptive behavior
    bool m_auto_hide_enabled = true;
    bool m_currently_hidden = false;
    float m_hide_timer = 0.0f;
    float m_auto_hide_delay = 3.0f;     // Seconds of inactivity before hiding
    
    // Touch feedback
    bool m_haptic_feedback_enabled = true;
    
public:
    MobileToolbar(ToolbarLayout layout = ToolbarLayout::BottomHorizontal);
    
    // Button management
    void add_button(const MobileToolButton& button);
    void remove_button(const std::string& id);
    void set_button_enabled(const std::string& id, bool enabled);
    void set_button_visible(const std::string& id, bool visible);
    void set_primary_button(const std::string& id);
    
    // Layout management
    void set_layout(ToolbarLayout layout);
    void adapt_to_orientation(ScreenOrientation orientation);
    void adapt_to_form_factor(DeviceFormFactor form_factor);
    
    // Auto-hide behavior
    void set_auto_hide_enabled(bool enabled) { m_auto_hide_enabled = enabled; }
    void set_auto_hide_delay(float seconds) { m_auto_hide_delay = seconds; }
    void show_temporarily(float duration = 5.0f);
    
    // Accessibility
    void update_accessibility_labels();
    void handle_accessibility_focus_change();
    
    void render() override;
    
private:
    void calculate_button_layout();
    void render_button(const MobileToolButton& button, const qcs::ui::events::Point2D& position, const qcs::ui::events::Point2D& size);
    bool handle_button_touch(const MobileToolButton& button, const qcs::ui::events::Point2D& touch_pos);
    void trigger_haptic_feedback();
};

/**
 * @brief Mobile-optimized property panel with collapsible sections
 */
class MobilePropertyPanel : public PropertyPanel {
private:
    struct PropertySection {
        std::string title;
        bool expanded = true;
        bool expandable = true;
        std::vector<std::string> property_ids;
        float header_height = 44.0f;    // Touch-friendly header
    };
    
    std::vector<PropertySection> m_sections;
    MobileUIConfig m_ui_config;
    
    // Scroll state
    float m_scroll_offset = 0.0f;
    float m_scroll_velocity = 0.0f;
    bool m_scrolling = false;
    
    // Touch interaction
    qcs::ui::events::Point2D m_last_touch_pos;
    bool m_touch_active = false;
    
public:
    MobilePropertyPanel();
    
    // Section management
    void add_section(const std::string& title, bool expanded = true, bool expandable = true);
    void remove_section(const std::string& title);
    void set_section_expanded(const std::string& title, bool expanded);
    void add_property_to_section(const std::string& section_title, 
                                const std::string& property_name, 
                                std::function<void()> render_func);
    
    // Touch scrolling
    bool handle_touch_event(const qcs::ui::events::TouchEvent& event);
    void update_scroll_physics(float delta_time);
    
    void render() override;
    
private:
    void render_section_header(const PropertySection& section, float y_offset);
    void render_section_content(const PropertySection& section, float y_offset);
    float calculate_total_height() const;
    bool is_point_in_section_header(const PropertySection& section, 
                                  const qcs::ui::events::Point2D& point, 
                                  float section_y) const;
};

/**
 * @brief Mobile canvas with gesture navigation
 */
class MobileCanvasPanel : public CanvasPanel {
private:
    // Gesture recognition
    enum class GestureState {
        None,
        Panning,
        Zooming,
        Rotating,
        Drawing
    };
    
    GestureState m_current_gesture = GestureState::None;
    
    // Multi-touch state
    struct TouchPoint {
        uint32_t id;
        qcs::ui::events::Point2D current_pos;
        qcs::ui::events::Point2D start_pos;
        qcs::ui::events::Point2D velocity;
        int64_t timestamp;
        bool is_stylus = false;
    };
    std::unordered_map<uint32_t, TouchPoint> m_active_touches;
    
    // Gesture parameters
    float m_pinch_start_distance = 0.0f;
    float m_pinch_start_zoom = 1.0f;
    qcs::ui::events::Point2D m_pan_start_offset;
    float m_rotation_start_angle = 0.0f;
    float m_rotation_accumulated = 0.0f;
    
    // Stylus/Apple Pencil specific
    bool m_stylus_mode_active = false;
    float m_stylus_zoom_factor = 2.0f;      // Zoom in when stylus detected
    bool m_palm_rejection_active = true;
    
    // Momentum scrolling
    qcs::ui::events::Point2D m_momentum_velocity;
    float m_momentum_friction = 0.95f;
    
    // Touch feedback
    bool m_show_touch_indicators = false;   // For debugging
    
public:
    MobileCanvasPanel();
    
    // Gesture handling
    bool handle_touch_event(const qcs::ui::events::TouchEvent& event);
    bool handle_stylus_event(const qcs::ui::events::StylusEvent& event);
    void update_gesture_recognition(float delta_time);
    
    // Stylus mode
    void set_stylus_mode_active(bool active);
    bool is_stylus_mode_active() const { return m_stylus_mode_active; }
    void set_stylus_zoom_factor(float factor) { m_stylus_zoom_factor = factor; }
    
    // Palm rejection
    void set_palm_rejection_enabled(bool enabled) { m_palm_rejection_active = enabled; }
    bool is_palm_rejection_enabled() const { return m_palm_rejection_active; }
    
    // Touch indicators (for debugging)
    void set_touch_indicators_visible(bool visible) { m_show_touch_indicators = visible; }
    
    void render() override;
    
private:
    void process_touch_down(uint32_t touch_id, const qcs::ui::events::Point2D& pos, bool is_stylus);
    void process_touch_move(uint32_t touch_id, const qcs::ui::events::Point2D& pos);
    void process_touch_up(uint32_t touch_id);
    
    void start_pan_gesture();
    void update_pan_gesture();
    void end_pan_gesture();
    
    void start_zoom_gesture();
    void update_zoom_gesture();
    void end_zoom_gesture();
    
    void start_rotation_gesture();
    void update_rotation_gesture();
    void end_rotation_gesture();
    
    void update_momentum_scrolling(float delta_time);
    bool is_palm_touch(const TouchPoint& touch) const;
    
    void render_touch_indicators();
};

// ============================================================================
// MOBILE UI MANAGER
// ============================================================================

/**
 * @brief Mobile-optimized UI manager
 */
class MobileUIManager : public UIManager {
private:
    // Mobile-specific configuration
    MobileUIConfig m_mobile_config;
    DeviceFormFactor m_device_form_factor = DeviceFormFactor::Unknown;
    ScreenOrientation m_current_orientation = ScreenOrientation::Portrait;
    
    // Mobile UI components
    std::unique_ptr<MobileToolbar> m_mobile_toolbar;
    std::unique_ptr<MobilePropertyPanel> m_mobile_property_panel;
    std::unique_ptr<MobileCanvasPanel> m_mobile_canvas_panel;
    
    // Gesture recognition system
    std::unique_ptr<qcs::ui::events::InputPredictor> m_input_predictor;
    
    // Layout adaptation
    bool m_layout_needs_update = true;
    float m_safe_area_insets[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // top, right, bottom, left
    
    // Performance optimization
    bool m_low_power_mode = false;
    int32_t m_active_animation_count = 0;
    
    // Accessibility state
    bool m_accessibility_enabled = false;
    bool m_voice_over_active = false;
    bool m_switch_control_active = false;
    float m_dynamic_type_scale = 1.0f;
    
    // Battery and thermal management
    float m_battery_level = 1.0f;
    bool m_low_power_mode_system = false;
    int32_t m_thermal_state = 0;  // Platform-specific thermal state

public:
    MobileUIManager();
    virtual ~MobileUIManager();
    
    // Mobile-specific initialization
    bool initialize(qcs::ui::window::WindowManager* window_manager,
                   qcs::core::rendering::RenderingEngine* rendering_engine) override;
    void shutdown() override;
    
    // Mobile configuration
    void set_mobile_config(const MobileUIConfig& config);
    const MobileUIConfig& get_mobile_config() const { return m_mobile_config; }
    
    // Device adaptation
    void set_device_form_factor(DeviceFormFactor form_factor);
    void handle_orientation_change(ScreenOrientation new_orientation);
    void set_safe_area_insets(float top, float right, float bottom, float left);
    
    // Mobile UI components
    MobileToolbar* get_mobile_toolbar() { return m_mobile_toolbar.get(); }
    MobilePropertyPanel* get_mobile_property_panel() { return m_mobile_property_panel.get(); }
    MobileCanvasPanel* get_mobile_canvas_panel() { return m_mobile_canvas_panel.get(); }
    
    // Gesture and input handling
    bool handle_touch_event(const qcs::ui::events::TouchEvent& event) override;
    bool handle_stylus_event(const qcs::ui::events::StylusEvent& event);
    bool handle_gesture_event(const qcs::ui::events::GestureEvent& event);
    
    // Frame lifecycle (mobile-optimized)
    void begin_frame() override;
    void end_frame() override;
    void render() override;
    
    // Layout management
    void update_layout_for_orientation();
    void adapt_ui_for_form_factor();
    void invalidate_layout() { m_layout_needs_update = true; }
    
    // Accessibility support
    void set_accessibility_enabled(bool enabled);
    void set_voice_over_active(bool active);
    void set_switch_control_active(bool active);
    void set_dynamic_type_scale(float scale);
    void handle_accessibility_settings_change();
    
    // Performance management
    void set_low_power_mode(bool enabled);
    void handle_thermal_state_change(int32_t thermal_state);
    void handle_battery_level_change(float battery_level);
    void optimize_for_performance();
    
    // Animation management
    void register_animation() { m_active_animation_count++; }
    void unregister_animation() { m_active_animation_count = std::max(0, m_active_animation_count - 1); }
    bool can_start_animation() const;
    
    // Mobile-specific UI helpers
    bool touch_button(const std::string& label, float width = 0.0f, float height = 0.0f);
    bool large_touch_button(const std::string& label, const std::string& description = "");
    bool accessibility_button(const std::string& label, const std::string& accessibility_label, 
                             const std::string& accessibility_hint = "");
    bool slider_touch(const std::string& label, float* value, float min_val, float max_val);
    bool color_picker_touch(const std::string& label, float* color);
    
    // Layout helpers for mobile
    void begin_safe_area();
    void end_safe_area();
    void begin_touch_friendly_group();
    void end_touch_friendly_group();
    
    // Haptic feedback (platform-specific implementation required)
    void trigger_haptic_feedback(int32_t type = 0);  // 0=light, 1=medium, 2=heavy
    void trigger_impact_feedback(float intensity = 0.5f);
    void trigger_selection_feedback();
    void trigger_notification_feedback(int32_t type = 0);  // 0=success, 1=warning, 2=error

private:
    void setup_mobile_components();
    void setup_gesture_recognition();
    void update_performance_settings();
    void apply_accessibility_settings();
    void calculate_optimal_layout();
    
    // Platform-specific implementations
    void setup_platform_specific_features();
    DeviceFormFactor detect_form_factor() const;
    ScreenOrientation detect_orientation() const;
    void get_system_safe_area_insets(float insets[4]) const;
    
    // Performance helpers
    void reduce_visual_effects();
    void enable_view_recycling();
    void limit_concurrent_operations();
};

// ============================================================================
// MOBILE UI UTILITIES
// ============================================================================

/**
 * @brief Mobile UI measurement utilities
 */
namespace mobile_ui_utils {
    // Size calculations
    float points_to_pixels(float points, float scale_factor);
    float pixels_to_points(float pixels, float scale_factor);
    qcs::ui::events::Point2D get_minimum_touch_target_size(const MobileUIConfig& config);
    
    // Gesture recognition helpers
    float calculate_distance(const qcs::ui::events::Point2D& p1, const qcs::ui::events::Point2D& p2);
    float calculate_angle(const qcs::ui::events::Point2D& center, 
                         const qcs::ui::events::Point2D& p1, 
                         const qcs::ui::events::Point2D& p2);
    bool is_swipe_gesture(const qcs::ui::events::Point2D& start, 
                         const qcs::ui::events::Point2D& end, 
                         float min_distance);
    
    // Accessibility helpers
    std::string generate_accessibility_label(const std::string& text, const std::string& role);
    bool meets_accessibility_contrast_ratio(uint32_t foreground, uint32_t background);
    float calculate_recommended_font_size(float base_size, float dynamic_type_scale);
    
    // Performance helpers
    bool should_reduce_animations(bool low_power_mode, int32_t thermal_state);
    int32_t calculate_optimal_frame_rate(DeviceFormFactor form_factor, bool low_power_mode);
    size_t calculate_memory_budget(DeviceFormFactor form_factor);
}

/**
 * @brief Factory functions for mobile UI
 */
std::unique_ptr<MobileUIManager> create_mobile_ui_manager(DeviceFormFactor form_factor = DeviceFormFactor::Unknown);
std::unique_ptr<MobileToolbar> create_mobile_toolbar_for_form_factor(DeviceFormFactor form_factor);

} // namespace qcs::ui::controls::mobile