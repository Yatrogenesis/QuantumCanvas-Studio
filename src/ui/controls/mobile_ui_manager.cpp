#include "mobile_ui_manager.hpp"
#include "../window/window_manager.hpp"
#include "../../core/rendering/rendering_engine.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace qcs::ui::controls::mobile {

// ============================================================================
// MOBILE UI CONFIG IMPLEMENTATIONS
// ============================================================================

MobileUIConfig MobileUIConfig::create_ios_config() {
    MobileUIConfig config;
    
    // iOS Human Interface Guidelines compliance
    config.min_touch_target_size = 44.0f;          // 44pt minimum in iOS HIG
    config.recommended_touch_target_size = 48.0f;   // Comfortable size
    config.touch_target_spacing = 8.0f;
    
    // iOS-specific gesture thresholds
    config.swipe_threshold_px = 50.0f;
    config.pan_threshold_px = 10.0f;
    config.long_press_duration_ms = 500;
    config.double_tap_duration_ms = 300;
    
    // Apple Pencil optimization
    config.enable_stylus_rejection = true;
    config.stylus_precision_mode_scale = 2.0f;
    config.stylus_hover_preview = true;
    
    // iOS accessibility defaults
    config.enable_dynamic_type = true;
    config.min_font_size = 12.0f;
    config.max_font_size = 32.0f;
    
    // iOS performance settings
    config.enable_view_recycling = true;
    config.max_concurrent_animations = 3;
    config.animation_speed_multiplier = 1.0f;
    
    return config;
}

MobileUIConfig MobileUIConfig::create_android_config() {
    MobileUIConfig config;
    
    // Android Material Design compliance
    config.min_touch_target_size = 48.0f;          // 48dp minimum in Material Design
    config.recommended_touch_target_size = 56.0f;   // Material Design recommendation
    config.touch_target_spacing = 8.0f;
    
    // Android-specific gesture thresholds
    config.swipe_threshold_px = 64.0f;             // Slightly higher for Android
    config.pan_threshold_px = 8.0f;
    config.long_press_duration_ms = 500;
    config.double_tap_duration_ms = 300;
    
    // Android stylus support (S-Pen, etc.)
    config.enable_stylus_rejection = true;
    config.stylus_precision_mode_scale = 2.0f;
    config.stylus_hover_preview = true;
    
    // Android accessibility defaults
    config.enable_dynamic_type = true;
    config.min_font_size = 14.0f;                  // Slightly larger default for Android
    config.max_font_size = 32.0f;
    
    // Android performance settings
    config.enable_view_recycling = true;
    config.max_concurrent_animations = 4;          // Android can handle slightly more
    config.animation_speed_multiplier = 1.0f;
    
    return config;
}

MobileUIConfig MobileUIConfig::create_accessibility_optimized() {
    MobileUIConfig config = create_ios_config();   // Start with iOS base
    
    // Larger touch targets for accessibility
    config.min_touch_target_size = 56.0f;
    config.recommended_touch_target_size = 64.0f;
    config.touch_target_spacing = 16.0f;           // More spacing
    
    // More forgiving gesture thresholds
    config.swipe_threshold_px = 80.0f;
    config.pan_threshold_px = 16.0f;
    config.long_press_duration_ms = 750;           // Longer for motor difficulties
    
    // Accessibility-optimized settings
    config.enable_dynamic_type = true;
    config.min_font_size = 16.0f;                  // Larger minimum
    config.max_font_size = 48.0f;                  // Higher maximum
    config.high_contrast_mode = true;
    config.reduce_motion = true;
    
    // Conservative performance for older devices often used with accessibility
    config.max_concurrent_animations = 2;
    config.animation_speed_multiplier = 0.75f;     // Slower animations
    
    return config;
}

// ============================================================================
// MOBILE TOOLBAR IMPLEMENTATION
// ============================================================================

MobileToolbar::MobileToolbar(ToolbarLayout layout) 
    : m_layout(layout)
    , m_ui_config(MobileUIConfig::create_ios_config())
{
    m_auto_hide_enabled = (layout == ToolbarLayout::TopHorizontal);
}

void MobileToolbar::add_button(const MobileToolButton& button) {
    m_buttons.push_back(button);
    calculate_button_layout();
}

void MobileToolbar::remove_button(const std::string& id) {
    m_buttons.erase(
        std::remove_if(m_buttons.begin(), m_buttons.end(),
            [&id](const MobileToolButton& button) { return button.id == id; }),
        m_buttons.end());
    calculate_button_layout();
}

void MobileToolbar::set_button_enabled(const std::string& id, bool enabled) {
    auto it = std::find_if(m_buttons.begin(), m_buttons.end(),
        [&id](MobileToolButton& button) { return button.id == id; });
    if (it != m_buttons.end()) {
        it->enabled = enabled;
    }
}

void MobileToolbar::set_button_visible(const std::string& id, bool visible) {
    auto it = std::find_if(m_buttons.begin(), m_buttons.end(),
        [&id](MobileToolButton& button) { return button.id == id; });
    if (it != m_buttons.end()) {
        it->visible = visible;
        calculate_button_layout();
    }
}

void MobileToolbar::set_primary_button(const std::string& id) {
    for (auto& button : m_buttons) {
        button.primary = (button.id == id);
    }
}

void MobileToolbar::adapt_to_orientation(ScreenOrientation orientation) {
    switch (orientation) {
        case ScreenOrientation::Portrait:
            if (m_layout == ToolbarLayout::LeftVertical) {
                m_layout = ToolbarLayout::BottomHorizontal;
                calculate_button_layout();
            }
            break;
        case ScreenOrientation::Landscape:
        case ScreenOrientation::LandscapeLeft:
        case ScreenOrientation::LandscapeRight:
            if (m_layout == ToolbarLayout::BottomHorizontal) {
                m_layout = ToolbarLayout::LeftVertical;
                calculate_button_layout();
            }
            break;
        default:
            break;
    }
}

void MobileToolbar::adapt_to_form_factor(DeviceFormFactor form_factor) {
    switch (form_factor) {
        case DeviceFormFactor::Phone:
        case DeviceFormFactor::PhonePlus:
            // Prefer bottom toolbar on phones for thumb accessibility
            if (m_layout == ToolbarLayout::TopHorizontal) {
                m_layout = ToolbarLayout::BottomHorizontal;
                calculate_button_layout();
            }
            break;
        case DeviceFormFactor::Tablet:
        case DeviceFormFactor::TabletPro:
            // Tablets can use top or floating toolbars more effectively
            if (m_layout == ToolbarLayout::BottomHorizontal) {
                m_layout = ToolbarLayout::FloatingCircular;
                calculate_button_layout();
            }
            break;
        default:
            break;
    }
}

void MobileToolbar::show_temporarily(float duration) {
    m_currently_hidden = false;
    m_hide_timer = duration;
}

void MobileToolbar::render() {
    if (m_currently_hidden && m_auto_hide_enabled) {
        return;
    }
    
    // Update auto-hide timer
    if (m_auto_hide_enabled && m_hide_timer > 0.0f) {
        m_hide_timer -= 1.0f / 60.0f;  // Assume 60 FPS for now
        if (m_hide_timer <= 0.0f) {
            m_currently_hidden = true;
        }
    }
    
    // Render buttons based on layout
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (!m_buttons[i].visible) continue;
        
        // Calculate position based on layout and index
        qcs::ui::events::Point2D position;
        qcs::ui::events::Point2D size;
        
        // Simplified position calculation - in real implementation this would be more sophisticated
        switch (m_layout) {
            case ToolbarLayout::BottomHorizontal:
                position.x = i * (m_ui_config.recommended_touch_target_size + m_ui_config.touch_target_spacing);
                position.y = 0;  // Bottom of screen
                break;
            case ToolbarLayout::TopHorizontal:
                position.x = i * (m_ui_config.recommended_touch_target_size + m_ui_config.touch_target_spacing);
                position.y = 0;  // Top of screen
                break;
            case ToolbarLayout::LeftVertical:
                position.x = 0;  // Left side of screen
                position.y = i * (m_ui_config.recommended_touch_target_size + m_ui_config.touch_target_spacing);
                break;
            default:
                break;
        }
        
        size.x = m_ui_config.recommended_touch_target_size;
        size.y = m_ui_config.recommended_touch_target_size;
        
        render_button(m_buttons[i], position, size);
    }
}

void MobileToolbar::calculate_button_layout() {
    // Calculate optimal button arrangement based on available space and layout
    // This is a simplified version - real implementation would be more complex
    
    int visible_buttons = 0;
    for (const auto& button : m_buttons) {
        if (button.visible) visible_buttons++;
    }
    
    // Adjust button sizes if too many buttons
    if (visible_buttons > 6 && m_layout != ToolbarLayout::AdaptiveOverlay) {
        // Consider switching to adaptive overlay or reducing button count
    }
}

void MobileToolbar::render_button(const MobileToolButton& button, const qcs::ui::events::Point2D& position, const qcs::ui::events::Point2D& size) {
    // Render button with appropriate visual state
    // This would integrate with the actual rendering system
    
    // Button background
    uint32_t button_color = button.primary ? 0xFF007AFF : 0xFFE5E5EA;  // iOS blue or light gray
    if (!button.enabled) {
        button_color = 0xFF999999;  // Disabled gray
    }
    
    // Button icon/text would be rendered here
    // Accessibility information would be attached here
}

bool MobileToolbar::handle_button_touch(const MobileToolButton& button, const qcs::ui::events::Point2D& touch_pos) {
    if (!button.enabled || !button.visible) return false;
    
    // Check if touch is within button bounds (including expanded touch area)
    // Simplified hit testing - real implementation would be more precise
    
    if (button.callback) {
        button.callback();
        
        if (m_haptic_feedback_enabled) {
            trigger_haptic_feedback();
        }
        
        return true;
    }
    
    return false;
}

void MobileToolbar::trigger_haptic_feedback() {
    // Platform-specific haptic feedback implementation would go here
    // iOS: UIImpactFeedbackGenerator
    // Android: Vibrator.vibrate()
}

// ============================================================================
// MOBILE PROPERTY PANEL IMPLEMENTATION
// ============================================================================

MobilePropertyPanel::MobilePropertyPanel() 
    : m_ui_config(MobileUIConfig::create_ios_config())
{
}

void MobilePropertyPanel::add_section(const std::string& title, bool expanded, bool expandable) {
    PropertySection section;
    section.title = title;
    section.expanded = expanded;
    section.expandable = expandable;
    section.header_height = m_ui_config.recommended_touch_target_size;
    
    m_sections.push_back(section);
}

void MobilePropertyPanel::remove_section(const std::string& title) {
    m_sections.erase(
        std::remove_if(m_sections.begin(), m_sections.end(),
            [&title](const PropertySection& section) { return section.title == title; }),
        m_sections.end());
}

void MobilePropertyPanel::set_section_expanded(const std::string& title, bool expanded) {
    auto it = std::find_if(m_sections.begin(), m_sections.end(),
        [&title](PropertySection& section) { return section.title == title; });
    if (it != m_sections.end() && it->expandable) {
        it->expanded = expanded;
    }
}

bool MobilePropertyPanel::handle_touch_event(const qcs::ui::events::TouchEvent& event) {
    switch (event.type) {
        case qcs::ui::events::TouchEventType::TouchDown:
            m_touch_active = true;
            m_last_touch_pos = event.position;
            m_scroll_velocity = 0.0f;
            return true;
            
        case qcs::ui::events::TouchEventType::TouchMove:
            if (m_touch_active) {
                float delta_y = event.position.y - m_last_touch_pos.y;
                m_scroll_offset += delta_y;
                m_scroll_velocity = delta_y * 60.0f;  // Convert to velocity per second
                m_last_touch_pos = event.position;
                m_scrolling = true;
                return true;
            }
            break;
            
        case qcs::ui::events::TouchEventType::TouchUp:
        case qcs::ui::events::TouchEventType::TouchCancel:
            m_touch_active = false;
            
            // Check if this was a tap on a section header
            if (!m_scrolling) {
                float current_y = 0.0f;
                for (auto& section : m_sections) {
                    if (is_point_in_section_header(section, event.position, current_y)) {
                        if (section.expandable) {
                            section.expanded = !section.expanded;
                            return true;
                        }
                    }
                    current_y += section.header_height;
                    if (section.expanded) {
                        current_y += section.property_ids.size() * 30.0f;  // Estimated property height
                    }
                }
            }
            
            m_scrolling = false;
            return true;
            
        default:
            break;
    }
    
    return false;
}

void MobilePropertyPanel::update_scroll_physics(float delta_time) {
    if (!m_touch_active && std::abs(m_scroll_velocity) > 0.1f) {
        // Apply momentum scrolling
        m_scroll_offset += m_scroll_velocity * delta_time;
        m_scroll_velocity *= 0.95f;  // Friction
        
        // Clamp scroll offset to valid range
        float total_height = calculate_total_height();
        float visible_height = 400.0f;  // This should come from actual panel height
        
        if (m_scroll_offset > 0.0f) {
            m_scroll_offset = 0.0f;
            m_scroll_velocity = 0.0f;
        } else if (m_scroll_offset < -(total_height - visible_height)) {
            m_scroll_offset = -(total_height - visible_height);
            m_scroll_velocity = 0.0f;
        }
    }
}

void MobilePropertyPanel::render() {
    float current_y = m_scroll_offset;
    
    for (const auto& section : m_sections) {
        // Render section header
        render_section_header(section, current_y);
        current_y += section.header_height;
        
        // Render section content if expanded
        if (section.expanded) {
            render_section_content(section, current_y);
            current_y += section.property_ids.size() * 30.0f;  // Estimated property height
        }
    }
}

void MobilePropertyPanel::render_section_header(const PropertySection& section, float y_offset) {
    // Render expandable section header with touch-friendly size
    // This would integrate with the actual rendering system
    
    // Header background
    uint32_t header_color = section.expanded ? 0xFFE5E5EA : 0xFFF2F2F7;
    
    // Expansion indicator (chevron)
    if (section.expandable) {
        // Render chevron icon pointing right (collapsed) or down (expanded)
    }
    
    // Section title text
    // This would render the section title with appropriate font and accessibility labels
}

void MobilePropertyPanel::render_section_content(const PropertySection& section, float y_offset) {
    // Render property controls for this section
    // Each property would be rendered with touch-friendly controls
    
    float property_y = y_offset;
    for (const auto& property_id : section.property_ids) {
        // Render individual property control
        // This would call the appropriate property renderer
        property_y += 30.0f;  // Standard property height
    }
}

float MobilePropertyPanel::calculate_total_height() const {
    float total_height = 0.0f;
    
    for (const auto& section : m_sections) {
        total_height += section.header_height;
        if (section.expanded) {
            total_height += section.property_ids.size() * 30.0f;
        }
    }
    
    return total_height;
}

bool MobilePropertyPanel::is_point_in_section_header(const PropertySection& section, 
                                                   const qcs::ui::events::Point2D& point, 
                                                   float section_y) const {
    return point.y >= section_y && point.y <= (section_y + section.header_height);
}

// ============================================================================
// MOBILE CANVAS PANEL IMPLEMENTATION
// ============================================================================

MobileCanvasPanel::MobileCanvasPanel() {
}

bool MobileCanvasPanel::handle_touch_event(const qcs::ui::events::TouchEvent& event) {
    switch (event.type) {
        case qcs::ui::events::TouchEventType::TouchDown:
            process_touch_down(event.touch_id, event.position, event.is_stylus);
            return true;
            
        case qcs::ui::events::TouchEventType::TouchMove:
            process_touch_move(event.touch_id, event.position);
            return true;
            
        case qcs::ui::events::TouchEventType::TouchUp:
        case qcs::ui::events::TouchEventType::TouchCancel:
            process_touch_up(event.touch_id);
            return true;
            
        default:
            break;
    }
    
    return false;
}

bool MobileCanvasPanel::handle_stylus_event(const qcs::ui::events::StylusEvent& event) {
    // Handle stylus-specific events with pressure, tilt, etc.
    
    if (!m_stylus_mode_active) {
        set_stylus_mode_active(true);
    }
    
    // Process stylus input with high precision
    TouchPoint stylus_touch;
    stylus_touch.id = event.touch_id;
    stylus_touch.current_pos = event.position;
    stylus_touch.is_stylus = true;
    stylus_touch.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    m_active_touches[event.touch_id] = stylus_touch;
    
    // Enter drawing mode when stylus is detected
    m_current_gesture = GestureState::Drawing;
    
    return true;
}

void MobileCanvasPanel::update_gesture_recognition(float delta_time) {
    // Update momentum scrolling
    update_momentum_scrolling(delta_time);
    
    // Update gesture states based on active touches
    if (m_active_touches.empty()) {
        m_current_gesture = GestureState::None;
        return;
    }
    
    // Single touch
    if (m_active_touches.size() == 1) {
        auto& touch = m_active_touches.begin()->second;
        
        if (touch.is_stylus) {
            m_current_gesture = GestureState::Drawing;
        } else {
            // Check if this is a pan gesture
            float distance = mobile_ui_utils::calculate_distance(touch.start_pos, touch.current_pos);
            if (distance > 10.0f && m_current_gesture == GestureState::None) {
                start_pan_gesture();
            } else if (m_current_gesture == GestureState::Panning) {
                update_pan_gesture();
            }
        }
    }
    // Two touches - zoom or rotation
    else if (m_active_touches.size() == 2) {
        if (m_current_gesture == GestureState::None) {
            start_zoom_gesture();
        } else if (m_current_gesture == GestureState::Zooming) {
            update_zoom_gesture();
        }
    }
}

void MobileCanvasPanel::set_stylus_mode_active(bool active) {
    if (m_stylus_mode_active != active) {
        m_stylus_mode_active = active;
        
        if (active) {
            // Zoom in for precision
            // Enable palm rejection
            // Show stylus-specific UI
        } else {
            // Return to normal zoom
            // Disable precision UI
        }
    }
}

void MobileCanvasPanel::render() {
    // Render the canvas content
    // This would integrate with the actual rendering pipeline
    
    // Render touch indicators if enabled (for debugging)
    if (m_show_touch_indicators) {
        render_touch_indicators();
    }
}

void MobileCanvasPanel::process_touch_down(uint32_t touch_id, const qcs::ui::events::Point2D& pos, bool is_stylus) {
    TouchPoint touch;
    touch.id = touch_id;
    touch.current_pos = pos;
    touch.start_pos = pos;
    touch.velocity = {0.0f, 0.0f};
    touch.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    touch.is_stylus = is_stylus;
    
    m_active_touches[touch_id] = touch;
    
    // Clear momentum when new touch starts
    m_momentum_velocity = {0.0f, 0.0f};
}

void MobileCanvasPanel::process_touch_move(uint32_t touch_id, const qcs::ui::events::Point2D& pos) {
    auto it = m_active_touches.find(touch_id);
    if (it != m_active_touches.end()) {
        TouchPoint& touch = it->second;
        
        // Update velocity
        int64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        float dt = (current_time - touch.timestamp) / 1000.0f;
        
        if (dt > 0.0f) {
            touch.velocity.x = (pos.x - touch.current_pos.x) / dt;
            touch.velocity.y = (pos.y - touch.current_pos.y) / dt;
        }
        
        touch.current_pos = pos;
        touch.timestamp = current_time;
    }
}

void MobileCanvasPanel::process_touch_up(uint32_t touch_id) {
    auto it = m_active_touches.find(touch_id);
    if (it != m_active_touches.end()) {
        // Store momentum for scrolling
        if (m_current_gesture == GestureState::Panning) {
            m_momentum_velocity = it->second.velocity;
            end_pan_gesture();
        } else if (m_current_gesture == GestureState::Zooming) {
            end_zoom_gesture();
        }
        
        m_active_touches.erase(it);
    }
}

void MobileCanvasPanel::start_pan_gesture() {
    m_current_gesture = GestureState::Panning;
    // Store initial canvas offset for relative panning
}

void MobileCanvasPanel::update_pan_gesture() {
    if (m_active_touches.size() == 1) {
        auto& touch = m_active_touches.begin()->second;
        // Update canvas pan offset based on touch movement
    }
}

void MobileCanvasPanel::end_pan_gesture() {
    m_current_gesture = GestureState::None;
}

void MobileCanvasPanel::start_zoom_gesture() {
    if (m_active_touches.size() >= 2) {
        m_current_gesture = GestureState::Zooming;
        
        auto it1 = m_active_touches.begin();
        auto it2 = std::next(it1);
        
        m_pinch_start_distance = mobile_ui_utils::calculate_distance(
            it1->second.current_pos, it2->second.current_pos);
        m_pinch_start_zoom = 1.0f;  // Current zoom level
    }
}

void MobileCanvasPanel::update_zoom_gesture() {
    if (m_active_touches.size() >= 2) {
        auto it1 = m_active_touches.begin();
        auto it2 = std::next(it1);
        
        float current_distance = mobile_ui_utils::calculate_distance(
            it1->second.current_pos, it2->second.current_pos);
        
        if (m_pinch_start_distance > 0.0f) {
            float scale_factor = current_distance / m_pinch_start_distance;
            // Apply zoom scaling to canvas
        }
    }
}

void MobileCanvasPanel::end_zoom_gesture() {
    m_current_gesture = GestureState::None;
}

void MobileCanvasPanel::update_momentum_scrolling(float delta_time) {
    if (m_current_gesture == GestureState::None && 
        (std::abs(m_momentum_velocity.x) > 0.1f || std::abs(m_momentum_velocity.y) > 0.1f)) {
        
        // Apply momentum scrolling
        // Update canvas pan offset
        
        // Apply friction
        m_momentum_velocity.x *= m_momentum_friction;
        m_momentum_velocity.y *= m_momentum_friction;
        
        if (std::abs(m_momentum_velocity.x) < 0.1f && std::abs(m_momentum_velocity.y) < 0.1f) {
            m_momentum_velocity = {0.0f, 0.0f};
        }
    }
}

bool MobileCanvasPanel::is_palm_touch(const TouchPoint& touch) const {
    if (!m_palm_rejection_active) return false;
    
    // Simple palm rejection based on touch size and position
    // Real implementation would be more sophisticated
    return false;  // Placeholder
}

void MobileCanvasPanel::render_touch_indicators() {
    // Render visual indicators for active touches (debugging)
    for (const auto& touch_pair : m_active_touches) {
        const TouchPoint& touch = touch_pair.second;
        
        uint32_t color = touch.is_stylus ? 0xFF00FF00 : 0xFFFF0000;  // Green for stylus, red for finger
        
        // Render circle at touch position
        // This would integrate with the actual rendering system
    }
}

// ============================================================================
// MOBILE UI MANAGER IMPLEMENTATION
// ============================================================================

MobileUIManager::MobileUIManager() {
    // Detect device characteristics
    m_device_form_factor = detect_form_factor();
    m_current_orientation = detect_orientation();
    
    // Set appropriate mobile configuration
#ifdef QCS_PLATFORM_IOS
    m_mobile_config = MobileUIConfig::create_ios_config();
#elif defined(QCS_PLATFORM_ANDROID)
    m_mobile_config = MobileUIConfig::create_android_config();
#else
    m_mobile_config = MobileUIConfig::create_ios_config();  // Default
#endif

    setup_platform_specific_features();
}

MobileUIManager::~MobileUIManager() {
    shutdown();
}

bool MobileUIManager::initialize(qcs::ui::window::WindowManager* window_manager,
                                qcs::core::rendering::RenderingEngine* rendering_engine) {
    if (!UIManager::initialize(window_manager, rendering_engine)) {
        return false;
    }
    
    // Initialize mobile-specific components
    setup_mobile_components();
    setup_gesture_recognition();
    
    // Get system safe area insets
    get_system_safe_area_insets(m_safe_area_insets);
    
    // Initialize performance monitoring
    update_performance_settings();
    
    // Apply accessibility settings
    apply_accessibility_settings();
    
    return true;
}

void MobileUIManager::shutdown() {
    m_mobile_toolbar.reset();
    m_mobile_property_panel.reset();
    m_mobile_canvas_panel.reset();
    m_input_predictor.reset();
    
    UIManager::shutdown();
}

void MobileUIManager::set_mobile_config(const MobileUIConfig& config) {
    m_mobile_config = config;
    
    // Update existing components with new config
    if (m_mobile_toolbar) {
        // Update toolbar with new config
    }
    if (m_mobile_property_panel) {
        // Update property panel with new config
    }
    
    m_layout_needs_update = true;
}

void MobileUIManager::set_device_form_factor(DeviceFormFactor form_factor) {
    if (m_device_form_factor != form_factor) {
        m_device_form_factor = form_factor;
        adapt_ui_for_form_factor();
        m_layout_needs_update = true;
    }
}

void MobileUIManager::handle_orientation_change(ScreenOrientation new_orientation) {
    if (m_current_orientation != new_orientation) {
        m_current_orientation = new_orientation;
        update_layout_for_orientation();
        
        // Notify components of orientation change
        if (m_mobile_toolbar) {
            m_mobile_toolbar->adapt_to_orientation(new_orientation);
        }
        
        m_layout_needs_update = true;
    }
}

void MobileUIManager::set_safe_area_insets(float top, float right, float bottom, float left) {
    m_safe_area_insets[0] = top;
    m_safe_area_insets[1] = right;
    m_safe_area_insets[2] = bottom;
    m_safe_area_insets[3] = left;
    
    m_layout_needs_update = true;
}

bool MobileUIManager::handle_touch_event(const qcs::ui::events::TouchEvent& event) {
    // First, let mobile components handle the event
    if (m_mobile_canvas_panel && m_mobile_canvas_panel->handle_touch_event(event)) {
        return true;
    }
    
    if (m_mobile_property_panel && m_mobile_property_panel->handle_touch_event(event)) {
        return true;
    }
    
    // Fall back to base UI manager
    return UIManager::handle_touch_event(event);
}

bool MobileUIManager::handle_stylus_event(const qcs::ui::events::StylusEvent& event) {
    // Stylus events are primarily handled by the canvas
    if (m_mobile_canvas_panel) {
        return m_mobile_canvas_panel->handle_stylus_event(event);
    }
    
    return false;
}

bool MobileUIManager::handle_gesture_event(const qcs::ui::events::GestureEvent& event) {
    // Handle high-level gesture events
    // This could include swipe navigation, pinch-to-zoom on UI panels, etc.
    
    return false;  // Placeholder
}

void MobileUIManager::begin_frame() {
    UIManager::begin_frame();
    
    // Update layout if needed
    if (m_layout_needs_update) {
        calculate_optimal_layout();
        m_layout_needs_update = false;
    }
    
    // Update gesture recognition
    if (m_mobile_canvas_panel) {
        m_mobile_canvas_panel->update_gesture_recognition(1.0f / 60.0f);  // Assume 60 FPS
    }
    
    // Update property panel physics
    if (m_mobile_property_panel) {
        m_mobile_property_panel->update_scroll_physics(1.0f / 60.0f);
    }
}

void MobileUIManager::end_frame() {
    // Mobile-specific end-of-frame processing
    
    // Limit concurrent animations if in low power mode
    if (m_low_power_mode && m_active_animation_count > m_mobile_config.max_concurrent_animations) {
        // Pause some animations
    }
    
    UIManager::end_frame();
}

void MobileUIManager::render() {
    // Render within safe area
    begin_safe_area();
    
    // Render mobile UI components
    if (m_mobile_canvas_panel) {
        m_mobile_canvas_panel->render();
    }
    
    if (m_mobile_property_panel) {
        m_mobile_property_panel->render();
    }
    
    if (m_mobile_toolbar) {
        m_mobile_toolbar->render();
    }
    
    end_safe_area();
    
    // Render base UI components
    UIManager::render();
}

void MobileUIManager::set_low_power_mode(bool enabled) {
    if (m_low_power_mode != enabled) {
        m_low_power_mode = enabled;
        optimize_for_performance();
    }
}

void MobileUIManager::handle_thermal_state_change(int32_t thermal_state) {
    m_thermal_state = thermal_state;
    
    // Adjust performance based on thermal state
    if (thermal_state > 1) {  // Thermal pressure
        reduce_visual_effects();
        limit_concurrent_operations();
    }
}

void MobileUIManager::optimize_for_performance() {
    if (m_low_power_mode || m_thermal_state > 1) {
        // Reduce animation complexity
        m_mobile_config.animation_speed_multiplier = 0.5f;
        m_mobile_config.max_concurrent_animations = 2;
        
        // Enable aggressive view recycling
        m_mobile_config.enable_view_recycling = true;
        
        // Reduce visual effects
        reduce_visual_effects();
    }
}

bool MobileUIManager::can_start_animation() const {
    return m_active_animation_count < m_mobile_config.max_concurrent_animations;
}

void MobileUIManager::begin_safe_area() {
    // Adjust rendering to respect safe area insets
    // This would typically set up clipping or offset transformations
}

void MobileUIManager::end_safe_area() {
    // Restore previous rendering state
}

void MobileUIManager::trigger_haptic_feedback(int32_t type) {
    // Platform-specific haptic feedback implementation
#ifdef QCS_PLATFORM_IOS
    // Use UIImpactFeedbackGenerator
#elif defined(QCS_PLATFORM_ANDROID)
    // Use Vibrator service
#endif
}

// Private methods
void MobileUIManager::setup_mobile_components() {
    // Create mobile-optimized UI components
    m_mobile_toolbar = std::make_unique<MobileToolbar>();
    m_mobile_property_panel = std::make_unique<MobilePropertyPanel>();
    m_mobile_canvas_panel = std::make_unique<MobileCanvasPanel>();
    
    // Configure components for current device
    if (m_mobile_toolbar) {
        m_mobile_toolbar->adapt_to_form_factor(m_device_form_factor);
        m_mobile_toolbar->adapt_to_orientation(m_current_orientation);
    }
}

void MobileUIManager::setup_gesture_recognition() {
    // Initialize gesture recognition system
    // m_input_predictor = std::make_unique<qcs::ui::events::InputPredictor>();
}

DeviceFormFactor MobileUIManager::detect_form_factor() const {
    // Platform-specific device detection
#ifdef QCS_PLATFORM_IOS
    // Use UIDevice and screen size to determine form factor
    return DeviceFormFactor::Phone;  // Placeholder
#elif defined(QCS_PLATFORM_ANDROID)
    // Use Configuration.smallestScreenWidthDp to determine form factor
    return DeviceFormFactor::Phone;  // Placeholder
#else
    return DeviceFormFactor::Unknown;
#endif
}

ScreenOrientation MobileUIManager::detect_orientation() const {
    // Platform-specific orientation detection
    return ScreenOrientation::Portrait;  // Placeholder
}

void MobileUIManager::get_system_safe_area_insets(float insets[4]) const {
    // Platform-specific safe area detection
#ifdef QCS_PLATFORM_IOS
    // Use UIView.safeAreaInsets
#elif defined(QCS_PLATFORM_ANDROID)
    // Use WindowInsets
#endif
    
    // Default values
    insets[0] = 44.0f;  // Status bar height
    insets[1] = 0.0f;
    insets[2] = 34.0f;  // Home indicator height
    insets[3] = 0.0f;
}

void MobileUIManager::setup_platform_specific_features() {
    // Platform-specific initialization
}

void MobileUIManager::calculate_optimal_layout() {
    // Calculate optimal layout based on current constraints
    // This would position and size UI components appropriately
}

// ============================================================================
// MOBILE UI UTILITIES IMPLEMENTATION
// ============================================================================

namespace mobile_ui_utils {

float points_to_pixels(float points, float scale_factor) {
    return points * scale_factor;
}

float pixels_to_points(float pixels, float scale_factor) {
    return pixels / scale_factor;
}

qcs::ui::events::Point2D get_minimum_touch_target_size(const MobileUIConfig& config) {
    return {config.min_touch_target_size, config.min_touch_target_size};
}

float calculate_distance(const qcs::ui::events::Point2D& p1, const qcs::ui::events::Point2D& p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return std::sqrt(dx * dx + dy * dy);
}

float calculate_angle(const qcs::ui::events::Point2D& center, 
                     const qcs::ui::events::Point2D& p1, 
                     const qcs::ui::events::Point2D& p2) {
    float angle1 = std::atan2(p1.y - center.y, p1.x - center.x);
    float angle2 = std::atan2(p2.y - center.y, p2.x - center.x);
    return angle2 - angle1;
}

bool is_swipe_gesture(const qcs::ui::events::Point2D& start, 
                     const qcs::ui::events::Point2D& end, 
                     float min_distance) {
    return calculate_distance(start, end) >= min_distance;
}

bool should_reduce_animations(bool low_power_mode, int32_t thermal_state) {
    return low_power_mode || thermal_state > 1;
}

int32_t calculate_optimal_frame_rate(DeviceFormFactor form_factor, bool low_power_mode) {
    if (low_power_mode) return 30;
    
    switch (form_factor) {
        case DeviceFormFactor::TabletPro:
            return 120;  // ProMotion displays
        case DeviceFormFactor::Tablet:
        case DeviceFormFactor::PhonePlus:
            return 90;   // High refresh rate phones/tablets
        default:
            return 60;   // Standard refresh rate
    }
}

} // namespace mobile_ui_utils

// ============================================================================
// FACTORY FUNCTIONS
// ============================================================================

std::unique_ptr<MobileUIManager> create_mobile_ui_manager(DeviceFormFactor form_factor) {
    auto manager = std::make_unique<MobileUIManager>();
    
    if (form_factor != DeviceFormFactor::Unknown) {
        manager->set_device_form_factor(form_factor);
    }
    
    return manager;
}

} // namespace qcs::ui::controls::mobile