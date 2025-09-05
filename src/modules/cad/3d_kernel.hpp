/**
 * @file 3d_kernel.hpp
 * @brief Advanced 3D modeling kernel for solid and surface modeling
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Enterprise-grade 3D modeling with BREP, CSG, and NURBS support
 */

#pragma once

#include "cad_types.hpp"
#include "cad_common.hpp"

#include <memory>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <functional>
#include <optional>

// Optional OpenCASCADE integration
#ifdef QCS_OPENCASCADE_ENABLED
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#endif

namespace qcs::cad {

// Forward declarations
class TopologyEntity;
class GeometryEntity;
class SolidModel;
class SurfaceModel;
class Feature;
class ModelingKernel;

// =============================================================================
// 3D Modeling Concepts and Types
// =============================================================================

/// Topological entity types in BREP representation
enum class TopologyType {
    Vertex,     ///< 0D - Point topology
    Edge,       ///< 1D - Curve topology  
    Wire,       ///< 1D - Connected edges
    Face,       ///< 2D - Surface topology
    Shell,      ///< 2D - Connected faces
    Solid,      ///< 3D - Volume topology
    CompSolid,  ///< 3D - Connected solids
    Compound    ///< Mixed - Collection of entities
};

/// Geometric entity types
enum class GeometryType {
    Point,          ///< 0D point
    Line,           ///< 1D infinite line
    Circle,         ///< 1D circular curve
    Ellipse,        ///< 1D elliptical curve
    BSplineCurve,   ///< 1D B-spline curve
    NURBSCurve,     ///< 1D NURBS curve
    Plane,          ///< 2D infinite plane
    Cylinder,       ///< 2D cylindrical surface
    Cone,           ///< 2D conical surface
    Sphere,         ///< 2D spherical surface
    Torus,          ///< 2D toroidal surface
    BSplineSurface, ///< 2D B-spline surface
    NURBSSurface    ///< 2D NURBS surface
};

/// Boolean operation types for CSG
enum class BooleanOperation {
    Union,          ///< A ∪ B
    Intersection,   ///< A ∩ B
    Difference,     ///< A - B
    SymmetricDiff   ///< A ⊕ B (XOR)
};

/// Surface continuity levels
enum class SurfaceContinuity {
    C0,     ///< Position continuity (touching)
    C1,     ///< Tangent continuity (smooth)
    C2,     ///< Curvature continuity (fair)
    G0,     ///< Geometric position continuity
    G1,     ///< Geometric tangent continuity
    G2      ///< Geometric curvature continuity
};

/// Feature operation types
enum class FeatureType {
    Extrude,        ///< Linear extrusion
    Revolve,        ///< Rotational sweep
    Sweep,          ///< Path-based sweep
    Loft,           ///< Multi-section loft
    Fillet,         ///< Edge rounding
    Chamfer,        ///< Edge beveling
    Draft,          ///< Draft angle
    Shell,          ///< Hollow operation
    Pattern,        ///< Feature pattern
    Mirror,         ///< Mirror operation
    Blend,          ///< Surface blending
    Offset,         ///< Surface offset
    Trim,           ///< Surface trimming
    Extend,         ///< Surface extension
    Stitch          ///< Surface stitching
};

/// Model validation states
enum class ValidationState {
    Valid,              ///< Model is geometrically valid
    Warning,            ///< Minor issues that don't affect functionality
    InvalidTopology,    ///< Topological inconsistency
    InvalidGeometry,    ///< Geometric degeneracy
    SelfIntersection,   ///< Self-intersecting geometry
    NonManifold,        ///< Non-manifold topology
    OpenVolume,         ///< Volume with openings
    InvalidOrientation, ///< Inconsistent face orientations
    Failed             ///< Validation failed to complete
};

// =============================================================================
// Topological Entities (BREP Structure)
// =============================================================================

/// Base class for all topological entities
class TopologyEntity {
protected:
    size_t id_;
    TopologyType type_;
    std::shared_ptr<GeometryEntity> geometry_;
    BoundingBox3D bounds_;
    bool is_valid_;
    
    // Topological relationships
    std::vector<std::weak_ptr<TopologyEntity>> parents_;
    std::vector<std::shared_ptr<TopologyEntity>> children_;

public:
    TopologyEntity(size_t id, TopologyType type);
    virtual ~TopologyEntity() = default;
    
    // Basic properties
    size_t get_id() const { return id_; }
    TopologyType get_type() const { return type_; }
    bool is_valid() const { return is_valid_; }
    const BoundingBox3D& get_bounds() const { return bounds_; }
    
    // Geometry association
    std::shared_ptr<GeometryEntity> get_geometry() const { return geometry_; }
    void set_geometry(std::shared_ptr<GeometryEntity> geometry);
    
    // Topological relationships
    const std::vector<std::shared_ptr<TopologyEntity>>& get_children() const { return children_; }
    std::vector<std::shared_ptr<TopologyEntity>> get_parents() const;
    
    void add_child(std::shared_ptr<TopologyEntity> child);
    void remove_child(std::shared_ptr<TopologyEntity> child);
    void add_parent(std::weak_ptr<TopologyEntity> parent);
    void remove_parent(std::weak_ptr<TopologyEntity> parent);
    
    // Virtual methods
    virtual void update_bounds() = 0;
    virtual bool validate() = 0;
    virtual std::unique_ptr<TopologyEntity> clone() const = 0;
    
    // Traversal utilities
    std::vector<std::shared_ptr<TopologyEntity>> get_entities_of_type(TopologyType type) const;
    template<typename EntityType>
    std::vector<std::shared_ptr<EntityType>> get_entities() const;
};

/// Vertex - 0D topological entity
class Vertex : public TopologyEntity {
private:
    Point3D position_;
    Precision tolerance_;

public:
    Vertex(size_t id, const Point3D& position, Precision tolerance = GEOMETRIC_TOLERANCE);
    
    const Point3D& get_position() const { return position_; }
    void set_position(const Point3D& position);
    
    Precision get_tolerance() const { return tolerance_; }
    void set_tolerance(Precision tolerance) { tolerance_ = tolerance; }
    
    // Inherited methods
    void update_bounds() override;
    bool validate() override;
    std::unique_ptr<TopologyEntity> clone() const override;
    
    // Vertex-specific operations
    bool is_coincident(const Vertex& other) const;
    Precision distance_to(const Point3D& point) const;
};

/// Edge - 1D topological entity
class Edge : public TopologyEntity {
private:
    std::shared_ptr<Vertex> start_vertex_;
    std::shared_ptr<Vertex> end_vertex_;
    bool is_closed_;
    bool is_degenerate_;
    Precision first_parameter_;
    Precision last_parameter_;

public:
    Edge(size_t id, std::shared_ptr<Vertex> start, std::shared_ptr<Vertex> end);
    
    std::shared_ptr<Vertex> get_start_vertex() const { return start_vertex_; }
    std::shared_ptr<Vertex> get_end_vertex() const { return end_vertex_; }
    
    void set_start_vertex(std::shared_ptr<Vertex> vertex);
    void set_end_vertex(std::shared_ptr<Vertex> vertex);
    
    bool is_closed() const { return is_closed_; }
    bool is_degenerate() const { return is_degenerate_; }
    
    Precision get_first_parameter() const { return first_parameter_; }
    Precision get_last_parameter() const { return last_parameter_; }
    void set_parameter_range(Precision first, Precision last);
    
    // Inherited methods
    void update_bounds() override;
    bool validate() override;
    std::unique_ptr<TopologyEntity> clone() const override;
    
    // Edge-specific operations
    Point3D evaluate_at_parameter(Precision t) const;
    Vector3D tangent_at_parameter(Precision t) const;
    Precision length() const;
    bool is_tangent_continuous_with(const Edge& other) const;
};

/// Wire - Collection of connected edges
class Wire : public TopologyEntity {
private:
    std::vector<std::shared_ptr<Edge>> edges_;
    bool is_closed_;
    bool is_valid_orientation_;

public:
    Wire(size_t id, const std::vector<std::shared_ptr<Edge>>& edges = {});
    
    const std::vector<std::shared_ptr<Edge>>& get_edges() const { return edges_; }
    void add_edge(std::shared_ptr<Edge> edge);
    void remove_edge(std::shared_ptr<Edge> edge);
    void clear_edges() { edges_.clear(); }
    
    bool is_closed() const { return is_closed_; }
    bool has_valid_orientation() const { return is_valid_orientation_; }
    
    // Inherited methods
    void update_bounds() override;
    bool validate() override;
    std::unique_ptr<TopologyEntity> clone() const override;
    
    // Wire-specific operations
    Precision total_length() const;
    std::vector<std::shared_ptr<Vertex>> get_vertices() const;
    bool make_closed();
    void reverse_orientation();
    bool fix_connectivity();
};

/// Face - 2D topological entity with surface
class Face : public TopologyEntity {
private:
    std::shared_ptr<Wire> outer_boundary_;
    std::vector<std::shared_ptr<Wire>> inner_boundaries_;  // Holes
    bool forward_orientation_;
    SurfaceContinuity continuity_level_;

public:
    Face(size_t id, std::shared_ptr<Wire> outer_boundary = nullptr);
    
    std::shared_ptr<Wire> get_outer_boundary() const { return outer_boundary_; }
    void set_outer_boundary(std::shared_ptr<Wire> boundary);
    
    const std::vector<std::shared_ptr<Wire>>& get_inner_boundaries() const { return inner_boundaries_; }
    void add_inner_boundary(std::shared_ptr<Wire> boundary);
    void remove_inner_boundary(std::shared_ptr<Wire> boundary);
    
    bool has_forward_orientation() const { return forward_orientation_; }
    void set_orientation(bool forward) { forward_orientation_ = forward; }
    void reverse_orientation() { forward_orientation_ = !forward_orientation_; }
    
    SurfaceContinuity get_continuity_level() const { return continuity_level_; }
    void set_continuity_level(SurfaceContinuity level) { continuity_level_ = level; }
    
    // Inherited methods
    void update_bounds() override;
    bool validate() override;
    std::unique_ptr<TopologyEntity> clone() const override;
    
    // Face-specific operations
    Point3D evaluate_at_parameters(Precision u, Precision v) const;
    Vector3D normal_at_parameters(Precision u, Precision v) const;
    Precision area() const;
    bool contains_point(const Point3D& point) const;
    std::pair<Precision, Precision> closest_parameters(const Point3D& point) const;
};

/// Shell - Collection of connected faces
class Shell : public TopologyEntity {
private:
    std::vector<std::shared_ptr<Face>> faces_;
    bool is_closed_;
    bool is_manifold_;
    bool has_consistent_orientation_;

public:
    Shell(size_t id, const std::vector<std::shared_ptr<Face>>& faces = {});
    
    const std::vector<std::shared_ptr<Face>>& get_faces() const { return faces_; }
    void add_face(std::shared_ptr<Face> face);
    void remove_face(std::shared_ptr<Face> face);
    
    bool is_closed() const { return is_closed_; }
    bool is_manifold() const { return is_manifold_; }
    bool has_consistent_orientation() const { return has_consistent_orientation_; }
    
    // Inherited methods
    void update_bounds() override;
    bool validate() override;
    std::unique_ptr<TopologyEntity> clone() const override;
    
    // Shell-specific operations
    Precision area() const;
    std::vector<std::shared_ptr<Edge>> get_free_edges() const;
    bool make_consistent_orientation();
    bool stitch_faces(Precision tolerance = GEOMETRIC_TOLERANCE);
};

/// Solid - 3D topological entity with volume
class Solid : public TopologyEntity {
private:
    std::vector<std::shared_ptr<Shell>> shells_;
    std::shared_ptr<Shell> outer_shell_;
    std::vector<std::shared_ptr<Shell>> inner_shells_;  // Cavities

public:
    Solid(size_t id, std::shared_ptr<Shell> outer_shell = nullptr);
    
    std::shared_ptr<Shell> get_outer_shell() const { return outer_shell_; }
    void set_outer_shell(std::shared_ptr<Shell> shell);
    
    const std::vector<std::shared_ptr<Shell>>& get_inner_shells() const { return inner_shells_; }
    void add_inner_shell(std::shared_ptr<Shell> shell);
    void remove_inner_shell(std::shared_ptr<Shell> shell);
    
    // Inherited methods
    void update_bounds() override;
    bool validate() override;
    std::unique_ptr<TopologyEntity> clone() const override;
    
    // Solid-specific operations
    Precision volume() const;
    Point3D center_of_mass() const;
    Matrix3D moment_of_inertia() const;
    bool contains_point(const Point3D& point) const;
    bool is_valid_solid() const;
};

// =============================================================================
// Geometric Entities
// =============================================================================

/// Base class for geometric entities
class GeometryEntity {
protected:
    size_t id_;
    GeometryType type_;
    BoundingBox3D bounds_;

public:
    GeometryEntity(size_t id, GeometryType type) : id_(id), type_(type) {}
    virtual ~GeometryEntity() = default;
    
    size_t get_id() const { return id_; }
    GeometryType get_type() const { return type_; }
    const BoundingBox3D& get_bounds() const { return bounds_; }
    
    virtual void update_bounds() = 0;
    virtual std::unique_ptr<GeometryEntity> clone() const = 0;
    virtual bool is_degenerate() const = 0;
};

/// 3D curve geometry
class CurveGeometry : public GeometryEntity {
public:
    CurveGeometry(size_t id, GeometryType type) : GeometryEntity(id, type) {}
    
    virtual Point3D evaluate_at(Precision t) const = 0;
    virtual Vector3D first_derivative_at(Precision t) const = 0;
    virtual Vector3D second_derivative_at(Precision t) const = 0;
    virtual Precision first_parameter() const = 0;
    virtual Precision last_parameter() const = 0;
    virtual bool is_closed() const = 0;
    virtual Precision length() const = 0;
};

/// 3D surface geometry
class SurfaceGeometry : public GeometryEntity {
public:
    SurfaceGeometry(size_t id, GeometryType type) : GeometryEntity(id, type) {}
    
    virtual Point3D evaluate_at(Precision u, Precision v) const = 0;
    virtual Vector3D first_u_derivative_at(Precision u, Precision v) const = 0;
    virtual Vector3D first_v_derivative_at(Precision u, Precision v) const = 0;
    virtual Vector3D normal_at(Precision u, Precision v) const = 0;
    virtual std::pair<Precision, Precision> u_parameter_range() const = 0;
    virtual std::pair<Precision, Precision> v_parameter_range() const = 0;
    virtual bool is_u_closed() const = 0;
    virtual bool is_v_closed() const = 0;
    virtual Precision area() const = 0;
};

// =============================================================================
// Primitive Geometry Implementations
// =============================================================================

/// Line geometry implementation
class LineGeometry : public CurveGeometry {
private:
    Point3D origin_;
    Vector3D direction_;
    Precision first_param_;
    Precision last_param_;

public:
    LineGeometry(size_t id, const Point3D& origin, const Vector3D& direction,
                Precision first = 0, Precision last = 1);
    
    const Point3D& get_origin() const { return origin_; }
    const Vector3D& get_direction() const { return direction_; }
    
    // Inherited methods
    Point3D evaluate_at(Precision t) const override;
    Vector3D first_derivative_at(Precision t) const override;
    Vector3D second_derivative_at(Precision t) const override;
    Precision first_parameter() const override { return first_param_; }
    Precision last_parameter() const override { return last_param_; }
    bool is_closed() const override { return false; }
    Precision length() const override;
    
    void update_bounds() override;
    std::unique_ptr<GeometryEntity> clone() const override;
    bool is_degenerate() const override;
};

/// Circle geometry implementation
class CircleGeometry : public CurveGeometry {
private:
    Point3D center_;
    Vector3D normal_;
    Vector3D u_axis_;
    Vector3D v_axis_;
    Precision radius_;

public:
    CircleGeometry(size_t id, const Point3D& center, const Vector3D& normal, Precision radius);
    
    const Point3D& get_center() const { return center_; }
    const Vector3D& get_normal() const { return normal_; }
    Precision get_radius() const { return radius_; }
    
    // Inherited methods
    Point3D evaluate_at(Precision t) const override;
    Vector3D first_derivative_at(Precision t) const override;
    Vector3D second_derivative_at(Precision t) const override;
    Precision first_parameter() const override { return 0; }
    Precision last_parameter() const override { return constants::TAU; }
    bool is_closed() const override { return true; }
    Precision length() const override { return constants::TAU * radius_; }
    
    void update_bounds() override;
    std::unique_ptr<GeometryEntity> clone() const override;
    bool is_degenerate() const override;
};

/// Plane geometry implementation
class PlaneGeometry : public SurfaceGeometry {
private:
    Point3D origin_;
    Vector3D u_axis_;
    Vector3D v_axis_;
    Vector3D normal_;

public:
    PlaneGeometry(size_t id, const Point3D& origin, const Vector3D& normal);
    PlaneGeometry(size_t id, const Point3D& origin, const Vector3D& u_axis, const Vector3D& v_axis);
    
    const Point3D& get_origin() const { return origin_; }
    const Vector3D& get_normal() const { return normal_; }
    const Vector3D& get_u_axis() const { return u_axis_; }
    const Vector3D& get_v_axis() const { return v_axis_; }
    
    // Inherited methods
    Point3D evaluate_at(Precision u, Precision v) const override;
    Vector3D first_u_derivative_at(Precision u, Precision v) const override;
    Vector3D first_v_derivative_at(Precision u, Precision v) const override;
    Vector3D normal_at(Precision u, Precision v) const override { return normal_; }
    std::pair<Precision, Precision> u_parameter_range() const override;
    std::pair<Precision, Precision> v_parameter_range() const override;
    bool is_u_closed() const override { return false; }
    bool is_v_closed() const override { return false; }
    Precision area() const override { return std::numeric_limits<Precision>::infinity(); }
    
    void update_bounds() override;
    std::unique_ptr<GeometryEntity> clone() const override;
    bool is_degenerate() const override;
    
    // Plane-specific operations
    Precision distance_to_point(const Point3D& point) const;
    Point3D closest_point(const Point3D& point) const;
    bool contains_point(const Point3D& point, Precision tolerance = GEOMETRIC_TOLERANCE) const;
};

// =============================================================================
// Feature-Based Modeling
// =============================================================================

/// Base class for modeling features
class Feature {
protected:
    size_t id_;
    std::string name_;
    FeatureType type_;
    std::vector<size_t> input_entity_ids_;
    std::shared_ptr<TopologyEntity> result_;
    bool is_suppressed_;
    bool is_valid_;

public:
    Feature(size_t id, const std::string& name, FeatureType type);
    virtual ~Feature() = default;
    
    size_t get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    FeatureType get_type() const { return type_; }
    bool is_suppressed() const { return is_suppressed_; }
    bool is_valid() const { return is_valid_; }
    
    void set_name(const std::string& name) { name_ = name; }
    void set_suppressed(bool suppressed) { is_suppressed_ = suppressed; }
    
    std::shared_ptr<TopologyEntity> get_result() const { return result_; }
    const std::vector<size_t>& get_input_entity_ids() const { return input_entity_ids_; }
    
    virtual bool execute() = 0;
    virtual bool validate() = 0;
    virtual std::unique_ptr<Feature> clone() const = 0;
    virtual std::string get_description() const = 0;
};

/// Extrude feature
class ExtrudeFeature : public Feature {
private:
    std::shared_ptr<Wire> profile_;
    Vector3D direction_;
    Precision distance_;
    bool both_directions_;
    Precision taper_angle_;

public:
    ExtrudeFeature(size_t id, const std::string& name, std::shared_ptr<Wire> profile,
                  const Vector3D& direction, Precision distance);
    
    const Vector3D& get_direction() const { return direction_; }
    Precision get_distance() const { return distance_; }
    bool is_both_directions() const { return both_directions_; }
    Precision get_taper_angle() const { return taper_angle_; }
    
    void set_direction(const Vector3D& direction) { direction_ = direction; }
    void set_distance(Precision distance) { distance_ = distance; }
    void set_both_directions(bool both) { both_directions_ = both; }
    void set_taper_angle(Precision angle) { taper_angle_ = angle; }
    
    // Inherited methods
    bool execute() override;
    bool validate() override;
    std::unique_ptr<Feature> clone() const override;
    std::string get_description() const override;
};

/// Revolve feature
class RevolveFeature : public Feature {
private:
    std::shared_ptr<Wire> profile_;
    Point3D axis_origin_;
    Vector3D axis_direction_;
    Precision angle_;
    bool full_revolution_;

public:
    RevolveFeature(size_t id, const std::string& name, std::shared_ptr<Wire> profile,
                  const Point3D& axis_origin, const Vector3D& axis_direction, Precision angle);
    
    const Point3D& get_axis_origin() const { return axis_origin_; }
    const Vector3D& get_axis_direction() const { return axis_direction_; }
    Precision get_angle() const { return angle_; }
    bool is_full_revolution() const { return full_revolution_; }
    
    void set_axis(const Point3D& origin, const Vector3D& direction);
    void set_angle(Precision angle);
    void set_full_revolution(bool full) { full_revolution_ = full; }
    
    // Inherited methods
    bool execute() override;
    bool validate() override;
    std::unique_ptr<Feature> clone() const override;
    std::string get_description() const override;
};

// =============================================================================
// 3D Modeling Kernel
// =============================================================================

/// Main 3D modeling kernel with BREP, CSG, and feature support
class ModelingKernel {
private:
    // Entity management
    std::unordered_map<size_t, std::shared_ptr<TopologyEntity>> topology_entities_;
    std::unordered_map<size_t, std::shared_ptr<GeometryEntity>> geometry_entities_;
    std::unordered_map<size_t, std::unique_ptr<Feature>> features_;
    
    size_t next_entity_id_;
    size_t next_geometry_id_;
    size_t next_feature_id_;
    
    // Kernel settings
    Precision modeling_tolerance_;
    Precision angular_tolerance_;
    bool enable_validation_;
    bool enable_healing_;

public:
    ModelingKernel();
    ~ModelingKernel();
    
    // Settings
    void set_modeling_tolerance(Precision tolerance) { modeling_tolerance_ = tolerance; }
    void set_angular_tolerance(Precision tolerance) { angular_tolerance_ = tolerance; }
    void enable_automatic_validation(bool enable) { enable_validation_ = enable; }
    void enable_automatic_healing(bool enable) { enable_healing_ = enable; }
    
    Precision get_modeling_tolerance() const { return modeling_tolerance_; }
    Precision get_angular_tolerance() const { return angular_tolerance_; }
    
    // Entity creation - Topology
    std::shared_ptr<Vertex> create_vertex(const Point3D& position);
    std::shared_ptr<Edge> create_edge(std::shared_ptr<Vertex> start, std::shared_ptr<Vertex> end,
                                     std::shared_ptr<CurveGeometry> curve = nullptr);
    std::shared_ptr<Wire> create_wire(const std::vector<std::shared_ptr<Edge>>& edges);
    std::shared_ptr<Face> create_face(std::shared_ptr<Wire> boundary,
                                     std::shared_ptr<SurfaceGeometry> surface = nullptr);
    std::shared_ptr<Shell> create_shell(const std::vector<std::shared_ptr<Face>>& faces);
    std::shared_ptr<Solid> create_solid(std::shared_ptr<Shell> shell);
    
    // Entity creation - Geometry
    std::shared_ptr<LineGeometry> create_line(const Point3D& start, const Point3D& end);
    std::shared_ptr<CircleGeometry> create_circle(const Point3D& center, const Vector3D& normal, 
                                                 Precision radius);
    std::shared_ptr<PlaneGeometry> create_plane(const Point3D& origin, const Vector3D& normal);
    
    // Primitive creation
    std::shared_ptr<Solid> create_box(const Point3D& corner, const Vector3D& dimensions);
    std::shared_ptr<Solid> create_cylinder(const Point3D& base_center, const Vector3D& axis,
                                         Precision radius, Precision height);
    std::shared_ptr<Solid> create_cone(const Point3D& base_center, const Vector3D& axis,
                                      Precision base_radius, Precision top_radius, Precision height);
    std::shared_ptr<Solid> create_sphere(const Point3D& center, Precision radius);
    std::shared_ptr<Solid> create_torus(const Point3D& center, const Vector3D& axis,
                                       Precision major_radius, Precision minor_radius);
    
    // Boolean operations (CSG)
    std::shared_ptr<Solid> boolean_union(std::shared_ptr<Solid> solid1, std::shared_ptr<Solid> solid2);
    std::shared_ptr<Solid> boolean_intersection(std::shared_ptr<Solid> solid1, std::shared_ptr<Solid> solid2);
    std::shared_ptr<Solid> boolean_difference(std::shared_ptr<Solid> solid1, std::shared_ptr<Solid> solid2);
    std::shared_ptr<Solid> boolean_symmetric_difference(std::shared_ptr<Solid> solid1, 
                                                       std::shared_ptr<Solid> solid2);
    
    // Advanced operations
    std::shared_ptr<Solid> fillet_edges(std::shared_ptr<Solid> solid,
                                       const std::vector<std::shared_ptr<Edge>>& edges,
                                       Precision radius);
    std::shared_ptr<Solid> chamfer_edges(std::shared_ptr<Solid> solid,
                                        const std::vector<std::shared_ptr<Edge>>& edges,
                                        Precision distance1, Precision distance2);
    std::shared_ptr<Shell> offset_faces(std::shared_ptr<Shell> shell, Precision offset);
    std::shared_ptr<Solid> shell_solid(std::shared_ptr<Solid> solid, 
                                      const std::vector<std::shared_ptr<Face>>& faces_to_remove,
                                      Precision thickness);
    
    // Feature management
    size_t add_feature(std::unique_ptr<Feature> feature);
    void remove_feature(size_t feature_id);
    Feature* get_feature(size_t feature_id);
    void execute_feature(size_t feature_id);
    void execute_all_features();
    void suppress_feature(size_t feature_id, bool suppress);
    
    // Model validation and repair
    ValidationState validate_model(std::shared_ptr<TopologyEntity> entity);
    bool heal_model(std::shared_ptr<TopologyEntity> entity);
    std::vector<std::string> get_validation_report(std::shared_ptr<TopologyEntity> entity);
    
    // Geometric queries
    Precision compute_volume(std::shared_ptr<Solid> solid);
    Precision compute_area(std::shared_ptr<Face> face);
    Point3D compute_center_of_mass(std::shared_ptr<Solid> solid);
    Matrix3D compute_moment_of_inertia(std::shared_ptr<Solid> solid);
    
    // Intersection and projection
    std::vector<Point3D> intersect_curve_curve(std::shared_ptr<CurveGeometry> curve1,
                                              std::shared_ptr<CurveGeometry> curve2);
    std::vector<std::shared_ptr<CurveGeometry>> intersect_surface_surface(
                                                    std::shared_ptr<SurfaceGeometry> surface1,
                                                    std::shared_ptr<SurfaceGeometry> surface2);
    std::shared_ptr<CurveGeometry> project_curve_on_surface(std::shared_ptr<CurveGeometry> curve,
                                                           std::shared_ptr<SurfaceGeometry> surface);
    
    // Tessellation and discretization
    std::vector<Point3D> tessellate_curve(std::shared_ptr<CurveGeometry> curve, 
                                         Precision tolerance = 1e-6);
    std::vector<std::array<Point3D, 3>> tessellate_face(std::shared_ptr<Face> face,
                                                       Precision tolerance = 1e-6);
    std::vector<std::array<Point3D, 3>> tessellate_solid(std::shared_ptr<Solid> solid,
                                                        Precision tolerance = 1e-6);
    
    // Entity management
    void remove_entity(size_t entity_id);
    void clear_all_entities();
    size_t get_entity_count() const;
    std::vector<std::shared_ptr<TopologyEntity>> get_all_solids() const;
    
    // Memory management and optimization
    void garbage_collect();
    void optimize_model();
    size_t get_memory_usage() const;
    
    // Import/Export integration
#ifdef QCS_OPENCASCADE_ENABLED
    std::shared_ptr<TopologyEntity> import_from_opencascade(const TopoDS_Shape& shape);
    TopoDS_Shape export_to_opencascade(std::shared_ptr<TopologyEntity> entity);
#endif
    
    // Debug and diagnostics
    void print_model_statistics() const;
    bool verify_model_consistency() const;
    std::string generate_model_report() const;

private:
    // Internal helper methods
    void update_entity_relationships(std::shared_ptr<TopologyEntity> entity);
    bool check_geometric_validity(std::shared_ptr<TopologyEntity> entity);
    void apply_healing_operations(std::shared_ptr<TopologyEntity> entity);
    size_t allocate_entity_id() { return ++next_entity_id_; }
    size_t allocate_geometry_id() { return ++next_geometry_id_; }
    size_t allocate_feature_id() { return ++next_feature_id_; }
};

} // namespace qcs::cad