/**
 * @file constraint_types.hpp
 * @brief Type definitions for geometric constraints
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Forward declarations and type definitions for constraint system
 */

#pragma once

#include "cad_types.hpp"

namespace qcs::cad {

// Forward declarations for constraint system
class GeometricConstraint;
class ConstraintVariable;
class ConstraintSystem;
class VariableManager;

// Constraint-specific type aliases
using ConstraintID = size_t;
using ConstraintPtr = std::unique_ptr<GeometricConstraint>;
using ConstraintCollection = std::vector<ConstraintPtr>;

// Variable management types
using VariableCollection = std::vector<std::unique_ptr<ConstraintVariable>>;
using VariableIDSet = std::set<VariableID>;

// Jacobian matrix types
using JacobianMatrix = std::vector<std::vector<Precision>>;
using ResidualVector = std::vector<Precision>;
using SolutionVector = std::vector<Precision>;

// Constraint graph types
using ConstraintGraph = std::unordered_map<ConstraintID, std::vector<ConstraintID>>;
using DependencyGraph = std::unordered_map<VariableID, std::set<ConstraintID>>;

} // namespace qcs::cad