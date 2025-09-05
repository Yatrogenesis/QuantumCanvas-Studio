/**
 * @file cad_types.cpp
 * @brief Implementation of core CAD data types and geometric primitives
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Implementation of high-precision CAD types and entities
 */

#include "cad_types.hpp"
#include "cad_common.hpp"
#include <algorithm>
#include <cmath>

namespace qcs::cad {

// =============================================================================
// Arc2D Implementation
// =============================================================================

Point2D Arc2D::start_point() const {
    return center + radius * Vector2D(std::cos(start_angle), std::sin(start_angle));
}

Point2D Arc2D::end_point() const {
    return center + radius * Vector2D(std::cos(end_angle), std::sin(end_angle));
}

// =============================================================================
// BoundingBox2D Implementation
// =============================================================================

void BoundingBox2D::expand(const Point2D& point) {
    if (!is_valid()) {
        min = max = point;
    } else {
        min = min.cwiseMin(point);
        max = max.cwiseMax(point);
    }
}

void BoundingBox2D::expand(const BoundingBox2D& other) {
    if (!other.is_valid()) return;
    
    if (!is_valid()) {
        *this = other;
    } else {
        min = min.cwiseMin(other.min);
        max = max.cwiseMax(other.max);
    }
}

bool BoundingBox2D::contains(const Point2D& point) const {
    if (!is_valid()) return false;
    
    return point.x() >= min.x() - GEOMETRIC_TOLERANCE &&
           point.x() <= max.x() + GEOMETRIC_TOLERANCE &&
           point.y() >= min.y() - GEOMETRIC_TOLERANCE &&
           point.y() <= max.y() + GEOMETRIC_TOLERANCE;
}

bool BoundingBox2D::intersects(const BoundingBox2D& other) const {
    if (!is_valid() || !other.is_valid()) return false;
    
    return !(max.x() < other.min.x() || min.x() > other.max.x() ||
             max.y() < other.min.y() || min.y() > other.max.y());
}

// =============================================================================
// BoundingBox3D Implementation
// =============================================================================

void BoundingBox3D::expand(const Point3D& point) {
    if (!is_valid()) {
        min = max = point;
    } else {
        min = min.cwiseMin(point);
        max = max.cwiseMax(point);
    }
}

void BoundingBox3D::expand(const BoundingBox3D& other) {
    if (!other.is_valid()) return;
    
    if (!is_valid()) {
        *this = other;
    } else {
        min = min.cwiseMin(other.min);
        max = max.cwiseMax(other.max);
    }
}

bool BoundingBox3D::contains(const Point3D& point) const {
    if (!is_valid()) return false;
    
    return point.x() >= min.x() - GEOMETRIC_TOLERANCE &&
           point.x() <= max.x() + GEOMETRIC_TOLERANCE &&
           point.y() >= min.y() - GEOMETRIC_TOLERANCE &&
           point.y() <= max.y() + GEOMETRIC_TOLERANCE &&
           point.z() >= min.z() - GEOMETRIC_TOLERANCE &&
           point.z() <= max.z() + GEOMETRIC_TOLERANCE;
}

bool BoundingBox3D::intersects(const BoundingBox3D& other) const {
    if (!is_valid() || !other.is_valid()) return false;
    
    return !(max.x() < other.min.x() || min.x() > other.max.x() ||
             max.y() < other.min.y() || min.y() > other.max.y() ||
             max.z() < other.min.z() || min.z() > other.max.z());
}

// =============================================================================
// Bezier Curve Implementations
// =============================================================================

Point2D QuadraticBezier2D::evaluate(Precision t) const {
    Precision t_clamped = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    Precision u = 1.0 - t_clamped;
    
    return u * u * p0 + 2.0 * u * t_clamped * p1 + t_clamped * t_clamped * p2;
}

Vector2D QuadraticBezier2D::tangent(Precision t) const {
    Precision t_clamped = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    Precision u = 1.0 - t_clamped;
    
    return 2.0 * u * (p1 - p0) + 2.0 * t_clamped * (p2 - p1);
}

Precision QuadraticBezier2D::curvature(Precision t) const {
    Vector2D first_deriv = tangent(t);
    Vector2D second_deriv = 2.0 * (p2 - 2.0 * p1 + p0);
    
    Precision cross = first_deriv.x() * second_deriv.y() - first_deriv.y() * second_deriv.x();
    Precision speed_cubed = std::pow(first_deriv.norm(), 3.0);
    
    if (is_zero(speed_cubed)) return 0.0;
    return std::abs(cross) / speed_cubed;
}

BoundingBox2D QuadraticBezier2D::bounds() const {
    BoundingBox2D bbox;
    bbox.expand(p0);
    bbox.expand(p2);
    
    // Find extrema by solving derivative = 0
    Vector2D a = p0 - 2.0 * p1 + p2;
    Vector2D b = 2.0 * (p1 - p0);
    
    // For each component, solve at + b = 0
    for (int i = 0; i < 2; ++i) {
        if (!is_zero(a[i])) {
            Precision t = -b[i] / (2.0 * a[i]);
            if (t > 0 && t < 1) {
                Point2D extremum = evaluate(t);
                bbox.expand(extremum);
            }
        }
    }
    
    return bbox;
}

Point2D CubicBezier2D::evaluate(Precision t) const {
    Precision t_clamped = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    Precision u = 1.0 - t_clamped;
    Precision t2 = t_clamped * t_clamped;
    Precision u2 = u * u;
    
    return u2 * u * p0 + 3.0 * u2 * t_clamped * p1 + 
           3.0 * u * t2 * p2 + t2 * t_clamped * p3;
}

Vector2D CubicBezier2D::tangent(Precision t) const {
    Precision t_clamped = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    Precision u = 1.0 - t_clamped;
    
    return 3.0 * u * u * (p1 - p0) + 6.0 * u * t_clamped * (p2 - p1) + 
           3.0 * t_clamped * t_clamped * (p3 - p2);
}

Precision CubicBezier2D::curvature(Precision t) const {
    Vector2D first_deriv = tangent(t);
    
    Precision u = 1.0 - t;
    Vector2D second_deriv = 6.0 * u * (p2 - 2.0 * p1 + p0) + 6.0 * t * (p3 - 2.0 * p2 + p1);
    
    Precision cross = first_deriv.x() * second_deriv.y() - first_deriv.y() * second_deriv.x();
    Precision speed_cubed = std::pow(first_deriv.norm(), 3.0);
    
    if (is_zero(speed_cubed)) return 0.0;
    return std::abs(cross) / speed_cubed;
}

BoundingBox2D CubicBezier2D::bounds() const {
    BoundingBox2D bbox;
    bbox.expand(p0);
    bbox.expand(p3);
    
    // Find extrema by solving derivative = 0
    // Derivative is quadratic: 3(p1-p0)(1-t)² + 6(p2-p1)(1-t)t + 3(p3-p2)t²
    Vector2D a = 3.0 * (p3 - 3.0 * p2 + 3.0 * p1 - p0);
    Vector2D b = 6.0 * (p2 - 2.0 * p1 + p0);
    Vector2D c = 3.0 * (p1 - p0);
    
    // Solve quadratic equation for each component
    for (int i = 0; i < 2; ++i) {
        if (!is_zero(a[i])) {
            Precision discriminant = b[i] * b[i] - 4.0 * a[i] * c[i];
            if (discriminant >= 0) {
                Precision sqrt_disc = std::sqrt(discriminant);
                Precision t1 = (-b[i] + sqrt_disc) / (2.0 * a[i]);
                Precision t2 = (-b[i] - sqrt_disc) / (2.0 * a[i]);
                
                if (t1 > 0 && t1 < 1) {
                    bbox.expand(evaluate(t1));
                }
                if (t2 > 0 && t2 < 1) {
                    bbox.expand(evaluate(t2));
                }
            }
        } else if (!is_zero(b[i])) {
            // Linear case
            Precision t = -c[i] / b[i];
            if (t > 0 && t < 1) {
                bbox.expand(evaluate(t));
            }
        }
    }
    
    return bbox;
}

std::pair<CubicBezier2D, CubicBezier2D> CubicBezier2D::split(Precision t) const {
    Precision t_clamped = clamp(t, static_cast<Precision>(0), static_cast<Precision>(1));
    
    // De Casteljau's algorithm
    Point2D q0 = lerp(p0, p1, t_clamped);
    Point2D q1 = lerp(p1, p2, t_clamped);
    Point2D q2 = lerp(p2, p3, t_clamped);
    
    Point2D r0 = lerp(q0, q1, t_clamped);
    Point2D r1 = lerp(q1, q2, t_clamped);
    
    Point2D s = lerp(r0, r1, t_clamped);
    
    CubicBezier2D left(p0, q0, r0, s);
    CubicBezier2D right(s, r1, q2, p3);
    
    return {left, right};
}

// =============================================================================
// CAD Entity Implementations
// =============================================================================

void CADEntity::remove_associated_entity(EntityID id) {
    // This would be implemented if we had association tracking
    // For now, it's just a placeholder for the interface
}

BoundingBox3D CADArc::calculate_bounds() const {
    BoundingBox3D bbox;
    
    // Add start and end points
    Point3D start = start_point();
    Point3D end = end_point();
    bbox.expand(start);
    bbox.expand(end);
    
    // Check for extrema in the arc
    Precision angles[4] = {0, constants::HALF_PI, constants::PI, 3.0 * constants::HALF_PI};
    
    for (int i = 0; i < 4; ++i) {
        if (angles[i] >= start_angle && angles[i] <= end_angle) {
            Vector3D dir(std::cos(angles[i]), std::sin(angles[i]), 0);
            
            // Rotate direction vector to align with normal
            // For now, assuming arc is in XY plane
            Point3D extremum = center + radius * dir;
            bbox.expand(extremum);
        }
    }
    
    return bbox;
}

Point3D CADArc::start_point() const {
    Vector3D u_axis = Vector3D::UnitX();
    Vector3D v_axis = normal.cross(u_axis).normalized();
    if (v_axis.norm() < 0.5) {
        u_axis = Vector3D::UnitY();
        v_axis = normal.cross(u_axis).normalized();
    }
    u_axis = v_axis.cross(normal).normalized();
    
    return center + radius * (std::cos(start_angle) * u_axis + std::sin(start_angle) * v_axis);
}

Point3D CADArc::end_point() const {
    Vector3D u_axis = Vector3D::UnitX();
    Vector3D v_axis = normal.cross(u_axis).normalized();
    if (v_axis.norm() < 0.5) {
        u_axis = Vector3D::UnitY();
        v_axis = normal.cross(u_axis).normalized();
    }
    u_axis = v_axis.cross(normal).normalized();
    
    return center + radius * (std::cos(end_angle) * u_axis + std::sin(end_angle) * v_axis);
}

// =============================================================================
// Unit Converter Implementation
// =============================================================================

Precision UnitConverter::convert_length(Precision value, LengthUnit from, LengthUnit to) {
    if (from == to) return value;
    
    // Convert to millimeters first
    Precision mm_value;
    switch (from) {
        case LengthUnit::Millimeter: mm_value = value; break;
        case LengthUnit::Centimeter: mm_value = value * 10.0; break;
        case LengthUnit::Meter: mm_value = value * 1000.0; break;
        case LengthUnit::Inch: mm_value = value * 25.4; break;
        case LengthUnit::Foot: mm_value = value * 304.8; break;
        case LengthUnit::Point: mm_value = value * 25.4 / 72.0; break;
        case LengthUnit::Pica: mm_value = value * 25.4 / 6.0; break;
        default: mm_value = value; break;
    }
    
    // Convert from millimeters to target unit
    switch (to) {
        case LengthUnit::Millimeter: return mm_value;
        case LengthUnit::Centimeter: return mm_value / 10.0;
        case LengthUnit::Meter: return mm_value / 1000.0;
        case LengthUnit::Inch: return mm_value / 25.4;
        case LengthUnit::Foot: return mm_value / 304.8;
        case LengthUnit::Point: return mm_value * 72.0 / 25.4;
        case LengthUnit::Pica: return mm_value * 6.0 / 25.4;
        default: return mm_value;
    }
}

Precision UnitConverter::convert_angle(Precision value, AngleUnit from, AngleUnit to) {
    if (from == to) return value;
    
    // Convert to radians first
    Precision rad_value;
    switch (from) {
        case AngleUnit::Radians: rad_value = value; break;
        case AngleUnit::Degrees: rad_value = value * constants::DEG_TO_RAD; break;
        case AngleUnit::Gradians: rad_value = value * constants::GRAD_TO_RAD; break;
        default: rad_value = value; break;
    }
    
    // Convert from radians to target unit
    switch (to) {
        case AngleUnit::Radians: return rad_value;
        case AngleUnit::Degrees: return rad_value * constants::RAD_TO_DEG;
        case AngleUnit::Gradians: return rad_value * constants::RAD_TO_GRAD;
        default: return rad_value;
    }
}

std::string UnitConverter::unit_name(LengthUnit unit) {
    switch (unit) {
        case LengthUnit::Millimeter: return "Millimeter";
        case LengthUnit::Centimeter: return "Centimeter";
        case LengthUnit::Meter: return "Meter";
        case LengthUnit::Inch: return "Inch";
        case LengthUnit::Foot: return "Foot";
        case LengthUnit::Point: return "Point";
        case LengthUnit::Pica: return "Pica";
        default: return "Unknown";
    }
}

std::string UnitConverter::unit_symbol(LengthUnit unit) {
    switch (unit) {
        case LengthUnit::Millimeter: return "mm";
        case LengthUnit::Centimeter: return "cm";
        case LengthUnit::Meter: return "m";
        case LengthUnit::Inch: return "in";
        case LengthUnit::Foot: return "ft";
        case LengthUnit::Point: return "pt";
        case LengthUnit::Pica: return "pc";
        default: return "";
    }
}

// =============================================================================
// NURBS Implementations (Basic)
// =============================================================================

Point3D NURBSCurve::evaluate(Precision u) const {
    // Basic NURBS evaluation - would need proper basis function implementation
    // This is a simplified placeholder
    if (control_points.empty()) return Point3D::Zero();
    
    // For now, simple linear interpolation as placeholder
    size_t n = control_points.size();
    if (n == 1) return control_points[0];
    
    Precision param = clamp(u, static_cast<Precision>(0), static_cast<Precision>(1));
    Precision scaled_param = param * (n - 1);
    size_t index = static_cast<size_t>(scaled_param);
    Precision t = scaled_param - index;
    
    if (index >= n - 1) return control_points.back();
    
    return lerp(control_points[index], control_points[index + 1], t);
}

Vector3D NURBSCurve::tangent(Precision u) const {
    // Placeholder implementation - proper NURBS tangent calculation needed
    if (control_points.size() < 2) return Vector3D::UnitX();
    
    size_t n = control_points.size();
    Precision param = clamp(u, static_cast<Precision>(0), static_cast<Precision>(1));
    Precision scaled_param = param * (n - 1);
    size_t index = static_cast<size_t>(scaled_param);
    
    if (index >= n - 1) index = n - 2;
    
    return (control_points[index + 1] - control_points[index]).normalized();
}

BoundingBox3D NURBSCurve::bounds() const {
    BoundingBox3D bbox;
    for (const auto& point : control_points) {
        bbox.expand(point);
    }
    return bbox;
}

bool NURBSCurve::is_rational() const {
    if (weights.empty()) return false;
    
    // Check if any weight is different from 1.0
    for (Precision weight : weights) {
        if (!approximately_equal(weight, 1.0)) {
            return true;
        }
    }
    return false;
}

Point3D NURBSSurface::evaluate(Precision u, Precision v) const {
    // Placeholder implementation for NURBS surface evaluation
    if (control_points.empty() || control_points[0].empty()) {
        return Point3D::Zero();
    }
    
    // Simple bilinear interpolation as placeholder
    size_t u_count = control_points.size();
    size_t v_count = control_points[0].size();
    
    Precision u_param = clamp(u, static_cast<Precision>(0), static_cast<Precision>(1));
    Precision v_param = clamp(v, static_cast<Precision>(0), static_cast<Precision>(1));
    
    Precision u_scaled = u_param * (u_count - 1);
    Precision v_scaled = v_param * (v_count - 1);
    
    size_t u_index = static_cast<size_t>(u_scaled);
    size_t v_index = static_cast<size_t>(v_scaled);
    
    Precision u_t = u_scaled - u_index;
    Precision v_t = v_scaled - v_index;
    
    if (u_index >= u_count - 1) { u_index = u_count - 2; u_t = 1.0; }
    if (v_index >= v_count - 1) { v_index = v_count - 2; v_t = 1.0; }
    
    Point3D p00 = control_points[u_index][v_index];
    Point3D p01 = control_points[u_index][v_index + 1];
    Point3D p10 = control_points[u_index + 1][v_index];
    Point3D p11 = control_points[u_index + 1][v_index + 1];
    
    Point3D p0 = lerp(p00, p01, v_t);
    Point3D p1 = lerp(p10, p11, v_t);
    
    return lerp(p0, p1, u_t);
}

Vector3D NURBSSurface::normal(Precision u, Precision v) const {
    // Placeholder - would need proper surface derivative calculation
    return Vector3D::UnitZ();
}

BoundingBox3D NURBSSurface::bounds() const {
    BoundingBox3D bbox;
    for (const auto& row : control_points) {
        for (const auto& point : row) {
            bbox.expand(point);
        }
    }
    return bbox;
}

} // namespace qcs::cad