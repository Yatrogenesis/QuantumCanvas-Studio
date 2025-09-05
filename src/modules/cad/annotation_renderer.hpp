/**
 * @file annotation_renderer.hpp
 * @brief Technical annotation and dimensioning system for CAD drawings
 * @version 1.0.0
 * @date 2025-09-03
 * 
 * QuantumCanvas Studio - CAD Graphics Engine
 * Professional annotation rendering with standards compliance (ISO, ANSI, DIN)
 */

#pragma once

#include "cad_types.hpp"
#include "cad_common.hpp"
#include "precision_renderer.hpp"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace qcs::cad {

// Forward declarations
class DimensionStyle;
class TextStyle;
class AnnotationEntity;
class DimensionEntity;

// =============================================================================
// Annotation Standards and Styles
// =============================================================================

/// Technical drawing standards
enum class DrawingStandard {
    ISO,        ///< ISO 128, ISO 5455
    ANSI,       ///< ANSI Y14.5
    DIN,        ///< DIN 406
    JIS,        ///< JIS B 0001
    BSI,        ///< BS 8888
    Custom      ///< User-defined standard
};

/// Dimension type classification
enum class DimensionType {
    Linear,         ///< Linear dimension (horizontal, vertical, aligned)
    Angular,        ///< Angular dimension
    Radial,         ///< Radius dimension
    Diametral,      ///< Diameter dimension
    Arc,            ///< Arc length dimension
    Ordinate,       ///< Ordinate/baseline dimension
    Chain,          ///< Chain dimension
    Continuous,     ///< Continuous dimension
    Leader,         ///< Leader with text
    Tolerance,      ///< Geometric tolerance
    Surface,        ///< Surface texture symbol
    Weld,           ///< Welding symbol
    Feature         ///< Feature control frame
};

/// Arrow head types for dimensions
enum class ArrowheadType {
    None,
    ClosedFilled,       ///< Filled closed arrowhead
    ClosedBlank,        ///< Blank closed arrowhead
    Open,               ///< Open arrowhead
    Architectural,      ///< Architectural tick
    Oblique,            ///< Oblique stroke
    Dot,                ///< Dot
    Origin,             ///< Origin indicator
    Right,              ///< Right angle
    Open30,             ///< 30-degree open arrow
    Closed30,           ///< 30-degree closed arrow
    DatumTriangle,      ///< Datum triangle
    IntegralSymbol,     ///< Integral symbol
    Custom              ///< User-defined
};

/// Text alignment options
enum class TextAlignment {
    Left,
    Center,
    Right,
    Justified,
    Middle,
    Top,
    Bottom,
    Baseline
};

/// Text direction for annotations
enum class TextDirection {
    Horizontal,     ///< Always horizontal
    Aligned,        ///< Aligned with dimension line
    ISO,            ///< ISO standard alignment
    Perpendicular   ///< Perpendicular to dimension line
};

// =============================================================================
// Text Style Management
// =============================================================================

/// Comprehensive text style for annotations
class TextStyle {
private:
    std::string name_;
    std::string font_name_;
    Precision font_size_;
    Precision width_factor_;
    Precision oblique_angle_;
    bool is_bold_;
    bool is_italic_;
    bool is_underlined_;
    Color text_color_;
    
    // Text-specific formatting
    TextAlignment horizontal_alignment_;
    TextAlignment vertical_alignment_;
    TextDirection text_direction_;
    Precision line_spacing_factor_;
    
    // Advanced typography
    bool use_true_type_;
    bool enable_kerning_;
    std::string character_encoding_;

public:
    TextStyle(const std::string& name = "Standard");
    
    // Basic properties
    const std::string& get_name() const { return name_; }
    const std::string& get_font_name() const { return font_name_; }
    Precision get_font_size() const { return font_size_; }
    Precision get_width_factor() const { return width_factor_; }
    Precision get_oblique_angle() const { return oblique_angle_; }
    const Color& get_text_color() const { return text_color_; }
    
    void set_font_name(const std::string& font) { font_name_ = font; }
    void set_font_size(Precision size) { font_size_ = size; }
    void set_width_factor(Precision factor) { width_factor_ = factor; }
    void set_oblique_angle(Precision angle) { oblique_angle_ = angle; }
    void set_text_color(const Color& color) { text_color_ = color; }
    
    // Style properties
    bool is_bold() const { return is_bold_; }
    bool is_italic() const { return is_italic_; }
    bool is_underlined() const { return is_underlined_; }
    
    void set_bold(bool bold) { is_bold_ = bold; }
    void set_italic(bool italic) { is_italic_ = italic; }
    void set_underlined(bool underlined) { is_underlined_ = underlined; }
    
    // Alignment and direction
    TextAlignment get_horizontal_alignment() const { return horizontal_alignment_; }
    TextAlignment get_vertical_alignment() const { return vertical_alignment_; }
    TextDirection get_text_direction() const { return text_direction_; }
    
    void set_horizontal_alignment(TextAlignment alignment) { horizontal_alignment_ = alignment; }
    void set_vertical_alignment(TextAlignment alignment) { vertical_alignment_ = alignment; }
    void set_text_direction(TextDirection direction) { text_direction_ = direction; }
    
    // Typography
    Precision get_line_spacing_factor() const { return line_spacing_factor_; }
    void set_line_spacing_factor(Precision factor) { line_spacing_factor_ = factor; }
    
    // Standard text styles
    static TextStyle standard_text();
    static TextStyle dimension_text();
    static TextStyle title_text();
    static TextStyle note_text();
    static TextStyle symbol_text();
};

// =============================================================================
// Dimension Style Management
// =============================================================================

/// Comprehensive dimension style with standards compliance
class DimensionStyle {
private:
    std::string name_;
    DrawingStandard standard_;
    TextStyle text_style_;
    
    // Dimension line properties
    Color dimension_line_color_;
    Precision dimension_line_weight_;
    std::string dimension_line_type_;
    bool suppress_dim_line1_;
    bool suppress_dim_line2_;
    
    // Extension line properties
    Color extension_line_color_;
    Precision extension_line_weight_;
    std::string extension_line_type_;
    bool suppress_ext_line1_;
    bool suppress_ext_line2_;
    Precision extension_line_extension_;
    Precision extension_line_offset_;
    
    // Arrowhead properties
    ArrowheadType arrowhead1_type_;
    ArrowheadType arrowhead2_type_;
    Precision arrowhead_size_;
    
    // Text placement and formatting
    bool text_inside_;
    bool text_above_;
    Precision text_gap_;
    Precision text_height_;
    bool force_text_inside_;
    bool text_movement_rule_;
    
    // Tolerance and precision
    int decimal_precision_;
    int angular_precision_;
    bool suppress_leading_zeros_;
    bool suppress_trailing_zeros_;
    std::string unit_suffix_;
    Precision unit_scale_factor_;
    
    // Measurement properties
    Precision measurement_scale_;
    bool use_alternate_units_;
    Precision alternate_unit_scale_;
    std::string alternate_unit_suffix_;
    int alternate_precision_;

public:
    DimensionStyle(const std::string& name = "Standard", DrawingStandard std = DrawingStandard::ISO);
    
    // Basic properties
    const std::string& get_name() const { return name_; }
    DrawingStandard get_standard() const { return standard_; }
    void set_standard(DrawingStandard std) { standard_ = std; }
    
    // Text style
    const TextStyle& get_text_style() const { return text_style_; }
    TextStyle& get_text_style() { return text_style_; }
    void set_text_style(const TextStyle& style) { text_style_ = style; }
    
    // Dimension line properties
    const Color& get_dimension_line_color() const { return dimension_line_color_; }
    Precision get_dimension_line_weight() const { return dimension_line_weight_; }
    bool is_dim_line_suppressed(int line_number) const;
    
    void set_dimension_line_color(const Color& color) { dimension_line_color_ = color; }
    void set_dimension_line_weight(Precision weight) { dimension_line_weight_ = weight; }
    void suppress_dim_line(int line_number, bool suppress);
    
    // Extension line properties
    const Color& get_extension_line_color() const { return extension_line_color_; }
    Precision get_extension_line_weight() const { return extension_line_weight_; }
    Precision get_extension_line_extension() const { return extension_line_extension_; }
    Precision get_extension_line_offset() const { return extension_line_offset_; }
    bool is_ext_line_suppressed(int line_number) const;
    
    void set_extension_line_properties(const Color& color, Precision weight, 
                                     Precision extension, Precision offset);
    void suppress_ext_line(int line_number, bool suppress);
    
    // Arrowhead properties
    ArrowheadType get_arrowhead_type(int arrow_number) const;
    Precision get_arrowhead_size() const { return arrowhead_size_; }
    
    void set_arrowhead_type(int arrow_number, ArrowheadType type);
    void set_arrowhead_size(Precision size) { arrowhead_size_ = size; }
    
    // Text placement
    bool is_text_inside() const { return text_inside_; }
    bool is_text_above() const { return text_above_; }
    Precision get_text_gap() const { return text_gap_; }
    Precision get_text_height() const { return text_height_; }
    
    void set_text_placement(bool inside, bool above, Precision gap);
    void set_text_height(Precision height) { text_height_ = height; }
    
    // Precision and formatting
    int get_decimal_precision() const { return decimal_precision_; }
    int get_angular_precision() const { return angular_precision_; }
    const std::string& get_unit_suffix() const { return unit_suffix_; }
    
    void set_decimal_precision(int precision) { decimal_precision_ = precision; }
    void set_angular_precision(int precision) { angular_precision_ = precision; }
    void set_unit_suffix(const std::string& suffix) { unit_suffix_ = suffix; }
    
    // Zero suppression
    bool suppress_leading_zeros() const { return suppress_leading_zeros_; }
    bool suppress_trailing_zeros() const { return suppress_trailing_zeros_; }
    void set_zero_suppression(bool leading, bool trailing);
    
    // Standard dimension styles
    static DimensionStyle iso_standard();
    static DimensionStyle ansi_standard();
    static DimensionStyle din_standard();
    static DimensionStyle architectural();
    static DimensionStyle mechanical();
};

// =============================================================================
// Annotation Entities
// =============================================================================

/// Base class for all annotation entities
class AnnotationEntity : public CADEntity {
protected:
    std::string text_style_name_;
    std::string layer_name_;
    bool is_associative_;
    std::vector<EntityID> associated_entities_;
    
    // Display properties
    Precision scale_factor_;
    bool visible_in_layout_;
    bool plottable_;

public:
    AnnotationEntity(EntityID id = INVALID_ENTITY_ID);
    virtual ~AnnotationEntity() = default;
    
    // Style management
    const std::string& get_text_style_name() const { return text_style_name_; }
    void set_text_style_name(const std::string& name) { text_style_name_ = name; }
    
    // Associativity
    bool is_associative() const { return is_associative_; }
    void set_associative(bool associative) { is_associative_ = associative; }
    
    const std::vector<EntityID>& get_associated_entities() const { return associated_entities_; }
    void add_associated_entity(EntityID id) { associated_entities_.push_back(id); }
    void remove_associated_entity(EntityID id);
    void clear_associations() { associated_entities_.clear(); }
    
    // Display properties
    Precision get_scale_factor() const { return scale_factor_; }
    void set_scale_factor(Precision scale) { scale_factor_ = scale; }
    
    bool is_visible_in_layout() const { return visible_in_layout_; }
    void set_visible_in_layout(bool visible) { visible_in_layout_ = visible; }
    
    // Virtual methods for annotation-specific behavior
    virtual void update_from_associations() = 0;
    virtual std::string get_display_text() const = 0;
    virtual void regenerate() = 0;
};

/// Text annotation entity
class TextAnnotation : public AnnotationEntity {
private:
    std::string text_content_;
    Point3D insertion_point_;
    Vector3D text_direction_;
    TextAlignment alignment_;
    Precision rotation_angle_;
    Precision height_;
    bool is_mirrored_;

public:
    TextAnnotation(const std::string& text, const Point3D& position, EntityID id = INVALID_ENTITY_ID);
    
    // Text properties
    const std::string& get_text_content() const { return text_content_; }
    void set_text_content(const std::string& text) { text_content_ = text; }
    
    const Point3D& get_insertion_point() const { return insertion_point_; }
    void set_insertion_point(const Point3D& point) { insertion_point_ = point; }
    
    const Vector3D& get_text_direction() const { return text_direction_; }
    void set_text_direction(const Vector3D& direction) { text_direction_ = direction; }
    
    Precision get_rotation_angle() const { return rotation_angle_; }
    void set_rotation_angle(Precision angle) { rotation_angle_ = angle; }
    
    Precision get_height() const { return height_; }
    void set_height(Precision height) { height_ = height; }
    
    // Alignment
    TextAlignment get_alignment() const { return alignment_; }
    void set_alignment(TextAlignment alignment) { alignment_ = alignment; }
    
    // Mirroring
    bool is_mirrored() const { return is_mirrored_; }
    void set_mirrored(bool mirrored) { is_mirrored_ = mirrored; }
    
    // Inherited methods
    BoundingBox3D calculate_bounds() const override;
    std::unique_ptr<CADEntity> clone() const override;
    void update_from_associations() override {}
    std::string get_display_text() const override { return text_content_; }
    void regenerate() override;
};

/// Linear dimension entity
class LinearDimension : public AnnotationEntity {
private:
    Point3D origin_;
    Point3D definition_point1_;
    Point3D definition_point2_;
    Point3D text_position_;
    std::string dimension_style_name_;
    
    // Computed values
    Precision measured_value_;
    std::string dimension_text_;
    bool text_override_;

public:
    LinearDimension(const Point3D& p1, const Point3D& p2, const Point3D& text_pos, 
                   EntityID id = INVALID_ENTITY_ID);
    
    // Definition points
    const Point3D& get_origin() const { return origin_; }
    const Point3D& get_definition_point1() const { return definition_point1_; }
    const Point3D& get_definition_point2() const { return definition_point2_; }
    const Point3D& get_text_position() const { return text_position_; }
    
    void set_origin(const Point3D& origin) { origin_ = origin; }
    void set_definition_point1(const Point3D& point) { definition_point1_ = point; }
    void set_definition_point2(const Point3D& point) { definition_point2_ = point; }
    void set_text_position(const Point3D& position) { text_position_ = position; }
    
    // Dimension style
    const std::string& get_dimension_style_name() const { return dimension_style_name_; }
    void set_dimension_style_name(const std::string& name) { dimension_style_name_ = name; }
    
    // Measured value and text
    Precision get_measured_value() const { return measured_value_; }
    const std::string& get_dimension_text() const { return dimension_text_; }
    void set_dimension_text_override(const std::string& text);
    void clear_text_override();
    
    // Inherited methods
    BoundingBox3D calculate_bounds() const override;
    std::unique_ptr<CADEntity> clone() const override;
    void update_from_associations() override;
    std::string get_display_text() const override { return dimension_text_; }
    void regenerate() override;

private:
    void calculate_measured_value();
    void format_dimension_text();
};

/// Angular dimension entity
class AngularDimension : public AnnotationEntity {
private:
    Point3D center_point_;
    Point3D start_point_;
    Point3D end_point_;
    Point3D arc_position_;
    std::string dimension_style_name_;
    
    // Computed values
    Precision measured_angle_;
    std::string dimension_text_;

public:
    AngularDimension(const Point3D& center, const Point3D& start, const Point3D& end,
                    const Point3D& arc_pos, EntityID id = INVALID_ENTITY_ID);
    
    // Definition points
    const Point3D& get_center_point() const { return center_point_; }
    const Point3D& get_start_point() const { return start_point_; }
    const Point3D& get_end_point() const { return end_point_; }
    const Point3D& get_arc_position() const { return arc_position_; }
    
    void set_center_point(const Point3D& center) { center_point_ = center; }
    void set_start_point(const Point3D& start) { start_point_ = start; }
    void set_end_point(const Point3D& end) { end_point_ = end; }
    void set_arc_position(const Point3D& arc_pos) { arc_position_ = arc_pos; }
    
    // Measured angle
    Precision get_measured_angle() const { return measured_angle_; }
    const std::string& get_dimension_text() const { return dimension_text_; }
    
    // Inherited methods
    BoundingBox3D calculate_bounds() const override;
    std::unique_ptr<CADEntity> clone() const override;
    void update_from_associations() override;
    std::string get_display_text() const override { return dimension_text_; }
    void regenerate() override;

private:
    void calculate_measured_angle();
    void format_dimension_text();
};

/// Radial dimension entity (radius/diameter)
class RadialDimension : public AnnotationEntity {
private:
    Point3D center_point_;
    Point3D definition_point_;
    Point3D leader_end_point_;
    bool is_diameter_;  // true for diameter, false for radius
    std::string dimension_style_name_;
    
    // Computed values
    Precision measured_value_;
    std::string dimension_text_;

public:
    RadialDimension(const Point3D& center, const Point3D& def_point, 
                   const Point3D& leader_end, bool diameter = false,
                   EntityID id = INVALID_ENTITY_ID);
    
    // Definition points
    const Point3D& get_center_point() const { return center_point_; }
    const Point3D& get_definition_point() const { return definition_point_; }
    const Point3D& get_leader_end_point() const { return leader_end_point_; }
    
    void set_center_point(const Point3D& center) { center_point_ = center; }
    void set_definition_point(const Point3D& def_point) { definition_point_ = def_point; }
    void set_leader_end_point(const Point3D& leader_end) { leader_end_point_ = leader_end; }
    
    // Dimension type
    bool is_diameter() const { return is_diameter_; }
    void set_diameter(bool diameter) { is_diameter_ = diameter; }
    
    // Measured value
    Precision get_measured_value() const { return measured_value_; }
    const std::string& get_dimension_text() const { return dimension_text_; }
    
    // Inherited methods
    BoundingBox3D calculate_bounds() const override;
    std::unique_ptr<CADEntity> clone() const override;
    void update_from_associations() override;
    std::string get_display_text() const override { return dimension_text_; }
    void regenerate() override;

private:
    void calculate_measured_value();
    void format_dimension_text();
};

// =============================================================================
// Annotation Renderer
// =============================================================================

/// Main annotation rendering system
class AnnotationRenderer {
private:
    std::shared_ptr<PrecisionRenderContext> render_context_;
    
    // Style management
    std::unordered_map<std::string, TextStyle> text_styles_;
    std::unordered_map<std::string, DimensionStyle> dimension_styles_;
    
    // Current rendering state
    std::string current_text_style_;
    std::string current_dimension_style_;
    DrawingStandard current_standard_;
    Precision annotation_scale_;
    
    // Rendering settings
    bool render_background_mask_;
    bool render_dimension_lines_;
    bool render_extension_lines_;
    bool render_arrowheads_;
    bool render_text_;
    
    // Performance optimization
    bool enable_text_caching_;
    std::unordered_map<size_t, std::vector<Point3D>> text_glyph_cache_;

public:
    AnnotationRenderer(std::shared_ptr<PrecisionRenderContext> context);
    ~AnnotationRenderer();
    
    // Context access
    PrecisionRenderContext* get_render_context() { return render_context_.get(); }
    
    // Style management
    void add_text_style(const TextStyle& style);
    void add_dimension_style(const DimensionStyle& style);
    
    const TextStyle* get_text_style(const std::string& name) const;
    const DimensionStyle* get_dimension_style(const std::string& name) const;
    
    void set_current_text_style(const std::string& name);
    void set_current_dimension_style(const std::string& name);
    void set_drawing_standard(DrawingStandard standard);
    
    // Rendering settings
    void set_annotation_scale(Precision scale) { annotation_scale_ = scale; }
    Precision get_annotation_scale() const { return annotation_scale_; }
    
    void set_render_flags(bool background_mask, bool dim_lines, bool ext_lines,
                         bool arrowheads, bool text);
    
    // High-level annotation rendering
    void render_annotation(const AnnotationEntity& annotation);
    void render_annotations(const std::vector<std::shared_ptr<AnnotationEntity>>& annotations);
    
    // Specific annotation rendering
    void render_text_annotation(const TextAnnotation& text);
    void render_linear_dimension(const LinearDimension& dimension);
    void render_angular_dimension(const AngularDimension& dimension);
    void render_radial_dimension(const RadialDimension& dimension);
    
    // Primitive rendering components
    void render_dimension_line(const Point3D& start, const Point3D& end, 
                              const DimensionStyle& style);
    void render_extension_line(const Point3D& start, const Point3D& end,
                              const DimensionStyle& style);
    void render_arrowhead(const Point3D& tip, const Vector3D& direction,
                         ArrowheadType type, Precision size);
    void render_dimension_text(const std::string& text, const Point3D& position,
                              const Vector3D& direction, const TextStyle& style);
    
    // Leader and callout rendering
    void render_leader(const std::vector<Point3D>& points, const std::string& text,
                      ArrowheadType arrow_type = ArrowheadType::ClosedFilled);
    void render_callout(const Point3D& target, const Point3D& text_position,
                       const std::string& text, bool with_border = true);
    
    // Symbol rendering
    void render_datum_symbol(const Point3D& position, const std::string& label,
                           const Vector3D& direction = Vector3D::UnitX());
    void render_feature_control_frame(const Point3D& position, 
                                     const std::vector<std::string>& symbols);
    void render_surface_finish_symbol(const Point3D& position, const std::string& specification);
    void render_welding_symbol(const Point3D& position, const std::string& specification);
    
    // Utility rendering
    void render_centerline(const Point3D& start, const Point3D& end, 
                          Precision extension = 2.0);
    void render_construction_line(const Point3D& start, const Point3D& end,
                                 const Color& color = Color(0.5, 0.5, 0.5, 0.5));
    
    // Text measurement and layout
    BoundingBox2D measure_text(const std::string& text, const TextStyle& style) const;
    std::vector<std::string> wrap_text(const std::string& text, Precision max_width,
                                      const TextStyle& style) const;
    Point3D calculate_text_insertion_point(const std::string& text, const Point3D& reference,
                                          TextAlignment h_align, TextAlignment v_align,
                                          const TextStyle& style) const;
    
    // Annotation placement optimization
    Point3D find_optimal_text_placement(const LinearDimension& dimension) const;
    Point3D find_optimal_arc_text_placement(const AngularDimension& dimension) const;
    std::vector<Point3D> calculate_leader_path(const Point3D& from, const Point3D& to,
                                              bool avoid_entities = true) const;
    
    // Standards compliance
    void apply_standard_formatting(DimensionStyle& style, DrawingStandard standard) const;
    bool validate_annotation_standards(const AnnotationEntity& annotation) const;
    std::vector<std::string> get_standards_violations(const AnnotationEntity& annotation) const;
    
    // Performance and caching
    void enable_text_caching(bool enable) { enable_text_caching_ = enable; }
    void clear_text_cache();
    size_t get_cache_memory_usage() const;
    
    // Export and documentation
    void export_annotation_styles(const std::string& filename) const;
    bool import_annotation_styles(const std::string& filename);
    void generate_standards_report(const std::string& filename) const;
    
    // Static utility functions
    static std::string format_dimension_value(Precision value, int precision, 
                                            const std::string& suffix = "");
    static std::string format_angular_value(Precision angle_radians, int precision);
    static std::string format_tolerance_value(Precision nominal, Precision plus, 
                                            Precision minus, int precision);
    static ArrowheadType standard_arrowhead_type(DrawingStandard standard);
    static std::vector<std::string> get_available_standards();

private:
    // Internal rendering helpers
    void render_text_with_background_mask(const std::string& text, const Point3D& position,
                                         const TextStyle& style);
    void render_multiline_text(const std::vector<std::string>& lines, const Point3D& position,
                              const TextStyle& style);
    
    // Arrow and symbol generation
    std::vector<Point3D> generate_arrowhead_geometry(ArrowheadType type, Precision size);
    std::vector<Point3D> generate_datum_triangle(Precision size);
    std::vector<Point3D> generate_center_mark(Precision size);
    
    // Dimension calculation helpers
    void calculate_dimension_geometry(const LinearDimension& dim,
                                    Point3D& dim_line_start, Point3D& dim_line_end,
                                    Point3D& ext_line1_start, Point3D& ext_line1_end,
                                    Point3D& ext_line2_start, Point3D& ext_line2_end) const;
    void calculate_angular_dimension_geometry(const AngularDimension& dim,
                                            std::vector<Point3D>& arc_points,
                                            Point3D& start_ray_end, Point3D& end_ray_end) const;
    
    // Text layout and formatting
    std::vector<std::string> split_text_into_lines(const std::string& text) const;
    Point3D adjust_text_position_for_alignment(const Point3D& base_position,
                                              const BoundingBox2D& text_bounds,
                                              TextAlignment h_align, TextAlignment v_align) const;
};

} // namespace qcs::cad