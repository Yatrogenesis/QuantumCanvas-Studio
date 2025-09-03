# QuantumCanvas Studio Color Pipeline Specification
## End-to-End Color Management for Professional Creative Work

### Overview

The QuantumCanvas Studio color pipeline implements a deterministic, high-precision color management system that ensures consistent color reproduction across different devices, media, and export formats while maintaining compatibility with professional printing workflows.

#### Design Principles
- **Deterministic**: Identical inputs always produce identical outputs (bit-exact reproducibility)
- **Professional**: Support for wide-gamut color spaces and high bit depths
- **Consistent**: Same visual appearance across different devices and viewing conditions
- **Efficient**: GPU-accelerated transforms with minimal performance impact
- **Standards-Compliant**: Full ICC profile support and industry standard workflows

---

## Color Space Architecture

### Working Color Space Hierarchy
```
Scene-Referred (Linear)
    ├── Working Space (Display P3, Adobe RGB, etc.)
    ├── Display Space (Monitor profile)
    ├── Output Space (sRGB, CMYK, etc.)
    └── Proof Space (Target printer profile)
```

### Core Color Spaces Support
```cpp
enum class ColorSpace {
    // Standard RGB
    sRGB,                    // IEC 61966-2-1
    DisplayP3,               // Apple Display P3
    AdobeRGB,                // Adobe RGB (1998)
    ProPhotoRGB,             // ROMM RGB
    Rec2020,                 // ITU-R BT.2020
    
    // Print Spaces
    CMYK_GRACoL2013,         // GRACoL 2013 CRPC6
    CMYK_FOGRA51,            // PSO Coated v3 (FOGRA51)
    CMYK_JapanColor2011,     // Japan Color 2011 Coated
    
    // Linear Spaces
    LinearDisplayP3,         // Linear Display P3 (for GPU processing)
    LinearRec2020,           // Linear Rec.2020
    ACES2065_1,              // Academy Color Encoding System
    ACEScct,                 // ACES Color Correction Transform
    
    // Lab/XYZ
    CIELab,                  // L*a*b* color space
    CIEXYZ,                  // CIE XYZ (1931)
    
    // Custom
    CustomICC                // User-provided ICC profile
};
```

---

## Color Pipeline Configuration

### Pipeline Configuration Structure
```cpp
// src/color/color_pipeline_config.hpp
namespace QuantumCanvas::Color {

struct ColorPipelineConfig {
    // Primary color spaces
    ColorSpace working_space = ColorSpace::DisplayP3;
    ColorSpace display_space = ColorSpace::sRGB;
    ColorSpace output_space = ColorSpace::sRGB;
    
    // ICC Profiles
    std::optional<std::string> working_profile_path;
    std::optional<std::string> display_profile_path;
    std::optional<std::string> output_profile_path;
    
    // Rendering intents
    RenderingIntent working_to_display = RenderingIntent::Perceptual;
    RenderingIntent working_to_output = RenderingIntent::RelativeColorimetric;
    
    // Advanced options
    bool black_point_compensation = true;
    bool use_display_compensation = true;
    float adaptation_state = 1.0f;  // Chromatic adaptation state (0-1)
    
    // Precision settings
    ColorPrecision precision = ColorPrecision::Float32;
    bool use_gpu_transforms = true;
    
    // Soft proofing
    struct SoftProofing {
        bool enabled = false;
        std::string target_profile_path;
        RenderingIntent rendering_intent = RenderingIntent::Perceptual;
        bool simulate_paper_color = true;
        bool simulate_black_ink = true;
        float adaptation_luminance = 80.0f;  // cd/m²
    } soft_proofing;
    
    // Gamut warning
    struct GamutWarning {
        bool enabled = false;
        ColorRGBA warning_color{1.0f, 0.0f, 1.0f, 1.0f};  // Magenta
        float threshold = 0.01f;  // ΔE threshold for out-of-gamut
    } gamut_warning;
};

enum class RenderingIntent {
    Perceptual,              // Best for photos
    RelativeColorimetric,    // Best for logos/graphics
    Saturation,              // Best for business graphics
    AbsoluteColorimetric     // Best for proofing
};

enum class ColorPrecision {
    UInt8,      // 8 bits per channel (standard)
    UInt16,     // 16 bits per channel (high precision)
    Float16,    // Half-precision floating point
    Float32     // Full precision floating point
};

}
```

### Configuration Examples

#### Standard sRGB Workflow
```json
{
  "working_space": "sRGB",
  "display_space": "sRGB",
  "output_space": "sRGB",
  "rendering_intent": "relative_colorimetric",
  "precision": "uint8"
}
```

#### Professional Photography Workflow
```json
{
  "working_space": "ProPhotoRGB",
  "display_space": "DisplayP3",
  "output_space": "AdobeRGB",
  "rendering_intent": "perceptual",
  "precision": "float32",
  "black_point_compensation": true,
  "soft_proofing": {
    "enabled": true,
    "target_profile": "profiles/canon_pro_platinum.icc",
    "simulate_paper_color": true
  }
}
```

#### Print Production Workflow
```json
{
  "working_space": "AdobeRGB",
  "display_space": "DisplayP3",
  "output_space": "CMYK_GRACoL2013",
  "rendering_intent": "relative_colorimetric",
  "precision": "uint16",
  "gamut_warning": {
    "enabled": true,
    "threshold": 0.02
  }
}
```

---

## Color Transform Implementation

### Core Color Pipeline Class
```cpp
// src/color/color_pipeline.hpp
namespace QuantumCanvas::Color {

class ColorPipeline {
public:
    explicit ColorPipeline(const ColorPipelineConfig& config);
    ~ColorPipeline();
    
    // Configuration
    void update_config(const ColorPipelineConfig& config);
    const ColorPipelineConfig& get_config() const noexcept;
    
    // Primary color transforms
    ColorRGBAF32 working_to_display(const ColorRGBAF32& color) const;
    ColorRGBAF32 display_to_working(const ColorRGBAF32& color) const;
    ColorRGBAF32 working_to_output(const ColorRGBAF32& color) const;
    
    // Batch transforms (GPU-accelerated)
    void transform_image_working_to_display(const Image& input, Image& output) const;
    void transform_image_working_to_output(const Image& input, Image& output) const;
    
    // Lab color space transforms
    ColorLab working_to_lab(const ColorRGBAF32& color) const;
    ColorRGBAF32 lab_to_working(const ColorLab& color) const;
    
    // Gamut mapping and analysis
    bool is_in_gamut(const ColorRGBAF32& color, ColorSpace target_space) const;
    ColorRGBAF32 map_to_gamut(const ColorRGBAF32& color, ColorSpace target_space,
                             GamutMappingMethod method = GamutMappingMethod::Perceptual) const;
    
    // Soft proofing
    ColorRGBAF32 apply_soft_proof(const ColorRGBAF32& color) const;
    Image apply_soft_proof(const Image& input) const;
    
    // Color difference calculations
    float delta_e_2000(const ColorLab& color1, const ColorLab& color2) const;
    float delta_e_76(const ColorLab& color1, const ColorLab& color2) const;
    
    // Profile management
    void set_custom_profile(const std::string& role, const ICCProfile& profile);
    std::optional<ICCProfile> get_profile(const std::string& role) const;
    
    // Validation and diagnostics
    ValidationResult validate_configuration() const;
    ColorPipelineDiagnostics get_diagnostics() const;
    
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};

// Color types with high precision support
struct ColorRGBAF32 {
    float r, g, b, a;
    
    ColorRGBAF32() : r(0), g(0), b(0), a(1) {}
    ColorRGBAF32(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    
    // Deterministic operations
    ColorRGBAF32 operator+(const ColorRGBAF32& other) const;
    ColorRGBAF32 operator*(float scalar) const;
    
    // Conversion utilities
    ColorRGBAU8 to_uint8() const;
    ColorRGBAU16 to_uint16() const;
    static ColorRGBAF32 from_uint8(const ColorRGBAU8& color);
};

struct ColorLab {
    float L, a, b, alpha;
    
    ColorLab() : L(0), a(0), b(0), alpha(1) {}
    ColorLab(float L, float a, float b, float alpha = 1.0f) : L(L), a(a), b(b), alpha(alpha) {}
};

}
```

### Transform Matrix System
```cpp
// src/color/color_matrix.hpp
namespace QuantumCanvas::Color {

class ColorTransformMatrix {
public:
    // Standard 3x3 color transformation matrix
    using Matrix3x3 = std::array<std::array<float, 3>, 3>;
    
    // Predefined matrices for common transformations
    static Matrix3x3 srgb_to_xyz_d65();
    static Matrix3x3 xyz_d65_to_srgb();
    static Matrix3x3 display_p3_to_xyz_d65();
    static Matrix3x3 xyz_d65_to_display_p3();
    static Matrix3x3 adobe_rgb_to_xyz_d65();
    static Matrix3x3 xyz_d65_to_adobe_rgb();
    
    // Bradford chromatic adaptation matrices
    static Matrix3x3 bradford_adaptation_matrix(const WhitePoint& source,
                                               const WhitePoint& dest);
    
    // Matrix operations
    static Matrix3x3 multiply(const Matrix3x3& a, const Matrix3x3& b);
    static Matrix3x3 inverse(const Matrix3x3& matrix);
    static ColorRGBF32 transform(const ColorRGBF32& color, const Matrix3x3& matrix);
    
    // GPU shader generation
    static std::string generate_transform_shader(const Matrix3x3& matrix);
    
private:
    // Deterministic floating-point operations
    static float deterministic_multiply(float a, float b);
    static float deterministic_add(float a, float b);
};

}
```

### GPU-Accelerated Color Transforms
```cpp
// src/color/gpu_color_transform.hpp
namespace QuantumCanvas::Color {

class GPUColorTransform {
public:
    explicit GPUColorTransform(const RenderingDevice& device);
    ~GPUColorTransform();
    
    // Compile transform shaders
    void compile_transform(const std::string& transform_name,
                          const ColorTransformMatrix::Matrix3x3& matrix);
    
    // Execute transforms on GPU
    void transform_texture(const std::string& transform_name,
                          const GPUTexture& input,
                          GPUTexture& output);
    
    // Batch processing
    void transform_image_batch(const std::string& transform_name,
                              const std::vector<GPUTexture>& inputs,
                              std::vector<GPUTexture>& outputs);
    
    // LUT-based transforms for complex ICC profiles
    void create_3d_lut(const std::string& lut_name,
                      const ICCProfile& source_profile,
                      const ICCProfile& dest_profile,
                      size_t lut_size = 64);
    
    void apply_3d_lut(const std::string& lut_name,
                     const GPUTexture& input,
                     GPUTexture& output);
    
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};

}
```

---

## ICC Profile Support

### ICC Profile Management
```cpp
// src/color/icc_profile.hpp
namespace QuantumCanvas::Color {

class ICCProfile {
public:
    // Profile loading and validation
    static std::unique_ptr<ICCProfile> load_from_file(const std::filesystem::path& path);
    static std::unique_ptr<ICCProfile> load_from_memory(const std::vector<uint8_t>& data);
    
    // Profile information
    std::string description() const;
    std::string manufacturer() const;
    std::string model() const;
    std::string copyright() const;
    ColorSpace color_space() const;
    WhitePoint white_point() const;
    Chromaticities chromaticities() const;
    
    // Transform creation
    std::unique_ptr<ColorTransform> create_transform_to(const ICCProfile& dest_profile,
                                                       RenderingIntent intent) const;
    
    // Gamut analysis
    float gamut_volume() const;
    bool is_matrix_based() const;
    bool supports_rendering_intent(RenderingIntent intent) const;
    
    // Profile validation
    ValidationResult validate() const;
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};

// System profile manager
class SystemProfileManager {
public:
    static SystemProfileManager& instance();
    
    // Standard profiles
    std::shared_ptr<ICCProfile> get_srgb_profile() const;
    std::shared_ptr<ICCProfile> get_display_p3_profile() const;
    std::shared_ptr<ICCProfile> get_adobe_rgb_profile() const;
    std::shared_ptr<ICCProfile> get_prophoto_rgb_profile() const;
    
    // System display profile
    std::shared_ptr<ICCProfile> get_display_profile(int display_index = 0) const;
    
    // Profile cache
    std::shared_ptr<ICCProfile> get_cached_profile(const std::string& path) const;
    void cache_profile(const std::string& key, std::shared_ptr<ICCProfile> profile);
    
private:
    SystemProfileManager();
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};

}
```

### Custom Transform Implementation
```cpp
// src/color/color_transform.hpp
namespace QuantumCanvas::Color {

class ColorTransform {
public:
    virtual ~ColorTransform() = default;
    
    // Transform single colors
    virtual ColorRGBAF32 transform(const ColorRGBAF32& color) const = 0;
    virtual ColorLab transform_to_lab(const ColorRGBAF32& color) const = 0;
    virtual ColorRGBAF32 transform_from_lab(const ColorLab& color) const = 0;
    
    // Batch transforms
    virtual void transform_array(const ColorRGBAF32* input, ColorRGBAF32* output, 
                               size_t count) const = 0;
    
    // Transform properties
    virtual bool is_identity() const = 0;
    virtual bool is_invertible() const = 0;
    virtual std::unique_ptr<ColorTransform> create_inverse() const = 0;
    
    // Performance characteristics
    virtual bool supports_gpu_acceleration() const = 0;
    virtual size_t estimated_lookup_table_size() const = 0;
};

// Matrix-based transform (fast)
class MatrixColorTransform : public ColorTransform {
public:
    MatrixColorTransform(const ColorTransformMatrix::Matrix3x3& matrix);
    
    ColorRGBAF32 transform(const ColorRGBAF32& color) const override;
    void transform_array(const ColorRGBAF32* input, ColorRGBAF32* output, 
                        size_t count) const override;
    
    bool is_identity() const override;
    std::unique_ptr<ColorTransform> create_inverse() const override;
    bool supports_gpu_acceleration() const override { return true; }
    
private:
    ColorTransformMatrix::Matrix3x3 matrix_;
    ColorTransformMatrix::Matrix3x3 inverse_matrix_;
    bool is_identity_;
};

// LUT-based transform (high accuracy)
class LUTColorTransform : public ColorTransform {
public:
    LUTColorTransform(const ICCProfile& source_profile,
                     const ICCProfile& dest_profile,
                     RenderingIntent intent,
                     size_t lut_size = 33);
    
    ColorRGBAF32 transform(const ColorRGBAF32& color) const override;
    void transform_array(const ColorRGBAF32* input, ColorRGBAF32* output, 
                        size_t count) const override;
    
    bool supports_gpu_acceleration() const override { return true; }
    size_t estimated_lookup_table_size() const override;
    
private:
    std::vector<ColorRGBAF32> lut_data_;
    size_t lut_size_;
    
    ColorRGBAF32 interpolate_trilinear(float r, float g, float b) const;
};

}
```

---

## Deterministic Color Operations

### Rounding and Precision Control
```cpp
// src/color/deterministic_math.hpp
namespace QuantumCanvas::Color {

// IEEE 754 compliant operations for deterministic results
class DeterministicColorMath {
public:
    // Precise floating-point operations
    static float add(float a, float b) noexcept;
    static float multiply(float a, float b) noexcept;
    static float divide(float a, float b) noexcept;
    
    // Gamma correction (deterministic)
    static float linear_to_srgb(float linear) noexcept;
    static float srgb_to_linear(float srgb) noexcept;
    
    // Color space conversions
    static ColorLab rgb_to_lab(const ColorRGBF32& rgb, const WhitePoint& white_point) noexcept;
    static ColorRGBF32 lab_to_rgb(const ColorLab& lab, const WhitePoint& white_point) noexcept;
    
    // Quantization with controlled rounding
    static uint8_t float_to_uint8(float value) noexcept;
    static uint16_t float_to_uint16(float value) noexcept;
    static float uint8_to_float(uint8_t value) noexcept;
    static float uint16_to_float(uint16_t value) noexcept;
    
    // Hash functions for testing reproducibility
    static uint64_t color_hash(const ColorRGBAF32& color) noexcept;
    static uint64_t image_hash(const Image& image) noexcept;
};

// Controlled rounding modes
enum class RoundingMode {
    ToNearest,      // Round to nearest, ties to even (IEEE 754 default)
    TowardZero,     // Truncate
    TowardPositive, // Ceiling
    TowardNegative  // Floor
};

}
```

### Color Validation and Testing
```cpp
// src/color/color_validation.hpp
namespace QuantumCanvas::Color {

struct ColorAccuracyTest {
    ColorRGBAF32 input_color;
    ColorSpace source_space;
    ColorSpace target_space;
    ColorRGBAF32 expected_output;
    float max_delta_e = 0.5f;  // Maximum acceptable color difference
};

class ColorPipelineValidator {
public:
    // Standard test suites
    static std::vector<ColorAccuracyTest> create_srgb_test_suite();
    static std::vector<ColorAccuracyTest> create_display_p3_test_suite();
    static std::vector<ColorAccuracyTest> create_adobe_rgb_test_suite();
    
    // Run validation tests
    ValidationResult validate_pipeline(const ColorPipeline& pipeline,
                                     const std::vector<ColorAccuracyTest>& tests);
    
    // Reproducibility testing
    struct ReproducibilityResult {
        bool is_deterministic;
        std::vector<std::string> inconsistencies;
        uint64_t reference_hash;
        std::vector<uint64_t> test_hashes;
    };
    
    ReproducibilityResult test_reproducibility(const ColorPipeline& pipeline,
                                             const std::vector<ColorRGBAF32>& test_colors,
                                             int iterations = 100);
    
    // Performance benchmarks
    struct PerformanceBenchmark {
        std::chrono::nanoseconds single_color_transform_time;
        std::chrono::nanoseconds batch_transform_time_per_pixel;
        float gpu_acceleration_factor;
        size_t memory_usage_bytes;
    };
    
    PerformanceBenchmark benchmark_pipeline(const ColorPipeline& pipeline,
                                          const Image& test_image);
};

}
```

---

## Advanced Color Features

### Gamut Mapping Algorithms
```cpp
// src/color/gamut_mapping.hpp
namespace QuantumCanvas::Color {

enum class GamutMappingMethod {
    Clipping,           // Simple clipping to gamut boundary
    Perceptual,         // Perceptually uniform compression
    Saturation,         // Preserve saturation, compress lightness
    HPMIN,              // Hue-preserving minimum ΔE
    SGCK,               // Spatial gamut clipping with knee function
    Custom              // User-defined mapping function
};

class GamutMapper {
public:
    // Primary mapping function
    static ColorLab map_to_gamut(const ColorLab& input_color,
                                const ColorSpace& target_space,
                                GamutMappingMethod method);
    
    // Advanced mapping with custom parameters
    struct MappingParameters {
        float knee_factor = 0.7f;           // Soft clipping threshold
        float compression_factor = 0.8f;    // Compression strength
        bool preserve_luminance = true;     // Preserve lightness when possible
        bool preserve_hue = true;           // Strict hue preservation
    };
    
    static ColorLab map_to_gamut_advanced(const ColorLab& input_color,
                                         const ColorSpace& target_space,
                                         GamutMappingMethod method,
                                         const MappingParameters& params);
    
    // Gamut analysis
    static bool is_in_gamut(const ColorLab& color, const ColorSpace& space);
    static float gamut_distance(const ColorLab& color, const ColorSpace& space);
    static ColorLab find_gamut_boundary(const ColorLab& color, const ColorSpace& space);
    
    // Custom mapping functions
    using CustomMappingFunction = std::function<ColorLab(const ColorLab&)>;
    static void register_custom_mapping(const std::string& name, CustomMappingFunction func);
};

}
```

### Soft Proofing Implementation
```cpp
// src/color/soft_proofing.hpp
namespace QuantumCanvas::Color {

class SoftProofRenderer {
public:
    struct ProofingConfig {
        std::shared_ptr<ICCProfile> target_profile;
        RenderingIntent rendering_intent = RenderingIntent::Perceptual;
        bool simulate_paper_color = true;
        bool simulate_black_ink = true;
        float adaptation_luminance = 80.0f;
        
        // Advanced options
        bool compensate_for_flare = true;
        float flare_percentage = 0.5f;
        float surround_gamma = 1.0f;
    };
    
    explicit SoftProofRenderer(const ProofingConfig& config);
    
    // Apply soft proofing to colors
    ColorRGBAF32 apply_proof(const ColorRGBAF32& color) const;
    
    // Apply soft proofing to images
    Image apply_proof(const Image& input) const;
    void apply_proof_gpu(const GPUTexture& input, GPUTexture& output) const;
    
    // Gamut warning overlay
    Image generate_gamut_warning(const Image& input, 
                                const ColorRGBAF32& warning_color) const;
    
    // Paper color simulation
    ColorRGBAF32 simulate_paper_white() const;
    ColorRGBAF32 simulate_maximum_black() const;
    
    // Update configuration
    void update_config(const ProofingConfig& config);
    
private:
    ProofingConfig config_;
    std::unique_ptr<ColorTransform> proof_transform_;
    std::unique_ptr<ColorTransform> inverse_proof_transform_;
    
    void rebuild_transforms();
};

}
```

---

## Integration with Document System

### Color-Managed Document
```cpp
// src/document/color_managed_document.hpp
namespace QuantumCanvas::Document {

class ColorManagedDocument : public DocumentModel {
public:
    // Color pipeline integration
    void set_color_pipeline(std::unique_ptr<Color::ColorPipeline> pipeline);
    const Color::ColorPipeline& color_pipeline() const;
    
    // Document color space
    void set_working_color_space(Color::ColorSpace space);
    Color::ColorSpace working_color_space() const;
    
    // Color-aware operations
    void apply_color_transform_to_layer(LayerId layer_id, 
                                       const Color::ColorTransform& transform);
    void convert_document_color_space(Color::ColorSpace target_space);
    
    // Profile embedding
    void embed_color_profile(const std::string& role, 
                           const Color::ICCProfile& profile);
    std::optional<Color::ICCProfile> get_embedded_profile(const std::string& role) const;
    
    // Validation
    std::vector<Color::ValidationIssue> validate_color_setup() const;
    
protected:
    void serialize_color_info(nlohmann::json& document_data) const override;
    void deserialize_color_info(const nlohmann::json& document_data) override;
    
private:
    std::unique_ptr<Color::ColorPipeline> color_pipeline_;
    std::unordered_map<std::string, Color::ICCProfile> embedded_profiles_;
};

}
```

This comprehensive color pipeline specification ensures professional-grade color management throughout the entire QuantumCanvas Studio application, from initial color input through final output, while maintaining deterministic, reproducible results across all platforms and workflows.