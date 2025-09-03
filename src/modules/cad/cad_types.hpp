/**
 * @file cad_types.hpp
 * @brief Core CAD data types and geometric primitives
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Enterprise-grade CAD data types with sub-pixel precision
 */

#pragma once

#include <memory>
#include <vector>
#include <array>
#include <optional>
#include <variant>
#include <string>
#include <unordered_map>

// Platform includes
#include "../../core/kernel/kernel_manager.hpp"
#include "../../math/precision_math.hpp"

// Third-party includes
#include <Eigen/Dense>
#include <Eigen/Geometry>

namespace qcs::cad {

// =============================================================================
// Precision Configuration
// =============================================================================

/// High-precision floating point type for CAD calculations
using Precision = double;

/// Geometric tolerance for CAD operations (1 nanometer precision)
constexpr Precision GEOMETRIC_TOLERANCE = 1e-12;

/// Angular tolerance for CAD operations (micro-radians)
constexpr Precision ANGULAR_TOLERANCE = 1e-10;

/// Minimum feature size in CAD units
constexpr Precision MIN_FEATURE_SIZE = 1e-9;

// =============================================================================
// Basic Geometric Types
// =============================================================================

/// 2D point with high precision
using Point2D = Eigen::Vector2<Precision>;

/// 3D point with high precision
using Point3D = Eigen::Vector3<Precision>;

/// 2D vector with high precision
using Vector2D = Eigen::Vector2<Precision>;

/// 3D vector with high precision  
using Vector3D = Eigen::Vector3<Precision>;

/// 4x4 transformation matrix
using Matrix4D = Eigen::Matrix4<Precision>;

/// 3x3 rotation matrix
using Matrix3D = Eigen::Matrix3<Precision>;

/// Quaternion for 3D rotations
using Quaternion = Eigen::Quaternion<Precision>;

/// RGBA color with high precision components
struct Color {
    Precision r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(1) {}
    Color(Precision r_, Precision g_, Precision b_, Precision a_ = 1.0)
        : r(r_), g(g_), b(b_), a(a_) {}
        
    static Color Black() { return {0, 0, 0, 1}; }
    static Color White() { return {1, 1, 1, 1}; }
    static Color Red() { return {1, 0, 0, 1}; }
    static Color Green() { return {0, 1, 0, 1}; }
    static Color Blue() { return {0, 0, 1, 1}; }
};

// =============================================================================
// Geometric Primitives
// =============================================================================

/// 2D line segment
struct LineSegment2D {
    Point2D start;
    Point2D end;
    
    LineSegment2D(const Point2D& s, const Point2D& e) : start(s), end(e) {}
    
    Precision length() const { return (end - start).norm(); }
    Vector2D direction() const { return (end - start).normalized(); }
    Point2D midpoint() const { return (start + end) * 0.5; }
};

/// 3D line segment
struct LineSegment3D {
    Point3D start;
    Point3D end;
    
    LineSegment3D(const Point3D& s, const Point3D& e) : start(s), end(e) {}
    
    Precision length() const { return (end - start).norm(); }
    Vector3D direction() const { return (end - start).normalized(); }
    Point3D midpoint() const { return (start + end) * 0.5; }
};

/// 2D circular arc
struct Arc2D {
    Point2D center;
    Precision radius;
    Precision start_angle;
    Precision end_angle;
    
    Arc2D(const Point2D& c, Precision r, Precision sa, Precision ea)
        : center(c), radius(r), start_angle(sa), end_angle(ea) {}
        
    Precision angular_span() const { return end_angle - start_angle; }
    Precision arc_length() const { return radius * std::abs(angular_span()); }
    Point2D start_point() const;
    Point2D end_point() const;
};

/// 3D circle
struct Circle3D {
    Point3D center;
    Vector3D normal;
    Precision radius;
    
    Circle3D(const Point3D& c, const Vector3D& n, Precision r)
        : center(c), normal(n.normalized()), radius(r) {}
        
    Precision circumference() const { return 2.0 * M_PI * radius; }
    Precision area() const { return M_PI * radius * radius; }
};

/// 2D bounding box
struct BoundingBox2D {
    Point2D min;
    Point2D max;
    
    BoundingBox2D() : min(Point2D::Zero()), max(Point2D::Zero()) {}
    BoundingBox2D(const Point2D& mi, const Point2D& ma) : min(mi), max(ma) {}
    
    bool is_valid() const { return min.x() <= max.x() && min.y() <= max.y(); }
    Vector2D size() const { return max - min; }
    Point2D center() const { return (min + max) * 0.5; }
    Precision area() const { auto s = size(); return s.x() * s.y(); }
    
    void expand(const Point2D& point);
    void expand(const BoundingBox2D& other);
    bool contains(const Point2D& point) const;
    bool intersects(const BoundingBox2D& other) const;
};

/// 3D bounding box
struct BoundingBox3D {
    Point3D min;
    Point3D max;
    
    BoundingBox3D() : min(Point3D::Zero()), max(Point3D::Zero()) {}
    BoundingBox3D(const Point3D& mi, const Point3D& ma) : min(mi), max(ma) {}
    
    bool is_valid() const { 
        return min.x() <= max.x() && min.y() <= max.y() && min.z() <= max.z(); 
    }
    Vector3D size() const { return max - min; }
    Point3D center() const { return (min + max) * 0.5; }
    Precision volume() const { auto s = size(); return s.x() * s.y() * s.z(); }
    
    void expand(const Point3D& point);
    void expand(const BoundingBox3D& other);
    bool contains(const Point3D& point) const;
    bool intersects(const BoundingBox3D& other) const;
};

// =============================================================================
// Curve Types
// =============================================================================

/// Quadratic Bezier curve in 2D
struct QuadraticBezier2D {
    Point2D p0, p1, p2;  // Control points
    
    QuadraticBezier2D(const Point2D& start, const Point2D& control, const Point2D& end)
        : p0(start), p1(control), p2(end) {}
        
    Point2D evaluate(Precision t) const;
    Vector2D tangent(Precision t) const;
    Precision curvature(Precision t) const;
    BoundingBox2D bounds() const;
};

/// Cubic Bezier curve in 2D
struct CubicBezier2D {
    Point2D p0, p1, p2, p3;  // Control points
    
    CubicBezier2D(const Point2D& start, const Point2D& control1, 
                  const Point2D& control2, const Point2D& end)
        : p0(start), p1(control1), p2(control2), p3(end) {}
        
    Point2D evaluate(Precision t) const;
    Vector2D tangent(Precision t) const;
    Precision curvature(Precision t) const;
    BoundingBox2D bounds() const;
    
    /// Split curve at parameter t into two cubic curves
    std::pair<CubicBezier2D, CubicBezier2D> split(Precision t) const;
};

/// NURBS curve representation
struct NURBSCurve {
    std::vector<Point3D> control_points;
    std::vector<Precision> weights;
    std::vector<Precision> knot_vector;
    int degree;
    
    NURBSCurve(int deg) : degree(deg) {}
    
    Point3D evaluate(Precision u) const;
    Vector3D tangent(Precision u) const;
    BoundingBox3D bounds() const;
    bool is_rational() const;
};

// =============================================================================
// Surface Types
// =============================================================================

/// NURBS surface representation
struct NURBSSurface {
    std::vector<std::vector<Point3D>> control_points;
    std::vector<std::vector<Precision>> weights;
    std::vector<Precision> u_knots;
    std::vector<Precision> v_knots;
    int u_degree;
    int v_degree;
    
    NURBSSurface(int u_deg, int v_deg) : u_degree(u_deg), v_degree(v_deg) {}
    
    Point3D evaluate(Precision u, Precision v) const;
    Vector3D normal(Precision u, Precision v) const;
    BoundingBox3D bounds() const;
};

// =============================================================================
// CAD Entity Types
// =============================================================================

/// Entity identifier type
using EntityID = uint64_t;

/// Invalid entity ID constant
constexpr EntityID INVALID_ENTITY_ID = 0;

/// CAD entity base class
class CADEntity {
public:
    EntityID id;
    std::string name;
    std::string layer;
    Color color;
    Matrix4D transform;
    BoundingBox3D bounds;
    bool visible;
    bool selectable;
    
    CADEntity(EntityID entity_id = INVALID_ENTITY_ID)
        : id(entity_id)
        , name("")
        , layer("Default")
        , color(Color::Black())
        , transform(Matrix4D::Identity())
        , visible(true)
        , selectable(true)
    {}
    
    virtual ~CADEntity() = default;
    virtual BoundingBox3D calculate_bounds() const = 0;
    virtual void update_bounds() { bounds = calculate_bounds(); }
    virtual std::unique_ptr<CADEntity> clone() const = 0;
};

/// Smart pointer for CAD entities
using CADEntityPtr = std::shared_ptr<CADEntity>;

// =============================================================================
// Geometric Entity Types
// =============================================================================

/// CAD point entity
class CADPoint : public CADEntity {
public:
    Point3D position;
    
    CADPoint(const Point3D& pos, EntityID id = INVALID_ENTITY_ID)
        : CADEntity(id), position(pos) {
        update_bounds();
    }
    
    BoundingBox3D calculate_bounds() const override {
        return BoundingBox3D(position, position);
    }
    
    std::unique_ptr<CADEntity> clone() const override {
        return std::make_unique<CADPoint>(*this);
    }
};

/// CAD line entity
class CADLine : public CADEntity {
public:
    Point3D start;
    Point3D end;
    Precision thickness;
    
    CADLine(const Point3D& s, const Point3D& e, EntityID id = INVALID_ENTITY_ID)
        : CADEntity(id), start(s), end(e), thickness(0.0) {
        update_bounds();
    }
    
    BoundingBox3D calculate_bounds() const override {
        BoundingBox3D bbox;
        bbox.expand(start);
        bbox.expand(end);
        return bbox;
    }
    
    std::unique_ptr<CADEntity> clone() const override {
        return std::make_unique<CADLine>(*this);
    }
    
    Precision length() const { return (end - start).norm(); }
    Vector3D direction() const { return (end - start).normalized(); }
};

/// CAD arc entity
class CADArc : public CADEntity {
public:
    Point3D center;
    Vector3D normal;
    Precision radius;
    Precision start_angle;
    Precision end_angle;
    
    CADArc(const Point3D& c, const Vector3D& n, Precision r, 
           Precision sa, Precision ea, EntityID id = INVALID_ENTITY_ID)
        : CADEntity(id), center(c), normal(n.normalized()), radius(r)
        , start_angle(sa), end_angle(ea) {
        update_bounds();
    }
    
    BoundingBox3D calculate_bounds() const override;
    
    std::unique_ptr<CADEntity> clone() const override {
        return std::make_unique<CADArc>(*this);
    }
    
    Precision arc_length() const { return radius * std::abs(end_angle - start_angle); }
    Point3D start_point() const;
    Point3D end_point() const;
};

// =============================================================================
// Collections and Hierarchies
// =============================================================================

/// Collection of CAD entities
using CADEntityCollection = std::vector<CADEntityPtr>;

/// Entity lookup map
using EntityMap = std::unordered_map<EntityID, CADEntityPtr>;

/// Layer definition
struct CADLayer {
    std::string name;
    Color color;
    bool visible;
    bool locked;
    bool printable;
    Precision line_weight;
    
    CADLayer(const std::string& layer_name = "Default")
        : name(layer_name)
        , color(Color::Black())
        , visible(true)
        , locked(false)
        , printable(true)
        , line_weight(0.25)
    {}
};

/// Layer collection
using LayerMap = std::unordered_map<std::string, CADLayer>;

// =============================================================================
// Measurement and Units
// =============================================================================

/// Length units enumeration
enum class LengthUnit {
    Millimeter,
    Centimeter,
    Meter,
    Inch,
    Foot,
    Point,  // 1/72 inch
    Pica    // 1/6 inch
};

/// Angle units enumeration
enum class AngleUnit {
    Radians,
    Degrees,
    Gradians
};

/// Unit conversion utilities
class UnitConverter {
public:
    static Precision convert_length(Precision value, LengthUnit from, LengthUnit to);
    static Precision convert_angle(Precision value, AngleUnit from, AngleUnit to);
    static std::string unit_name(LengthUnit unit);
    static std::string unit_symbol(LengthUnit unit);
};

// =============================================================================
// Precision and Tolerance Management
// =============================================================================

/// Tolerance context for geometric operations
struct ToleranceContext {
    Precision geometric_tolerance = GEOMETRIC_TOLERANCE;
    Precision angular_tolerance = ANGULAR_TOLERANCE;
    Precision curve_tolerance = 1e-8;
    Precision surface_tolerance = 1e-8;
    
    bool are_points_equal(const Point2D& p1, const Point2D& p2) const;
    bool are_points_equal(const Point3D& p1, const Point3D& p2) const;
    bool are_vectors_parallel(const Vector2D& v1, const Vector2D& v2) const;
    bool are_vectors_parallel(const Vector3D& v1, const Vector3D& v2) const;
    bool is_zero_length(Precision length) const;
    bool is_zero_area(Precision area) const;
    bool is_zero_volume(Precision volume) const;
};

/// Global tolerance context
extern ToleranceContext g_tolerance;

} // namespace qcs::cad