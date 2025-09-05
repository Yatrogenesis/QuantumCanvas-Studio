/**
 * @file test_constraint_solver.cpp
 * @brief Unit tests for constraint solver system
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Comprehensive tests for geometric constraint solving
 */

#include <gtest/gtest.h>
#include "../constraint_solver.hpp"
#include "../cad_common.hpp"
#include <cmath>

namespace qcs::cad::test {

// =============================================================================
// ConstraintVariable Tests
// =============================================================================

TEST(ConstraintSolverTest, ConstraintVariable) {
    ConstraintVariable var(1, "test_var", 5.0);
    
    EXPECT_EQ(var.get_id(), 1u);
    EXPECT_EQ(var.get_name(), "test_var");
    EXPECT_EQ(var.get_value(), 5.0);
    EXPECT_FALSE(var.is_fixed());
    EXPECT_TRUE(var.is_active());
    
    // Test bounds
    var.set_bounds(0.0, 10.0);
    EXPECT_TRUE(var.is_in_bounds(5.0));
    EXPECT_FALSE(var.is_in_bounds(-1.0));
    EXPECT_FALSE(var.is_in_bounds(11.0));
    
    // Test clamping
    var.set_value(15.0);
    EXPECT_EQ(var.get_value(), 10.0); // Should be clamped
    
    var.set_value(-5.0);
    EXPECT_EQ(var.get_value(), 0.0); // Should be clamped
    
    // Test tolerance
    EXPECT_TRUE(var.is_within_tolerance(5.0 + GEOMETRIC_TOLERANCE * 0.5));
    EXPECT_FALSE(var.is_within_tolerance(5.0 + GEOMETRIC_TOLERANCE * 2.0));
}

// =============================================================================
// VariableManager Tests
// =============================================================================

TEST(ConstraintSolverTest, VariableManager) {
    VariableManager manager;
    
    // Create variables
    VariableID id1 = manager.create_variable("x", 1.0);
    VariableID id2 = manager.create_variable("y", 2.0);
    
    EXPECT_EQ(manager.get_variable_count(), 2u);
    EXPECT_EQ(manager.get_active_variable_count(), 2u);
    EXPECT_EQ(manager.get_free_variable_count(), 2u);
    
    // Test variable access
    ConstraintVariable* var1 = manager.get_variable(id1);
    ASSERT_NE(var1, nullptr);
    EXPECT_EQ(var1->get_name(), "x");
    EXPECT_EQ(var1->get_value(), 1.0);
    
    ConstraintVariable* var_by_name = manager.get_variable_by_name("y");
    ASSERT_NE(var_by_name, nullptr);
    EXPECT_EQ(var_by_name->get_value(), 2.0);
    
    // Fix a variable
    var1->set_fixed(true);
    EXPECT_EQ(manager.get_free_variable_count(), 1u);
    
    // Test batch operations
    std::vector<VariableID> ids = {id1, id2};
    std::vector<Precision> values = {3.0, 4.0};
    
    manager.set_variable_values(ids, values);
    std::vector<Precision> retrieved = manager.get_variable_values(ids);
    
    EXPECT_EQ(retrieved[0], 3.0);
    EXPECT_EQ(retrieved[1], 4.0);
    
    // Test cleanup
    manager.remove_variable(id1);
    EXPECT_EQ(manager.get_variable_count(), 1u);
    EXPECT_EQ(manager.get_variable(id1), nullptr);
}

// =============================================================================
// Distance Constraint Tests
// =============================================================================

TEST(ConstraintSolverTest, DistanceConstraint) {
    VariableManager manager;
    
    // Create variables for two points
    VariableID p1x = manager.create_variable("p1x", 0.0);
    VariableID p1y = manager.create_variable("p1y", 0.0);
    VariableID p2x = manager.create_variable("p2x", 3.0);
    VariableID p2y = manager.create_variable("p2y", 4.0);
    
    // Create distance constraint (target distance = 5.0)
    DistanceConstraint constraint(1, p1x, p1y, p2x, p2y, 5.0);
    
    // Current distance is 5.0, so error should be 0
    Precision error = constraint.evaluate_error(manager);
    EXPECT_NEAR(error, 0.0, GEOMETRIC_TOLERANCE);
    EXPECT_TRUE(constraint.is_satisfied(manager));
    
    // Change point position
    manager.get_variable(p2x)->set_value(6.0);
    manager.get_variable(p2y)->set_value(8.0);
    
    // New distance is 10.0, so error should be 5.0
    error = constraint.evaluate_error(manager);
    EXPECT_NEAR(error, 5.0, GEOMETRIC_TOLERANCE);
    EXPECT_FALSE(constraint.is_satisfied(manager));
    
    // Test gradient
    std::vector<Precision> gradient = constraint.evaluate_gradient(manager);
    EXPECT_EQ(gradient.size(), 4u);
    
    // Gradient should point in direction that reduces error
    Precision grad_norm = 0.0;
    for (Precision g : gradient) {
        grad_norm += g * g;
    }
    EXPECT_GT(grad_norm, 0.0);
    
    // Test constraint description
    std::string desc = constraint.get_description();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("5.000"), std::string::npos);
}

// =============================================================================
// Horizontal/Vertical Constraint Tests
// =============================================================================

TEST(ConstraintSolverTest, HorizontalConstraint) {
    VariableManager manager;
    
    VariableID p1y = manager.create_variable("p1y", 2.0);
    VariableID p2y = manager.create_variable("p2y", 2.0);
    
    HorizontalConstraint constraint(1, p1y, p2y);
    
    // Points at same Y level - should be satisfied
    EXPECT_NEAR(constraint.evaluate_error(manager), 0.0, GEOMETRIC_TOLERANCE);
    EXPECT_TRUE(constraint.is_satisfied(manager));
    EXPECT_TRUE(constraint.is_linear());
    
    // Move one point
    manager.get_variable(p2y)->set_value(3.0);
    EXPECT_NEAR(constraint.evaluate_error(manager), 1.0, GEOMETRIC_TOLERANCE);
    EXPECT_FALSE(constraint.is_satisfied(manager));
    
    // Test gradient
    std::vector<Precision> gradient = constraint.evaluate_gradient(manager);
    EXPECT_EQ(gradient.size(), 2u);
    EXPECT_NEAR(gradient[0], -1.0, GEOMETRIC_TOLERANCE);
    EXPECT_NEAR(gradient[1], 1.0, GEOMETRIC_TOLERANCE);
}

TEST(ConstraintSolverTest, VerticalConstraint) {
    VariableManager manager;
    
    VariableID p1x = manager.create_variable("p1x", 1.0);
    VariableID p2x = manager.create_variable("p2x", 1.0);
    
    VerticalConstraint constraint(1, p1x, p2x);
    
    // Points at same X level - should be satisfied
    EXPECT_NEAR(constraint.evaluate_error(manager), 0.0, GEOMETRIC_TOLERANCE);
    EXPECT_TRUE(constraint.is_satisfied(manager));
    EXPECT_TRUE(constraint.is_linear());
    
    // Test degree of freedom reduction
    EXPECT_EQ(constraint.get_degree_of_freedom_reduction(), 1);
}

// =============================================================================
// Parallel/Perpendicular Constraint Tests
// =============================================================================

TEST(ConstraintSolverTest, ParallelConstraint) {
    VariableManager manager;
    
    // Create two horizontal lines
    VariableID l1sx = manager.create_variable("l1sx", 0.0);
    VariableID l1sy = manager.create_variable("l1sy", 0.0);
    VariableID l1ex = manager.create_variable("l1ex", 1.0);
    VariableID l1ey = manager.create_variable("l1ey", 0.0);
    
    VariableID l2sx = manager.create_variable("l2sx", 0.0);
    VariableID l2sy = manager.create_variable("l2sy", 1.0);
    VariableID l2ex = manager.create_variable("l2ex", 1.0);
    VariableID l2ey = manager.create_variable("l2ey", 1.0);
    
    ParallelConstraint constraint(1, l1sx, l1sy, l1ex, l1ey, l2sx, l2sy, l2ex, l2ey);
    
    // Both lines are horizontal - should be parallel
    EXPECT_NEAR(constraint.evaluate_error(manager), 0.0, GEOMETRIC_TOLERANCE);
    EXPECT_TRUE(constraint.is_satisfied(manager));
    
    // Rotate second line
    manager.get_variable(l2ey)->set_value(2.0);
    
    Precision error = constraint.evaluate_error(manager);
    EXPECT_GT(std::abs(error), GEOMETRIC_TOLERANCE);
    EXPECT_FALSE(constraint.is_satisfied(manager));
    
    // Test gradient
    std::vector<Precision> gradient = constraint.evaluate_gradient(manager);
    EXPECT_EQ(gradient.size(), 8u);
}

TEST(ConstraintSolverTest, PerpendicularConstraint) {
    VariableManager manager;
    
    // Create horizontal and vertical lines
    VariableID l1sx = manager.create_variable("l1sx", 0.0);
    VariableID l1sy = manager.create_variable("l1sy", 0.0);
    VariableID l1ex = manager.create_variable("l1ex", 1.0);
    VariableID l1ey = manager.create_variable("l1ey", 0.0);
    
    VariableID l2sx = manager.create_variable("l2sx", 0.0);
    VariableID l2sy = manager.create_variable("l2sy", 0.0);
    VariableID l2ex = manager.create_variable("l2ex", 0.0);
    VariableID l2ey = manager.create_variable("l2ey", 1.0);
    
    PerpendicularConstraint constraint(1, l1sx, l1sy, l1ex, l1ey, l2sx, l2sy, l2ex, l2ey);
    
    // Horizontal and vertical lines - should be perpendicular
    EXPECT_NEAR(constraint.evaluate_error(manager), 0.0, GEOMETRIC_TOLERANCE);
    EXPECT_TRUE(constraint.is_satisfied(manager));
    
    // Make lines parallel
    manager.get_variable(l2ex)->set_value(1.0);
    manager.get_variable(l2ey)->set_value(0.0);
    
    Precision error = constraint.evaluate_error(manager);
    EXPECT_GT(std::abs(error), GEOMETRIC_TOLERANCE);
    EXPECT_FALSE(constraint.is_satisfied(manager));
}

// =============================================================================
// Coincident Constraint Tests
// =============================================================================

TEST(ConstraintSolverTest, CoincidentConstraint) {
    VariableManager manager;
    
    VariableID p1x = manager.create_variable("p1x", 1.0);
    VariableID p1y = manager.create_variable("p1y", 2.0);
    VariableID p2x = manager.create_variable("p2x", 1.0);
    VariableID p2y = manager.create_variable("p2y", 2.0);
    
    CoincidentConstraint constraint(1, p1x, p1y, p2x, p2y);
    
    // Points at same location - should be satisfied
    EXPECT_NEAR(constraint.evaluate_error(manager), 0.0, GEOMETRIC_TOLERANCE);
    EXPECT_TRUE(constraint.is_satisfied(manager));
    EXPECT_TRUE(constraint.is_linear());
    EXPECT_EQ(constraint.get_degree_of_freedom_reduction(), 2);
    
    // Move one point
    manager.get_variable(p2x)->set_value(4.0);
    manager.get_variable(p2y)->set_value(6.0);
    
    // Distance is 5.0
    Precision error = constraint.evaluate_error(manager);
    EXPECT_NEAR(error, 5.0, GEOMETRIC_TOLERANCE);
    EXPECT_FALSE(constraint.is_satisfied(manager));
    
    // Test gradient points toward coincidence
    std::vector<Precision> gradient = constraint.evaluate_gradient(manager);
    EXPECT_EQ(gradient.size(), 4u);
    
    // Gradient should be unit vector in direction of closure
    EXPECT_NEAR(gradient[0], -0.6, GEOMETRIC_TOLERANCE); // -3/5
    EXPECT_NEAR(gradient[1], -0.8, GEOMETRIC_TOLERANCE); // -4/5
    EXPECT_NEAR(gradient[2], 0.6, GEOMETRIC_TOLERANCE);  // 3/5
    EXPECT_NEAR(gradient[3], 0.8, GEOMETRIC_TOLERANCE);  // 4/5
}

// =============================================================================
// Constraint State Management Tests
// =============================================================================

TEST(ConstraintSolverTest, ConstraintState) {
    VariableManager manager;
    
    VariableID p1x = manager.create_variable("p1x", 0.0);
    VariableID p1y = manager.create_variable("p1y", 0.0);
    VariableID p2x = manager.create_variable("p2x", 3.0);
    VariableID p2y = manager.create_variable("p2y", 4.0);
    
    DistanceConstraint constraint(1, p1x, p1y, p2x, p2y, 5.0);
    
    // Initial state should be inactive
    EXPECT_EQ(constraint.get_state(), ConstraintState::Inactive);
    
    // Update state based on satisfaction
    constraint.update_state(manager);
    EXPECT_EQ(constraint.get_state(), ConstraintState::Satisfied);
    
    // Change target distance to create violation
    constraint.set_target_distance(10.0);
    constraint.update_state(manager);
    EXPECT_EQ(constraint.get_state(), ConstraintState::Violated);
    
    // Test priority and weight
    constraint.set_priority(ConstraintPriority::High);
    EXPECT_EQ(constraint.get_priority(), ConstraintPriority::High);
    
    constraint.set_weight(2.0);
    constraint.set_tolerance(1e-6);
    
    // Test active/inactive toggle
    constraint.set_active(false);
    EXPECT_FALSE(constraint.is_active());
}

// =============================================================================
// Constraint Clone Tests
// =============================================================================

TEST(ConstraintSolverTest, ConstraintCloning) {
    VariableManager manager;
    
    VariableID p1x = manager.create_variable("p1x", 0.0);
    VariableID p1y = manager.create_variable("p1y", 0.0);
    
    HorizontalConstraint original(1, p1x, p1y);
    original.set_priority(ConstraintPriority::High);
    original.set_weight(2.0);
    
    auto clone = original.clone();
    EXPECT_NE(clone.get(), &original);
    EXPECT_EQ(clone->get_id(), original.get_id());
    EXPECT_EQ(clone->get_name(), original.get_name());
    EXPECT_EQ(clone->get_priority(), original.get_priority());
    
    // Test that clone works independently
    clone->set_priority(ConstraintPriority::Low);
    EXPECT_EQ(original.get_priority(), ConstraintPriority::High);
    EXPECT_EQ(clone->get_priority(), ConstraintPriority::Low);
}

// =============================================================================
// Error and Performance Tests
// =============================================================================

TEST(ConstraintSolverTest, ErrorHandling) {
    VariableManager manager;
    
    // Test constraint with invalid variable IDs
    DistanceConstraint constraint(1, 999, 998, 997, 996, 1.0);
    
    // Should handle gracefully
    Precision error = constraint.evaluate_error(manager);
    EXPECT_EQ(error, 0.0);
    
    std::vector<Precision> gradient = constraint.evaluate_gradient(manager);
    EXPECT_EQ(gradient.size(), 4u);
    for (Precision g : gradient) {
        EXPECT_EQ(g, 0.0);
    }
}

TEST(ConstraintSolverTest, PerformanceBaseline) {
    VariableManager manager;
    
    // Create many variables
    std::vector<VariableID> ids;
    for (int i = 0; i < 1000; ++i) {
        ids.push_back(manager.create_variable("var" + std::to_string(i), i * 0.1));
    }
    
    EXPECT_EQ(manager.get_variable_count(), 1000u);
    
    // Batch operations should be efficient
    std::vector<Precision> values(1000, 1.0);
    manager.set_variable_values(ids, values);
    
    std::vector<Precision> retrieved = manager.get_variable_values(ids);
    EXPECT_EQ(retrieved.size(), 1000u);
    
    for (Precision val : retrieved) {
        EXPECT_EQ(val, 1.0);
    }
}

} // namespace qcs::cad::test