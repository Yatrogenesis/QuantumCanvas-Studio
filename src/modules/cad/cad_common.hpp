/**
 * @file cad_common.hpp
 * @brief Common CAD utilities and helper functions
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Common functionality shared across all CAD components
 */

#pragma once

#include "cad_types.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <type_traits>

namespace qcs::cad {

// =============================================================================
// Mathematical Constants
// =============================================================================

/// High-precision mathematical constants
namespace constants {
    constexpr Precision PI = 3.141592653589793238462643383279502884;
    constexpr Precision TAU = 2.0 * PI;  // Full circle in radians
    constexpr Precision HALF_PI = PI * 0.5;
    constexpr Precision QUARTER_PI = PI * 0.25;
    constexpr Precision INV_PI = 1.0 / PI;
    constexpr Precision SQRT_2 = 1.414213562373095048801688724209698079;
    constexpr Precision SQRT_3 = 1.732050807568877293527446341505872367;
    constexpr Precision E = 2.718281828459045235360287471352662498;
    constexpr Precision GOLDEN_RATIO = 1.618033988749894848204586834365638118;
    
    // Angle conversions
    constexpr Precision DEG_TO_RAD = PI / 180.0;
    constexpr Precision RAD_TO_DEG = 180.0 / PI;
    constexpr Precision GRAD_TO_RAD = PI / 200.0;
    constexpr Precision RAD_TO_GRAD = 200.0 / PI;
}

// =============================================================================
// Utility Functions
// =============================================================================

/// Safe floating-point comparison with tolerance
template<typename T>
inline bool approximately_equal(T a, T b, T tolerance = static_cast<T>(GEOMETRIC_TOLERANCE)) {
    return std::abs(a - b) <= tolerance;
}

/// Check if value is effectively zero
template<typename T>
inline bool is_zero(T value, T tolerance = static_cast<T>(GEOMETRIC_TOLERANCE)) {
    return std::abs(value) <= tolerance;
}

/// Clamp value to range [min, max]
template<typename T>
inline T clamp(T value, T min_val, T max_val) {
    return std::min(std::max(value, min_val), max_val);
}

/// Linear interpolation between two values
template<typename T>
inline T lerp(const T& a, const T& b, Precision t) {
    return a + t * (b - a);
}

/// Normalize angle to [0, 2π) range
inline Precision normalize_angle_2pi(Precision angle) {
    angle = std::fmod(angle, constants::TAU);
    if (angle < 0) angle += constants::TAU;
    return angle;
}

/// Normalize angle to [-π, π) range
inline Precision normalize_angle_pi(Precision angle) {
    angle = normalize_angle_2pi(angle);
    if (angle > constants::PI) angle -= constants::TAU;
    return angle;
}

/// Compute angle between two vectors
template<typename VectorType>
inline Precision angle_between_vectors(const VectorType& v1, const VectorType& v2) {
    Precision dot = v1.normalized().dot(v2.normalized());
    dot = clamp(dot, static_cast<Precision>(-1), static_cast<Precision>(1));
    return std::acos(dot);
}

/// Check if three points are collinear
inline bool are_collinear(const Point2D& p1, const Point2D& p2, const Point2D& p3,
                         Precision tolerance = GEOMETRIC_TOLERANCE) {
    Vector2D v1 = p2 - p1;
    Vector2D v2 = p3 - p1;
    Precision cross = v1.x() * v2.y() - v1.y() * v2.x();
    return is_zero(cross, tolerance);
}

inline bool are_collinear(const Point3D& p1, const Point3D& p2, const Point3D& p3,
                         Precision tolerance = GEOMETRIC_TOLERANCE) {
    Vector3D v1 = p2 - p1;
    Vector3D v2 = p3 - p1;
    Vector3D cross = v1.cross(v2);
    return cross.norm() <= tolerance;
}

// =============================================================================
// Distance Functions
// =============================================================================

/// Distance from point to line segment
Precision point_to_line_segment_distance(const Point2D& point, const LineSegment2D& segment);
Precision point_to_line_segment_distance(const Point3D& point, const LineSegment3D& segment);

/// Distance from point to infinite line
Precision point_to_line_distance(const Point2D& point, const Point2D& line_point, const Vector2D& line_dir);
Precision point_to_line_distance(const Point3D& point, const Point3D& line_point, const Vector3D& line_dir);

/// Distance from point to plane
Precision point_to_plane_distance(const Point3D& point, const Point3D& plane_point, const Vector3D& plane_normal);

/// Closest point on line segment to given point
Point2D closest_point_on_segment(const Point2D& point, const LineSegment2D& segment);
Point3D closest_point_on_segment(const Point3D& point, const LineSegment3D& segment);

/// Closest point on infinite line to given point
Point2D closest_point_on_line(const Point2D& point, const Point2D& line_point, const Vector2D& line_dir);
Point3D closest_point_on_line(const Point3D& point, const Point3D& line_point, const Vector3D& line_dir);

/// Closest point on plane to given point
Point3D closest_point_on_plane(const Point3D& point, const Point3D& plane_point, const Vector3D& plane_normal);

// =============================================================================
// Intersection Functions
// =============================================================================

/// Line-line intersection in 2D
std::optional<Point2D> intersect_lines_2d(const Point2D& p1, const Vector2D& d1,
                                          const Point2D& p2, const Vector2D& d2);

/// Line segment intersection in 2D
std::optional<Point2D> intersect_segments_2d(const LineSegment2D& seg1, const LineSegment2D& seg2);

/// Line-plane intersection
std::optional<Point3D> intersect_line_plane(const Point3D& line_point, const Vector3D& line_dir,
                                           const Point3D& plane_point, const Vector3D& plane_normal);

/// Circle-circle intersection
std::vector<Point2D> intersect_circles(const Point2D& c1, Precision r1,
                                       const Point2D& c2, Precision r2);

/// Line-circle intersection
std::vector<Point2D> intersect_line_circle(const Point2D& line_point, const Vector2D& line_dir,
                                          const Point2D& circle_center, Precision radius);

// =============================================================================
// Geometric Transformations
// =============================================================================

/// Create 2D rotation matrix
Matrix3D rotation_matrix_2d(Precision angle);

/// Create 3D rotation matrix around axis
Matrix4D rotation_matrix_3d(const Vector3D& axis, Precision angle);

/// Create translation matrix
Matrix4D translation_matrix(const Vector3D& translation);

/// Create scaling matrix
Matrix4D scaling_matrix(const Vector3D& scale);

/// Create transformation matrix from components
Matrix4D transformation_matrix(const Vector3D& translation, 
                              const Quaternion& rotation, 
                              const Vector3D& scale);

/// Apply transformation to point
Point2D transform_point(const Point2D& point, const Matrix3D& transform);
Point3D transform_point(const Point3D& point, const Matrix4D& transform);

/// Apply transformation to vector (ignores translation)
Vector2D transform_vector(const Vector2D& vector, const Matrix3D& transform);
Vector3D transform_vector(const Vector3D& vector, const Matrix4D& transform);

// =============================================================================
// Curve Utilities
// =============================================================================

/// Compute arc length of curve using adaptive quadrature
template<typename CurveType>
Precision compute_arc_length(const CurveType& curve, Precision t_start = 0, Precision t_end = 1,
                            Precision tolerance = 1e-8);

/// Find parameter t for given arc length along curve
template<typename CurveType>
Precision arc_length_to_parameter(const CurveType& curve, Precision arc_length,
                                 Precision tolerance = 1e-8);

/// Compute curvature of curve at parameter t
template<typename CurveType>
Precision compute_curvature(const CurveType& curve, Precision t);

/// Find inflection points of cubic curve
std::vector<Precision> find_inflection_points(const CubicBezier2D& curve);

/// Subdivide curve at given parameters
template<typename CurveType>
std::vector<CurveType> subdivide_curve(const CurveType& curve, const std::vector<Precision>& params);

// =============================================================================
// Polygon Operations
// =============================================================================

/// Calculate area of 2D polygon (positive = counter-clockwise)
Precision polygon_area(const std::vector<Point2D>& vertices);

/// Check if point is inside polygon (ray casting algorithm)
bool point_in_polygon(const Point2D& point, const std::vector<Point2D>& vertices);

/// Compute centroid of polygon
Point2D polygon_centroid(const std::vector<Point2D>& vertices);

/// Check if polygon is convex
bool is_polygon_convex(const std::vector<Point2D>& vertices);

/// Triangulate simple polygon using ear clipping
std::vector<std::array<size_t, 3>> triangulate_polygon(const std::vector<Point2D>& vertices);

// =============================================================================
// Bounding Volume Operations
// =============================================================================

/// Merge multiple bounding boxes
template<typename BoundingBoxType>
BoundingBoxType merge_bounding_boxes(const std::vector<BoundingBoxType>& boxes);

/// Check if bounding box is degenerate (zero volume/area)
template<typename BoundingBoxType>
bool is_degenerate(const BoundingBoxType& bbox);

/// Expand bounding box by margin
template<typename BoundingBoxType, typename VectorType>
BoundingBoxType expand_bounding_box(const BoundingBoxType& bbox, const VectorType& margin);

// =============================================================================
// Hash Functions for Geometric Types
// =============================================================================

/// Hash function for Point2D (for use in unordered containers)
struct Point2DHash {
    size_t operator()(const Point2D& p) const {
        auto h1 = std::hash<Precision>{}(p.x());
        auto h2 = std::hash<Precision>{}(p.y());
        return h1 ^ (h2 << 1);
    }
};

/// Hash function for Point3D
struct Point3DHash {
    size_t operator()(const Point3D& p) const {
        auto h1 = std::hash<Precision>{}(p.x());
        auto h2 = std::hash<Precision>{}(p.y());
        auto h3 = std::hash<Precision>{}(p.z());
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// =============================================================================
// Debug and Validation Utilities
// =============================================================================

/// Validate geometric entity for consistency
bool validate_entity(const CADEntity& entity);

/// Check if transformation matrix is valid
bool is_valid_transformation(const Matrix4D& transform);

/// Detect degeneracies in geometric entities
enum class DegeneracyType {
    None,
    ZeroLength,
    ZeroArea,
    ZeroVolume,
    CollinearPoints,
    CoplanarPoints,
    InvalidParameters
};

DegeneracyType detect_degeneracy(const LineSegment2D& segment);
DegeneracyType detect_degeneracy(const LineSegment3D& segment);
DegeneracyType detect_degeneracy(const CubicBezier2D& curve);

/// Error reporting for geometric operations
class GeometricError : public std::runtime_error {
public:
    explicit GeometricError(const std::string& message) 
        : std::runtime_error("Geometric Error: " + message) {}
};

// =============================================================================
// Performance Profiling Helpers
// =============================================================================

#ifdef QCS_DEBUG_BUILD
/// RAII timer for profiling geometric operations
class GeometricTimer {
    std::chrono::high_resolution_clock::time_point start_time_;
    std::string operation_name_;
    
public:
    explicit GeometricTimer(const std::string& name);
    ~GeometricTimer();
};

#define QCS_PROFILE_GEOMETRY(name) GeometricTimer timer(name)
#else
#define QCS_PROFILE_GEOMETRY(name)
#endif

} // namespace qcs::cad