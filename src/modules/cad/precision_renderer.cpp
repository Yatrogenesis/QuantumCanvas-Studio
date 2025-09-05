/**
 * @file precision_renderer.cpp
 * @brief Implementation of high-precision CAD rendering engine
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Sub-pixel precision rendering for technical drawings and CAD models
 */

#include "precision_renderer.hpp"
#include "cad_common.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace qcs::cad {

// =============================================================================
// CADViewport Implementation
// =============================================================================

CADViewport::CADViewport(int width, int height)
    : viewport_rect_{0, 0, width, height}
    , matrices_dirty_(true) {
    
    // Initialize default camera parameters
    view_params_.position = Point3D(0, 0, 10);
    view_params_.target = Point3D::Zero();
    view_params_.up = Vector3D::UnitY();
    view_params_.fov = constants::PI / 4.0; // 45 degrees
    view_params_.near_plane = 0.1;
    view_params_.far_plane = 1000.0;
    view_params_.orthographic = false;
    view_params_.ortho_scale = 10.0;
    
    update_matrices();
}

void CADViewport::set_viewport_rect(int x, int y, int width, int height) {
    viewport_rect_ = {x, y, width, height};
    matrices_dirty_ = true;
}

void CADViewport::set_camera_position(const Point3D& position) {
    view_params_.position = position;
    matrices_dirty_ = true;
}

void CADViewport::set_camera_target(const Point3D& target) {
    view_params_.target = target;
    matrices_dirty_ = true;
}

void CADViewport::set_camera_up(const Vector3D& up) {
    view_params_.up = up.normalized();
    matrices_dirty_ = true;
}

void CADViewport::set_field_of_view(Precision fov) {
    view_params_.fov = clamp(fov, 0.01, constants::PI - 0.01);
    matrices_dirty_ = true;
}

void CADViewport::set_orthographic(bool ortho, Precision scale) {
    view_params_.orthographic = ortho;
    view_params_.ortho_scale = scale;
    matrices_dirty_ = true;
}

void CADViewport::set_clipping_planes(Precision near_plane, Precision far_plane) {
    view_params_.near_plane = std::max(near_plane, static_cast<Precision>(0.001));
    view_params_.far_plane = std::max(far_plane, view_params_.near_plane + 0.001);
    matrices_dirty_ = true;
}

void CADViewport::look_at(const Point3D& eye, const Point3D& center, const Vector3D& up) {
    view_params_.position = eye;
    view_params_.target = center;
    view_params_.up = up.normalized();
    matrices_dirty_ = true;
}

void CADViewport::fit_to_bounds(const BoundingBox3D& bounds, Precision margin) {
    if (!bounds.is_valid()) return;
    
    Point3D center = bounds.center();
    Vector3D size = bounds.size();
    Precision max_extent = std::max({size.x(), size.y(), size.z()});
    
    if (view_params_.orthographic) {
        view_params_.ortho_scale = max_extent * (1.0 + margin);
        view_params_.target = center;
    } else {
        Precision distance = max_extent * (1.0 + margin) / std::tan(view_params_.fov * 0.5);
        Vector3D view_dir = (view_params_.target - view_params_.position).normalized();
        view_params_.position = center - distance * view_dir;
        view_params_.target = center;
    }
    
    matrices_dirty_ = true;
}

void CADViewport::zoom_to_rectangle(const BoundingBox2D& rect) {
    if (!rect.is_valid()) return;
    
    // Convert screen rectangle to world coordinates
    Point3D world_min = screen_to_world(rect.min);
    Point3D world_max = screen_to_world(rect.max);
    
    BoundingBox3D world_rect;
    world_rect.expand(world_min);
    world_rect.expand(world_max);
    
    fit_to_bounds(world_rect);
}

void CADViewport::pan(const Vector2D& delta) {
    // Convert screen delta to world delta
    Vector3D world_delta = screen_to_world_vector(delta);
    
    view_params_.position += world_delta;
    view_params_.target += world_delta;
    matrices_dirty_ = true;
}

void CADViewport::rotate(const Vector2D& delta) {
    // Rotate around target point
    Vector3D to_camera = view_params_.position - view_params_.target;
    Precision distance = to_camera.norm();
    
    // Create rotation around up axis (yaw)
    Matrix4D yaw_rot = rotation_matrix_3d(view_params_.up, -delta.x() * 0.01);
    
    // Create rotation around right axis (pitch)  
    Vector3D forward = -to_camera.normalized();
    Vector3D right = forward.cross(view_params_.up).normalized();
    Matrix4D pitch_rot = rotation_matrix_3d(right, -delta.y() * 0.01);
    
    // Apply rotations
    Matrix4D combined_rot = yaw_rot * pitch_rot;
    Vector3D new_to_camera = transform_vector(to_camera, combined_rot);
    
    view_params_.position = view_params_.target + new_to_camera;
    matrices_dirty_ = true;
}

void CADViewport::zoom(Precision factor, const Point2D& center) {
    if (view_params_.orthographic) {
        view_params_.ortho_scale *= factor;
    } else {
        Vector3D to_target = view_params_.target - view_params_.position;
        view_params_.position += to_target * (1.0 - 1.0 / factor);
    }
    matrices_dirty_ = true;
}

Point3D CADViewport::world_to_view(const Point3D& world_point) const {
    const_cast<CADViewport*>(this)->update_matrices();
    
    Eigen::Vector4<Precision> world_homo(world_point.x(), world_point.y(), world_point.z(), 1.0);
    Eigen::Vector4<Precision> view_homo = view_matrix_ * world_homo;
    
    return Point3D(view_homo.x(), view_homo.y(), view_homo.z());
}

Point2D CADViewport::world_to_screen(const Point3D& world_point) const {
    const_cast<CADViewport*>(this)->update_matrices();
    
    Eigen::Vector4<Precision> world_homo(world_point.x(), world_point.y(), world_point.z(), 1.0);
    Eigen::Vector4<Precision> clip_homo = view_projection_matrix_ * world_homo;
    
    // Perspective divide
    if (std::abs(clip_homo.w()) > 1e-10) {
        clip_homo /= clip_homo.w();
    }
    
    // Convert to screen coordinates
    Precision screen_x = (clip_homo.x() + 1.0) * 0.5 * viewport_rect_.width + viewport_rect_.x;
    Precision screen_y = (1.0 - clip_homo.y()) * 0.5 * viewport_rect_.height + viewport_rect_.y;
    
    return Point2D(screen_x, screen_y);
}

Point3D CADViewport::screen_to_world(const Point2D& screen_point, Precision depth) const {
    const_cast<CADViewport*>(this)->update_matrices();
    
    // Convert screen to normalized device coordinates
    Precision ndc_x = 2.0 * (screen_point.x() - viewport_rect_.x) / viewport_rect_.width - 1.0;
    Precision ndc_y = 1.0 - 2.0 * (screen_point.y() - viewport_rect_.y) / viewport_rect_.height;
    
    Eigen::Vector4<Precision> clip_point(ndc_x, ndc_y, depth, 1.0);
    
    // Transform to world coordinates
    Matrix4D inverse_vp = view_projection_matrix_.inverse();
    Eigen::Vector4<Precision> world_homo = inverse_vp * clip_point;
    
    if (std::abs(world_homo.w()) > 1e-10) {
        world_homo /= world_homo.w();
    }
    
    return Point3D(world_homo.x(), world_homo.y(), world_homo.z());
}

Vector3D CADViewport::screen_to_world_vector(const Vector2D& screen_vector) const {
    Point3D origin = screen_to_world(Point2D::Zero());
    Point3D end = screen_to_world(screen_vector);
    return end - origin;
}

const Matrix4D& CADViewport::get_view_matrix() const {
    const_cast<CADViewport*>(this)->update_matrices();
    return view_matrix_;
}

const Matrix4D& CADViewport::get_projection_matrix() const {
    const_cast<CADViewport*>(this)->update_matrices();
    return projection_matrix_;
}

const Matrix4D& CADViewport::get_view_projection_matrix() const {
    const_cast<CADViewport*>(this)->update_matrices();
    return view_projection_matrix_;
}

bool CADViewport::is_point_visible(const Point3D& point) const {
    Point2D screen_point = world_to_screen(point);
    
    return screen_point.x() >= viewport_rect_.x &&
           screen_point.x() < viewport_rect_.x + viewport_rect_.width &&
           screen_point.y() >= viewport_rect_.y &&
           screen_point.y() < viewport_rect_.y + viewport_rect_.height;
}

bool CADViewport::is_bounds_visible(const BoundingBox3D& bounds) const {
    if (!bounds.is_valid()) return false;
    
    // Test all 8 corners of the bounding box
    std::array<Point3D, 8> corners = {
        Point3D(bounds.min.x(), bounds.min.y(), bounds.min.z()),
        Point3D(bounds.max.x(), bounds.min.y(), bounds.min.z()),
        Point3D(bounds.min.x(), bounds.max.y(), bounds.min.z()),
        Point3D(bounds.max.x(), bounds.max.y(), bounds.min.z()),
        Point3D(bounds.min.x(), bounds.min.y(), bounds.max.z()),
        Point3D(bounds.max.x(), bounds.min.y(), bounds.max.z()),
        Point3D(bounds.min.x(), bounds.max.y(), bounds.max.z()),
        Point3D(bounds.max.x(), bounds.max.y(), bounds.max.z())
    };
    
    // If any corner is visible, the bounds intersect the view frustum
    for (const auto& corner : corners) {
        if (is_point_visible(corner)) {
            return true;
        }
    }
    
    return false;
}

void CADViewport::update_matrices() {
    if (!matrices_dirty_) return;
    
    // Create view matrix (look-at)
    Vector3D forward = (view_params_.target - view_params_.position).normalized();
    Vector3D right = forward.cross(view_params_.up).normalized();
    Vector3D up = right.cross(forward);
    
    view_matrix_ = Matrix4D::Identity();
    view_matrix_(0, 0) = right.x();   view_matrix_(0, 1) = right.y();   view_matrix_(0, 2) = right.z();
    view_matrix_(1, 0) = up.x();      view_matrix_(1, 1) = up.y();      view_matrix_(1, 2) = up.z();
    view_matrix_(2, 0) = -forward.x(); view_matrix_(2, 1) = -forward.y(); view_matrix_(2, 2) = -forward.z();
    
    view_matrix_(0, 3) = -right.dot(view_params_.position);
    view_matrix_(1, 3) = -up.dot(view_params_.position);
    view_matrix_(2, 3) = forward.dot(view_params_.position);
    
    // Create projection matrix
    projection_matrix_ = Matrix4D::Zero();
    
    if (view_params_.orthographic) {
        Precision aspect = static_cast<Precision>(viewport_rect_.width) / viewport_rect_.height;
        Precision width = view_params_.ortho_scale * aspect;
        Precision height = view_params_.ortho_scale;
        
        projection_matrix_(0, 0) = 2.0 / width;
        projection_matrix_(1, 1) = 2.0 / height;
        projection_matrix_(2, 2) = -2.0 / (view_params_.far_plane - view_params_.near_plane);
        projection_matrix_(2, 3) = -(view_params_.far_plane + view_params_.near_plane) / 
                                   (view_params_.far_plane - view_params_.near_plane);
        projection_matrix_(3, 3) = 1.0;
    } else {
        Precision aspect = static_cast<Precision>(viewport_rect_.width) / viewport_rect_.height;
        Precision tan_half_fov = std::tan(view_params_.fov * 0.5);
        
        projection_matrix_(0, 0) = 1.0 / (aspect * tan_half_fov);
        projection_matrix_(1, 1) = 1.0 / tan_half_fov;
        projection_matrix_(2, 2) = -(view_params_.far_plane + view_params_.near_plane) / 
                                   (view_params_.far_plane - view_params_.near_plane);
        projection_matrix_(2, 3) = -2.0 * view_params_.far_plane * view_params_.near_plane / 
                                   (view_params_.far_plane - view_params_.near_plane);
        projection_matrix_(3, 2) = -1.0;
    }
    
    // Combine matrices
    view_projection_matrix_ = projection_matrix_ * view_matrix_;
    
    update_frustum();
    matrices_dirty_ = false;
}

void CADViewport::update_frustum() {
    // Update view frustum for culling
    // Implementation would calculate frustum planes from view-projection matrix
    // For now, we use the simple bounding box approach
}

// =============================================================================
// TechnicalLinetype Implementation
// =============================================================================

TechnicalLinetype::TechnicalLinetype(const std::string& name)
    : name_(name)
    , description_("")
    , total_length_(0)
    , is_continuous_(true) {
}

void TechnicalLinetype::add_element(LinePatternElement::Type type, Precision length, Precision scale) {
    pattern_.emplace_back(type, length, scale);
    is_continuous_ = false;
}

void TechnicalLinetype::clear_pattern() {
    pattern_.clear();
    total_length_ = 0;
    is_continuous_ = true;
}

void TechnicalLinetype::finalize_pattern() {
    total_length_ = 0;
    for (const auto& element : pattern_) {
        total_length_ += element.length * element.scale;
    }
    
    if (pattern_.empty()) {
        is_continuous_ = true;
    }
}

bool TechnicalLinetype::is_visible_at_parameter(Precision t) const {
    if (is_continuous_) return true;
    if (pattern_.empty()) return true;
    if (is_zero(total_length_)) return true;
    
    // Normalize parameter to pattern length
    Precision param = std::fmod(t * total_length_, total_length_);
    if (param < 0) param += total_length_;
    
    Precision current_pos = 0;
    for (const auto& element : pattern_) {
        Precision element_length = element.length * element.scale;
        
        if (param >= current_pos && param < current_pos + element_length) {
            return (element.type == LinePatternElement::Line || 
                    element.type == LinePatternElement::Dot);
        }
        
        current_pos += element_length;
    }
    
    return false;
}

Precision TechnicalLinetype::get_dash_phase(Precision start_param) const {
    if (is_continuous_ || pattern_.empty()) return 0;
    
    return std::fmod(start_param * total_length_, total_length_);
}

// Standard linetypes
TechnicalLinetype TechnicalLinetype::continuous() {
    TechnicalLinetype linetype("Continuous");
    linetype.description_ = "Solid continuous line";
    return linetype;
}

TechnicalLinetype TechnicalLinetype::hidden() {
    TechnicalLinetype linetype("Hidden");
    linetype.description_ = "Hidden lines (dashed)";
    linetype.add_element(LinePatternElement::Line, 3.0);
    linetype.add_element(LinePatternElement::Gap, 1.5);
    linetype.finalize_pattern();
    return linetype;
}

TechnicalLinetype TechnicalLinetype::center() {
    TechnicalLinetype linetype("Center");
    linetype.description_ = "Center lines";
    linetype.add_element(LinePatternElement::Line, 6.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Line, 1.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.finalize_pattern();
    return linetype;
}

TechnicalLinetype TechnicalLinetype::phantom() {
    TechnicalLinetype linetype("Phantom");
    linetype.description_ = "Phantom lines";
    linetype.add_element(LinePatternElement::Line, 6.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Line, 1.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Line, 1.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.finalize_pattern();
    return linetype;
}

TechnicalLinetype TechnicalLinetype::dashed() {
    TechnicalLinetype linetype("Dashed");
    linetype.description_ = "Dashed lines";
    linetype.add_element(LinePatternElement::Dash, 5.0);
    linetype.add_element(LinePatternElement::Gap, 2.5);
    linetype.finalize_pattern();
    return linetype;
}

TechnicalLinetype TechnicalLinetype::dot_dash() {
    TechnicalLinetype linetype("DotDash");
    linetype.description_ = "Dot-dash lines";
    linetype.add_element(LinePatternElement::Line, 4.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Dot, 0.5);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.finalize_pattern();
    return linetype;
}

TechnicalLinetype TechnicalLinetype::border() {
    TechnicalLinetype linetype("Border");
    linetype.description_ = "Border lines";
    linetype.add_element(LinePatternElement::Line, 8.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Line, 1.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Line, 8.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Dot, 0.5);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.finalize_pattern();
    return linetype;
}

TechnicalLinetype TechnicalLinetype::divide() {
    TechnicalLinetype linetype("Divide");
    linetype.description_ = "Divide lines";
    linetype.add_element(LinePatternElement::Line, 6.0);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Dot, 0.5);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.add_element(LinePatternElement::Dot, 0.5);
    linetype.add_element(LinePatternElement::Gap, 1.0);
    linetype.finalize_pattern();
    return linetype;
}

// =============================================================================
// Utility Functions Implementation
// =============================================================================

Precision line_weight_to_thickness(LineWeight weight) {
    switch (weight) {
        case LineWeight::Thin: return 0.13;       // 0.13 mm
        case LineWeight::Normal: return 0.25;     // 0.25 mm  
        case LineWeight::Medium: return 0.35;     // 0.35 mm
        case LineWeight::Thick: return 0.50;      // 0.50 mm
        case LineWeight::ExtraThick: return 0.70; // 0.70 mm
        default: return 0.25;
    }
}

LineWeight thickness_to_line_weight(Precision thickness) {
    if (thickness <= 0.19) return LineWeight::Thin;
    if (thickness <= 0.30) return LineWeight::Normal;
    if (thickness <= 0.42) return LineWeight::Medium;
    if (thickness <= 0.60) return LineWeight::Thick;
    return LineWeight::ExtraThick;
}

TessellationParams calculate_tessellation_params(const CADViewport& viewport,
                                              const BoundingBox3D& entity_bounds,
                                              RenderingQuality quality) {
    TessellationParams params;
    
    // Calculate screen-space size of entity
    Point2D screen_min = viewport.world_to_screen(entity_bounds.min);
    Point2D screen_max = viewport.world_to_screen(entity_bounds.max);
    Precision screen_size = (screen_max - screen_min).norm();
    
    // Base tolerance on quality and screen size
    switch (quality) {
        case RenderingQuality::Draft:
            params.tolerance = entity_bounds.size().norm() * 0.01;
            params.max_depth = 4;
            break;
        case RenderingQuality::Normal:
            params.tolerance = entity_bounds.size().norm() * 0.001;
            params.max_depth = 6;
            break;
        case RenderingQuality::HighQuality:
            params.tolerance = entity_bounds.size().norm() * 0.0001;
            params.max_depth = 8;
            break;
        case RenderingQuality::Publication:
            params.tolerance = entity_bounds.size().norm() * 0.00001;
            params.max_depth = 10;
            break;
    }
    
    // Adjust for screen size - smaller objects need less tessellation
    if (screen_size < 10.0) {
        params.tolerance *= 10.0;
        params.max_depth = std::max(2, params.max_depth - 2);
    }
    
    params.adaptive = true;
    return params;
}

uint32_t cad_color_to_rgba(const Color& color) {
    uint8_t r = static_cast<uint8_t>(clamp(color.r, 0.0, 1.0) * 255);
    uint8_t g = static_cast<uint8_t>(clamp(color.g, 0.0, 1.0) * 255);
    uint8_t b = static_cast<uint8_t>(clamp(color.b, 0.0, 1.0) * 255);
    uint8_t a = static_cast<uint8_t>(clamp(color.a, 0.0, 1.0) * 255);
    
    return (static_cast<uint32_t>(r) << 24) |
           (static_cast<uint32_t>(g) << 16) |
           (static_cast<uint32_t>(b) << 8) |
           static_cast<uint32_t>(a);
}

Color rgba_to_cad_color(uint32_t rgba) {
    uint8_t r = static_cast<uint8_t>((rgba >> 24) & 0xFF);
    uint8_t g = static_cast<uint8_t>((rgba >> 16) & 0xFF);
    uint8_t b = static_cast<uint8_t>((rgba >> 8) & 0xFF);
    uint8_t a = static_cast<uint8_t>(rgba & 0xFF);
    
    return Color(
        static_cast<Precision>(r) / 255.0,
        static_cast<Precision>(g) / 255.0,
        static_cast<Precision>(b) / 255.0,
        static_cast<Precision>(a) / 255.0
    );
}

} // namespace qcs::cad