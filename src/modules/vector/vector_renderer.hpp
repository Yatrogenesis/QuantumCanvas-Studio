#pragma once

#include "../../core/rendering/rendering_engine.hpp"
#include <vector>
#include <memory>
#include <array>

namespace QuantumCanvas::Vector {

// Forward declarations
class VectorPath;
class VectorObject;
class GPUTessellator;

// Vector rendering configuration
struct VectorRenderConfig {
    // Anti-aliasing settings
    bool enableMSAA = true;
    uint32_t msaaSamples = 8;           // 8x MSAA for smooth curves
    bool enableSuperSampling = true;
    float superSamplingFactor = 2.0f;
    
    // Tessellation settings
    float tessellationTolerance = 0.25f; // Maximum error in pixels
    uint32_t maxTessellationDepth = 8;
    bool adaptiveTessellation = true;
    
    // Quality settings
    bool enableSubpixelAccuracy = true;
    bool enableGammaCorrection = true;
    float gamma = 2.2f;
    
    // Performance settings
    bool enableGPUTessellation = true;
    bool enableBatching = true;
    uint32_t maxBatchSize = 1024;
    
    // Curve quality
    enum class CurveQuality {
        Low,     // Fast rendering, lower quality
        Medium,  // Balanced quality/performance
        High,    // High quality, slower rendering
        Ultra    // Maximum quality for printing
    };
    CurveQuality curveQuality = CurveQuality::High;
};

// Vector primitive types
struct VectorVertex {
    std::array<float, 2> position;     // x, y
    std::array<float, 2> texCoord;     // u, v for gradients
    std::array<float, 4> color;        // r, g, b, a
    float coverage;                    // Anti-aliasing coverage
    
    bool operator==(const VectorVertex& other) const;
};

// Tessellated path data
struct TessellatedPath {
    std::vector<VectorVertex> vertices;
    std::vector<uint32_t> indices;
    std::array<float, 4> bounds;       // min_x, min_y, max_x, max_y
    uint32_t triangleCount;
    bool isConvex;
    
    // GPU resources
    Rendering::ResourceId vertexBufferId = 0;
    Rendering::ResourceId indexBufferId = 0;
    bool isUploaded = false;
};

// Fill styles for vector objects
struct VectorFillStyle {
    enum class Type {
        None,
        Solid,
        LinearGradient,
        RadialGradient,
        ConicGradient,
        Pattern
    };
    
    Type type = Type::Solid;
    std::array<float, 4> color{0.0f, 0.0f, 0.0f, 1.0f}; // RGBA
    
    // Gradient settings
    struct Gradient {
        std::vector<std::pair<float, std::array<float, 4>>> stops; // position, color
        std::array<float, 2> start{0.0f, 0.0f};
        std::array<float, 2> end{1.0f, 1.0f};
        float angle = 0.0f; // For conic gradients
        
        // GPU resources
        Rendering::ResourceId gradientTextureId = 0;
        bool needsUpdate = true;
    };
    std::shared_ptr<Gradient> gradient;
    
    // Pattern settings
    struct Pattern {
        Rendering::ResourceId textureId = 0;
        std::array<float, 2> scale{1.0f, 1.0f};
        std::array<float, 2> offset{0.0f, 0.0f};
        float rotation = 0.0f;
    };
    std::shared_ptr<Pattern> pattern;
};

// Stroke styles for vector objects
struct VectorStrokeStyle {
    bool enabled = false;
    std::array<float, 4> color{0.0f, 0.0f, 0.0f, 1.0f};
    float width = 1.0f;
    
    enum class Cap {
        Butt,
        Round,
        Square
    };
    Cap cap = Cap::Round;
    
    enum class Join {
        Miter,
        Round,
        Bevel
    };
    Join join = Join::Round;
    
    float miterLimit = 4.0f;
    std::vector<float> dashPattern;
    float dashOffset = 0.0f;
};

// High-performance vector renderer with GPU acceleration
class VectorRenderer final {
public:
    explicit VectorRenderer(Rendering::RenderingEngine& engine, 
                          const VectorRenderConfig& config = {});
    ~VectorRenderer();
    
    // Disable copy, enable move
    VectorRenderer(const VectorRenderer&) = delete;
    VectorRenderer& operator=(const VectorRenderer&) = delete;
    VectorRenderer(VectorRenderer&& other) noexcept;
    VectorRenderer& operator=(VectorRenderer&& other) noexcept;
    
    // Initialization
    bool initialize();
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // Single path rendering
    void render_path(const VectorPath& path, const VectorFillStyle& fill, 
                    const VectorStrokeStyle& stroke = {});
    
    // Batch rendering for performance
    void begin_batch();
    void add_to_batch(const VectorPath& path, const VectorFillStyle& fill, 
                     const VectorStrokeStyle& stroke = {});
    void render_batch();
    void end_batch();
    
    // Direct object rendering
    void render_object(const VectorObject& object);
    void render_objects(const std::vector<std::shared_ptr<VectorObject>>& objects);
    
    // Advanced rendering modes
    void render_with_clipping(const VectorPath& clipPath, 
                             const std::vector<std::shared_ptr<VectorObject>>& objects);
    void render_with_effects(const std::vector<std::shared_ptr<VectorObject>>& objects,
                            const std::vector<class VectorEffect*>& effects);
    
    // Tessellation control
    std::shared_ptr<TessellatedPath> tessellate_path(const VectorPath& path);
    void cache_tessellation(const VectorPath& path, std::shared_ptr<TessellatedPath> tessellation);
    void clear_tessellation_cache();
    
    // Configuration
    void update_config(const VectorRenderConfig& config);
    const VectorRenderConfig& get_config() const { return config_; }
    
    // Performance statistics
    struct VectorRenderStats {
        uint32_t pathsRendered = 0;
        uint32_t trianglesGenerated = 0;
        uint32_t tessellationCacheHits = 0;
        uint32_t tessellationCacheMisses = 0;
        uint32_t batchesRendered = 0;
        uint32_t verticesUploaded = 0;
        std::chrono::microseconds tessellationTime{0};
        std::chrono::microseconds renderTime{0};
        size_t gpuMemoryUsed = 0;
    };
    
    VectorRenderStats get_stats() const;
    void reset_stats();
    
private:
    Rendering::RenderingEngine& engine_;
    VectorRenderConfig config_;
    std::atomic<bool> initialized_{false};
    
    // GPU tessellator for curve subdivision
    std::unique_ptr<GPUTessellator> tessellator_;
    
    // Shader programs
    Rendering::PipelineId fillPipelineId_ = 0;
    Rendering::PipelineId strokePipelineId_ = 0;
    Rendering::PipelineId gradientPipelineId_ = 0;
    Rendering::PipelineId patternPipelineId_ = 0;
    Rendering::PipelineId tessellationComputeId_ = 0;
    
    // Uniform buffers
    Rendering::ResourceId transformUniformId_ = 0;
    Rendering::ResourceId styleUniformId_ = 0;
    
    // Tessellation cache
    mutable std::mutex tessellation_cache_mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<TessellatedPath>> tessellation_cache_;
    size_t max_cache_size_ = 1000;
    
    // Batch rendering state
    struct BatchState {
        bool active = false;
        std::vector<VectorVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Rendering::DrawCall> drawCalls;
        Rendering::ResourceId batchVertexBufferId = 0;
        Rendering::ResourceId batchIndexBufferId = 0;
        size_t maxVertices = 65536;
        size_t maxIndices = 196608;
    };
    BatchState batch_state_;
    
    // Performance statistics
    mutable std::mutex stats_mutex_;
    VectorRenderStats stats_;
    
    // Internal methods
    bool create_shaders();
    bool create_uniforms();
    void destroy_resources();
    
    std::shared_ptr<TessellatedPath> tessellate_path_internal(const VectorPath& path);
    void upload_tessellation_to_gpu(TessellatedPath& tessellation);
    
    void render_fill(const TessellatedPath& tessellation, const VectorFillStyle& fill);
    void render_stroke(const VectorPath& path, const VectorStrokeStyle& stroke);
    void render_gradient_fill(const TessellatedPath& tessellation, const VectorFillStyle::Gradient& gradient);
    void render_pattern_fill(const TessellatedPath& tessellation, const VectorFillStyle::Pattern& pattern);
    
    void update_transform_uniform(const std::array<float, 16>& transform);
    void update_style_uniform(const VectorFillStyle& fill, const VectorStrokeStyle& stroke);
    
    void add_to_batch_internal(const TessellatedPath& tessellation, 
                              const VectorFillStyle& fill,
                              const VectorStrokeStyle& stroke);
    void flush_batch_if_needed();
    
    uint64_t compute_path_hash(const VectorPath& path) const;
    void cleanup_tessellation_cache();
    
    // Curve quality mapping
    float get_tessellation_tolerance() const;
    uint32_t get_curve_subdivision_level() const;
};

// GPU-accelerated tessellation engine
class GPUTessellator {
public:
    explicit GPUTessellator(Rendering::RenderingEngine& engine);
    ~GPUTessellator();
    
    bool initialize();
    void shutdown();
    
    // Tessellate Bézier curves on GPU
    std::shared_ptr<TessellatedPath> tessellate_cubic_bezier(
        const std::vector<std::array<float, 2>>& controlPoints,
        float tolerance);
    
    std::shared_ptr<TessellatedPath> tessellate_quadratic_bezier(
        const std::vector<std::array<float, 2>>& controlPoints,
        float tolerance);
    
    // Tessellate complex paths
    std::shared_ptr<TessellatedPath> tessellate_path(const VectorPath& path, float tolerance);
    
    // Stroke tessellation
    std::shared_ptr<TessellatedPath> tessellate_stroke(const VectorPath& path, 
                                                     const VectorStrokeStyle& style);
    
private:
    Rendering::RenderingEngine& engine_;
    
    // Compute shaders
    Rendering::PipelineId cubicBezierTessellationId_ = 0;
    Rendering::PipelineId quadraticBezierTessellationId_ = 0;
    Rendering::PipelineId strokeTessellationId_ = 0;
    
    // Buffers
    Rendering::ResourceId controlPointsBufferId_ = 0;
    Rendering::ResourceId outputVerticesBufferId_ = 0;
    Rendering::ResourceId outputIndicesBufferId_ = 0;
    Rendering::ResourceId parametersBufferId_ = 0;
    
    bool create_compute_shaders();
    void destroy_resources();
    
    // Curve subdivision algorithms
    void subdivide_cubic_bezier_recursive(const std::array<float, 8>& control_points,
                                        float tolerance, uint32_t depth,
                                        std::vector<VectorVertex>& vertices);
    
    void subdivide_quadratic_bezier_recursive(const std::array<float, 6>& control_points,
                                            float tolerance, uint32_t depth,
                                            std::vector<VectorVertex>& vertices);
    
    float compute_curve_flatness(const std::array<float, 8>& cubic_bezier) const;
    std::array<std::array<float, 8>, 2> split_cubic_bezier(const std::array<float, 8>& curve, 
                                                          float t) const;
};

} // namespace QuantumCanvas::Vector