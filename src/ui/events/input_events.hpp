/**
 * @file input_events.hpp
 * @brief Input event system for QuantumCanvas Studio
 * 
 * High-performance input event handling with support for:
 * - Mouse, keyboard, and touch input
 * - Stylus pressure and tilt sensitivity
 * - Gesture recognition
 * - Input prediction and smoothing
 * 
 * @author QuantumCanvas Team  
 * @date 2025-01-14
 * @version 1.0.0-alpha
 */

#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <chrono>

namespace qcs::ui::window {
    using WindowId = uint32_t;
}

namespace qcs::ui::events {

// ============================================================================
// INPUT EVENT TYPES
// ============================================================================

/**
 * @brief Input device types
 */
enum class InputDevice {
    Mouse,
    Keyboard, 
    Touch,
    Stylus,
    Trackpad,
    GameController,
    Unknown
};

/**
 * @brief Mouse button identifiers
 */
enum class MouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    X1 = 3,     // Side button
    X2 = 4,     // Side button
    Count = 5
};

/**
 * @brief Keyboard key codes (using virtual key codes)
 */
enum class KeyCode {
    // Letters
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    
    // Function keys
    F1 = 112, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // Special keys
    Escape = 27,
    Tab = 9,
    CapsLock = 20,
    Shift = 16,
    Control = 17,
    Alt = 18,
    Space = 32,
    Enter = 13,
    Backspace = 8,
    Delete = 46,
    Insert = 45,
    Home = 36,
    End = 35,
    PageUp = 33,
    PageDown = 34,
    
    // Arrow keys
    Left = 37,
    Up = 38,
    Right = 39,
    Down = 40,
    
    // Numpad
    Numpad0 = 96, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadMultiply = 106,
    NumpadAdd = 107,
    NumpadSubtract = 109,
    NumpadDecimal = 110,
    NumpadDivide = 111,
    
    Unknown = 0
};

/**
 * @brief Modifier key flags (can be combined)
 */
enum class ModifierKeys : uint32_t {
    None = 0,
    Shift = 1 << 0,
    Control = 1 << 1, 
    Alt = 1 << 2,
    Meta = 1 << 3,    // Windows key / Cmd key
    CapsLock = 1 << 4,
    NumLock = 1 << 5
};

inline ModifierKeys operator|(ModifierKeys a, ModifierKeys b) {
    return static_cast<ModifierKeys>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ModifierKeys operator&(ModifierKeys a, ModifierKeys b) {
    return static_cast<ModifierKeys>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool has_modifier(ModifierKeys flags, ModifierKeys modifier) {
    return (flags & modifier) == modifier;
}

/**
 * @brief Touch phase states
 */
enum class TouchPhase {
    Begin,      // Touch started
    Move,       // Touch moved
    End,        // Touch ended
    Cancel      // Touch cancelled by system
};

// ============================================================================
// INPUT EVENT STRUCTURES
// ============================================================================

/**
 * @brief 2D point/vector structure
 */
struct Point2D {
    float x = 0.0f;
    float y = 0.0f;
    
    Point2D() = default;
    Point2D(float x_, float y_) : x(x_), y(y_) {}
    
    Point2D operator+(const Point2D& other) const {
        return Point2D(x + other.x, y + other.y);
    }
    
    Point2D operator-(const Point2D& other) const {
        return Point2D(x - other.x, y - other.y);
    }
    
    float distance_to(const Point2D& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

/**
 * @brief Base input event structure
 */
struct InputEvent {
    InputDevice device = InputDevice::Unknown;
    qcs::ui::window::WindowId window_id = 0;
    uint64_t timestamp = 0;  // Microseconds since epoch
    ModifierKeys modifiers = ModifierKeys::None;
    
    InputEvent() {
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
    }
};

/**
 * @brief Mouse event data
 */
struct MouseEvent : public InputEvent {
    enum Type {
        ButtonDown,
        ButtonUp,
        Move,
        Wheel,
        Enter,      // Mouse entered window
        Leave       // Mouse left window
    } type;
    
    MouseButton button = MouseButton::Left;
    Point2D position;           // Window-relative coordinates
    Point2D global_position;    // Screen coordinates
    Point2D delta;              // Movement delta (for Move events)
    float wheel_delta = 0.0f;   // Wheel scroll amount
    int32_t click_count = 0;    // For double/triple click detection
    
    MouseEvent() { device = InputDevice::Mouse; }
};

/**
 * @brief Keyboard event data
 */
struct KeyboardEvent : public InputEvent {
    enum Type {
        KeyDown,
        KeyUp,
        Char        // Character input (after key mapping)
    } type;
    
    KeyCode key = KeyCode::Unknown;
    uint32_t character = 0;     // Unicode character (for Char events)
    uint32_t scan_code = 0;     // Hardware scan code
    bool is_repeat = false;     // Key repeat event
    
    KeyboardEvent() { device = InputDevice::Keyboard; }
};

/**
 * @brief Touch event data
 */
struct TouchEvent : public InputEvent {
    struct TouchPoint {
        uint32_t id = 0;            // Unique touch ID
        TouchPhase phase = TouchPhase::Begin;
        Point2D position;           // Window-relative coordinates  
        Point2D global_position;    // Screen coordinates
        float pressure = 1.0f;      // Pressure (0.0 to 1.0)
        float radius = 0.0f;        // Touch radius in pixels
        Point2D velocity;           // Touch velocity (pixels/second)
    };
    
    std::vector<TouchPoint> touches;
    uint32_t primary_touch_id = 0;  // ID of primary touch
    
    TouchEvent() { device = InputDevice::Touch; }
};

/**
 * @brief Stylus event data (extends touch with additional properties)
 */
struct StylusEvent : public TouchEvent {
    float tilt_x = 0.0f;        // Tilt angle X (-90 to 90 degrees)
    float tilt_y = 0.0f;        // Tilt angle Y (-90 to 90 degrees)
    float rotation = 0.0f;      // Stylus rotation (0 to 360 degrees)
    bool eraser_active = false; // Eraser end being used
    bool barrel_button = false; // Barrel button pressed
    
    StylusEvent() { device = InputDevice::Stylus; }
};

/**
 * @brief Gesture event data
 */
struct GestureEvent : public InputEvent {
    enum Type {
        Pan,
        Pinch,
        Rotate,
        Swipe,
        Tap,
        DoubleTap,
        LongPress
    } type;
    
    Point2D center;             // Gesture center point
    Point2D delta;              // Translation delta
    float scale = 1.0f;         // Pinch scale factor
    float rotation = 0.0f;      // Rotation angle (radians)
    float velocity = 0.0f;      // Gesture velocity
    
    GestureEvent() { device = InputDevice::Touch; }
};

// ============================================================================
// INPUT EVENT HANDLING
// ============================================================================

/**
 * @brief Input event handler interface
 */
class InputHandler {
public:
    virtual ~InputHandler() = default;
    
    // Event handling methods (return true if event was handled)
    virtual bool handle_mouse_event(const MouseEvent& event) { return false; }
    virtual bool handle_keyboard_event(const KeyboardEvent& event) { return false; }
    virtual bool handle_touch_event(const TouchEvent& event) { return false; }
    virtual bool handle_stylus_event(const StylusEvent& event) { return false; }
    virtual bool handle_gesture_event(const GestureEvent& event) { return false; }
    
    // Handler priority (higher numbers = higher priority)
    virtual int32_t get_priority() const { return 0; }
    
    // Handler can be enabled/disabled
    virtual bool is_enabled() const { return true; }
};

/**
 * @brief Input event dispatcher
 * 
 * Manages input event routing to registered handlers with priority-based
 * dispatch and event filtering capabilities.
 */
class InputDispatcher {
private:
    struct HandlerEntry {
        InputHandler* handler;
        int32_t priority;
        bool enabled;
        
        HandlerEntry(InputHandler* h, int32_t p) 
            : handler(h), priority(p), enabled(true) {}
    };
    
    std::vector<HandlerEntry> m_handlers;
    bool m_handlers_sorted = false;
    
    // Event filtering
    struct EventFilter {
        InputDevice device_filter = InputDevice::Unknown;  // Filter by device
        qcs::ui::window::WindowId window_filter = 0;       // Filter by window
        std::function<bool(const InputEvent&)> custom_filter;
    };
    std::vector<EventFilter> m_event_filters;
    
    // Performance tracking
    struct PerformanceStats {
        uint64_t events_dispatched = 0;
        uint64_t events_handled = 0;
        double avg_dispatch_time = 0.0;  // microseconds
    } m_stats;
    
    void ensure_handlers_sorted();
    bool passes_filters(const InputEvent& event);

public:
    InputDispatcher();
    ~InputDispatcher();
    
    // Handler management
    void register_handler(InputHandler* handler);
    void unregister_handler(InputHandler* handler);
    void set_handler_enabled(InputHandler* handler, bool enabled);
    size_t get_handler_count() const { return m_handlers.size(); }
    
    // Event dispatching
    bool dispatch_mouse_event(const MouseEvent& event);
    bool dispatch_keyboard_event(const KeyboardEvent& event);
    bool dispatch_touch_event(const TouchEvent& event);
    bool dispatch_stylus_event(const StylusEvent& event);
    bool dispatch_gesture_event(const GestureEvent& event);
    
    // Event filtering
    void add_device_filter(InputDevice device);
    void add_window_filter(qcs::ui::window::WindowId window_id);
    void add_custom_filter(std::function<bool(const InputEvent&)> filter);
    void clear_filters();
    
    // Performance and diagnostics
    PerformanceStats get_performance_stats() const { return m_stats; }
    void reset_performance_stats() { m_stats = {}; }
};

// ============================================================================
// INPUT PREDICTION AND SMOOTHING
// ============================================================================

/**
 * @brief Input prediction system for stylus/touch smoothing
 */
class InputPredictor {
private:
    struct InputSample {
        Point2D position;
        float pressure;
        uint64_t timestamp;
    };
    
    static constexpr size_t MAX_SAMPLES = 8;
    std::vector<InputSample> m_samples;
    float m_prediction_time = 16.0f; // milliseconds
    
public:
    // Add input sample for prediction
    void add_sample(const Point2D& position, float pressure, uint64_t timestamp);
    
    // Get predicted position
    Point2D predict_position(uint64_t future_timestamp) const;
    
    // Get smoothed position
    Point2D get_smoothed_position() const;
    
    // Configuration
    void set_prediction_time(float milliseconds) { m_prediction_time = milliseconds; }
    float get_prediction_time() const { return m_prediction_time; }
    
    void clear() { m_samples.clear(); }
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Convert platform-specific key codes to KeyCode enum
 */
KeyCode platform_key_to_keycode(uint32_t platform_key);

/**
 * @brief Convert KeyCode to human-readable string
 */
std::string keycode_to_string(KeyCode key);

/**
 * @brief Get modifier keys state from platform
 */
ModifierKeys get_current_modifiers();

/**
 * @brief Check if a point is inside a rectangle
 */
bool point_in_rect(const Point2D& point, const Point2D& rect_pos, const Point2D& rect_size);

/**
 * @brief Calculate distance between two points
 */
float distance_between(const Point2D& a, const Point2D& b);

/**
 * @brief Create global input dispatcher instance
 */
InputDispatcher& get_global_input_dispatcher();

} // namespace qcs::ui::events