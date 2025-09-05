/**
 * @file constraint_solver.cpp
 * @brief Implementation of geometric constraint solver for parametric CAD modeling
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Advanced constraint solving using numerical methods for parametric design
 */

#include "constraint_solver.hpp"
#include "cad_common.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace qcs::cad {

// =============================================================================
// ConstraintVariable Implementation
// =============================================================================

ConstraintVariable::ConstraintVariable(VariableID id, const std::string& name, Precision initial_value)
    : id_(id)
    , name_(name)
    , value_(initial_value)
    , lower_bound_(-std::numeric_limits<Precision>::infinity())
    , upper_bound_(std::numeric_limits<Precision>::infinity())
    , is_fixed_(false)
    , is_active_(true)
    , tolerance_(GEOMETRIC_TOLERANCE)
    , initial_value_(initial_value)
    , delta_(0.0) {
}

void ConstraintVariable::set_value(Precision value) {
    value_ = clamp_to_bounds(value);
}

void ConstraintVariable::set_bounds(Precision lower, Precision upper) {
    lower_bound_ = lower;
    upper_bound_ = std::max(upper, lower);
    value_ = clamp_to_bounds(value_);
}

bool ConstraintVariable::is_in_bounds(Precision value) const {
    return value >= lower_bound_ - tolerance_ && value <= upper_bound_ + tolerance_;
}

bool ConstraintVariable::is_within_tolerance(Precision value) const {
    return std::abs(value - value_) <= tolerance_;
}

Precision ConstraintVariable::clamp_to_bounds(Precision value) const {
    return clamp(value, lower_bound_, upper_bound_);
}

// =============================================================================
// VariableManager Implementation
// =============================================================================

VariableManager::VariableManager() : next_id_(1) {}

VariableID VariableManager::create_variable(const std::string& name, Precision initial_value) {
    VariableID id = next_id_++;
    auto variable = std::make_unique<ConstraintVariable>(id, name, initial_value);
    
    name_to_id_[name] = id;
    variables_[id] = std::move(variable);
    
    return id;
}

void VariableManager::remove_variable(VariableID id) {
    auto it = variables_.find(id);
    if (it != variables_.end()) {
        name_to_id_.erase(it->second->get_name());
        variables_.erase(it);
    }
}

void VariableManager::clear_variables() {
    variables_.clear();
    name_to_id_.clear();
    next_id_ = 1;
}

ConstraintVariable* VariableManager::get_variable(VariableID id) {
    auto it = variables_.find(id);
    return it != variables_.end() ? it->second.get() : nullptr;
}

const ConstraintVariable* VariableManager::get_variable(VariableID id) const {
    auto it = variables_.find(id);
    return it != variables_.end() ? it->second.get() : nullptr;
}

ConstraintVariable* VariableManager::get_variable_by_name(const std::string& name) {
    auto it = name_to_id_.find(name);
    return it != name_to_id_.end() ? get_variable(it->second) : nullptr;
}

const ConstraintVariable* VariableManager::get_variable_by_name(const std::string& name) const {
    auto it = name_to_id_.find(name);
    return it != name_to_id_.end() ? get_variable(it->second) : nullptr;
}

size_t VariableManager::get_active_variable_count() const {
    return std::count_if(variables_.begin(), variables_.end(),
        [](const auto& pair) { return pair.second->is_active(); });
}

size_t VariableManager::get_free_variable_count() const {
    return std::count_if(variables_.begin(), variables_.end(),
        [](const auto& pair) { return pair.second->is_active() && !pair.second->is_fixed(); });
}

std::vector<VariableID> VariableManager::get_all_variable_ids() const {
    std::vector<VariableID> ids;
    ids.reserve(variables_.size());
    
    for (const auto& pair : variables_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

std::vector<VariableID> VariableManager::get_active_variable_ids() const {
    std::vector<VariableID> ids;
    
    for (const auto& pair : variables_) {
        if (pair.second->is_active()) {
            ids.push_back(pair.first);
        }
    }
    
    return ids;
}

void VariableManager::set_all_initial_values() {
    for (auto& pair : variables_) {
        pair.second->set_initial_value(pair.second->get_value());
    }
}

void VariableManager::restore_initial_values() {
    for (auto& pair : variables_) {
        pair.second->set_value(pair.second->get_initial_value());
    }
}

std::vector<Precision> VariableManager::get_variable_values(const std::vector<VariableID>& ids) const {
    std::vector<Precision> values;
    values.reserve(ids.size());
    
    for (VariableID id : ids) {
        const auto* var = get_variable(id);
        values.push_back(var ? var->get_value() : 0.0);
    }
    
    return values;
}

void VariableManager::set_variable_values(const std::vector<VariableID>& ids, const std::vector<Precision>& values) {
    size_t count = std::min(ids.size(), values.size());
    
    for (size_t i = 0; i < count; ++i) {
        auto* var = get_variable(ids[i]);
        if (var) {
            var->set_value(values[i]);
        }
    }
}

// =============================================================================
// GeometricConstraint Implementation
// =============================================================================

GeometricConstraint::GeometricConstraint(size_t id, const std::string& name, ConstraintCategory category)
    : id_(id)
    , name_(name)
    , category_(category)
    , priority_(ConstraintPriority::Medium)
    , state_(ConstraintState::Inactive)
    , tolerance_(GEOMETRIC_TOLERANCE)
    , weight_(1.0)
    , is_active_(true)
    , current_error_(0.0)
    , max_error_(0.0) {
}

bool GeometricConstraint::is_satisfied(const VariableManager& vars) const {
    return std::abs(evaluate_error(vars)) <= tolerance_;
}

void GeometricConstraint::update_state(const VariableManager& vars) {
    current_error_ = evaluate_error(vars);
    max_error_ = std::max(max_error_, std::abs(current_error_));
    
    if (is_satisfied(vars)) {
        state_ = ConstraintState::Satisfied;
    } else {
        state_ = ConstraintState::Violated;
    }
}

// =============================================================================
// Specific Constraint Implementations
// =============================================================================

DistanceConstraint::DistanceConstraint(size_t id, VariableID p1x, VariableID p1y, VariableID p2x, VariableID p2y,
                                     Precision distance)
    : GeometricConstraint(id, "Distance", ConstraintCategory::Dimensional)
    , point1_x_(p1x), point1_y_(p1y)
    , point2_x_(p2x), point2_y_(p2y)
    , target_distance_(distance) {
    
    variables_ = {p1x, p1y, p2x, p2y};
}

Precision DistanceConstraint::evaluate_error(const VariableManager& vars) const {
    const auto* p1x = vars.get_variable(point1_x_);
    const auto* p1y = vars.get_variable(point1_y_);
    const auto* p2x = vars.get_variable(point2_x_);
    const auto* p2y = vars.get_variable(point2_y_);
    
    if (!p1x || !p1y || !p2x || !p2y) return 0.0;
    
    Precision dx = p2x->get_value() - p1x->get_value();
    Precision dy = p2y->get_value() - p1y->get_value();
    Precision actual_distance = std::sqrt(dx * dx + dy * dy);
    
    return actual_distance - target_distance_;
}

std::vector<Precision> DistanceConstraint::evaluate_gradient(const VariableManager& vars) const {
    const auto* p1x = vars.get_variable(point1_x_);
    const auto* p1y = vars.get_variable(point1_y_);
    const auto* p2x = vars.get_variable(point2_x_);
    const auto* p2y = vars.get_variable(point2_y_);
    
    std::vector<Precision> gradient(4, 0.0);
    
    if (!p1x || !p1y || !p2x || !p2y) return gradient;
    
    Precision dx = p2x->get_value() - p1x->get_value();
    Precision dy = p2y->get_value() - p1y->get_value();
    Precision distance = std::sqrt(dx * dx + dy * dy);
    
    if (is_zero(distance)) return gradient;
    
    // Partial derivatives: d/dx(sqrt((x2-x1)² + (y2-y1)²))
    gradient[0] = -dx / distance;  // d/dp1x
    gradient[1] = -dy / distance;  // d/dp1y
    gradient[2] = dx / distance;   // d/dp2x
    gradient[3] = dy / distance;   // d/dp2y
    
    return gradient;
}

std::unique_ptr<GeometricConstraint> DistanceConstraint::clone() const {
    return std::make_unique<DistanceConstraint>(*this);
}

std::string DistanceConstraint::get_description() const {
    std::ostringstream oss;
    oss << "Distance constraint: " << std::fixed << std::setprecision(3) << target_distance_;
    return oss.str();
}

HorizontalConstraint::HorizontalConstraint(size_t id, VariableID p1y, VariableID p2y)
    : GeometricConstraint(id, "Horizontal", ConstraintCategory::Geometric)
    , point1_y_(p1y), point2_y_(p2y) {
    
    variables_ = {p1y, p2y};
}

Precision HorizontalConstraint::evaluate_error(const VariableManager& vars) const {
    const auto* p1y = vars.get_variable(point1_y_);
    const auto* p2y = vars.get_variable(point2_y_);
    
    if (!p1y || !p2y) return 0.0;
    
    return p2y->get_value() - p1y->get_value();
}

std::vector<Precision> HorizontalConstraint::evaluate_gradient(const VariableManager& vars) const {
    return {-1.0, 1.0}; // d/dp1y = -1, d/dp2y = 1
}

std::unique_ptr<GeometricConstraint> HorizontalConstraint::clone() const {
    return std::make_unique<HorizontalConstraint>(*this);
}

std::string HorizontalConstraint::get_description() const {
    return "Horizontal constraint: points have same Y coordinate";
}

VerticalConstraint::VerticalConstraint(size_t id, VariableID p1x, VariableID p2x)
    : GeometricConstraint(id, "Vertical", ConstraintCategory::Geometric)
    , point1_x_(p1x), point2_x_(p2x) {
    
    variables_ = {p1x, p2x};
}

Precision VerticalConstraint::evaluate_error(const VariableManager& vars) const {
    const auto* p1x = vars.get_variable(point1_x_);
    const auto* p2x = vars.get_variable(point2_x_);
    
    if (!p1x || !p2x) return 0.0;
    
    return p2x->get_value() - p1x->get_value();
}

std::vector<Precision> VerticalConstraint::evaluate_gradient(const VariableManager& vars) const {
    return {-1.0, 1.0}; // d/dp1x = -1, d/dp2x = 1
}

std::unique_ptr<GeometricConstraint> VerticalConstraint::clone() const {
    return std::make_unique<VerticalConstraint>(*this);
}

std::string VerticalConstraint::get_description() const {
    return "Vertical constraint: points have same X coordinate";
}

ParallelConstraint::ParallelConstraint(size_t id, VariableID l1sx, VariableID l1sy, VariableID l1ex, VariableID l1ey,
                                     VariableID l2sx, VariableID l2sy, VariableID l2ex, VariableID l2ey)
    : GeometricConstraint(id, "Parallel", ConstraintCategory::Geometric)
    , line1_start_x_(l1sx), line1_start_y_(l1sy), line1_end_x_(l1ex), line1_end_y_(l1ey)
    , line2_start_x_(l2sx), line2_start_y_(l2sy), line2_end_x_(l2ex), line2_end_y_(l2ey) {
    
    variables_ = {l1sx, l1sy, l1ex, l1ey, l2sx, l2sy, l2ex, l2ey};
}

Precision ParallelConstraint::evaluate_error(const VariableManager& vars) const {
    // Get all variables
    const auto* l1sx = vars.get_variable(line1_start_x_);
    const auto* l1sy = vars.get_variable(line1_start_y_);
    const auto* l1ex = vars.get_variable(line1_end_x_);
    const auto* l1ey = vars.get_variable(line1_end_y_);
    const auto* l2sx = vars.get_variable(line2_start_x_);
    const auto* l2sy = vars.get_variable(line2_start_y_);
    const auto* l2ex = vars.get_variable(line2_end_x_);
    const auto* l2ey = vars.get_variable(line2_end_y_);
    
    if (!l1sx || !l1sy || !l1ex || !l1ey || !l2sx || !l2sy || !l2ex || !l2ey) return 0.0;
    
    // Calculate direction vectors
    Precision dx1 = l1ex->get_value() - l1sx->get_value();
    Precision dy1 = l1ey->get_value() - l1sy->get_value();
    Precision dx2 = l2ex->get_value() - l2sx->get_value();
    Precision dy2 = l2ey->get_value() - l2sy->get_value();
    
    // Cross product (should be zero for parallel lines)
    return dx1 * dy2 - dy1 * dx2;
}

std::vector<Precision> ParallelConstraint::evaluate_gradient(const VariableManager& vars) const {
    const auto* l2sx = vars.get_variable(line2_start_x_);
    const auto* l2sy = vars.get_variable(line2_start_y_);
    const auto* l2ex = vars.get_variable(line2_end_x_);
    const auto* l2ey = vars.get_variable(line2_end_y_);
    
    std::vector<Precision> gradient(8, 0.0);
    
    if (!l2sx || !l2sy || !l2ex || !l2ey) return gradient;
    
    Precision dx2 = l2ex->get_value() - l2sx->get_value();
    Precision dy2 = l2ey->get_value() - l2sy->get_value();
    
    // Gradients for cross product dx1*dy2 - dy1*dx2
    gradient[0] = -dy2;  // d/dl1sx
    gradient[1] = dx2;   // d/dl1sy  
    gradient[2] = dy2;   // d/dl1ex
    gradient[3] = -dx2;  // d/dl1ey
    
    const auto* l1sx = vars.get_variable(line1_start_x_);
    const auto* l1sy = vars.get_variable(line1_start_y_);
    const auto* l1ex = vars.get_variable(line1_end_x_);
    const auto* l1ey = vars.get_variable(line1_end_y_);
    
    if (l1sx && l1sy && l1ex && l1ey) {
        Precision dx1 = l1ex->get_value() - l1sx->get_value();
        Precision dy1 = l1ey->get_value() - l1sy->get_value();
        
        gradient[4] = dy1;   // d/dl2sx
        gradient[5] = -dx1;  // d/dl2sy
        gradient[6] = -dy1;  // d/dl2ex  
        gradient[7] = dx1;   // d/dl2ey
    }
    
    return gradient;
}

std::unique_ptr<GeometricConstraint> ParallelConstraint::clone() const {
    return std::make_unique<ParallelConstraint>(*this);
}

std::string ParallelConstraint::get_description() const {
    return "Parallel constraint: lines are parallel";
}

PerpendicularConstraint::PerpendicularConstraint(size_t id, VariableID l1sx, VariableID l1sy, VariableID l1ex, VariableID l1ey,
                                               VariableID l2sx, VariableID l2sy, VariableID l2ex, VariableID l2ey)
    : GeometricConstraint(id, "Perpendicular", ConstraintCategory::Geometric)
    , line1_start_x_(l1sx), line1_start_y_(l1sy), line1_end_x_(l1ex), line1_end_y_(l1ey)
    , line2_start_x_(l2sx), line2_start_y_(l2sy), line2_end_x_(l2ex), line2_end_y_(l2ey) {
    
    variables_ = {l1sx, l1sy, l1ex, l1ey, l2sx, l2sy, l2ex, l2ey};
}

Precision PerpendicularConstraint::evaluate_error(const VariableManager& vars) const {
    // Get all variables
    const auto* l1sx = vars.get_variable(line1_start_x_);
    const auto* l1sy = vars.get_variable(line1_start_y_);
    const auto* l1ex = vars.get_variable(line1_end_x_);
    const auto* l1ey = vars.get_variable(line1_end_y_);
    const auto* l2sx = vars.get_variable(line2_start_x_);
    const auto* l2sy = vars.get_variable(line2_start_y_);
    const auto* l2ex = vars.get_variable(line2_end_x_);
    const auto* l2ey = vars.get_variable(line2_end_y_);
    
    if (!l1sx || !l1sy || !l1ex || !l1ey || !l2sx || !l2sy || !l2ex || !l2ey) return 0.0;
    
    // Calculate direction vectors
    Precision dx1 = l1ex->get_value() - l1sx->get_value();
    Precision dy1 = l1ey->get_value() - l1sy->get_value();
    Precision dx2 = l2ex->get_value() - l2sx->get_value();
    Precision dy2 = l2ey->get_value() - l2sy->get_value();
    
    // Dot product (should be zero for perpendicular lines)
    return dx1 * dx2 + dy1 * dy2;
}

std::vector<Precision> PerpendicularConstraint::evaluate_gradient(const VariableManager& vars) const {
    const auto* l1sx = vars.get_variable(line1_start_x_);
    const auto* l1sy = vars.get_variable(line1_start_y_);
    const auto* l1ex = vars.get_variable(line1_end_x_);
    const auto* l1ey = vars.get_variable(line1_end_y_);
    const auto* l2sx = vars.get_variable(line2_start_x_);
    const auto* l2sy = vars.get_variable(line2_start_y_);
    const auto* l2ex = vars.get_variable(line2_end_x_);
    const auto* l2ey = vars.get_variable(line2_end_y_);
    
    std::vector<Precision> gradient(8, 0.0);
    
    if (!l1sx || !l1sy || !l1ex || !l1ey || !l2sx || !l2sy || !l2ex || !l2ey) return gradient;
    
    Precision dx1 = l1ex->get_value() - l1sx->get_value();
    Precision dy1 = l1ey->get_value() - l1sy->get_value();
    Precision dx2 = l2ex->get_value() - l2sx->get_value();
    Precision dy2 = l2ey->get_value() - l2sy->get_value();
    
    // Gradients for dot product dx1*dx2 + dy1*dy2
    gradient[0] = -dx2;  // d/dl1sx
    gradient[1] = -dy2;  // d/dl1sy
    gradient[2] = dx2;   // d/dl1ex
    gradient[3] = dy2;   // d/dl1ey
    gradient[4] = -dx1;  // d/dl2sx
    gradient[5] = -dy1;  // d/dl2sy
    gradient[6] = dx1;   // d/dl2ex
    gradient[7] = dy1;   // d/dl2ey
    
    return gradient;
}

std::unique_ptr<GeometricConstraint> PerpendicularConstraint::clone() const {
    return std::make_unique<PerpendicularConstraint>(*this);
}

std::string PerpendicularConstraint::get_description() const {
    return "Perpendicular constraint: lines are perpendicular";
}

CoincidentConstraint::CoincidentConstraint(size_t id, VariableID p1x, VariableID p1y, VariableID p2x, VariableID p2y)
    : GeometricConstraint(id, "Coincident", ConstraintCategory::Geometric)
    , point1_x_(p1x), point1_y_(p1y)
    , point2_x_(p2x), point2_y_(p2y) {
    
    variables_ = {p1x, p1y, p2x, p2y};
}

Precision CoincidentConstraint::evaluate_error(const VariableManager& vars) const {
    const auto* p1x = vars.get_variable(point1_x_);
    const auto* p1y = vars.get_variable(point1_y_);
    const auto* p2x = vars.get_variable(point2_x_);
    const auto* p2y = vars.get_variable(point2_y_);
    
    if (!p1x || !p1y || !p2x || !p2y) return 0.0;
    
    // Distance between points (should be zero)
    Precision dx = p2x->get_value() - p1x->get_value();
    Precision dy = p2y->get_value() - p1y->get_value();
    return std::sqrt(dx * dx + dy * dy);
}

std::vector<Precision> CoincidentConstraint::evaluate_gradient(const VariableManager& vars) const {
    const auto* p1x = vars.get_variable(point1_x_);
    const auto* p1y = vars.get_variable(point1_y_);
    const auto* p2x = vars.get_variable(point2_x_);
    const auto* p2y = vars.get_variable(point2_y_);
    
    std::vector<Precision> gradient(4, 0.0);
    
    if (!p1x || !p1y || !p2x || !p2y) return gradient;
    
    Precision dx = p2x->get_value() - p1x->get_value();
    Precision dy = p2y->get_value() - p1y->get_value();
    Precision distance = std::sqrt(dx * dx + dy * dy);
    
    if (is_zero(distance)) return gradient;
    
    gradient[0] = -dx / distance;  // d/dp1x
    gradient[1] = -dy / distance;  // d/dp1y
    gradient[2] = dx / distance;   // d/dp2x
    gradient[3] = dy / distance;   // d/dp2y
    
    return gradient;
}

std::unique_ptr<GeometricConstraint> CoincidentConstraint::clone() const {
    return std::make_unique<CoincidentConstraint>(*this);
}

std::string CoincidentConstraint::get_description() const {
    return "Coincident constraint: points are at same location";
}

} // namespace qcs::cad