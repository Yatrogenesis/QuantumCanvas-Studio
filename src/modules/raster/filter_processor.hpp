#pragma once

#include "../../core/rendering/rendering_engine.hpp"
#include "../../core/math/vector2.hpp"
#include "../../core/math/vector3.hpp"
#include "../../core/math/vector4.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <array>
#include <variant>
#include <atomic>
#include <mutex>

namespace QuantumCanvas::Raster {

// Forward declarations
class Image;
class FilterChain;

// Filter categories for organization
enum class FilterCategory {
    // Basic adjustments
    Color,
    Tone,
    Exposure,
    
    // Blur effects
    Blur,
    Motion,
    
    // Sharpen effects
    Sharpen,
    Enhancement,
    
    // Artistic effects
    Artistic,
    Stylize,
    Brush,
    
    // Distortion effects
    Distort,
    Warp,
    Lens,
    
    // Noise effects
    Noise,
    Texture,
    
    // Edge detection
    Edge,
    Emboss,
    
    // Lighting effects
    Light,
    Shadow,
    
    // Custom/User-defined
    Custom
};

// Parameter types for filter configuration
using FilterParameter = std::variant<
    float,                           // Single float value
    int,                            // Single integer value
    bool,                           // Boolean flag
    std::array<float, 2>,           // 2D vector (e.g., offset)
    std::array<float, 3>,           // 3D vector (e.g., RGB color)
    std::array<float, 4>,           // 4D vector (e.g., RGBA color, rectangle)
    std::string,                    // String value (e.g., file path)
    Rendering::ResourceId           // Texture resource
>;

// Parameter metadata for UI generation and validation
struct FilterParameterInfo {
    std::string name;
    std::string displayName;
    std::string description;
    FilterParameter defaultValue;
    FilterParameter minValue;
    FilterParameter maxValue;
    std::vector<std::string> enumValues;  // For discrete choices
    bool isAdvanced = false;              // Hide in basic UI
    bool isAnimatable = true;             // Can be keyframed
};

// Base filter interface
class Filter {
public:
    Filter(const std::string& name, FilterCategory category);
    virtual ~Filter() = default;
    
    // Filter identification
    const std::string& getName() const { return name_; }
    const std::string& getDisplayName() const { return displayName_; }
    const std::string& getDescription() const { return description_; }
    FilterCategory getCategory() const { return category_; }
    
    // Parameter management
    void setParameter(const std::string& name, const FilterParameter& value);
    FilterParameter getParameter(const std::string& name) const;
    bool hasParameter(const std::string& name) const;
    
    const std::unordered_map<std::string, FilterParameter>& getParameters() const { return parameters_; }
    const std::vector<FilterParameterInfo>& getParameterInfo() const { return parameterInfo_; }
    
    // Filter state
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
    float getOpacity() const { return opacity_; }
    void setOpacity(float opacity) { opacity_ = std::clamp(opacity, 0.0f, 1.0f); }
    
    // Filter application - must be implemented by derived classes
    virtual bool apply(const Image& input, Image& output) = 0;
    virtual bool applyGPU(Rendering::ResourceId inputTexture, 
                         Rendering::ResourceId outputTexture,
                         const std::array<uint32_t, 2>& size) = 0;
    
    // GPU resource requirements
    virtual size_t getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const = 0;
    virtual std::vector<Rendering::ResourceId> getRequiredTextures() const { return {}; }
    
    // Performance characteristics
    virtual bool supportsInPlace() const { return false; }
    virtual bool isGPUAccelerated() const { return true; }
    virtual float getComplexityScore() const { return 1.0f; }
    
    // Serialization
    virtual std::vector<uint8_t> serialize() const;
    virtual bool deserialize(const std::vector<uint8_t>& data);
    
    // Filter cloning
    virtual std::unique_ptr<Filter> clone() const = 0;
    
    // Preview generation (for UI)
    virtual Image generatePreview(const Image& input, 
                                 const std::array<uint32_t, 2>& previewSize = {128, 128});

protected:
    std::string name_;
    std::string displayName_;
    std::string description_;
    FilterCategory category_;
    
    std::unordered_map<std::string, FilterParameter> parameters_;
    std::vector<FilterParameterInfo> parameterInfo_;
    
    bool enabled_ = true;
    float opacity_ = 1.0f;
    
    // Helper methods for derived classes
    void addParameterInfo(const FilterParameterInfo& info);
    template<typename T>
    T getParameterValue(const std::string& name) const;
    
    // GPU helper methods
    virtual Rendering::PipelineId createFilterPipeline(Rendering::RenderingEngine& engine) = 0;
    virtual void updateFilterUniforms(Rendering::RenderingEngine& engine, 
                                     Rendering::ResourceId uniformBuffer) = 0;
};

// Template implementation for type-safe parameter access
template<typename T>
T Filter::getParameterValue(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end()) {
        if (std::holds_alternative<T>(it->second)) {
            return std::get<T>(it->second);
        }
    }
    
    // Return default value if not found or wrong type
    for (const auto& info : parameterInfo_) {
        if (info.name == name && std::holds_alternative<T>(info.defaultValue)) {
            return std::get<T>(info.defaultValue);
        }
    }
    
    return T{};
}

// Gaussian blur filter
class GaussianBlurFilter final : public Filter {
public:
    GaussianBlurFilter();
    ~GaussianBlurFilter() override = default;
    
    bool apply(const Image& input, Image& output) override;
    bool applyGPU(Rendering::ResourceId inputTexture, 
                 Rendering::ResourceId outputTexture,
                 const std::array<uint32_t, 2>& size) override;
    
    size_t getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const override;
    bool supportsInPlace() const override { return false; }
    float getComplexityScore() const override { return 2.0f; }
    
    std::unique_ptr<Filter> clone() const override;

protected:
    Rendering::PipelineId createFilterPipeline(Rendering::RenderingEngine& engine) override;
    void updateFilterUniforms(Rendering::RenderingEngine& engine, 
                             Rendering::ResourceId uniformBuffer) override;

private:
    void generateKernel(float sigma, std::vector<float>& kernel, int& kernelSize);
    void applyGaussian1D(const Image& input, Image& output, 
                        const std::vector<float>& kernel, bool horizontal);
};

// Unsharp mask filter for sharpening
class UnsharpMaskFilter final : public Filter {
public:
    UnsharpMaskFilter();
    ~UnsharpMaskFilter() override = default;
    
    bool apply(const Image& input, Image& output) override;
    bool applyGPU(Rendering::ResourceId inputTexture, 
                 Rendering::ResourceId outputTexture,
                 const std::array<uint32_t, 2>& size) override;
    
    size_t getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const override;
    float getComplexityScore() const override { return 3.0f; }
    
    std::unique_ptr<Filter> clone() const override;

protected:
    Rendering::PipelineId createFilterPipeline(Rendering::RenderingEngine& engine) override;
    void updateFilterUniforms(Rendering::RenderingEngine& engine, 
                             Rendering::ResourceId uniformBuffer) override;
};

// Edge detection filter (Sobel, Canny, etc.)
class EdgeDetectionFilter final : public Filter {
public:
    enum EdgeMethod {
        Sobel,
        Prewitt,
        Roberts,
        Canny,
        Laplacian
    };
    
    EdgeDetectionFilter();
    ~EdgeDetectionFilter() override = default;
    
    bool apply(const Image& input, Image& output) override;
    bool applyGPU(Rendering::ResourceId inputTexture, 
                 Rendering::ResourceId outputTexture,
                 const std::array<uint32_t, 2>& size) override;
    
    size_t getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const override;
    float getComplexityScore() const override { return 2.5f; }
    
    std::unique_ptr<Filter> clone() const override;

protected:
    Rendering::PipelineId createFilterPipeline(Rendering::RenderingEngine& engine) override;
    void updateFilterUniforms(Rendering::RenderingEngine& engine, 
                             Rendering::ResourceId uniformBuffer) override;

private:
    void applySobel(const Image& input, Image& output);
    void applyCanny(const Image& input, Image& output, float lowThreshold, float highThreshold);
};

// Distortion filter (barrel, pincushion, wave, etc.)
class DistortionFilter final : public Filter {
public:
    enum DistortionType {
        Barrel,
        Pincushion,
        Wave,
        Ripple,
        Twirl,
        Fisheye,
        Spherize
    };
    
    DistortionFilter();
    ~DistortionFilter() override = default;
    
    bool apply(const Image& input, Image& output) override;
    bool applyGPU(Rendering::ResourceId inputTexture, 
                 Rendering::ResourceId outputTexture,
                 const std::array<uint32_t, 2>& size) override;
    
    size_t getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const override;
    bool supportsInPlace() const override { return false; }
    float getComplexityScore() const override { return 2.0f; }
    
    std::unique_ptr<Filter> clone() const override;

protected:
    Rendering::PipelineId createFilterPipeline(Rendering::RenderingEngine& engine) override;
    void updateFilterUniforms(Rendering::RenderingEngine& engine, 
                             Rendering::ResourceId uniformBuffer) override;

private:
    std::array<float, 2> distortPoint(const std::array<float, 2>& point, 
                                     DistortionType type, 
                                     const std::unordered_map<std::string, FilterParameter>& params);
};

// Color adjustment filter (levels, curves, hue/sat, etc.)
class ColorAdjustmentFilter final : public Filter {
public:
    enum AdjustmentType {
        Levels,
        Curves,
        HueSaturation,
        ColorBalance,
        BrightnessContrast,
        Gamma,
        Exposure,
        Vibrance
    };
    
    ColorAdjustmentFilter(AdjustmentType type);
    ~ColorAdjustmentFilter() override = default;
    
    bool apply(const Image& input, Image& output) override;
    bool applyGPU(Rendering::ResourceId inputTexture, 
                 Rendering::ResourceId outputTexture,
                 const std::array<uint32_t, 2>& size) override;
    
    size_t getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const override;
    bool supportsInPlace() const override { return true; }
    float getComplexityScore() const override { return 1.5f; }
    
    std::unique_ptr<Filter> clone() const override;

protected:
    Rendering::PipelineId createFilterPipeline(Rendering::RenderingEngine& engine) override;
    void updateFilterUniforms(Rendering::RenderingEngine& engine, 
                             Rendering::ResourceId uniformBuffer) override;

private:
    AdjustmentType adjustmentType_;
    
    void setupParametersForType(AdjustmentType type);
    std::array<float, 4> adjustPixel(const std::array<float, 4>& pixel, AdjustmentType type);
};

// Custom shader filter for user-defined effects
class CustomShaderFilter final : public Filter {
public:
    CustomShaderFilter(const std::string& name, const std::string& shaderCode);
    ~CustomShaderFilter() override = default;
    
    bool apply(const Image& input, Image& output) override;
    bool applyGPU(Rendering::ResourceId inputTexture, 
                 Rendering::ResourceId outputTexture,
                 const std::array<uint32_t, 2>& size) override;
    
    size_t getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const override;
    bool supportsInPlace() const override { return customSupportsInPlace_; }
    float getComplexityScore() const override { return customComplexityScore_; }
    
    std::unique_ptr<Filter> clone() const override;
    
    // Custom shader management
    void setShaderCode(const std::string& shaderCode);
    const std::string& getShaderCode() const { return shaderCode_; }
    
    void setCustomProperties(bool supportsInPlace, float complexityScore);

protected:
    Rendering::PipelineId createFilterPipeline(Rendering::RenderingEngine& engine) override;
    void updateFilterUniforms(Rendering::RenderingEngine& engine, 
                             Rendering::ResourceId uniformBuffer) override;

private:
    std::string shaderCode_;
    bool customSupportsInPlace_ = false;
    float customComplexityScore_ = 1.0f;
    mutable Rendering::PipelineId cachedPipelineId_ = 0;
    mutable std::string cachedShaderCode_;
};

// Filter chain for applying multiple filters in sequence
class FilterChain {
public:
    FilterChain() = default;
    ~FilterChain() = default;
    
    // Chain management
    void addFilter(std::unique_ptr<Filter> filter);
    void insertFilter(size_t index, std::unique_ptr<Filter> filter);
    std::unique_ptr<Filter> removeFilter(size_t index);
    void moveFilter(size_t fromIndex, size_t toIndex);
    void clear();
    
    // Filter access
    size_t getFilterCount() const { return filters_.size(); }
    Filter* getFilter(size_t index);
    const Filter* getFilter(size_t index) const;
    
    // Chain application
    bool apply(const Image& input, Image& output);
    bool applyGPU(Rendering::ResourceId inputTexture, 
                 Rendering::ResourceId outputTexture,
                 const std::array<uint32_t, 2>& size);
    
    // Performance analysis
    float getTotalComplexityScore() const;
    size_t getTotalRequiredMemory(const std::array<uint32_t, 2>& imageSize) const;
    
    // Optimization
    void optimize();  // Merge compatible filters, reorder for efficiency
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    bool deserialize(const std::vector<uint8_t>& data);

private:
    std::vector<std::unique_ptr<Filter>> filters_;
    
    void optimizeFilterOrder();
    void mergeCompatibleFilters();
};

// High-performance filter processor with GPU acceleration
class FilterProcessor final {
public:
    explicit FilterProcessor(Rendering::RenderingEngine& engine);
    ~FilterProcessor();
    
    // Disable copy, enable move
    FilterProcessor(const FilterProcessor&) = delete;
    FilterProcessor& operator=(const FilterProcessor&) = delete;
    FilterProcessor(FilterProcessor&& other) noexcept;
    FilterProcessor& operator=(FilterProcessor&& other) noexcept;
    
    // Initialization
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Filter registration and management
    void registerFilter(const std::string& name, 
                       std::function<std::unique_ptr<Filter>()> factory);
    void unregisterFilter(const std::string& name);
    std::vector<std::string> getAvailableFilters() const;
    std::unique_ptr<Filter> createFilter(const std::string& name);
    
    // Single filter application
    bool applyFilter(Filter* filter, const Image& input, Image& output);
    bool applyFilterGPU(Filter* filter, 
                       Rendering::ResourceId inputTexture,
                       Rendering::ResourceId outputTexture,
                       const std::array<uint32_t, 2>& size);
    
    // Filter chain application
    bool applyFilterChain(const FilterChain& chain, const Image& input, Image& output);
    bool applyFilterChainGPU(const FilterChain& chain,
                            Rendering::ResourceId inputTexture,
                            Rendering::ResourceId outputTexture,
                            const std::array<uint32_t, 2>& size);
    
    // Real-time preview system
    void beginPreview();
    void updatePreviewFilter(Filter* filter);
    void updatePreviewChain(const FilterChain& chain);
    Rendering::ResourceId getPreviewTexture() const { return previewTexture_; }
    void endPreview();
    bool isPreviewActive() const { return previewActive_; }
    
    // Batch processing
    struct BatchJob {
        std::string inputPath;
        std::string outputPath;
        FilterChain filterChain;
        std::array<uint32_t, 2> outputSize{0, 0};  // 0,0 = keep original size
        bool overwriteExisting = false;
    };
    
    void processBatch(const std::vector<BatchJob>& jobs,
                     std::function<void(size_t, bool, const std::string&)> progressCallback = nullptr);
    
    // Memory management
    void setMemoryBudget(size_t budgetBytes) { memoryBudget_ = budgetBytes; }
    size_t getMemoryBudget() const { return memoryBudget_; }
    size_t getMemoryUsage() const;
    void clearCache();
    
    // Performance settings
    void setPreferGPU(bool preferGPU) { preferGPU_ = preferGPU; }
    bool getPreferGPU() const { return preferGPU_; }
    
    void setMaxConcurrency(uint32_t maxThreads) { maxConcurrency_ = maxThreads; }
    uint32_t getMaxConcurrency() const { return maxConcurrency_; }
    
    // Statistics
    struct FilterProcessorStats {
        uint32_t filtersApplied = 0;
        uint32_t chainsApplied = 0;
        uint64_t pixelsProcessed = 0;
        std::chrono::microseconds processingTime{0};
        std::chrono::microseconds gpuTime{0};
        size_t gpuMemoryUsed = 0;
        uint32_t cacheHits = 0;
        uint32_t cacheMisses = 0;
        float averageComplexityScore = 0.0f;
    };
    
    FilterProcessorStats getStats() const;
    void resetStats();

private:
    Rendering::RenderingEngine& engine_;
    std::atomic<bool> initialized_{false};
    
    // Filter factories
    std::unordered_map<std::string, std::function<std::unique_ptr<Filter>()>> filterFactories_;
    mutable std::mutex factoriesMutex_;
    
    // GPU resources
    std::unordered_map<std::string, Rendering::PipelineId> filterPipelines_;
    Rendering::ResourceId filterUniformBuffer_ = 0;
    
    // Preview system
    std::atomic<bool> previewActive_{false};
    Rendering::ResourceId previewTexture_ = 0;
    Rendering::ResourceId previewInputTexture_ = 0;
    std::array<uint32_t, 2> previewSize_{512, 512};
    std::unique_ptr<FilterChain> previewChain_;
    
    // Memory management
    size_t memoryBudget_ = 1024 * 1024 * 1024;  // 1GB default
    
    // Performance settings
    bool preferGPU_ = true;
    uint32_t maxConcurrency_ = 0;  // 0 = auto-detect
    
    // Resource cache
    struct CacheEntry {
        Rendering::ResourceId textureId;
        std::array<uint32_t, 2> size;
        size_t memorySize;
        std::chrono::system_clock::time_point lastUsed;
        uint64_t contentHash;
    };
    
    mutable std::mutex cacheMutex_;
    std::unordered_map<uint64_t, CacheEntry> textureCache_;
    
    // Statistics
    mutable std::mutex statsMutex_;
    FilterProcessorStats stats_;
    
    // Internal methods
    bool createUniformBuffers();
    void destroyResources();
    
    Rendering::ResourceId getCachedTexture(const std::array<uint32_t, 2>& size, uint64_t contentHash);
    void storeCachedTexture(const std::array<uint32_t, 2>& size, uint64_t contentHash, 
                           Rendering::ResourceId textureId);
    
    bool applyFilterInternal(Filter* filter,
                            Rendering::ResourceId inputTexture,
                            Rendering::ResourceId outputTexture,
                            const std::array<uint32_t, 2>& size);
    
    void cleanupCache();
    uint64_t calculateContentHash(const Image& image);
    uint64_t calculateChainHash(const FilterChain& chain);
    
    void registerBuiltInFilters();
};

// Utility functions for filter operations
namespace FilterUtils {
    // Kernel generation
    std::vector<float> createGaussianKernel(float sigma, int& kernelSize);
    std::vector<float> createBoxKernel(int size);
    std::array<std::array<float, 3>, 3> createSobelKernel(bool horizontal);
    std::array<std::array<float, 3>, 3> createLaplacianKernel();
    
    // Color space conversions for filter operations
    std::array<float, 3> RGBtoHSV(const std::array<float, 3>& rgb);
    std::array<float, 3> HSVtoRGB(const std::array<float, 3>& hsv);
    std::array<float, 3> RGBtoLAB(const std::array<float, 3>& rgb);
    std::array<float, 3> LABtoRGB(const std::array<float, 3>& lab);
    
    // Image sampling and interpolation
    std::array<float, 4> sampleBilinear(const Image& image, float x, float y);
    std::array<float, 4> sampleBicubic(const Image& image, float x, float y);
    
    // Convolution operations
    void convolve2D(const Image& input, Image& output, 
                   const std::vector<std::vector<float>>& kernel);
    void separableConvolve(const Image& input, Image& output,
                          const std::vector<float>& horizontalKernel,
                          const std::vector<float>& verticalKernel);
    
    // Edge detection utilities
    float calculateGradientMagnitude(float gx, float gy);
    float calculateGradientDirection(float gx, float gy);
    void nonMaximumSuppression(Image& gradientMagnitude, const Image& gradientDirection);
    void hysteresisThresholding(Image& edges, float lowThreshold, float highThreshold);
    
    // Histogram operations
    std::array<uint32_t, 256> calculateHistogram(const Image& image, int channel = -1);  // -1 = luminance
    std::array<float, 256> calculateCumulativeHistogram(const std::array<uint32_t, 256>& histogram);
    void equalizeHistogram(Image& image);
    
    // Performance utilities
    bool shouldUseGPU(const std::array<uint32_t, 2>& imageSize, float complexityScore);
    size_t estimateFilterMemoryUsage(const std::array<uint32_t, 2>& imageSize, 
                                    float complexityScore);
}

} // namespace QuantumCanvas::Raster