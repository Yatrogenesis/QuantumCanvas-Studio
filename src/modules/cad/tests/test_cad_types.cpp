/**
 * @file test_cad_types.cpp
 * @brief Unit tests for CAD types and geometric primitives
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Comprehensive tests for precision geometry and CAD entities
 */

#include <gtest/gtest.h>
#include "../cad_types.hpp"
#include "../cad_common.hpp"
#include <cmath>

namespace qcs::cad::test {

// =============================================================================
// Tolerance and Precision Tests
// =============================================================================

TEST(CADTypesTest, ToleranceContext) {
    ToleranceContext tolerance;
    
    // Test point equality
    Point2D p1(1.0, 2.0);
    Point2D p2(1.0 + GEOMETRIC_TOLERANCE * 0.5, 2.0);
    Point2D p3(1.0 + GEOMETRIC_TOLERANCE * 2.0, 2.0);
    
    EXPECT_TRUE(tolerance.are_points_equal(p1, p2));
    EXPECT_FALSE(tolerance.are_points_equal(p1, p3));
    
    // Test vector parallelism
    Vector2D v1(1.0, 0.0);
    Vector2D v2(1.0, ANGULAR_TOLERANCE * 0.5);
    Vector2D v3(0.0, 1.0);
    
    EXPECT_TRUE(tolerance.are_vectors_parallel(v1, v2));
    EXPECT_FALSE(tolerance.are_vectors_parallel(v1, v3));
    
    // Test zero checks
    EXPECT_TRUE(tolerance.is_zero_length(GEOMETRIC_TOLERANCE * 0.5));
    EXPECT_FALSE(tolerance.is_zero_length(GEOMETRIC_TOLERANCE * 2.0));
}

// =============================================================================
// BoundingBox Tests
// =============================================================================

TEST(CADTypesTest, BoundingBox2D) {
    BoundingBox2D bbox;
    
    // Initially invalid
    EXPECT_FALSE(bbox.is_valid());
    
    // Add first point
    Point2D p1(1.0, 2.0);
    bbox.expand(p1);
    EXPECT_TRUE(bbox.is_valid());
    EXPECT_EQ(bbox.min, p1);
    EXPECT_EQ(bbox.max, p1);
    
    // Add second point
    Point2D p2(3.0, 1.0);
    bbox.expand(p2);
    EXPECT_EQ(bbox.min, Point2D(1.0, 1.0));
    EXPECT_EQ(bbox.max, Point2D(3.0, 2.0));
    
    // Test center and size
    EXPECT_EQ(bbox.center(), Point2D(2.0, 1.5));
    EXPECT_EQ(bbox.size(), Vector2D(2.0, 1.0));
    EXPECT_NEAR(bbox.area(), 2.0, GEOMETRIC_TOLERANCE);
    
    // Test containment
    EXPECT_TRUE(bbox.contains(Point2D(2.0, 1.5)));
    EXPECT_FALSE(bbox.contains(Point2D(4.0, 1.5)));
    
    // Test intersection
    BoundingBox2D bbox2;
    bbox2.expand(Point2D(2.5, 0.5));
    bbox2.expand(Point2D(4.0, 2.5));
    
    EXPECT_TRUE(bbox.intersects(bbox2));
    
    BoundingBox2D bbox3;
    bbox3.expand(Point2D(5.0, 5.0));
    bbox3.expand(Point2D(6.0, 6.0));
    
    EXPECT_FALSE(bbox.intersects(bbox3));
}

TEST(CADTypesTest, BoundingBox3D) {
    BoundingBox3D bbox;
    
    Point3D p1(1.0, 2.0, 3.0);
    Point3D p2(-1.0, 4.0, 1.0);
    
    bbox.expand(p1);
    bbox.expand(p2);
    
    EXPECT_EQ(bbox.min, Point3D(-1.0, 2.0, 1.0));
    EXPECT_EQ(bbox.max, Point3D(1.0, 4.0, 3.0));
    
    Vector3D size = bbox.size();
    EXPECT_NEAR(size.x(), 2.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(size.y(), 2.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(size.z(), 2.0, GEOMETRIC_TOLERANCE);
    
    EXPECT_NEAR(bbox.volume(), 8.0, GEOMETRIC_TOLERANCE);
    EXPECT_TRUE(bbox.contains(Point3D(0.0, 3.0, 2.0)));
}

// =============================================================================
// Bezier Curve Tests
// =============================================================================

TEST(CADTypesTest, QuadraticBezier) {
    Point2D p0(0.0, 0.0);
    Point2D p1(1.0, 2.0);
    Point2D p2(2.0, 0.0);
    
    QuadraticBezier2D curve(p0, p1, p2);
    
    // Test evaluation at endpoints
    EXPECT_EQ(curve.evaluate(0.0), p0);
    EXPECT_EQ(curve.evaluate(1.0), p2);
    
    // Test evaluation at midpoint
    Point2D mid = curve.evaluate(0.5);
    EXPECT_NEAR(mid.x(), 1.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(mid.y(), 1.0, GEOMETRIC_TOLERANCE);
    
    // Test tangent vectors
    Vector2D tangent_start = curve.tangent(0.0);
    Vector2D tangent_end = curve.tangent(1.0);
    
    EXPECT_GT(tangent_start.norm(), 0.0);
    EXPECT_GT(tangent_end.norm(), 0.0);
    
    // Test bounding box
    BoundingBox2D bounds = curve.bounds();
    EXPECT_TRUE(bounds.contains(p0));
    EXPECT_TRUE(bounds.contains(p2));
}

TEST(CADTypesTest, CubicBezier) {
    Point2D p0(0.0, 0.0);
    Point2D p1(1.0, 1.0);
    Point2D p2(2.0, 1.0);
    Point2D p3(3.0, 0.0);
    
    CubicBezier2D curve(p0, p1, p2, p3);
    
    // Test evaluation at endpoints
    EXPECT_EQ(curve.evaluate(0.0), p0);
    EXPECT_EQ(curve.evaluate(1.0), p3);
    
    // Test curve split
    auto [left, right] = curve.split(0.5);
    
    EXPECT_EQ(left.p0, p0);
    EXPECT_EQ(right.p3, p3);
    EXPECT_EQ(left.p3, right.p0); // Split point should match
    
    // Test bounding box contains control points
    BoundingBox2D bounds = curve.bounds();
    EXPECT_TRUE(bounds.contains(p0));
    EXPECT_TRUE(bounds.contains(p3));
}

// =============================================================================
// CAD Entity Tests
// =============================================================================

TEST(CADTypesTest, CADPoint) {
    Point3D position(1.0, 2.0, 3.0);
    CADPoint cad_point(position, 123);
    
    EXPECT_EQ(cad_point.id, 123u);
    EXPECT_EQ(cad_point.position, position);
    
    BoundingBox3D bounds = cad_point.calculate_bounds();
    EXPECT_EQ(bounds.min, position);
    EXPECT_EQ(bounds.max, position);
    
    // Test clone
    auto clone = cad_point.clone();
    EXPECT_NE(clone.get(), &cad_point);
    EXPECT_EQ(static_cast<CADPoint*>(clone.get())->position, position);
}

TEST(CADTypesTest, CADLine) {
    Point3D start(0.0, 0.0, 0.0);
    Point3D end(3.0, 4.0, 0.0);
    CADLine line(start, end, 456);
    
    EXPECT_EQ(line.start, start);
    EXPECT_EQ(line.end, end);
    EXPECT_NEAR(line.length(), 5.0, GEOMETRIC_TOLERANCE);
    
    Vector3D direction = line.direction();
    EXPECT_NEAR(direction.x(), 0.6, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(direction.y(), 0.8, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(direction.z(), 0.0, GEOMETRIC_TOLERANCE);
    
    BoundingBox3D bounds = line.calculate_bounds();
    EXPECT_TRUE(bounds.contains(start));
    EXPECT_TRUE(bounds.contains(end));
}

TEST(CADTypesTest, CADArc) {
    Point3D center(0.0, 0.0, 0.0);
    Vector3D normal(0.0, 0.0, 1.0);
    Precision radius = 2.0;
    Precision start_angle = 0.0;
    Precision end_angle = constants::HALF_PI;
    
    CADArc arc(center, normal, radius, start_angle, end_angle, 789);
    
    EXPECT_EQ(arc.center, center);
    EXPECT_EQ(arc.normal, normal);
    EXPECT_EQ(arc.radius, radius);
    EXPECT_NEAR(arc.arc_length(), constants::PI, GEOMETRIC_TOLERANCE);
    
    Point3D start_point = arc.start_point();
    Point3D end_point = arc.end_point();
    
    EXPECT_NEAR(start_point.x(), 2.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(start_point.y(), 0.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(end_point.x(), 0.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(end_point.y(), 2.0, GEOMETRIC_TOLERANCE);
}

// =============================================================================
// Unit Converter Tests
// =============================================================================

TEST(CADTypesTest, UnitConverter) {
    // Test length conversions
    EXPECT_NEAR(UnitConverter::convert_length(1.0, LengthUnit::Meter, LengthUnit::Millimeter), 
                1000.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(UnitConverter::convert_length(25.4, LengthUnit::Millimeter, LengthUnit::Inch), 
                1.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(UnitConverter::convert_length(72.0, LengthUnit::Point, LengthUnit::Inch), 
                1.0, GEOMETRIC_TOLERANCE);
    
    // Test angle conversions
    EXPECT_NEAR(UnitConverter::convert_angle(constants::PI, AngleUnit::Radians, AngleUnit::Degrees), 
                180.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(UnitConverter::convert_angle(90.0, AngleUnit::Degrees, AngleUnit::Radians), 
                constants::HALF_PI, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(UnitConverter::convert_angle(100.0, AngleUnit::Gradians, AngleUnit::Degrees), 
                90.0, GEOMETRIC_TOLERANCE);
    
    // Test unit names
    EXPECT_EQ(UnitConverter::unit_name(LengthUnit::Millimeter), "Millimeter");
    EXPECT_EQ(UnitConverter::unit_symbol(LengthUnit::Millimeter), "mm");
}

// =============================================================================
// NURBS Tests (Basic)
// =============================================================================

TEST(CADTypesTest, NURBSCurve) {
    NURBSCurve curve(3); // Cubic
    
    curve.control_points = {
        Point3D(0.0, 0.0, 0.0),
        Point3D(1.0, 1.0, 0.0),
        Point3D(2.0, 1.0, 0.0),
        Point3D(3.0, 0.0, 0.0)
    };
    
    // Test evaluation (simplified linear interpolation)
    Point3D start = curve.evaluate(0.0);
    Point3D end = curve.evaluate(1.0);
    
    EXPECT_EQ(start, curve.control_points.front());
    EXPECT_EQ(end, curve.control_points.back());
    
    // Test bounds contain all control points
    BoundingBox3D bounds = curve.bounds();
    for (const auto& cp : curve.control_points) {
        EXPECT_TRUE(bounds.contains(cp));
    }
    
    // Test rational curve detection
    EXPECT_FALSE(curve.is_rational()); // No weights set
    
    curve.weights = {1.0, 2.0, 1.0, 1.0};
    EXPECT_TRUE(curve.is_rational()); // Has non-uniform weights
}

} // namespace qcs::cad::test