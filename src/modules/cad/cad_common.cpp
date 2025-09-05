/**
 * @file cad_common.cpp
 * @brief Implementation of common CAD utilities and helper functions
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Common functionality shared across all CAD components
 */

#include "cad_common.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>

#ifdef QCS_DEBUG_BUILD
#include <iostream>
#include <iomanip>
#endif

namespace qcs::cad {

// =============================================================================
// Global Tolerance Context
// =============================================================================

ToleranceContext g_tolerance;

bool ToleranceContext::are_points_equal(const Point2D& p1, const Point2D& p2) const {
    return (p1 - p2).norm() <= geometric_tolerance;
}

bool ToleranceContext::are_points_equal(const Point3D& p1, const Point3D& p2) const {
    return (p1 - p2).norm() <= geometric_tolerance;
}

bool ToleranceContext::are_vectors_parallel(const Vector2D& v1, const Vector2D& v2) const {
    Vector2D n1 = v1.normalized();
    Vector2D n2 = v2.normalized();
    Precision cross = std::abs(n1.x() * n2.y() - n1.y() * n2.x());
    return cross <= angular_tolerance;
}

bool ToleranceContext::are_vectors_parallel(const Vector3D& v1, const Vector3D& v2) const {
    Vector3D n1 = v1.normalized();
    Vector3D n2 = v2.normalized();
    Vector3D cross = n1.cross(n2);
    return cross.norm() <= angular_tolerance;
}

bool ToleranceContext::is_zero_length(Precision length) const {
    return std::abs(length) <= geometric_tolerance;
}

bool ToleranceContext::is_zero_area(Precision area) const {
    return std::abs(area) <= geometric_tolerance * geometric_tolerance;
}

bool ToleranceContext::is_zero_volume(Precision volume) const {
    return std::abs(volume) <= geometric_tolerance * geometric_tolerance * geometric_tolerance;
}

// =============================================================================
// Distance Functions
// =============================================================================

Precision point_to_line_segment_distance(const Point2D& point, const LineSegment2D& segment) {
    Vector2D seg_vec = segment.end - segment.start;
    Vector2D point_vec = point - segment.start;
    
    Precision seg_length_sq = seg_vec.squaredNorm();
    if (is_zero(seg_length_sq)) {
        return (point - segment.start).norm();
    }
    
    Precision t = point_vec.dot(seg_vec) / seg_length_sq;
    t = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    
    Point2D closest = segment.start + t * seg_vec;
    return (point - closest).norm();
}

Precision point_to_line_segment_distance(const Point3D& point, const LineSegment3D& segment) {
    Vector3D seg_vec = segment.end - segment.start;
    Vector3D point_vec = point - segment.start;
    
    Precision seg_length_sq = seg_vec.squaredNorm();
    if (is_zero(seg_length_sq)) {
        return (point - segment.start).norm();
    }
    
    Precision t = point_vec.dot(seg_vec) / seg_length_sq;
    t = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    
    Point3D closest = segment.start + t * seg_vec;
    return (point - closest).norm();
}

Precision point_to_line_distance(const Point2D& point, const Point2D& line_point, const Vector2D& line_dir) {
    Vector2D normalized_dir = line_dir.normalized();
    Vector2D point_vec = point - line_point;
    
    // Distance = |cross product| / |direction|
    Precision cross = std::abs(point_vec.x() * normalized_dir.y() - point_vec.y() * normalized_dir.x());
    return cross;
}

Precision point_to_line_distance(const Point3D& point, const Point3D& line_point, const Vector3D& line_dir) {
    Vector3D normalized_dir = line_dir.normalized();
    Vector3D point_vec = point - line_point;
    
    // Distance = |cross product|
    Vector3D cross = point_vec.cross(normalized_dir);
    return cross.norm();
}

Precision point_to_plane_distance(const Point3D& point, const Point3D& plane_point, const Vector3D& plane_normal) {
    Vector3D normalized_normal = plane_normal.normalized();
    Vector3D point_vec = point - plane_point;
    return std::abs(point_vec.dot(normalized_normal));
}

// =============================================================================
// Closest Point Functions
// =============================================================================

Point2D closest_point_on_segment(const Point2D& point, const LineSegment2D& segment) {
    Vector2D seg_vec = segment.end - segment.start;
    Vector2D point_vec = point - segment.start;
    
    Precision seg_length_sq = seg_vec.squaredNorm();
    if (is_zero(seg_length_sq)) {
        return segment.start;
    }
    
    Precision t = point_vec.dot(seg_vec) / seg_length_sq;
    t = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    
    return segment.start + t * seg_vec;
}

Point3D closest_point_on_segment(const Point3D& point, const LineSegment3D& segment) {
    Vector3D seg_vec = segment.end - segment.start;
    Vector3D point_vec = point - segment.start;
    
    Precision seg_length_sq = seg_vec.squaredNorm();
    if (is_zero(seg_length_sq)) {
        return segment.start;
    }
    
    Precision t = point_vec.dot(seg_vec) / seg_length_sq;
    t = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    
    return segment.start + t * seg_vec;
}

Point2D closest_point_on_line(const Point2D& point, const Point2D& line_point, const Vector2D& line_dir) {
    Vector2D normalized_dir = line_dir.normalized();
    Vector2D point_vec = point - line_point;
    Precision t = point_vec.dot(normalized_dir);
    return line_point + t * normalized_dir;
}

Point3D closest_point_on_line(const Point3D& point, const Point3D& line_point, const Vector3D& line_dir) {
    Vector3D normalized_dir = line_dir.normalized();
    Vector3D point_vec = point - line_point;
    Precision t = point_vec.dot(normalized_dir);
    return line_point + t * normalized_dir;
}

Point3D closest_point_on_plane(const Point3D& point, const Point3D& plane_point, const Vector3D& plane_normal) {
    Vector3D normalized_normal = plane_normal.normalized();
    Vector3D point_vec = point - plane_point;
    Precision distance = point_vec.dot(normalized_normal);
    return point - distance * normalized_normal;
}

// =============================================================================
// Intersection Functions
// =============================================================================

std::optional<Point2D> intersect_lines_2d(const Point2D& p1, const Vector2D& d1,
                                          const Point2D& p2, const Vector2D& d2) {
    // Solve: p1 + t1*d1 = p2 + t2*d2
    // Rearrange to: t1*d1 - t2*d2 = p2 - p1
    
    Precision det = d1.x() * d2.y() - d1.y() * d2.x();
    if (is_zero(det)) {
        return std::nullopt; // Lines are parallel
    }
    
    Vector2D dp = p2 - p1;
    Precision t1 = (dp.x() * d2.y() - dp.y() * d2.x()) / det;
    
    return p1 + t1 * d1;
}

std::optional<Point2D> intersect_segments_2d(const LineSegment2D& seg1, const LineSegment2D& seg2) {
    Vector2D d1 = seg1.end - seg1.start;
    Vector2D d2 = seg2.end - seg2.start;
    
    auto intersection = intersect_lines_2d(seg1.start, d1, seg2.start, d2);
    if (!intersection) {
        return std::nullopt;
    }
    
    // Check if intersection point is within both segments
    Vector2D to_int1 = intersection.value() - seg1.start;
    Vector2D to_int2 = intersection.value() - seg2.start;
    
    Precision t1 = d1.squaredNorm() > 0 ? to_int1.dot(d1) / d1.squaredNorm() : 0;
    Precision t2 = d2.squaredNorm() > 0 ? to_int2.dot(d2) / d2.squaredNorm() : 0;
    
    if (t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1) {
        return intersection;
    }
    
    return std::nullopt;
}

std::optional<Point3D> intersect_line_plane(const Point3D& line_point, const Vector3D& line_dir,
                                           const Point3D& plane_point, const Vector3D& plane_normal) {
    Vector3D normalized_normal = plane_normal.normalized();
    Vector3D normalized_dir = line_dir.normalized();
    
    Precision denominator = normalized_dir.dot(normalized_normal);
    if (is_zero(denominator)) {
        return std::nullopt; // Line is parallel to plane
    }
    
    Vector3D to_plane = plane_point - line_point;
    Precision t = to_plane.dot(normalized_normal) / denominator;
    
    return line_point + t * normalized_dir;
}

std::vector<Point2D> intersect_circles(const Point2D& c1, Precision r1,
                                       const Point2D& c2, Precision r2) {
    std::vector<Point2D> intersections;
    
    Precision d = (c2 - c1).norm();
    
    // Check for non-intersecting circles
    if (d > r1 + r2 || d < std::abs(r1 - r2) || is_zero(d)) {
        return intersections;
    }
    
    // Calculate intersection points
    Precision a = (r1 * r1 - r2 * r2 + d * d) / (2 * d);
    Precision h = std::sqrt(r1 * r1 - a * a);
    
    Vector2D to_c2 = (c2 - c1).normalized();
    Point2D p = c1 + a * to_c2;
    
    Vector2D perpendicular(-to_c2.y(), to_c2.x());
    
    intersections.push_back(p + h * perpendicular);
    if (!is_zero(h)) {
        intersections.push_back(p - h * perpendicular);
    }
    
    return intersections;
}

std::vector<Point2D> intersect_line_circle(const Point2D& line_point, const Vector2D& line_dir,
                                          const Point2D& circle_center, Precision radius) {
    std::vector<Point2D> intersections;
    
    Vector2D normalized_dir = line_dir.normalized();
    Vector2D to_center = circle_center - line_point;
    
    // Project center onto line
    Precision proj_length = to_center.dot(normalized_dir);
    Point2D closest_point = line_point + proj_length * normalized_dir;
    
    // Distance from center to line
    Precision distance = (circle_center - closest_point).norm();
    
    if (distance > radius) {
        return intersections; // No intersection
    }
    
    if (approximately_equal(distance, radius)) {
        intersections.push_back(closest_point); // Tangent
        return intersections;
    }
    
    // Two intersections
    Precision half_chord = std::sqrt(radius * radius - distance * distance);
    intersections.push_back(closest_point - half_chord * normalized_dir);
    intersections.push_back(closest_point + half_chord * normalized_dir);
    
    return intersections;
}

// =============================================================================
// Geometric Transformations
// =============================================================================

Matrix3D rotation_matrix_2d(Precision angle) {
    Matrix3D rot = Matrix3D::Identity();
    Precision cos_a = std::cos(angle);
    Precision sin_a = std::sin(angle);
    
    rot(0, 0) = cos_a; rot(0, 1) = -sin_a;
    rot(1, 0) = sin_a; rot(1, 1) = cos_a;
    
    return rot;
}

Matrix4D rotation_matrix_3d(const Vector3D& axis, Precision angle) {
    Matrix4D rot = Matrix4D::Identity();
    
    Vector3D normalized_axis = axis.normalized();
    Precision cos_a = std::cos(angle);
    Precision sin_a = std::sin(angle);
    Precision one_minus_cos = 1.0 - cos_a;
    
    Precision x = normalized_axis.x();
    Precision y = normalized_axis.y();
    Precision z = normalized_axis.z();
    
    // Rodrigues' rotation formula
    rot(0, 0) = cos_a + x*x*one_minus_cos;
    rot(0, 1) = x*y*one_minus_cos - z*sin_a;
    rot(0, 2) = x*z*one_minus_cos + y*sin_a;
    
    rot(1, 0) = y*x*one_minus_cos + z*sin_a;
    rot(1, 1) = cos_a + y*y*one_minus_cos;
    rot(1, 2) = y*z*one_minus_cos - x*sin_a;
    
    rot(2, 0) = z*x*one_minus_cos - y*sin_a;
    rot(2, 1) = z*y*one_minus_cos + x*sin_a;
    rot(2, 2) = cos_a + z*z*one_minus_cos;
    
    return rot;
}

Matrix4D translation_matrix(const Vector3D& translation) {
    Matrix4D trans = Matrix4D::Identity();
    trans(0, 3) = translation.x();
    trans(1, 3) = translation.y();
    trans(2, 3) = translation.z();
    return trans;
}

Matrix4D scaling_matrix(const Vector3D& scale) {
    Matrix4D scaling = Matrix4D::Identity();
    scaling(0, 0) = scale.x();
    scaling(1, 1) = scale.y();
    scaling(2, 2) = scale.z();
    return scaling;
}

Matrix4D transformation_matrix(const Vector3D& translation, 
                              const Quaternion& rotation, 
                              const Vector3D& scale) {
    Matrix4D transform = Matrix4D::Identity();
    
    // Apply scale
    Matrix4D scale_matrix = scaling_matrix(scale);
    
    // Apply rotation
    Matrix4D rot_matrix = Matrix4D::Identity();
    rot_matrix.block<3,3>(0,0) = rotation.toRotationMatrix();
    
    // Apply translation
    Matrix4D trans_matrix = translation_matrix(translation);
    
    // Combine: T * R * S
    transform = trans_matrix * rot_matrix * scale_matrix;
    
    return transform;
}

Point2D transform_point(const Point2D& point, const Matrix3D& transform) {
    Vector3D homogeneous(point.x(), point.y(), 1.0);
    Vector3D transformed = transform * homogeneous;
    return Point2D(transformed.x(), transformed.y());
}

Point3D transform_point(const Point3D& point, const Matrix4D& transform) {
    Eigen::Vector4<Precision> homogeneous(point.x(), point.y(), point.z(), 1.0);
    Eigen::Vector4<Precision> transformed = transform * homogeneous;
    return Point3D(transformed.x(), transformed.y(), transformed.z());
}

Vector2D transform_vector(const Vector2D& vector, const Matrix3D& transform) {
    Vector3D homogeneous(vector.x(), vector.y(), 0.0); // w=0 for vectors
    Vector3D transformed = transform * homogeneous;
    return Vector2D(transformed.x(), transformed.y());
}

Vector3D transform_vector(const Vector3D& vector, const Matrix4D& transform) {
    Eigen::Vector4<Precision> homogeneous(vector.x(), vector.y(), vector.z(), 0.0); // w=0 for vectors
    Eigen::Vector4<Precision> transformed = transform * homogeneous;
    return Vector3D(transformed.x(), transformed.y(), transformed.z());
}

// =============================================================================
// Polygon Operations
// =============================================================================

Precision polygon_area(const std::vector<Point2D>& vertices) {
    if (vertices.size() < 3) return 0.0;
    
    Precision area = 0.0;
    size_t n = vertices.size();
    
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        area += vertices[i].x() * vertices[j].y();
        area -= vertices[j].x() * vertices[i].y();
    }
    
    return std::abs(area) * 0.5;
}

bool point_in_polygon(const Point2D& point, const std::vector<Point2D>& vertices) {
    if (vertices.size() < 3) return false;
    
    bool inside = false;
    size_t n = vertices.size();
    
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        if (((vertices[i].y() > point.y()) != (vertices[j].y() > point.y())) &&
            (point.x() < (vertices[j].x() - vertices[i].x()) * (point.y() - vertices[i].y()) / 
             (vertices[j].y() - vertices[i].y()) + vertices[i].x())) {
            inside = !inside;
        }
    }
    
    return inside;
}

Point2D polygon_centroid(const std::vector<Point2D>& vertices) {
    if (vertices.empty()) return Point2D::Zero();
    
    Point2D centroid = Point2D::Zero();
    Precision area = 0.0;
    size_t n = vertices.size();
    
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        Precision cross = vertices[i].x() * vertices[j].y() - vertices[j].x() * vertices[i].y();
        area += cross;
        centroid += (vertices[i] + vertices[j]) * cross;
    }
    
    area *= 0.5;
    if (is_zero(area)) {
        // Degenerate polygon, return arithmetic mean
        for (const auto& vertex : vertices) {
            centroid += vertex;
        }
        return centroid / static_cast<Precision>(vertices.size());
    }
    
    return centroid / (6.0 * area);
}

bool is_polygon_convex(const std::vector<Point2D>& vertices) {
    if (vertices.size() < 3) return false;
    
    size_t n = vertices.size();
    bool positive = false, negative = false;
    
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        size_t k = (i + 2) % n;
        
        Vector2D v1 = vertices[j] - vertices[i];
        Vector2D v2 = vertices[k] - vertices[j];
        
        Precision cross = v1.x() * v2.y() - v1.y() * v2.x();
        
        if (cross > GEOMETRIC_TOLERANCE) positive = true;
        if (cross < -GEOMETRIC_TOLERANCE) negative = true;
        
        if (positive && negative) return false;
    }
    
    return true;
}

// =============================================================================
// Debug and Validation Utilities
// =============================================================================

bool validate_entity(const CADEntity& entity) {
    // Basic validation checks
    if (entity.id == INVALID_ENTITY_ID) return false;
    if (entity.name.empty()) return false;
    if (!entity.bounds.is_valid()) return false;
    
    // Entity-specific validation would be implemented by derived classes
    return true;
}

bool is_valid_transformation(const Matrix4D& transform) {
    // Check if matrix is finite
    if (!transform.allFinite()) return false;
    
    // Check if bottom row is [0, 0, 0, 1]
    if (!approximately_equal(transform(3, 0), 0.0) ||
        !approximately_equal(transform(3, 1), 0.0) ||
        !approximately_equal(transform(3, 2), 0.0) ||
        !approximately_equal(transform(3, 3), 1.0)) {
        return false;
    }
    
    // Check if the transformation matrix has reasonable scale
    Matrix3D rotation_scale = transform.block<3,3>(0, 0);
    Precision det = rotation_scale.determinant();
    return det > 1e-6; // Avoid singular transformations
}

DegeneracyType detect_degeneracy(const LineSegment2D& segment) {
    if ((segment.end - segment.start).norm() < GEOMETRIC_TOLERANCE) {
        return DegeneracyType::ZeroLength;
    }
    return DegeneracyType::None;
}

DegeneracyType detect_degeneracy(const LineSegment3D& segment) {
    if ((segment.end - segment.start).norm() < GEOMETRIC_TOLERANCE) {
        return DegeneracyType::ZeroLength;
    }
    return DegeneracyType::None;
}

DegeneracyType detect_degeneracy(const CubicBezier2D& curve) {
    // Check if all control points are collinear
    if (are_collinear(curve.p0, curve.p1, curve.p2) && 
        are_collinear(curve.p0, curve.p2, curve.p3)) {
        return DegeneracyType::CollinearPoints;
    }
    
    // Check for zero-length curve
    BoundingBox2D bounds = curve.bounds();
    if (bounds.size().norm() < GEOMETRIC_TOLERANCE) {
        return DegeneracyType::ZeroLength;
    }
    
    return DegeneracyType::None;
}

#ifdef QCS_DEBUG_BUILD
// =============================================================================
// Performance Profiling (Debug Only)
// =============================================================================

GeometricTimer::GeometricTimer(const std::string& name) 
    : start_time_(std::chrono::high_resolution_clock::now())
    , operation_name_(name) {
}

GeometricTimer::~GeometricTimer() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    
    std::cout << "[GEOMETRY PROFILE] " << operation_name_ 
              << " took " << duration.count() << " μs" << std::endl;
}
#endif

} // namespace qcs::cad