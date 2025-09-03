#pragma once

#include "../../core/rendering/rendering_engine.hpp"
#include <array>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace QuantumCanvas::Raster {

// Forward declarations
class Image;
class BrushStroke;
class FluidSimulator;

// Brush engine configuration
struct BrushEngineConfig {
    // Performance settings
    bool enableGPUAcceleration = true;
    bool enableMultithreading = true;
    uint32_t workerThreadCount = 0; // 0 = auto-detect
    
    // Quality settings
    bool enablePressureSmoothing = true;
    float pressureSmoothingFactor = 0.3f;
    bool enableSubpixelSampling = true;
    uint32_t subpixelSamples = 4;
    
    // Memory settings
    size_t maxBrushCacheSize = 256 * 1024 * 1024; // 256MB
    size_t maxTextureSize = 4096;
    bool enableBrushCompression = true;
    
    // Fluid simulation settings
    bool enableFluidSimulation = false;
    float fluidViscosity = 0.5f;
    float fluidDensity = 1.0f;
    uint32_t fluidSteps = 8;
};

// Brush tip shapes
enum class BrushTipShape {
    Round,
    Square,
    Triangle,
    Diamond,
    Custom
};

// Blending modes for brushes
enum class BrushBlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    SoftLight,
    HardLight,
    ColorDodge,
    ColorBurn,
    Darken,
    Lighten,
    Difference,
    Exclusion,
    Dissolve,
    Behind,       // Paint behind existing pixels
    Clear,        // Erase mode
    Smudge,       // Smudge existing colors
    Blend         // Blend with existing colors
};

// Brush dynamics settings
struct BrushDynamics {
    // Size dynamics
    bool sizeFromPressure = true;
    float sizePressureSensitivity = 0.5f;
    bool sizeFromVelocity = false;
    float sizeVelocitySensitivity = 0.2f;
    float minSize = 0.1f;
    float maxSize = 1.0f;
    
    // Opacity dynamics
    bool opacityFromPressure = true;
    float opacityPressureSensitivity = 0.3f;
    bool opacityFromVelocity = false;
    float opacityVelocitySensitivity = 0.1f;
    
    // Flow dynamics
    bool flowFromPressure = false;
    float flowPressureSensitivity = 0.2f;
    
    // Color dynamics
    bool colorFromPressure = false;
    std::array<float, 4> pressureColor{1.0f, 0.0f, 0.0f, 1.0f}; // RGBA
    bool colorFromVelocity = false;
    std::array<float, 4> velocityColor{0.0f, 1.0f, 0.0f, 1.0f}; // RGBA
    
    // Texture dynamics
    bool textureFromPressure = false;
    float texturePressureScale = 1.0f;
    bool textureFromAngle = false;
    
    // Scatter/jitter
    float scatterAmount = 0.0f;
    float jitterAmount = 0.0f;
    
    // Dual brush settings
    bool enableDualBrush = false;
    float dualBrushBlending = 0.5f;
    BrushBlendMode dualBrushMode = BrushBlendMode::Multiply;
};

// Advanced brush settings
struct BrushSettings {
    // Basic properties
    float size = 20.0f;                    // Brush size in pixels
    float opacity = 1.0f;                  // 0.0 to 1.0
    float flow = 1.0f;                     // Paint flow rate
    float spacing = 0.25f;                 // Spacing between dabs (0.0 to 1.0)
    float hardness = 0.8f;                 // Edge hardness
    float density = 1.0f;                  // Bristle density for textured brushes
    
    // Shape and angle
    BrushTipShape tipShape = BrushTipShape::Round;
    float angle = 0.0f;                    // Brush angle in radians
    float roundness = 1.0f;                // 0.0 = flat, 1.0 = circular
    
    // Texture settings
    Rendering::ResourceId textureId = 0;   // Brush texture
    float textureScale = 1.0f;
    float textureStrength = 0.5f;
    bool invertTexture = false;
    
    // Color and blending
    std::array<float, 4> color{0.0f, 0.0f, 0.0f, 1.0f}; // RGBA
    BrushBlendMode blendMode = BrushBlendMode::Normal;
    
    // Dynamics
    BrushDynamics dynamics;
    
    // Advanced features
    bool enableWetEdges = false;
    bool enableBuildup = true;
    bool enableSmoothing = true;
    float smoothingStrength = 0.5f;
    
    // Airbrush mode
    bool airbrushing = false;
    float airbrushRate = 0.1f;
    
    bool operator==(const BrushSettings& other) const;
    bool operator!=(const BrushSettings& other) const { return !(*this == other); }
};

// Individual brush stroke point
struct StrokePoint {
    std::array<float, 2> position;         // x, y coordinates
    float pressure = 1.0f;                 // 0.0 to 1.0
    float tilt = 0.0f;                     // Stylus tilt angle
    float bearing = 0.0f;                  // Stylus bearing/azimuth
    float rotation = 0.0f;                 // Stylus rotation
    float velocity = 0.0f;                 // Movement velocity
    std::chrono::system_clock::time_point timestamp;
    
    // Computed properties (filled by brush engine)
    float computedSize = 0.0f;
    float computedOpacity = 0.0f;
    float computedFlow = 0.0f;
    std::array<float, 4> computedColor{0.0f, 0.0f, 0.0f, 1.0f};
};

// Complete brush stroke data
class BrushStroke {
public:
    BrushStroke() = default;
    explicit BrushStroke(const BrushSettings& settings);
    
    // Stroke data
    std::vector<StrokePoint> points;
    BrushSettings settings;
    uint64_t strokeId = 0;
    
    // Stroke properties
    std::array<float, 4> bounds() const;   // min_x, min_y, max_x, max_y
    float totalLength() const;
    std::chrono::milliseconds duration() const;
    
    // Stroke manipulation
    void addPoint(const StrokePoint& point);
    void smoothStroke(float strength = 0.5f);
    void resampleStroke(float targetSpacing);
    BrushStroke extractSection(size_t startIndex, size_t endIndex) const;
    
    // Optimization
    void simplifyStroke(float tolerance = 1.0f);
    void removeRedundantPoints();
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);
    
private:
    void updateBounds();
    mutable std::optional<std::array<float, 4>> cachedBounds_;
};

// High-performance brush engine with GPU acceleration
class BrushEngine final {
public:
    explicit BrushEngine(Rendering::RenderingEngine& engine, 
                        const BrushEngineConfig& config = {});
    ~BrushEngine();
    
    // Disable copy, enable move
    BrushEngine(const BrushEngine&) = delete;
    BrushEngine& operator=(const BrushEngine&) = delete;
    BrushEngine(BrushEngine&& other) noexcept;
    BrushEngine& operator=(BrushEngine&& other) noexcept;
    
    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Stroke rendering
    void applyStroke(Image& targetImage, const BrushStroke& stroke);
    void applyDab(Image& targetImage, const StrokePoint& point, 
                  const BrushSettings& settings);
    
    // Real-time stroke rendering
    void beginStroke(const BrushSettings& settings);
    void addStrokePoint(Image& targetImage, const StrokePoint& point);
    void endStroke();
    bool isStrokeActive() const { return strokeActive_; }
    
    // Batch rendering for performance
    void beginBatch();
    void addStrokeToBatch(const BrushStroke& stroke);
    void renderBatch(Image& targetImage);
    void endBatch();
    
    // Brush preview generation
    Image generateBrushPreview(const BrushSettings& settings, 
                              const std::array<uint32_t, 2>& size = {128, 128});
    
    // Medium simulation
    void enableMediumSimulation(bool enable) { mediumSimulation_ = enable; }
    void setMediumProperties(float viscosity, float absorption, float drying);
    void simulateMedium(Image& image, float deltaTime);
    
    // Texture management
    Rendering::ResourceId loadBrushTexture(const std::filesystem::path& path);
    void createProceduralTexture(const std::string& name, 
                                std::function<float(float, float)> generator,
                                const std::array<uint32_t, 2>& size = {256, 256});
    
    // Configuration
    void updateConfig(const BrushEngineConfig& config);
    const BrushEngineConfig& getConfig() const { return config_; }
    
    // Performance statistics
    struct BrushEngineStats {
        uint32_t strokesRendered = 0;
        uint32_t dabsRendered = 0;
        uint64_t pixelsProcessed = 0;
        std::chrono::microseconds renderTime{0};
        std::chrono::microseconds gpuTime{0};
        size_t gpuMemoryUsed = 0;
        uint32_t cacheHits = 0;
        uint32_t cacheMisses = 0;
        float averageFrameRate = 0.0f;
    };
    
    BrushEngineStats getStats() const;
    void resetStats();
    
private:
    Rendering::RenderingEngine& engine_;
    BrushEngineConfig config_;
    std::atomic<bool> initialized_{false};
    
    // GPU shaders and pipelines
    Rendering::PipelineId brushPipelineId_ = 0;
    Rendering::PipelineId blendPipelineId_ = 0;
    Rendering::PipelineId texturedBrushPipelineId_ = 0;
    Rendering::PipelineId smudgePipelineId_ = 0;
    Rendering::PipelineId fluidSimPipelineId_ = 0;
    
    // Uniform buffers
    Rendering::ResourceId brushUniformId_ = 0;
    Rendering::ResourceId dynamicsUniformId_ = 0;
    
    // Brush cache
    struct BrushCacheEntry {
        uint64_t settingsHash;
        Rendering::ResourceId textureId;
        std::array<uint32_t, 2> size;
        std::chrono::system_clock::time_point lastUsed;
    };
    mutable std::mutex cacheMutex_;
    std::unordered_map<uint64_t, BrushCacheEntry> brushCache_;
    
    // Current stroke state
    std::atomic<bool> strokeActive_{false};
    BrushSettings currentStrokeSettings_;
    std::vector<StrokePoint> currentStrokePoints_;
    std::array<float, 2> lastDabPosition_{0.0f, 0.0f};
    
    // Batch rendering state
    struct BatchState {
        bool active = false;
        std::vector<BrushStroke> strokes;
        Rendering::ResourceId batchTextureId = 0;
        std::array<uint32_t, 2> batchSize{2048, 2048};
    };
    BatchState batchState_;
    
    // Medium simulation
    std::atomic<bool> mediumSimulation_{false};
    std::unique_ptr<FluidSimulator> fluidSimulator_;
    float mediumViscosity_ = 0.5f;
    float mediumAbsorption_ = 0.1f;
    float mediumDrying_ = 0.05f;
    
    // Performance statistics
    mutable std::mutex statsMutex_;
    BrushEngineStats stats_;
    std::chrono::high_resolution_clock::time_point lastFrameTime_;
    
    // Internal methods
    bool createShaders();
    bool createBuffers();
    void destroyResources();
    
    void renderDab(Image& targetImage, const StrokePoint& point, 
                  const BrushSettings& settings);
    void renderGPUDab(const StrokePoint& point, const BrushSettings& settings,
                     Rendering::ResourceId targetTexture);
    
    Rendering::ResourceId getCachedBrushTexture(const BrushSettings& settings);
    Rendering::ResourceId generateBrushTexture(const BrushSettings& settings);
    
    void applyBrushDynamics(StrokePoint& point, const BrushSettings& settings,
                           const StrokePoint* previousPoint = nullptr);
    
    void updateBrushUniforms(const BrushSettings& settings);
    void updateDynamicsUniforms(const BrushDynamics& dynamics);
    
    float calculateSpacing(const BrushSettings& settings, const StrokePoint& point) const;
    bool shouldPlaceDab(const StrokePoint& point, const BrushSettings& settings) const;
    
    void cleanupBrushCache();
    uint64_t hashBrushSettings(const BrushSettings& settings) const;
    
    // Blend mode implementations
    void applyBlendMode(Image& targetImage, const Image& brushStamp,
                       const std::array<uint32_t, 2>& position,
                       BrushBlendMode blendMode, float opacity);
    
    // Special brush types
    void renderSmudgeBrush(Image& targetImage, const StrokePoint& point,
                          const BrushSettings& settings);
    void renderAirbrush(Image& targetImage, const StrokePoint& point,
                       const BrushSettings& settings, float deltaTime);
    void renderWetEdges(Image& targetImage, const StrokePoint& point,
                       const BrushSettings& settings);
};

// Fluid simulation for realistic paint behavior
class FluidSimulator {
public:
    explicit FluidSimulator(Rendering::RenderingEngine& engine);
    ~FluidSimulator();
    
    bool initialize(const std::array<uint32_t, 2>& gridSize);
    void shutdown();
    
    // Simulation step
    void step(float deltaTime);
    
    // Add paint to simulation
    void addPaint(const std::array<float, 2>& position, 
                 const std::array<float, 4>& color,
                 float amount, float viscosity);
    
    // Get paint distribution for rendering
    Rendering::ResourceId getPaintTexture() const { return paintTexture_; }
    
    // Configuration
    void setViscosity(float viscosity) { viscosity_ = viscosity; }
    void setDiffusion(float diffusion) { diffusion_ = diffusion; }
    void setEvaporation(float evaporation) { evaporation_ = evaporation; }
    
private:
    Rendering::RenderingEngine& engine_;
    std::array<uint32_t, 2> gridSize_{256, 256};
    
    // Simulation parameters
    float viscosity_ = 0.5f;
    float diffusion_ = 0.1f;
    float evaporation_ = 0.01f;
    
    // GPU resources
    Rendering::PipelineId advectionPipelineId_ = 0;
    Rendering::PipelineId diffusionPipelineId_ = 0;
    Rendering::PipelineId pressurePipelineId_ = 0;
    Rendering::ResourceId velocityTexture_ = 0;
    Rendering::ResourceId paintTexture_ = 0;
    Rendering::ResourceId pressureTexture_ = 0;
    
    // Simulation state
    bool initialized_ = false;
    
    bool createSimulationShaders();
    void destroyResources();
};

} // namespace QuantumCanvas::Raster