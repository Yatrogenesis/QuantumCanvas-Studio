#pragma once

#include "../../core/rendering/rendering_engine.hpp"
#include "../../core/memory/memory_manager.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <array>
#include <atomic>
#include <mutex>

namespace QuantumCanvas::Raster {

// Forward declarations
class Image;
class Layer;
class LayerGroup;

// Blend modes supported by the compositor
enum class BlendMode : uint16_t {
    // Normal blend modes
    Normal = 0,
    Dissolve,
    
    // Darkening blend modes
    Darken,
    Multiply,
    ColorBurn,
    LinearBurn,
    DarkerColor,
    
    // Lightening blend modes
    Lighten,
    Screen,
    ColorDodge,
    LinearDodge,
    LighterColor,
    
    // Contrast blend modes
    Overlay,
    SoftLight,
    HardLight,
    VividLight,
    LinearLight,
    PinLight,
    HardMix,
    
    // Comparative blend modes
    Difference,
    Exclusion,
    Subtract,
    Divide,
    
    // Component blend modes
    Hue,
    Saturation,
    Color,
    Luminosity,
    
    // Special blend modes
    Behind,      // Paint behind existing pixels
    Clear,       // Clear/erase pixels
    Replace,     // Replace pixels entirely
    Add,         // Additive blending
    
    // Custom blend modes
    Custom = 1000  // Starting point for user-defined blend modes
};

// Layer composition flags
enum class LayerFlags : uint32_t {
    None = 0,
    Visible = 1 << 0,
    Locked = 1 << 1,
    Clipping = 1 << 2,           // Layer is a clipping mask
    AlphaLocked = 1 << 3,        // Preserve transparency
    PixelLocked = 1 << 4,        // Lock pixels from editing
    PositionLocked = 1 << 5,     // Lock position/transform
    PassThrough = 1 << 6,        // Group blend mode passes through
    Isolated = 1 << 7,           // Isolate blending to group
    KnockOut = 1 << 8,           // Knock out lower layers
    
    // Default flags for new layers
    Default = Visible
};

// Inline operators for LayerFlags
inline LayerFlags operator|(LayerFlags a, LayerFlags b) {
    return static_cast<LayerFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline LayerFlags operator&(LayerFlags a, LayerFlags b) {
    return static_cast<LayerFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline LayerFlags& operator|=(LayerFlags& a, LayerFlags b) {
    return a = a | b;
}

inline LayerFlags& operator&=(LayerFlags& a, LayerFlags b) {
    return a = a & b;
}

inline bool hasFlag(LayerFlags flags, LayerFlags flag) {
    return (flags & flag) == flag;
}

// Layer transformation data
struct LayerTransform {
    std::array<float, 2> position{0.0f, 0.0f};      // Translation
    std::array<float, 2> scale{1.0f, 1.0f};         // Scale factors
    float rotation = 0.0f;                           // Rotation in radians
    std::array<float, 2> skew{0.0f, 0.0f};          // Skew factors
    std::array<float, 2> anchor{0.5f, 0.5f};        // Transform anchor point (0-1)
    
    // Perspective transform (for advanced transformations)
    std::array<float, 16> perspectiveMatrix{         // 4x4 matrix for perspective
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    bool usePerspective = false;
    
    // Get combined transformation matrix (3x3)
    std::array<float, 9> getMatrix() const;
    
    // Apply transform to point
    std::array<float, 2> transformPoint(const std::array<float, 2>& point) const;
    
    // Inverse transform
    LayerTransform inverse() const;
    
    // Compose transforms
    LayerTransform compose(const LayerTransform& other) const;
    
    // Reset to identity
    void reset();
    
    bool operator==(const LayerTransform& other) const;
    bool operator!=(const LayerTransform& other) const { return !(*this == other); }
};

// Layer effects/filters
struct LayerEffect {
    enum Type {
        DropShadow,
        InnerShadow,
        OuterGlow,
        InnerGlow,
        Bevel,
        Emboss,
        Stroke,
        ColorOverlay,
        GradientOverlay,
        PatternOverlay,
        Satin
    };
    
    Type type;
    bool enabled = true;
    BlendMode blendMode = BlendMode::Normal;
    float opacity = 1.0f;
    
    // Effect-specific parameters (stored as generic map)
    std::unordered_map<std::string, float> floatParams;
    std::unordered_map<std::string, std::array<float, 4>> colorParams;
    std::unordered_map<std::string, Rendering::ResourceId> textureParams;
};

// Layer mask data
struct LayerMask {
    enum Type {
        Alpha,      // Standard alpha mask
        Vector,     // Vector-based mask
        Clipping    // Clipping mask from another layer
    };
    
    Type type = Alpha;
    bool enabled = true;
    bool inverted = false;
    float density = 1.0f;         // Mask strength
    float feather = 0.0f;         // Feather amount
    
    // Mask data
    Rendering::ResourceId maskTexture = 0;    // For alpha/raster masks
    std::vector<uint8_t> vectorData;          // For vector masks
    uint32_t clippingLayerId = 0;             // For clipping masks
    
    // Mask transform (independent of layer transform)
    LayerTransform transform;
    
    // Link mask to layer transform
    bool linkedToLayer = true;
};

// Individual layer in the composition
class Layer {
public:
    explicit Layer(const std::string& name = "Layer");
    virtual ~Layer();
    
    // Disable copy, enable move
    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;
    Layer(Layer&& other) noexcept;
    Layer& operator=(Layer&& other) noexcept;
    
    // Layer properties
    uint32_t getId() const { return id_; }
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    // Visibility and flags
    LayerFlags getFlags() const { return flags_; }
    void setFlags(LayerFlags flags) { flags_ = flags; }
    bool isVisible() const { return hasFlag(flags_, LayerFlags::Visible); }
    void setVisible(bool visible);
    bool isLocked() const { return hasFlag(flags_, LayerFlags::Locked); }
    void setLocked(bool locked);
    
    // Blending properties
    BlendMode getBlendMode() const { return blendMode_; }
    void setBlendMode(BlendMode mode) { blendMode_ = mode; }
    float getOpacity() const { return opacity_; }
    void setOpacity(float opacity) { opacity_ = std::clamp(opacity, 0.0f, 1.0f); }
    
    // Transform
    const LayerTransform& getTransform() const { return transform_; }
    void setTransform(const LayerTransform& transform) { transform_ = transform; }
    LayerTransform& getTransform() { return transform_; }
    
    // Layer mask
    const LayerMask* getMask() const { return mask_.get(); }
    LayerMask* getMask() { return mask_.get(); }
    void setMask(std::unique_ptr<LayerMask> mask) { mask_ = std::move(mask); }
    void clearMask() { mask_.reset(); }
    bool hasMask() const { return mask_ != nullptr; }
    
    // Layer effects
    void addEffect(const LayerEffect& effect) { effects_.push_back(effect); }
    void removeEffect(size_t index);
    void clearEffects() { effects_.clear(); }
    const std::vector<LayerEffect>& getEffects() const { return effects_; }
    std::vector<LayerEffect>& getEffects() { return effects_; }
    
    // Layer content (override in derived classes)
    virtual Rendering::ResourceId getContentTexture() const = 0;
    virtual std::array<uint32_t, 2> getContentSize() const = 0;
    virtual std::array<float, 4> getBounds() const = 0;  // min_x, min_y, max_x, max_y
    
    // Layer type identification
    virtual std::string getLayerType() const = 0;
    
    // Dirty flag for optimization
    bool isDirty() const { return dirty_; }
    void markDirty() { dirty_ = true; }
    void markClean() { dirty_ = false; }
    
    // Serialization
    virtual std::vector<uint8_t> serialize() const;
    virtual bool deserialize(const std::vector<uint8_t>& data);

protected:
    uint32_t id_;
    std::string name_;
    LayerFlags flags_ = LayerFlags::Default;
    BlendMode blendMode_ = BlendMode::Normal;
    float opacity_ = 1.0f;
    LayerTransform transform_;
    std::unique_ptr<LayerMask> mask_;
    std::vector<LayerEffect> effects_;
    mutable std::atomic<bool> dirty_{true};
    
    static uint32_t nextId_;
};

// Raster layer - contains pixel data
class RasterLayer final : public Layer {
public:
    explicit RasterLayer(const std::string& name = "Raster Layer");
    explicit RasterLayer(std::unique_ptr<Image> image, const std::string& name = "Raster Layer");
    ~RasterLayer() override;
    
    // Layer interface implementation
    Rendering::ResourceId getContentTexture() const override;
    std::array<uint32_t, 2> getContentSize() const override;
    std::array<float, 4> getBounds() const override;
    std::string getLayerType() const override { return "RasterLayer"; }
    
    // Raster-specific methods
    Image* getImage() { return image_.get(); }
    const Image* getImage() const { return image_.get(); }
    void setImage(std::unique_ptr<Image> image);
    
    // Content modification
    void resize(const std::array<uint32_t, 2>& newSize);
    void clear(const std::array<float, 4>& color = {0.0f, 0.0f, 0.0f, 0.0f});
    
    // Serialization
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;

private:
    std::unique_ptr<Image> image_;
    mutable Rendering::ResourceId cachedTexture_ = 0;
    mutable bool textureDirty_ = true;
    
    void updateTexture() const;
};

// Adjustment layer - applies color/tone adjustments
class AdjustmentLayer final : public Layer {
public:
    enum AdjustmentType {
        Brightness,
        Contrast,
        Levels,
        Curves,
        HueSaturation,
        ColorBalance,
        BlackAndWhite,
        PhotoFilter,
        ChannelMixer,
        SelectiveColor,
        Threshold,
        Posterize,
        Invert
    };
    
    explicit AdjustmentLayer(AdjustmentType type, const std::string& name = "Adjustment");
    ~AdjustmentLayer() override;
    
    // Layer interface implementation
    Rendering::ResourceId getContentTexture() const override;
    std::array<uint32_t, 2> getContentSize() const override;
    std::array<float, 4> getBounds() const override;
    std::string getLayerType() const override { return "AdjustmentLayer"; }
    
    // Adjustment-specific methods
    AdjustmentType getAdjustmentType() const { return adjustmentType_; }
    void setParameter(const std::string& name, float value);
    float getParameter(const std::string& name) const;
    
    // Serialization
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;

private:
    AdjustmentType adjustmentType_;
    std::unordered_map<std::string, float> parameters_;
    mutable Rendering::ResourceId adjustmentTexture_ = 0;
    mutable bool parametersDirty_ = true;
    
    void updateAdjustmentTexture() const;
    void initializeDefaultParameters();
};

// Layer group - contains multiple layers
class LayerGroup final : public Layer {
public:
    explicit LayerGroup(const std::string& name = "Group");
    ~LayerGroup() override;
    
    // Layer interface implementation
    Rendering::ResourceId getContentTexture() const override;
    std::array<uint32_t, 2> getContentSize() const override;
    std::array<float, 4> getBounds() const override;
    std::string getLayerType() const override { return "LayerGroup"; }
    
    // Group management
    void addLayer(std::unique_ptr<Layer> layer);
    void insertLayer(size_t index, std::unique_ptr<Layer> layer);
    std::unique_ptr<Layer> removeLayer(size_t index);
    std::unique_ptr<Layer> removeLayer(uint32_t layerId);
    void moveLayer(size_t fromIndex, size_t toIndex);
    
    // Layer access
    size_t getLayerCount() const { return layers_.size(); }
    Layer* getLayer(size_t index);
    const Layer* getLayer(size_t index) const;
    Layer* findLayer(uint32_t layerId);
    const Layer* findLayer(uint32_t layerId) const;
    
    // Group properties
    bool isPassThrough() const { return hasFlag(getFlags(), LayerFlags::PassThrough); }
    void setPassThrough(bool passThrough);
    bool isIsolated() const { return hasFlag(getFlags(), LayerFlags::Isolated); }
    void setIsolated(bool isolated);
    
    // Serialization
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;

private:
    std::vector<std::unique_ptr<Layer>> layers_;
    mutable Rendering::ResourceId groupTexture_ = 0;
    mutable std::array<uint32_t, 2> groupSize_{0, 0};
    mutable bool contentDirty_ = true;
    
    void updateGroupTexture() const;
};

// High-performance layer compositor with GPU acceleration
class LayerCompositor final {
public:
    explicit LayerCompositor(Rendering::RenderingEngine& engine);
    ~LayerCompositor();
    
    // Disable copy, enable move
    LayerCompositor(const LayerCompositor&) = delete;
    LayerCompositor& operator=(const LayerCompositor&) = delete;
    LayerCompositor(LayerCompositor&& other) noexcept;
    LayerCompositor& operator=(LayerCompositor&& other) noexcept;
    
    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Layer stack composition
    void compositeToTarget(const std::vector<Layer*>& layers, 
                          Rendering::ResourceId targetTexture,
                          const std::array<uint32_t, 2>& targetSize,
                          const std::array<float, 4>& bounds = {0.0f, 0.0f, 0.0f, 0.0f});
    
    Rendering::ResourceId compositeToTexture(const std::vector<Layer*>& layers,
                                           const std::array<uint32_t, 2>& size,
                                           const std::array<float, 4>& bounds = {0.0f, 0.0f, 0.0f, 0.0f});
    
    // Single layer composition
    void compositeLayer(Layer* layer,
                       Rendering::ResourceId targetTexture,
                       const std::array<uint32_t, 2>& targetSize,
                       const std::array<float, 4>& bounds = {0.0f, 0.0f, 0.0f, 0.0f});
    
    // Blend mode utilities
    void blendTextures(Rendering::ResourceId baseTexture,
                      Rendering::ResourceId overlayTexture,
                      Rendering::ResourceId targetTexture,
                      BlendMode blendMode,
                      float opacity = 1.0f,
                      const std::array<uint32_t, 2>& size = {0, 0});
    
    // Custom blend mode support
    void registerCustomBlendMode(BlendMode mode, const std::string& shaderCode);
    void unregisterCustomBlendMode(BlendMode mode);
    
    // Layer effects rendering
    Rendering::ResourceId applyLayerEffects(Layer* layer,
                                          Rendering::ResourceId sourceTexture,
                                          const std::array<uint32_t, 2>& size);
    
    // Clipping mask support
    void setClippingMask(Rendering::ResourceId maskTexture);
    void clearClippingMask();
    
    // Performance settings
    void setMultisampleCount(uint32_t samples) { multisampleCount_ = samples; }
    uint32_t getMultisampleCount() const { return multisampleCount_; }
    
    void setColorSpace(Rendering::ColorSpace colorSpace) { colorSpace_ = colorSpace; }
    Rendering::ColorSpace getColorSpace() const { return colorSpace_; }
    
    // Statistics
    struct CompositionStats {
        uint32_t layersComposited = 0;
        uint32_t effectsApplied = 0;
        uint64_t pixelsProcessed = 0;
        std::chrono::microseconds compositionTime{0};
        std::chrono::microseconds gpuTime{0};
        size_t gpuMemoryUsed = 0;
        uint32_t blendOperations = 0;
        uint32_t transformOperations = 0;
    };
    
    CompositionStats getStats() const;
    void resetStats();

private:
    Rendering::RenderingEngine& engine_;
    std::atomic<bool> initialized_{false};
    
    // GPU pipelines for different blend modes
    std::unordered_map<BlendMode, Rendering::PipelineId> blendPipelines_;
    Rendering::PipelineId transformPipelineId_ = 0;
    Rendering::PipelineId maskPipelineId_ = 0;
    
    // Effect pipelines
    std::unordered_map<LayerEffect::Type, Rendering::PipelineId> effectPipelines_;
    
    // Custom blend modes
    std::unordered_map<BlendMode, Rendering::PipelineId> customBlendPipelines_;
    
    // Uniform buffers
    Rendering::ResourceId blendUniformId_ = 0;
    Rendering::ResourceId transformUniformId_ = 0;
    Rendering::ResourceId effectUniformId_ = 0;
    
    // Composition settings
    uint32_t multisampleCount_ = 1;
    Rendering::ColorSpace colorSpace_ = Rendering::ColorSpace::sRGB;
    
    // Clipping state
    Rendering::ResourceId clippingMask_ = 0;
    bool hasClippingMask_ = false;
    
    // Performance statistics
    mutable std::mutex statsMutex_;
    CompositionStats stats_;
    
    // Internal methods
    bool createBlendPipelines();
    bool createEffectPipelines();
    bool createUniformBuffers();
    void destroyResources();
    
    void applyLayerTransform(Layer* layer, const std::array<uint32_t, 2>& targetSize);
    void applyLayerMask(Layer* layer, Rendering::ResourceId sourceTexture);
    void applyLayerEffect(const LayerEffect& effect, 
                         Rendering::ResourceId sourceTexture,
                         Rendering::ResourceId targetTexture,
                         const std::array<uint32_t, 2>& size);
    
    void updateBlendUniforms(BlendMode mode, float opacity);
    void updateTransformUniforms(const LayerTransform& transform, 
                                const std::array<uint32_t, 2>& targetSize);
    void updateEffectUniforms(const LayerEffect& effect);
    
    Rendering::PipelineId getBlendPipeline(BlendMode mode);
    
    std::array<float, 4> calculateLayerBounds(const std::vector<Layer*>& layers);
    bool layerIntersectsBounds(Layer* layer, const std::array<float, 4>& bounds);
    
    // Blend mode implementations (used by shaders)
    static std::string getBlendModeShaderCode(BlendMode mode);
};

// Utility functions for layer composition
namespace LayerUtils {
    // Color space conversions
    std::array<float, 3> RGBtoHSV(const std::array<float, 3>& rgb);
    std::array<float, 3> HSVtoRGB(const std::array<float, 3>& hsv);
    std::array<float, 3> RGBtoLAB(const std::array<float, 3>& rgb);
    std::array<float, 3> LABtoRGB(const std::array<float, 3>& lab);
    
    // Blend mode calculations (CPU implementations for reference)
    std::array<float, 4> blendNormal(const std::array<float, 4>& base, 
                                    const std::array<float, 4>& overlay, 
                                    float opacity);
    std::array<float, 4> blendMultiply(const std::array<float, 4>& base, 
                                      const std::array<float, 4>& overlay, 
                                      float opacity);
    std::array<float, 4> blendScreen(const std::array<float, 4>& base, 
                                    const std::array<float, 4>& overlay, 
                                    float opacity);
    std::array<float, 4> blendOverlay(const std::array<float, 4>& base, 
                                     const std::array<float, 4>& overlay, 
                                     float opacity);
    std::array<float, 4> blendSoftLight(const std::array<float, 4>& base, 
                                       const std::array<float, 4>& overlay, 
                                       float opacity);
    
    // Matrix operations for transforms
    std::array<float, 9> multiplyMatrices(const std::array<float, 9>& a, 
                                         const std::array<float, 9>& b);
    std::array<float, 9> invertMatrix(const std::array<float, 9>& matrix);
    std::array<float, 2> transformPoint(const std::array<float, 2>& point, 
                                       const std::array<float, 9>& matrix);
    
    // Bounds calculations
    std::array<float, 4> transformBounds(const std::array<float, 4>& bounds, 
                                        const std::array<float, 9>& matrix);
    std::array<float, 4> unionBounds(const std::array<float, 4>& a, 
                                    const std::array<float, 4>& b);
    std::array<float, 4> intersectBounds(const std::array<float, 4>& a, 
                                        const std::array<float, 4>& b);
    bool boundsOverlap(const std::array<float, 4>& a, 
                      const std::array<float, 4>& b);
}

} // namespace QuantumCanvas::Raster