# QuantumCanvas Studio Plugin API Types Specification
## Comprehensive Plugin Development Interface

### Overview

The QuantumCanvas Studio Plugin API provides a stable, high-performance interface for extending the application with custom tools, filters, effects, and workflows. The API emphasizes type safety, memory efficiency, and cross-language compatibility.

#### Design Principles
- **Stable ABI**: Binary compatibility across compiler versions and languages
- **Type Safety**: Strong typing with comprehensive validation
- **Performance**: Zero-copy operations and GPU acceleration support
- **Memory Safe**: Automatic resource management with RAII
- **Cross-Platform**: Consistent behavior across Windows, macOS, and Linux
- **Versioned**: Forward and backward compatibility with semantic versioning

---

## Core Type System

### Fundamental Types
```cpp
// src/plugins/api/plugin_types.hpp
namespace QuantumCanvas::Plugin::API {

// Version information
struct APIVersion {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    
    constexpr APIVersion(uint16_t maj, uint16_t min, uint16_t p) 
        : major(maj), minor(min), patch(p) {}
    
    bool is_compatible_with(const APIVersion& other) const;
    std::string to_string() const;
};

constexpr APIVersion CURRENT_API_VERSION{1, 0, 0};

// Basic geometric types
struct Point2D {
    double x, y;
    Point2D() : x(0), y(0) {}
    Point2D(double x, double y) : x(x), y(y) {}
};

struct Point3D {
    double x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double x, double y, double z) : x(x), y(y), z(z) {}
};

struct Size2D {
    double width, height;
    Size2D() : width(0), height(0) {}
    Size2D(double w, double h) : width(w), height(h) {}
};

struct Rect2D {
    Point2D origin;
    Size2D size;
    Rect2D() = default;
    Rect2D(const Point2D& o, const Size2D& s) : origin(o), size(s) {}
    Rect2D(double x, double y, double w, double h) : origin(x, y), size(w, h) {}
    
    bool contains(const Point2D& point) const;
    bool intersects(const Rect2D& other) const;
    Rect2D intersection(const Rect2D& other) const;
    Rect2D union_with(const Rect2D& other) const;
};

// Color types with high precision
struct ColorRGBA {
    float r, g, b, a;
    ColorRGBA() : r(0), g(0), b(0), a(1) {}
    ColorRGBA(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    
    // Convenience constructors
    static ColorRGBA from_uint8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    static ColorRGBA from_hex(uint32_t hex);
    
    uint32_t to_uint32() const;
    std::string to_hex_string() const;
};

// Matrix types for transformations
struct Matrix2D {
    double m[6]; // [a, b, c, d, tx, ty] - affine transform matrix
    
    Matrix2D(); // Identity matrix
    Matrix2D(double a, double b, double c, double d, double tx, double ty);
    
    static Matrix2D identity();
    static Matrix2D translation(double tx, double ty);
    static Matrix2D rotation(double angle_radians);
    static Matrix2D scale(double sx, double sy);
    
    Matrix2D operator*(const Matrix2D& other) const;
    Point2D transform_point(const Point2D& point) const;
    Matrix2D inverse() const;
    bool is_identity() const;
};

}
```

### Resource Management
```cpp
// src/plugins/api/resource_management.hpp
namespace QuantumCanvas::Plugin::API {

// Base class for all plugin-manageable resources
class IResource {
public:
    virtual ~IResource() = default;
    
    // Resource identification
    virtual uint64_t resource_id() const = 0;
    virtual std::string resource_type() const = 0;
    
    // Memory management
    virtual size_t memory_usage() const = 0;
    virtual bool is_gpu_resident() const = 0;
    
    // Serialization support
    virtual bool is_serializable() const = 0;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual bool deserialize(const std::vector<uint8_t>& data) = 0;
};

// Smart pointer wrapper for resources
template<typename T>
class ResourceHandle {
public:
    ResourceHandle() = default;
    explicit ResourceHandle(std::shared_ptr<T> resource) : resource_(resource) {}
    
    T* get() const { return resource_.get(); }
    T& operator*() const { return *resource_; }
    T* operator->() const { return resource_.get(); }
    
    bool is_valid() const { return resource_ != nullptr; }
    void reset() { resource_.reset(); }
    
    // Reference counting
    long use_count() const { return resource_.use_count(); }
    
private:
    std::shared_ptr<T> resource_;
};

// Resource factory for plugins
class ResourceFactory {
public:
    template<typename T, typename... Args>
    static ResourceHandle<T> create(Args&&... args);
    
    template<typename T>
    static ResourceHandle<T> wrap(std::shared_ptr<T> resource);
    
    // Resource tracking
    static size_t get_total_resource_count();
    static size_t get_total_memory_usage();
    static std::vector<uint64_t> get_active_resource_ids();
};

}
```

---

## Vector Graphics Types

### Path and Shape Definitions
```cpp
// src/plugins/api/vector_types.hpp
namespace QuantumCanvas::Plugin::API {

enum class PathCommandType {
    MoveTo,
    LineTo,
    CurveTo,     // Cubic Bézier curve
    QuadTo,      // Quadratic Bézier curve  
    ArcTo,       // Elliptical arc
    ClosePath
};

struct PathCommand {
    PathCommandType type;
    std::vector<Point2D> points; // Variable number based on command type
    
    PathCommand(PathCommandType t, std::initializer_list<Point2D> pts = {})
        : type(t), points(pts) {}
};

class Path : public IResource {
public:
    Path();
    explicit Path(const std::vector<PathCommand>& commands);
    
    // Path construction
    void move_to(const Point2D& point);
    void line_to(const Point2D& point);
    void curve_to(const Point2D& control1, const Point2D& control2, const Point2D& end);
    void quad_to(const Point2D& control, const Point2D& end);
    void arc_to(const Point2D& center, double radius, double start_angle, double end_angle);
    void close_path();
    void clear();
    
    // Path queries
    bool is_closed() const;
    bool is_empty() const;
    size_t command_count() const;
    const std::vector<PathCommand>& commands() const;
    Rect2D bounds() const;
    
    // Path operations
    Path simplified(double tolerance = 0.1) const;
    Path stroked(double width, LineCapStyle cap = LineCapStyle::Round,
                LineJoinStyle join = LineJoinStyle::Round) const;
    std::vector<Path> dash(const std::vector<double>& pattern, double offset = 0) const;
    
    // Boolean operations
    Path union_with(const Path& other) const;
    Path intersect_with(const Path& other) const;
    Path subtract(const Path& other) const;
    Path xor_with(const Path& other) const;
    
    // Geometric queries
    bool contains_point(const Point2D& point, FillRule rule = FillRule::EvenOdd) const;
    double distance_to_point(const Point2D& point) const;
    std::vector<Point2D> intersect_with_line(const Point2D& start, const Point2D& end) const;
    
    // Transformations
    void transform(const Matrix2D& matrix);
    Path transformed(const Matrix2D& matrix) const;
    
    // IResource implementation
    uint64_t resource_id() const override;
    std::string resource_type() const override { return "Path"; }
    size_t memory_usage() const override;
    bool is_gpu_resident() const override { return false; }
    
private:
    std::vector<PathCommand> commands_;
    mutable std::optional<Rect2D> cached_bounds_;
    uint64_t resource_id_;
    
    void invalidate_cache();
};

// Path style definitions
enum class LineCapStyle {
    Butt,
    Round,
    Square
};

enum class LineJoinStyle {
    Miter,
    Round,
    Bevel
};

enum class FillRule {
    EvenOdd,
    NonZero
};

struct StrokeStyle {
    double width = 1.0;
    LineCapStyle cap = LineCapStyle::Round;
    LineJoinStyle join = LineJoinStyle::Round;
    double miter_limit = 4.0;
    std::vector<double> dash_pattern;
    double dash_offset = 0.0;
    ColorRGBA color{0, 0, 0, 1};
    
    bool operator==(const StrokeStyle& other) const;
};

struct FillStyle {
    ColorRGBA color{0, 0, 0, 1};
    FillRule rule = FillRule::EvenOdd;
    
    // Gradient support (optional)
    struct Gradient {
        enum class Type { Linear, Radial, Conic };
        Type type = Type::Linear;
        Point2D start;
        Point2D end;
        double radius = 0.0; // For radial gradients
        
        struct Stop {
            double offset; // 0.0 to 1.0
            ColorRGBA color;
        };
        std::vector<Stop> stops;
    };
    
    std::optional<Gradient> gradient;
    
    bool operator==(const FillStyle& other) const;
};

}
```

### Vector Object Types
```cpp
// src/plugins/api/vector_objects.hpp
namespace QuantumCanvas::Plugin::API {

class VectorObject : public IResource {
public:
    virtual ~VectorObject() = default;
    
    // Object identification
    virtual std::string object_type() const = 0;
    virtual uint64_t object_id() const = 0;
    
    // Geometric properties
    virtual Rect2D bounds() const = 0;
    virtual Path outline() const = 0;
    virtual bool contains_point(const Point2D& point) const = 0;
    
    // Style properties
    virtual bool has_fill() const = 0;
    virtual bool has_stroke() const = 0;
    virtual FillStyle fill_style() const = 0;
    virtual StrokeStyle stroke_style() const = 0;
    virtual void set_fill_style(const FillStyle& style) = 0;
    virtual void set_stroke_style(const StrokeStyle& style) = 0;
    
    // Transformations
    virtual Matrix2D transform() const = 0;
    virtual void set_transform(const Matrix2D& matrix) = 0;
    virtual void apply_transform(const Matrix2D& matrix) = 0;
    
    // Cloning
    virtual std::unique_ptr<VectorObject> clone() const = 0;
};

// Concrete vector object implementations
class PathObject : public VectorObject {
public:
    explicit PathObject(ResourceHandle<Path> path);
    
    // VectorObject implementation
    std::string object_type() const override { return "PathObject"; }
    uint64_t object_id() const override;
    Rect2D bounds() const override;
    Path outline() const override;
    bool contains_point(const Point2D& point) const override;
    
    // Path-specific methods
    ResourceHandle<Path> path() const { return path_; }
    void set_path(ResourceHandle<Path> path);
    
    std::unique_ptr<VectorObject> clone() const override;
    
private:
    ResourceHandle<Path> path_;
    FillStyle fill_style_;
    StrokeStyle stroke_style_;
    Matrix2D transform_;
    uint64_t object_id_;
};

class RectangleObject : public VectorObject {
public:
    RectangleObject(const Rect2D& rect);
    
    std::string object_type() const override { return "RectangleObject"; }
    Rect2D bounds() const override;
    Path outline() const override;
    
    // Rectangle-specific properties
    Rect2D rectangle() const { return rect_; }
    void set_rectangle(const Rect2D& rect);
    
    double corner_radius() const { return corner_radius_; }
    void set_corner_radius(double radius);
    
    std::unique_ptr<VectorObject> clone() const override;
    
private:
    Rect2D rect_;
    double corner_radius_ = 0.0;
    FillStyle fill_style_;
    StrokeStyle stroke_style_;
    Matrix2D transform_;
    uint64_t object_id_;
};

class EllipseObject : public VectorObject {
public:
    EllipseObject(const Point2D& center, double radius_x, double radius_y);
    
    std::string object_type() const override { return "EllipseObject"; }
    Rect2D bounds() const override;
    Path outline() const override;
    
    // Ellipse-specific properties
    Point2D center() const { return center_; }
    void set_center(const Point2D& center);
    
    double radius_x() const { return radius_x_; }
    double radius_y() const { return radius_y_; }
    void set_radii(double rx, double ry);
    
    std::unique_ptr<VectorObject> clone() const override;
    
private:
    Point2D center_;
    double radius_x_, radius_y_;
    FillStyle fill_style_;
    StrokeStyle stroke_style_;
    Matrix2D transform_;
    uint64_t object_id_;
};

}
```

---

## Raster Graphics Types

### Image and Pixel Data
```cpp
// src/plugins/api/raster_types.hpp
namespace QuantumCanvas::Plugin::API {

enum class PixelFormat {
    RGBA8,          // 8-bit per channel RGBA
    RGBA16,         // 16-bit per channel RGBA
    RGBA32F,        // 32-bit float per channel RGBA
    RGB8,           // 8-bit per channel RGB
    RGB16,          // 16-bit per channel RGB
    RGB32F,         // 32-bit float per channel RGB
    Gray8,          // 8-bit grayscale
    Gray16,         // 16-bit grayscale
    Gray32F,        // 32-bit float grayscale
    CMYK8,          // 8-bit CMYK for print
    Lab32F          // Lab color space
};

struct PixelFormatInfo {
    PixelFormat format;
    uint8_t channels;
    uint8_t bits_per_channel;
    uint8_t bytes_per_pixel;
    bool is_float;
    bool has_alpha;
    std::string name;
    
    static PixelFormatInfo get_info(PixelFormat format);
};

class Image : public IResource {
public:
    Image();
    Image(uint32_t width, uint32_t height, PixelFormat format);
    Image(uint32_t width, uint32_t height, PixelFormat format, const void* data);
    
    // Image properties
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    Size2D size() const { return Size2D(width_, height_); }
    PixelFormat format() const { return format_; }
    size_t bytes_per_row() const;
    size_t total_bytes() const;
    
    // Data access
    void* data() { return data_.data(); }
    const void* data() const { return data_.data(); }
    
    template<typename T>
    T* typed_data() { return static_cast<T*>(data()); }
    
    template<typename T>
    const T* typed_data() const { return static_cast<const T*>(data()); }
    
    // Pixel access
    ColorRGBA get_pixel(uint32_t x, uint32_t y) const;
    void set_pixel(uint32_t x, uint32_t y, const ColorRGBA& color);
    
    // Region operations
    Image crop(const Rect2D& rect) const;
    void paste(const Image& source, const Point2D& destination);
    void clear(const ColorRGBA& color = ColorRGBA(0, 0, 0, 0));
    
    // Format conversion
    Image converted_to_format(PixelFormat new_format) const;
    void convert_in_place(PixelFormat new_format);
    
    // Transformations
    Image scaled(const Size2D& new_size, InterpolationMethod method = InterpolationMethod::Bilinear) const;
    Image rotated(double angle_degrees, const Point2D& center, InterpolationMethod method = InterpolationMethod::Bilinear) const;
    Image transformed(const Matrix2D& matrix, const Size2D& output_size, InterpolationMethod method = InterpolationMethod::Bilinear) const;
    
    // Statistics
    struct Statistics {
        ColorRGBA mean;
        ColorRGBA std_deviation;
        ColorRGBA min_value;
        ColorRGBA max_value;
        std::array<std::vector<uint32_t>, 4> histograms; // RGBA histograms
    };
    
    Statistics compute_statistics() const;
    
    // IResource implementation
    uint64_t resource_id() const override;
    std::string resource_type() const override { return "Image"; }
    size_t memory_usage() const override;
    bool is_gpu_resident() const override;
    
    // GPU acceleration
    void upload_to_gpu();
    void download_from_gpu();
    bool is_gpu_resident() const { return gpu_texture_id_ != 0; }
    
private:
    uint32_t width_, height_;
    PixelFormat format_;
    std::vector<uint8_t> data_;
    uint64_t resource_id_;
    uint32_t gpu_texture_id_ = 0;
    
    void ensure_size();
};

enum class InterpolationMethod {
    Nearest,        // Nearest neighbor
    Bilinear,       // Bilinear interpolation
    Bicubic,        // Bicubic interpolation
    Lanczos         // Lanczos resampling
};

}
```

### Brush and Painting Types
```cpp
// src/plugins/api/brush_types.hpp
namespace QuantumCanvas::Plugin::API {

struct BrushSettings {
    double size = 10.0;                    // Brush size in pixels
    double opacity = 1.0;                  // 0.0 to 1.0
    double hardness = 0.8;                 // Edge hardness 0.0 to 1.0
    double spacing = 0.2;                  // Spacing between dabs (0.0 to 1.0)
    double flow = 1.0;                     // Paint flow rate
    double density = 1.0;                  // Bristle density for textured brushes
    
    // Pressure sensitivity
    struct PressureCurve {
        bool enabled = true;
        double size_sensitivity = 0.5;     // How much pressure affects size
        double opacity_sensitivity = 0.3;  // How much pressure affects opacity
        double flow_sensitivity = 0.2;     // How much pressure affects flow
        std::vector<Point2D> curve_points; // Custom pressure response curve
    } pressure;
    
    // Tilt and rotation (for stylus)
    struct StylusSettings {
        bool use_tilt = false;
        bool use_rotation = false;
        double tilt_sensitivity = 0.5;
        double rotation_sensitivity = 0.3;
    } stylus;
    
    // Texture settings
    struct TextureSettings {
        bool enabled = false;
        ResourceHandle<Image> texture;
        double scale = 1.0;
        double strength = 0.5;
        bool invert = false;
    } texture;
    
    ColorRGBA color{0, 0, 0, 1};
    
    bool operator==(const BrushSettings& other) const;
};

struct BrushStroke {
    std::vector<Point2D> points;
    std::vector<double> pressures;  // Corresponding pressure values
    std::vector<double> tilts;      // Stylus tilt values
    std::vector<double> rotations;  // Stylus rotation values
    std::vector<std::chrono::system_clock::time_point> timestamps;
    
    BrushSettings settings;
    uint64_t stroke_id;
    
    // Stroke properties
    Rect2D bounds() const;
    double total_length() const;
    std::vector<BrushStroke> simplified(double tolerance = 1.0) const;
    
    // Stroke manipulation
    void smooth(double strength = 0.5);
    void resample(double target_spacing);
    BrushStroke section(size_t start_index, size_t end_index) const;
};

// Abstract brush engine interface
class IBrushEngine {
public:
    virtual ~IBrushEngine() = default;
    
    // Brush information
    virtual std::string brush_name() const = 0;
    virtual std::string brush_description() const = 0;
    virtual APIVersion required_api_version() const = 0;
    
    // Brush rendering
    virtual void apply_stroke(Image& target_image, const BrushStroke& stroke) = 0;
    virtual void apply_dab(Image& target_image, const Point2D& position, 
                          double pressure, double tilt, double rotation,
                          const BrushSettings& settings) = 0;
    
    // Preview generation
    virtual Image generate_preview(const Size2D& size, const BrushSettings& settings) = 0;
    
    // Settings validation
    virtual bool validate_settings(const BrushSettings& settings) const = 0;
    virtual BrushSettings get_default_settings() const = 0;
    
    // Performance characteristics
    virtual bool supports_gpu_acceleration() const = 0;
    virtual bool supports_pressure() const = 0;
    virtual bool supports_tilt() const = 0;
    virtual bool supports_rotation() const = 0;
};

}
```

---

## Layer System Types

### Layer Hierarchy and Management
```cpp
// src/plugins/api/layer_types.hpp
namespace QuantumCanvas::Plugin::API {

enum class LayerType {
    Group,          // Container for other layers
    Vector,         // Vector graphics layer
    Raster,         // Raster/bitmap layer  
    Text,           // Text layer
    Adjustment,     // Color/tone adjustment layer
    Effects,        // Effects/filters layer
    Reference       // Reference/guide layer
};

enum class BlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    SoftLight,
    HardLight,
    ColorDodge,
    ColorBurn,
    Darken,
    Lighten,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity
};

struct LayerProperties {
    std::string name;
    bool visible = true;
    bool locked = false;
    double opacity = 1.0;              // 0.0 to 1.0
    BlendMode blend_mode = BlendMode::Normal;
    Matrix2D transform = Matrix2D::identity();
    
    // Clipping and masking
    bool use_clipping_mask = false;
    uint64_t clipping_mask_layer_id = 0;
    
    // Layer effects
    struct Effects {
        bool drop_shadow = false;
        bool inner_shadow = false;
        bool outer_glow = false;
        bool inner_glow = false;
        bool bevel_emboss = false;
        bool color_overlay = false;
        bool gradient_overlay = false;
        bool pattern_overlay = false;
        bool stroke = false;
    } effects;
    
    bool operator==(const LayerProperties& other) const;
};

class Layer : public IResource {
public:
    virtual ~Layer() = default;
    
    // Layer identification
    virtual uint64_t layer_id() const = 0;
    virtual LayerType layer_type() const = 0;
    virtual uint64_t parent_id() const = 0;
    
    // Properties
    virtual LayerProperties properties() const = 0;
    virtual void set_properties(const LayerProperties& props) = 0;
    
    // Hierarchy management
    virtual std::vector<uint64_t> child_layer_ids() const = 0;
    virtual void add_child(uint64_t child_id) = 0;
    virtual void remove_child(uint64_t child_id) = 0;
    virtual void reorder_child(uint64_t child_id, size_t new_index) = 0;
    
    // Rendering
    virtual Rect2D bounds() const = 0;
    virtual Image render(const Size2D& canvas_size, double dpi = 72.0) const = 0;
    virtual Image render_region(const Rect2D& region, double dpi = 72.0) const = 0;
    
    // Layer-specific content access
    virtual bool has_vector_content() const = 0;
    virtual bool has_raster_content() const = 0;
    virtual std::vector<ResourceHandle<VectorObject>> get_vector_objects() const = 0;
    virtual ResourceHandle<Image> get_raster_content() const = 0;
    
    // Transformations
    virtual void apply_transform(const Matrix2D& matrix) = 0;
    virtual void reset_transform() = 0;
    
    // Cloning
    virtual std::unique_ptr<Layer> clone() const = 0;
};

// Concrete layer implementations
class VectorLayer : public Layer {
public:
    VectorLayer(const std::string& name = "Vector Layer");
    
    LayerType layer_type() const override { return LayerType::Vector; }
    
    // Vector object management
    void add_object(ResourceHandle<VectorObject> object);
    void remove_object(uint64_t object_id);
    std::vector<ResourceHandle<VectorObject>> get_vector_objects() const override;
    
    // Vector-specific rendering
    Image render(const Size2D& canvas_size, double dpi = 72.0) const override;
    
    bool has_vector_content() const override { return !objects_.empty(); }
    bool has_raster_content() const override { return false; }
    
private:
    std::vector<ResourceHandle<VectorObject>> objects_;
};

class RasterLayer : public Layer {
public:
    RasterLayer(const std::string& name = "Raster Layer");
    RasterLayer(ResourceHandle<Image> image, const std::string& name = "Raster Layer");
    
    LayerType layer_type() const override { return LayerType::Raster; }
    
    // Raster content management
    ResourceHandle<Image> get_raster_content() const override { return image_; }
    void set_raster_content(ResourceHandle<Image> image);
    
    // Raster-specific operations
    void apply_brush_stroke(const BrushStroke& stroke);
    void apply_filter(const std::string& filter_name, const std::map<std::string, double>& parameters);
    
    bool has_vector_content() const override { return false; }
    bool has_raster_content() const override { return image_.is_valid(); }
    
private:
    ResourceHandle<Image> image_;
};

class GroupLayer : public Layer {
public:
    GroupLayer(const std::string& name = "Group");
    
    LayerType layer_type() const override { return LayerType::Group; }
    
    // Group-specific rendering (composite children)
    Image render(const Size2D& canvas_size, double dpi = 72.0) const override;
    
    bool has_vector_content() const override;
    bool has_raster_content() const override;
    
private:
    // Child management is handled by base Layer class
};

}
```

---

## Selection System Types

### Selection Management
```cpp
// src/plugins/api/selection_types.hpp
namespace QuantumCanvas::Plugin::API {

enum class SelectionMode {
    Replace,    // Replace current selection
    Add,        // Add to current selection
    Subtract,   // Remove from current selection
    Intersect   // Intersect with current selection
};

class Selection : public IResource {
public:
    Selection();
    
    // Selection content
    void add_layer(uint64_t layer_id);
    void remove_layer(uint64_t layer_id);
    void add_object(uint64_t object_id);
    void remove_object(uint64_t object_id);
    void clear();
    
    // Query selection
    bool is_empty() const;
    std::vector<uint64_t> selected_layers() const;
    std::vector<uint64_t> selected_objects() const;
    bool contains_layer(uint64_t layer_id) const;
    bool contains_object(uint64_t object_id) const;
    size_t count() const;
    
    // Selection bounds
    Rect2D bounds() const;
    Point2D center() const;
    
    // Selection operations
    void select_all_in_layer(uint64_t layer_id);
    void select_by_type(const std::string& object_type);
    void select_by_color(const ColorRGBA& color, double tolerance = 0.1);
    void select_intersecting(const Rect2D& rect);
    void select_containing_point(const Point2D& point);
    
    // Marching ants visualization
    struct MarchingAnts {
        std::vector<Path> paths;
        double dash_length = 5.0;
        double animation_speed = 1.0;
        ColorRGBA color{0, 0, 0, 1};
    };
    
    MarchingAnts generate_marching_ants() const;
    
    // IResource implementation
    uint64_t resource_id() const override;
    std::string resource_type() const override { return "Selection"; }
    size_t memory_usage() const override;
    
private:
    std::set<uint64_t> selected_layers_;
    std::set<uint64_t> selected_objects_;
    uint64_t resource_id_;
    mutable std::optional<Rect2D> cached_bounds_;
    
    void invalidate_cache();
};

// Selection tools interface
class ISelectionTool {
public:
    virtual ~ISelectionTool() = default;
    
    // Tool information
    virtual std::string tool_name() const = 0;
    virtual std::string tool_description() const = 0;
    
    // Selection operations
    virtual void begin_selection(const Point2D& start_point, SelectionMode mode) = 0;
    virtual void update_selection(const Point2D& current_point) = 0;
    virtual Selection finalize_selection() = 0;
    virtual void cancel_selection() = 0;
    
    // Tool state
    virtual bool is_active() const = 0;
    virtual Path preview_path() const = 0;  // For visual feedback
    
    // Settings
    virtual std::map<std::string, double> get_settings() const = 0;
    virtual void set_setting(const std::string& name, double value) = 0;
};

// Concrete selection tool implementations
class RectangleSelectionTool : public ISelectionTool {
public:
    std::string tool_name() const override { return "Rectangle Selection"; }
    void begin_selection(const Point2D& start_point, SelectionMode mode) override;
    void update_selection(const Point2D& current_point) override;
    Selection finalize_selection() override;
    
private:
    Point2D start_point_;
    Point2D current_point_;
    SelectionMode mode_;
    bool is_active_ = false;
};

class LassoSelectionTool : public ISelectionTool {
public:
    std::string tool_name() const override { return "Lasso Selection"; }
    void begin_selection(const Point2D& start_point, SelectionMode mode) override;
    void update_selection(const Point2D& current_point) override;
    Selection finalize_selection() override;
    
private:
    std::vector<Point2D> lasso_points_;
    SelectionMode mode_;
    bool is_active_ = false;
};

}
```

---

## Plugin Interface Definition

### Main Plugin Interface
```cpp
// src/plugins/api/plugin_interface.hpp
namespace QuantumCanvas::Plugin::API {

enum class PluginCapability {
    VectorTools      = 1 << 0,   // Can create vector tools
    RasterTools      = 1 << 1,   // Can create raster tools
    Filters          = 1 << 2,   // Can create image filters
    Effects          = 1 << 3,   // Can create layer effects
    Importers        = 1 << 4,   // Can import file formats
    Exporters        = 1 << 5,   // Can export file formats
    UIComponents     = 1 << 6,   // Can create UI panels
    Generators       = 1 << 7,   // Can generate content
    AI_ML            = 1 << 8,   // Uses AI/ML features
    NetworkAccess    = 1 << 9,   // Requires network access
    FileSystemAccess = 1 << 10,  // Requires file system access
    GPUAcceleration  = 1 << 11   // Uses GPU acceleration
};

using PluginCapabilities = uint32_t; // Bitfield of PluginCapability values

struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string website;
    std::string license;
    
    APIVersion required_api_version;
    APIVersion maximum_api_version;
    PluginCapabilities capabilities;
    
    // Resource limits
    size_t max_memory_usage_mb = 512;
    size_t max_gpu_memory_mb = 256;
    bool requires_internet = false;
    
    // Platform requirements
    std::vector<std::string> supported_platforms; // "windows", "macos", "linux"
    std::string minimum_os_version;
};

class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // Plugin lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool is_initialized() const = 0;
    
    // Plugin information
    virtual PluginInfo get_info() const = 0;
    virtual std::string get_error_message() const = 0;
    
    // Tool creation (if supported)
    virtual std::vector<std::string> get_available_tools() const = 0;
    virtual std::unique_ptr<ITool> create_tool(const std::string& tool_name) = 0;
    
    // Filter creation (if supported)
    virtual std::vector<std::string> get_available_filters() const = 0;
    virtual std::unique_ptr<IFilter> create_filter(const std::string& filter_name) = 0;
    
    // UI component creation (if supported)
    virtual std::vector<std::string> get_available_ui_components() const = 0;
    virtual std::unique_ptr<IUIComponent> create_ui_component(const std::string& component_name) = 0;
    
    // File format support (if supported)
    virtual std::vector<std::string> get_supported_import_formats() const = 0;
    virtual std::vector<std::string> get_supported_export_formats() const = 0;
    virtual std::unique_ptr<IImporter> create_importer(const std::string& format) = 0;
    virtual std::unique_ptr<IExporter> create_exporter(const std::string& format) = 0;
    
    // Configuration
    virtual std::map<std::string, std::string> get_configuration() const = 0;
    virtual void set_configuration(const std::map<std::string, std::string>& config) = 0;
    
    // Resource management
    virtual size_t get_memory_usage() const = 0;
    virtual void optimize_memory_usage() = 0;
    
    // Error handling
    virtual void set_error_callback(std::function<void(const std::string&)> callback) = 0;
};

// Plugin entry point (must be implemented by plugin)
extern "C" {
    // Plugin must export these functions
    PLUGIN_API IPlugin* create_plugin();
    PLUGIN_API void destroy_plugin(IPlugin* plugin);
    PLUGIN_API APIVersion get_plugin_api_version();
    PLUGIN_API const char* get_plugin_name();
}

#ifdef _WIN32
    #define PLUGIN_API __declspec(dllexport)
#else
    #define PLUGIN_API __attribute__((visibility("default")))
#endif

}
```

### Tool Interface
```cpp
// src/plugins/api/tool_interface.hpp
namespace QuantumCanvas::Plugin::API {

enum class ToolType {
    Selection,      // Selection tools
    Drawing,        // Drawing/painting tools
    Shape,          // Shape creation tools
    Transform,      // Transformation tools
    Text,           // Text tools
    Measurement,    // Measurement/annotation tools
    Navigation      // Navigation/zoom tools
};

enum class CursorType {
    Default,
    Crosshair,
    Hand,
    IBeam,
    SizeAll,
    SizeNS,
    SizeWE,
    SizeNWSE,
    SizeNESW,
    Wait,
    Custom
};

struct ToolSettings {
    std::map<std::string, double> numeric_settings;
    std::map<std::string, std::string> string_settings;
    std::map<std::string, bool> boolean_settings;
    std::map<std::string, ColorRGBA> color_settings;
    
    // Convenience accessors
    double get_numeric(const std::string& key, double default_value = 0.0) const;
    void set_numeric(const std::string& key, double value);
    
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    void set_string(const std::string& key, const std::string& value);
    
    bool get_boolean(const std::string& key, bool default_value = false) const;
    void set_boolean(const std::string& key, bool value);
    
    ColorRGBA get_color(const std::string& key, const ColorRGBA& default_value = ColorRGBA()) const;
    void set_color(const std::string& key, const ColorRGBA& value);
};

class ITool {
public:
    virtual ~ITool() = default;
    
    // Tool information
    virtual std::string tool_name() const = 0;
    virtual std::string tool_description() const = 0;
    virtual ToolType tool_type() const = 0;
    virtual CursorType cursor_type() const = 0;
    
    // Tool lifecycle
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual bool is_active() const = 0;
    
    // Input handling
    virtual void mouse_press(const Point2D& position, int button, int modifiers) = 0;
    virtual void mouse_move(const Point2D& position, int modifiers) = 0;
    virtual void mouse_release(const Point2D& position, int button, int modifiers) = 0;
    virtual void key_press(int key, int modifiers) = 0;
    virtual void key_release(int key, int modifiers) = 0;
    
    // Rendering
    virtual void render_overlay(void* render_context) = 0; // Platform-specific render context
    virtual Rect2D overlay_bounds() const = 0;
    
    // Settings
    virtual ToolSettings get_settings() const = 0;
    virtual void set_settings(const ToolSettings& settings) = 0;
    virtual std::vector<std::string> get_setting_names() const = 0;
    
    // Settings UI description (for automatic UI generation)
    struct SettingDescriptor {
        std::string name;
        std::string display_name;
        std::string description;
        std::string type; // "numeric", "string", "boolean", "color", "choice"
        
        // For numeric settings
        double min_value = 0.0;
        double max_value = 100.0;
        double step = 1.0;
        
        // For choice settings
        std::vector<std::string> choices;
        
        // For all settings
        std::string default_value;
        std::string group; // For organizing settings in UI
    };
    
    virtual std::vector<SettingDescriptor> get_setting_descriptors() const = 0;
    
    // Undo/Redo support
    virtual bool has_pending_changes() const = 0;
    virtual void commit_changes() = 0;
    virtual void cancel_changes() = 0;
};

}
```

This comprehensive plugin API specification provides a stable, high-performance foundation for extending QuantumCanvas Studio with custom functionality while maintaining type safety and cross-platform compatibility.