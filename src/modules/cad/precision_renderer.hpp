/**
 * @file precision_renderer.hpp
 * @brief High-precision CAD rendering engine
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Sub-pixel precision rendering for technical drawings and CAD models
 */

#pragma once

#include "cad_types.hpp"
#include "cad_common.hpp"
#include "../../core/rendering/rendering_engine.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace qcs::cad {

// Forward declarations
class PrecisionRenderContext;
class CADViewport;
class TechnicalLinetype;

// =============================================================================
// Rendering Configuration
// =============================================================================

/// Rendering quality levels for CAD precision
enum class RenderingQuality {
    Draft,          ///< Fast rendering for interactive work
    Normal,         ///< Standard quality for general use
    HighQuality,    ///< High quality for presentations
    Publication     ///< Maximum quality for technical documentation
};

/// Line weight standards (in points)
enum class LineWeight {
    Thin = 0,       ///< 0.13 mm (5 mil)
    Normal = 1,     ///< 0.25 mm (10 mil)
    Medium = 2,     ///< 0.35 mm (14 mil)
    Thick = 3,      ///< 0.50 mm (20 mil)
    ExtraThick = 4  ///< 0.70 mm (28 mil)
};

/// CAD-specific rendering modes
enum class CADRenderMode {
    Wireframe,      ///< Lines and curves only
    Hidden,         ///< Hidden line removal
    Shaded,         ///< Flat shading with edges
    Rendered,       ///< Full rendering with materials
    Technical,      ///< Technical illustration style
    Blueprint       ///< Blueprint/schematic style
};

/// Precision rendering settings
struct PrecisionRenderSettings {
    RenderingQuality quality = RenderingQuality::Normal;
    CADRenderMode render_mode = CADRenderMode::Wireframe;
    
    // Sub-pixel precision settings
    bool enable_subpixel_precision = true;
    int subpixel_samples = 4;  ///< 2x2, 4x4, 8x8 sampling
    
    // Line rendering settings
    bool enable_antialiasing = true;
    bool enable_line_smoothing = true;
    Precision line_weight_scale = 1.0;
    
    // Curve tessellation settings
    Precision curve_tolerance = 1e-6;
    int max_tessellation_depth = 8;
    bool adaptive_tessellation = true;
    
    // Performance settings
    bool enable_frustum_culling = true;
    bool enable_occlusion_culling = false;  // Usually off for CAD
    bool enable_instancing = true;
    
    // Debug visualization
    bool show_control_points = false;
    bool show_tessellation = false;
    bool show_bounding_boxes = false;
};

// =============================================================================
// CAD Viewport Management
// =============================================================================

/// CAD viewport with precision coordinate system
class CADViewport {
public:
    /// Viewport rectangle in screen coordinates
    struct ViewRect {
        int x, y;           ///< Top-left corner
        int width, height;  ///< Size in pixels
    };
    
    /// Camera/view parameters
    struct ViewParams {
        Point3D position;       ///< Camera position
        Point3D target;         ///< Look-at target
        Vector3D up;            ///< Up vector
        Precision fov;          ///< Field of view (radians)
        Precision near_plane;   ///< Near clipping plane
        Precision far_plane;    ///< Far clipping plane
        bool orthographic;      ///< Orthographic vs perspective projection
        Precision ortho_scale;  ///< Scale for orthographic projection
    };

private:
    ViewRect viewport_rect_;
    ViewParams view_params_;
    Matrix4D view_matrix_;
    Matrix4D projection_matrix_;
    Matrix4D view_projection_matrix_;
    BoundingBox3D view_frustum_;
    bool matrices_dirty_;

public:
    CADViewport(int width = 800, int height = 600);
    
    // Viewport management
    void set_viewport_rect(int x, int y, int width, int height);
    const ViewRect& get_viewport_rect() const { return viewport_rect_; }
    
    // Camera control
    void set_camera_position(const Point3D& position);
    void set_camera_target(const Point3D& target);
    void set_camera_up(const Vector3D& up);
    void set_field_of_view(Precision fov);
    void set_orthographic(bool ortho, Precision scale = 1.0);
    void set_clipping_planes(Precision near_plane, Precision far_plane);
    
    // View transformations
    void look_at(const Point3D& eye, const Point3D& center, const Vector3D& up);
    void fit_to_bounds(const BoundingBox3D& bounds, Precision margin = 0.1);
    void zoom_to_rectangle(const BoundingBox2D& rect);
    void pan(const Vector2D& delta);
    void rotate(const Vector2D& delta);
    void zoom(Precision factor, const Point2D& center = Point2D::Zero());
    
    // Coordinate transformations
    Point3D world_to_view(const Point3D& world_point) const;
    Point2D world_to_screen(const Point3D& world_point) const;
    Point3D screen_to_world(const Point2D& screen_point, Precision depth = 0) const;
    Vector3D screen_to_world_vector(const Vector2D& screen_vector) const;
    
    // Matrix access
    const Matrix4D& get_view_matrix() const;
    const Matrix4D& get_projection_matrix() const;
    const Matrix4D& get_view_projection_matrix() const;
    
    // Frustum culling
    bool is_point_visible(const Point3D& point) const;
    bool is_bounds_visible(const BoundingBox3D& bounds) const;
    
private:
    void update_matrices();
    void update_frustum();
};

// =============================================================================
// Technical Line Types
// =============================================================================

/// Pattern element for technical linetypes
struct LinePatternElement {
    enum Type { Line, Gap, Dot, Dash };
    
    Type type;
    Precision length;       ///< Length of element (in drawing units)
    Precision scale;        ///< Scale factor
    
    LinePatternElement(Type t, Precision len, Precision sc = 1.0)
        : type(t), length(len), scale(sc) {}
};

/// Technical linetype definition
class TechnicalLinetype {
private:
    std::string name_;
    std::string description_;
    std::vector<LinePatternElement> pattern_;
    Precision total_length_;
    bool is_continuous_;

public:
    TechnicalLinetype(const std::string& name);
    
    // Pattern management
    void add_element(LinePatternElement::Type type, Precision length, Precision scale = 1.0);
    void clear_pattern();
    void finalize_pattern();  // Calculate total length and optimize
    
    // Accessors
    const std::string& get_name() const { return name_; }
    const std::string& get_description() const { return description_; }
    const std::vector<LinePatternElement>& get_pattern() const { return pattern_; }
    Precision get_total_length() const { return total_length_; }
    bool is_continuous() const { return is_continuous_; }
    
    // Pattern evaluation
    bool is_visible_at_parameter(Precision t) const;
    Precision get_dash_phase(Precision start_param) const;
    
    // Standard linetypes
    static TechnicalLinetype continuous();
    static TechnicalLinetype hidden();
    static TechnicalLinetype center();
    static TechnicalLinetype phantom();
    static TechnicalLinetype dashed();
    static TechnicalLinetype dot_dash();
    static TechnicalLinetype border();
    static TechnicalLinetype divide();
};

// =============================================================================
// Precision Rendering Context
// =============================================================================

/// High-precision rendering context for CAD operations
class PrecisionRenderContext {
private:
    std::shared_ptr<core::RenderingEngine> rendering_engine_;
    PrecisionRenderSettings settings_;
    CADViewport viewport_;
    LayerMap layers_;
    std::unordered_map<std::string, TechnicalLinetype> linetypes_;
    
    // Rendering state
    Matrix4D current_transform_;
    std::string current_layer_;
    Color current_color_;
    LineWeight current_line_weight_;
    std::string current_linetype_;
    Precision current_line_scale_;
    
    // Performance tracking
    mutable size_t rendered_entities_count_;
    mutable size_t culled_entities_count_;
    mutable size_t tessellation_cache_hits_;
    mutable size_t tessellation_cache_misses_;

public:
    PrecisionRenderContext(std::shared_ptr<core::RenderingEngine> engine);
    ~PrecisionRenderContext();
    
    // Settings and configuration
    void set_settings(const PrecisionRenderSettings& settings);
    const PrecisionRenderSettings& get_settings() const { return settings_; }
    
    // Viewport management
    CADViewport& get_viewport() { return viewport_; }
    const CADViewport& get_viewport() const { return viewport_; }
    
    // Layer management
    void add_layer(const CADLayer& layer);
    void set_current_layer(const std::string& name);
    void set_layer_visibility(const std::string& name, bool visible);
    const CADLayer* get_layer(const std::string& name) const;
    
    // Linetype management
    void add_linetype(const TechnicalLinetype& linetype);
    void set_current_linetype(const std::string& name, Precision scale = 1.0);
    const TechnicalLinetype* get_linetype(const std::string& name) const;
    
    // Rendering state
    void push_transform(const Matrix4D& transform);
    void pop_transform();
    void set_color(const Color& color);
    void set_line_weight(LineWeight weight);
    
    // High-level rendering operations
    void begin_frame();
    void end_frame();
    void clear_buffers();
    
    // Entity rendering
    void render_entity(const CADEntity& entity);
    void render_entities(const CADEntityCollection& entities);
    void render_point(const CADPoint& point);
    void render_line(const CADLine& line);
    void render_arc(const CADArc& arc);
    
    // Primitive rendering with sub-pixel precision
    void render_line_segment(const Point3D& start, const Point3D& end);
    void render_polyline(const std::vector<Point3D>& points, bool closed = false);
    void render_circle(const Point3D& center, const Vector3D& normal, Precision radius);
    void render_arc_segment(const Point3D& center, const Vector3D& normal, 
                           Precision radius, Precision start_angle, Precision end_angle);
    
    // Curve rendering with adaptive tessellation
    void render_bezier_curve(const CubicBezier2D& curve, const Matrix4D& transform = Matrix4D::Identity());
    void render_nurbs_curve(const NURBSCurve& curve);
    void render_nurbs_surface(const NURBSSurface& surface, bool show_control_net = false);
    
    // Text rendering (for annotations)
    void render_text(const std::string& text, const Point3D& position, 
                    const Vector3D& direction, Precision height);
    
    // Utility rendering
    void render_coordinate_system(const Point3D& origin, const Matrix3D& axes, Precision scale = 1.0);
    void render_bounding_box(const BoundingBox3D& bbox, const Color& color = Color::Red());
    void render_grid(const Point3D& origin, const Vector3D& u_axis, const Vector3D& v_axis,
                    int u_divisions, int v_divisions, Precision spacing);
    
    // Performance and statistics
    void reset_statistics();
    size_t get_rendered_entities_count() const { return rendered_entities_count_; }
    size_t get_culled_entities_count() const { return culled_entities_count_; }
    size_t get_tessellation_cache_efficiency() const;
    
    // Screenshot and export
    bool save_screenshot(const std::string& filename, int width = 0, int height = 0);
    bool export_to_pdf(const std::string& filename);
    bool export_to_svg(const std::string& filename);

private:
    // Internal rendering helpers
    void setup_line_pattern(const TechnicalLinetype& linetype, Precision scale);
    void render_patterned_line(const Point3D& start, const Point3D& end);
    
    // Tessellation and curve subdivision
    std::vector<Point3D> tessellate_curve(const std::function<Point3D(Precision)>& curve_func,
                                         Precision start_t, Precision end_t);
    std::vector<Point3D> tessellate_arc(const Point3D& center, const Vector3D& normal,
                                       Precision radius, Precision start_angle, Precision end_angle);
    
    // Culling and visibility
    bool is_entity_visible(const CADEntity& entity) const;
    bool is_line_visible(const Point3D& start, const Point3D& end) const;
    
    // Sub-pixel precision helpers
    void enable_sub_pixel_rendering();
    void disable_sub_pixel_rendering();
    Point2D apply_sub_pixel_offset(const Point2D& point) const;
};

// =============================================================================
// Precision Renderer Main Class
// =============================================================================

/// Main precision renderer for CAD graphics
class PrecisionRenderer {
private:
    std::unique_ptr<PrecisionRenderContext> context_;
    PrecisionRenderSettings default_settings_;
    
    // Caching and optimization
    mutable std::unordered_map<EntityID, BoundingBox3D> bounds_cache_;
    mutable std::unordered_map<size_t, std::vector<Point3D>> tessellation_cache_;
    bool cache_enabled_;

public:
    PrecisionRenderer(std::shared_ptr<core::RenderingEngine> engine);
    ~PrecisionRenderer();
    
    // Context access
    PrecisionRenderContext* get_context() { return context_.get(); }
    const PrecisionRenderContext* get_context() const { return context_.get(); }
    
    // High-level rendering interface
    void render_scene(const CADEntityCollection& entities, const CADViewport& viewport);
    void render_entity_selection(const std::vector<EntityID>& selected_ids,
                                const CADEntityCollection& entities);
    void render_preview(const CADEntity& entity, const Color& preview_color = Color::Red());
    
    // Batch rendering for performance
    void begin_batch_rendering();
    void add_to_batch(const CADEntity& entity);
    void render_batch();
    void end_batch_rendering();
    
    // View-dependent rendering
    void render_level_of_detail(const CADEntityCollection& entities, Precision view_scale);
    void render_with_depth_cueing(const CADEntityCollection& entities, 
                                 const Color& fog_color, Precision fog_density);
    
    // Specialized CAD rendering modes
    void render_technical_drawing(const CADEntityCollection& entities,
                                 const PrecisionRenderSettings& settings);
    void render_blueprint_style(const CADEntityCollection& entities,
                               const Color& background = Color::Blue(),
                               const Color& foreground = Color::White());
    
    // Cache management
    void enable_caching(bool enable) { cache_enabled_ = enable; }
    void clear_caches();
    void optimize_caches();
    size_t get_cache_memory_usage() const;
    
    // Settings management
    void set_default_settings(const PrecisionRenderSettings& settings);
    const PrecisionRenderSettings& get_default_settings() const { return default_settings_; }
    
    // Utility functions
    static Precision calculate_tessellation_tolerance(const CADViewport& viewport, 
                                                    const BoundingBox3D& entity_bounds);
    static int calculate_optimal_sampling(RenderingQuality quality, 
                                         const CADViewport& viewport);
    static LineWeight screen_size_to_line_weight(Precision screen_size);
    
    // Debug and profiling
    void enable_debug_visualization(bool enable);
    void render_debug_info(const Point2D& screen_position);
    std::string get_performance_report() const;
};

// =============================================================================
// Utility Functions
// =============================================================================

/// Convert line weight enum to actual thickness in points
Precision line_weight_to_thickness(LineWeight weight);

/// Convert thickness in points to appropriate line weight
LineWeight thickness_to_line_weight(Precision thickness);

/// Calculate optimal tessellation parameters for given view
struct TessellationParams {
    Precision tolerance;
    int max_depth;
    bool adaptive;
};

TessellationParams calculate_tessellation_params(const CADViewport& viewport,
                                              const BoundingBox3D& entity_bounds,
                                              RenderingQuality quality);

/// Convert CAD color to rendering engine color format
uint32_t cad_color_to_rgba(const Color& color);
Color rgba_to_cad_color(uint32_t rgba);

} // namespace qcs::cad