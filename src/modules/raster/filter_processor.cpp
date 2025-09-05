#include "filter_processor.hpp"
#include "../../core/rendering/rendering_engine.hpp"
#include "../../core/memory/memory_manager.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <cassert>
#include <fstream>
#include <thread>
#include <future>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace QuantumCanvas::Raster {

// Forward declaration for Image class (simplified implementation)
class Image {
public:
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<float> data; // RGBA float data
    
    Image(uint32_t w, uint32_t h) : width(w), height(h) {
        data.resize(w * h * 4); // RGBA
    }
    
    void setPixel(uint32_t x, uint32_t y, const std::array<float, 4>& color) {
        if (x >= width || y >= height) return;
        size_t idx = (y * width + x) * 4;
        data[idx + 0] = color[0];
        data[idx + 1] = color[1];
        data[idx + 2] = color[2];
        data[idx + 3] = color[3];
    }
    
    std::array<float, 4> getPixel(uint32_t x, uint32_t y) const {
        if (x >= width || y >= height) return {0.0f, 0.0f, 0.0f, 0.0f};
        size_t idx = (y * width + x) * 4;
        return {data[idx + 0], data[idx + 1], data[idx + 2], data[idx + 3]};
    }
    
    std::array<float, 4> sampleBilinear(float x, float y) const {
        x = std::clamp(x, 0.0f, static_cast<float>(width - 1));
        y = std::clamp(y, 0.0f, static_cast<float>(height - 1));
        
        int x1 = static_cast<int>(x);
        int y1 = static_cast<int>(y);
        int x2 = std::min(x1 + 1, static_cast<int>(width - 1));
        int y2 = std::min(y1 + 1, static_cast<int>(height - 1));
        
        float fx = x - x1;
        float fy = y - y1;
        
        auto p11 = getPixel(x1, y1);
        auto p12 = getPixel(x1, y2);
        auto p21 = getPixel(x2, y1);
        auto p22 = getPixel(x2, y2);
        
        std::array<float, 4> result;
        for (int i = 0; i < 4; ++i) {
            float top = p11[i] * (1 - fx) + p21[i] * fx;
            float bottom = p12[i] * (1 - fx) + p22[i] * fx;
            result[i] = top * (1 - fy) + bottom * fy;
        }
        
        return result;
    }
};

// Filter base implementation
Filter::Filter(const std::string& name, FilterCategory category) 
    : name_(name), displayName_(name), category_(category) {
}

void Filter::setParameter(const std::string& name, const FilterParameter& value) {
    parameters_[name] = value;
}

FilterParameter Filter::getParameter(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end()) {
        return it->second;
    }
    
    // Return default value
    for (const auto& info : parameterInfo_) {
        if (info.name == name) {
            return info.defaultValue;
        }
    }
    
    return FilterParameter{};
}

bool Filter::hasParameter(const std::string& name) const {
    return parameters_.find(name) != parameters_.end();
}

std::vector<uint8_t> Filter::serialize() const {
    // Simplified serialization
    std::vector<uint8_t> data;
    // Would implement proper binary serialization
    return data;
}

bool Filter::deserialize(const std::vector<uint8_t>& data) {
    // Would implement deserialization
    return true;
}

Image Filter::generatePreview(const Image& input, const std::array<uint32_t, 2>& previewSize) {
    // Create scaled down version for preview
    Image preview(previewSize[0], previewSize[1]);
    
    float scaleX = static_cast<float>(input.width) / previewSize[0];
    float scaleY = static_cast<float>(input.height) / previewSize[1];
    
    for (uint32_t y = 0; y < previewSize[1]; ++y) {
        for (uint32_t x = 0; x < previewSize[0]; ++x) {
            float srcX = x * scaleX;
            float srcY = y * scaleY;
            auto pixel = input.sampleBilinear(srcX, srcY);
            preview.setPixel(x, y, pixel);
        }
    }
    
    // Apply filter to preview
    Image result(previewSize[0], previewSize[1]);
    apply(preview, result);
    
    return result;
}

void Filter::addParameterInfo(const FilterParameterInfo& info) {
    parameterInfo_.push_back(info);
}

// GaussianBlurFilter implementation
GaussianBlurFilter::GaussianBlurFilter() : Filter("GaussianBlur", FilterCategory::Blur) {
    displayName_ = "Gaussian Blur";
    description_ = "Applies Gaussian blur with configurable radius and quality";
    
    addParameterInfo({
        "radius", "Radius", "Blur radius in pixels",
        5.0f, 0.1f, 100.0f, {}, false, true
    });
    
    addParameterInfo({
        "quality", "Quality", "Blur quality (higher = better)",
        1.0f, 0.1f, 3.0f, {}, true, false
    });
}

bool GaussianBlurFilter::apply(const Image& input, Image& output) {
    float radius = getParameterValue<float>("radius");
    float quality = getParameterValue<float>("quality");
    
    if (radius <= 0) {
        output = input;
        return true;
    }
    
    // Generate Gaussian kernel
    std::vector<float> kernel;
    int kernelSize;
    generateKernel(radius * quality, kernel, kernelSize);
    
    // Apply separable Gaussian blur (horizontal then vertical)
    Image temp(input.width, input.height);
    
    // Horizontal pass
    applyGaussian1D(input, temp, kernel, true);
    
    // Vertical pass
    applyGaussian1D(temp, output, kernel, false);
    
    return true;
}

bool GaussianBlurFilter::applyGPU(Rendering::ResourceId inputTexture, 
                                 Rendering::ResourceId outputTexture,
                                 const std::array<uint32_t, 2>& size) {
    // Would implement GPU version using compute shaders
    return false; // Fall back to CPU for now
}

size_t GaussianBlurFilter::getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const {
    return imageSize[0] * imageSize[1] * 4 * sizeof(float) * 2; // Input + temp buffer
}

std::unique_ptr<Filter> GaussianBlurFilter::clone() const {
    auto cloned = std::make_unique<GaussianBlurFilter>();
    cloned->parameters_ = parameters_;
    cloned->enabled_ = enabled_;
    cloned->opacity_ = opacity_;
    return cloned;
}

Rendering::PipelineId GaussianBlurFilter::createFilterPipeline(Rendering::RenderingEngine& engine) {
    // Would create GPU pipeline
    return 0;
}

void GaussianBlurFilter::updateFilterUniforms(Rendering::RenderingEngine& engine, 
                                             Rendering::ResourceId uniformBuffer) {
    // Would update GPU uniforms
}

void GaussianBlurFilter::generateKernel(float sigma, std::vector<float>& kernel, int& kernelSize) {
    // Calculate kernel size (should be odd)
    kernelSize = static_cast<int>(std::ceil(sigma * 3.0f)) * 2 + 1;
    kernel.resize(kernelSize);
    
    float sum = 0.0f;
    int center = kernelSize / 2;
    
    // Generate Gaussian weights
    for (int i = 0; i < kernelSize; ++i) {
        float x = i - center;
        kernel[i] = std::exp(-(x * x) / (2 * sigma * sigma));
        sum += kernel[i];
    }
    
    // Normalize kernel
    for (float& weight : kernel) {
        weight /= sum;
    }
}

void GaussianBlurFilter::applyGaussian1D(const Image& input, Image& output, 
                                        const std::vector<float>& kernel, bool horizontal) {
    int kernelSize = static_cast<int>(kernel.size());
    int center = kernelSize / 2;
    
    output.width = input.width;
    output.height = input.height;
    output.data.resize(input.data.size());
    
    if (horizontal) {
        // Horizontal convolution
        for (uint32_t y = 0; y < input.height; ++y) {
            for (uint32_t x = 0; x < input.width; ++x) {
                std::array<float, 4> sum = {0, 0, 0, 0};
                
                for (int k = 0; k < kernelSize; ++k) {
                    int sampleX = static_cast<int>(x) + k - center;
                    sampleX = std::clamp(sampleX, 0, static_cast<int>(input.width - 1));
                    
                    auto pixel = input.getPixel(sampleX, y);
                    float weight = kernel[k];
                    
                    for (int c = 0; c < 4; ++c) {
                        sum[c] += pixel[c] * weight;
                    }
                }
                
                output.setPixel(x, y, sum);
            }
        }
    } else {
        // Vertical convolution
        for (uint32_t y = 0; y < input.height; ++y) {
            for (uint32_t x = 0; x < input.width; ++x) {
                std::array<float, 4> sum = {0, 0, 0, 0};
                
                for (int k = 0; k < kernelSize; ++k) {
                    int sampleY = static_cast<int>(y) + k - center;
                    sampleY = std::clamp(sampleY, 0, static_cast<int>(input.height - 1));
                    
                    auto pixel = input.getPixel(x, sampleY);
                    float weight = kernel[k];
                    
                    for (int c = 0; c < 4; ++c) {
                        sum[c] += pixel[c] * weight;
                    }
                }
                
                output.setPixel(x, y, sum);
            }
        }
    }
}

// UnsharpMaskFilter implementation
UnsharpMaskFilter::UnsharpMaskFilter() : Filter("UnsharpMask", FilterCategory::Sharpen) {
    displayName_ = "Unsharp Mask";
    description_ = "Sharpens image using unsharp masking technique";
    
    addParameterInfo({
        "amount", "Amount", "Sharpening strength",
        1.0f, 0.0f, 5.0f, {}, false, true
    });
    
    addParameterInfo({
        "radius", "Radius", "Sharpening radius",
        2.0f, 0.1f, 10.0f, {}, false, true
    });
    
    addParameterInfo({
        "threshold", "Threshold", "Edge threshold",
        0.0f, 0.0f, 1.0f, {}, false, true
    });
}

bool UnsharpMaskFilter::apply(const Image& input, Image& output) {
    float amount = getParameterValue<float>("amount");
    float radius = getParameterValue<float>("radius");
    float threshold = getParameterValue<float>("threshold");
    
    // Create Gaussian blur
    GaussianBlurFilter blur;
    blur.setParameter("radius", radius);
    
    Image blurred(input.width, input.height);
    blur.apply(input, blurred);
    
    // Apply unsharp mask formula: original + amount * (original - blurred)
    output.width = input.width;
    output.height = input.height;
    output.data.resize(input.data.size());
    
    for (uint32_t y = 0; y < input.height; ++y) {
        for (uint32_t x = 0; x < input.width; ++x) {
            auto original = input.getPixel(x, y);
            auto blur_pixel = blurred.getPixel(x, y);
            
            std::array<float, 4> result;
            for (int c = 0; c < 3; ++c) { // Don't modify alpha
                float diff = original[c] - blur_pixel[c];
                
                // Apply threshold
                if (std::abs(diff) > threshold) {
                    result[c] = std::clamp(original[c] + amount * diff, 0.0f, 1.0f);
                } else {
                    result[c] = original[c];
                }
            }
            result[3] = original[3]; // Preserve alpha
            
            output.setPixel(x, y, result);
        }
    }
    
    return true;
}

bool UnsharpMaskFilter::applyGPU(Rendering::ResourceId inputTexture, 
                                Rendering::ResourceId outputTexture,
                                const std::array<uint32_t, 2>& size) {
    return false; // Fall back to CPU
}

size_t UnsharpMaskFilter::getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const {
    return imageSize[0] * imageSize[1] * 4 * sizeof(float) * 2; // Input + blurred
}

std::unique_ptr<Filter> UnsharpMaskFilter::clone() const {
    auto cloned = std::make_unique<UnsharpMaskFilter>();
    cloned->parameters_ = parameters_;
    cloned->enabled_ = enabled_;
    cloned->opacity_ = opacity_;
    return cloned;
}

Rendering::PipelineId UnsharpMaskFilter::createFilterPipeline(Rendering::RenderingEngine& engine) {
    return 0;
}

void UnsharpMaskFilter::updateFilterUniforms(Rendering::RenderingEngine& engine, 
                                            Rendering::ResourceId uniformBuffer) {
}

// EdgeDetectionFilter implementation
EdgeDetectionFilter::EdgeDetectionFilter() : Filter("EdgeDetection", FilterCategory::Edge) {
    displayName_ = "Edge Detection";
    description_ = "Detects edges using various algorithms";
    
    addParameterInfo({
        "method", "Method", "Edge detection algorithm",
        static_cast<int>(EdgeMethod::Sobel), static_cast<int>(EdgeMethod::Sobel), 
        static_cast<int>(EdgeMethod::Laplacian), {"Sobel", "Prewitt", "Roberts", "Canny", "Laplacian"}
    });
    
    addParameterInfo({
        "threshold", "Threshold", "Edge threshold",
        0.5f, 0.0f, 1.0f, {}, false, true
    });
}

bool EdgeDetectionFilter::apply(const Image& input, Image& output) {
    EdgeMethod method = static_cast<EdgeMethod>(getParameterValue<int>("method"));
    
    output.width = input.width;
    output.height = input.height;
    output.data.resize(input.data.size());
    
    switch (method) {
        case EdgeMethod::Sobel:
            applySobel(input, output);
            break;
        case EdgeMethod::Canny: {
            float threshold = getParameterValue<float>("threshold");
            applyCanny(input, output, threshold * 0.5f, threshold);
            break;
        }
        default:
            applySobel(input, output); // Default to Sobel
            break;
    }
    
    return true;
}

bool EdgeDetectionFilter::applyGPU(Rendering::ResourceId inputTexture, 
                                  Rendering::ResourceId outputTexture,
                                  const std::array<uint32_t, 2>& size) {
    return false;
}

size_t EdgeDetectionFilter::getRequiredMemory(const std::array<uint32_t, 2>& imageSize) const {
    return imageSize[0] * imageSize[1] * 4 * sizeof(float) * 2;
}

std::unique_ptr<Filter> EdgeDetectionFilter::clone() const {
    auto cloned = std::make_unique<EdgeDetectionFilter>();
    cloned->parameters_ = parameters_;
    cloned->enabled_ = enabled_;
    cloned->opacity_ = opacity_;
    return cloned;
}

Rendering::PipelineId EdgeDetectionFilter::createFilterPipeline(Rendering::RenderingEngine& engine) {
    return 0;
}

void EdgeDetectionFilter::updateFilterUniforms(Rendering::RenderingEngine& engine, 
                                              Rendering::ResourceId uniformBuffer) {
}

void EdgeDetectionFilter::applySobel(const Image& input, Image& output) {
    // Sobel operators
    float sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    float sobelY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    for (uint32_t y = 1; y < input.height - 1; ++y) {
        for (uint32_t x = 1; x < input.width - 1; ++x) {
            float gx = 0, gy = 0;
            
            // Apply Sobel operators
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    auto pixel = input.getPixel(x + dx, y + dy);
                    float intensity = (pixel[0] + pixel[1] + pixel[2]) / 3.0f; // Convert to grayscale
                    
                    gx += intensity * sobelX[dy + 1][dx + 1];
                    gy += intensity * sobelY[dy + 1][dx + 1];
                }
            }
            
            float magnitude = std::sqrt(gx * gx + gy * gy);
            magnitude = std::clamp(magnitude, 0.0f, 1.0f);
            
            output.setPixel(x, y, {magnitude, magnitude, magnitude, 1.0f});
        }
    }
}

void EdgeDetectionFilter::applyCanny(const Image& input, Image& output, 
                                    float lowThreshold, float highThreshold) {
    // Simplified Canny edge detection
    // 1. Apply Gaussian blur
    GaussianBlurFilter blur;
    blur.setParameter("radius", 1.0f);
    
    Image smoothed(input.width, input.height);
    blur.apply(input, smoothed);
    
    // 2. Apply Sobel for gradient calculation
    applySobel(smoothed, output);
    
    // 3. Apply thresholding (simplified)
    for (uint32_t y = 0; y < output.height; ++y) {
        for (uint32_t x = 0; x < output.width; ++x) {
            auto pixel = output.getPixel(x, y);
            float intensity = pixel[0];
            
            if (intensity > highThreshold) {
                output.setPixel(x, y, {1.0f, 1.0f, 1.0f, 1.0f}); // Strong edge
            } else if (intensity > lowThreshold) {
                output.setPixel(x, y, {0.5f, 0.5f, 0.5f, 1.0f}); // Weak edge
            } else {
                output.setPixel(x, y, {0.0f, 0.0f, 0.0f, 1.0f}); // No edge
            }
        }
    }
}

// FilterChain implementation
void FilterChain::addFilter(std::unique_ptr<Filter> filter) {
    filters_.push_back(std::move(filter));
}

void FilterChain::insertFilter(size_t index, std::unique_ptr<Filter> filter) {
    if (index > filters_.size()) {
        index = filters_.size();
    }
    filters_.insert(filters_.begin() + index, std::move(filter));
}

std::unique_ptr<Filter> FilterChain::removeFilter(size_t index) {
    if (index >= filters_.size()) {
        return nullptr;
    }
    
    auto filter = std::move(filters_[index]);
    filters_.erase(filters_.begin() + index);
    return filter;
}

void FilterChain::moveFilter(size_t fromIndex, size_t toIndex) {
    if (fromIndex >= filters_.size() || toIndex >= filters_.size()) {
        return;
    }
    
    auto filter = std::move(filters_[fromIndex]);
    filters_.erase(filters_.begin() + fromIndex);
    
    if (toIndex > fromIndex) {
        toIndex--;
    }
    
    filters_.insert(filters_.begin() + toIndex, std::move(filter));
}

void FilterChain::clear() {
    filters_.clear();
}

Filter* FilterChain::getFilter(size_t index) {
    if (index >= filters_.size()) {
        return nullptr;
    }
    return filters_[index].get();
}

const Filter* FilterChain::getFilter(size_t index) const {
    if (index >= filters_.size()) {
        return nullptr;
    }
    return filters_[index].get();
}

bool FilterChain::apply(const Image& input, Image& output) {
    if (filters_.empty()) {
        output = input;
        return true;
    }
    
    Image temp1 = input;
    Image temp2(input.width, input.height);
    
    for (size_t i = 0; i < filters_.size(); ++i) {
        if (!filters_[i] || !filters_[i]->isEnabled()) {
            continue;
        }
        
        bool success = false;
        if (i % 2 == 0) {
            success = filters_[i]->apply(temp1, temp2);
        } else {
            success = filters_[i]->apply(temp2, temp1);
        }
        
        if (!success) {
            return false;
        }
    }
    
    // Copy final result to output
    if ((filters_.size() - 1) % 2 == 0) {
        output = temp2;
    } else {
        output = temp1;
    }
    
    return true;
}

bool FilterChain::applyGPU(Rendering::ResourceId inputTexture, 
                          Rendering::ResourceId outputTexture,
                          const std::array<uint32_t, 2>& size) {
    // GPU filter chain would pipeline multiple filters
    return false; // Fall back to CPU for now
}

float FilterChain::getTotalComplexityScore() const {
    float total = 0.0f;
    for (const auto& filter : filters_) {
        if (filter && filter->isEnabled()) {
            total += filter->getComplexityScore();
        }
    }
    return total;
}

size_t FilterChain::getTotalRequiredMemory(const std::array<uint32_t, 2>& imageSize) const {
    size_t maxMemory = 0;
    for (const auto& filter : filters_) {
        if (filter && filter->isEnabled()) {
            maxMemory = std::max(maxMemory, filter->getRequiredMemory(imageSize));
        }
    }
    return maxMemory;
}

void FilterChain::optimize() {
    // Remove disabled filters
    filters_.erase(
        std::remove_if(filters_.begin(), filters_.end(),
                      [](const std::unique_ptr<Filter>& f) {
                          return !f || !f->isEnabled();
                      }),
        filters_.end()
    );
    
    optimizeFilterOrder();
    mergeCompatibleFilters();
}

std::vector<uint8_t> FilterChain::serialize() const {
    // Would implement proper serialization
    return {};
}

bool FilterChain::deserialize(const std::vector<uint8_t>& data) {
    // Would implement deserialization
    return true;
}

void FilterChain::optimizeFilterOrder() {
    // Reorder filters for better performance
    // For example, put expensive filters last
    std::sort(filters_.begin(), filters_.end(),
              [](const std::unique_ptr<Filter>& a, const std::unique_ptr<Filter>& b) {
                  return a->getComplexityScore() < b->getComplexityScore();
              });
}

void FilterChain::mergeCompatibleFilters() {
    // Would implement filter merging optimization
    // For example, merge multiple blur filters into one
}

// FilterProcessor implementation
FilterProcessor::FilterProcessor(Rendering::RenderingEngine& engine) : engine_(engine) {
    maxConcurrency_ = std::thread::hardware_concurrency();
    if (maxConcurrency_ == 0) {
        maxConcurrency_ = 4; // Default to 4 threads
    }
    
    std::cout << "[FilterProcessor] Initialized with " << maxConcurrency_ << " threads" << std::endl;
}

FilterProcessor::~FilterProcessor() {
    shutdown();
}

FilterProcessor::FilterProcessor(FilterProcessor&& other) noexcept
    : engine_(other.engine_)
    , initialized_(other.initialized_.load())
    , filterFactories_(std::move(other.filterFactories_))
    , previewActive_(other.previewActive_.load())
    , memoryBudget_(other.memoryBudget_)
    , preferGPU_(other.preferGPU_)
    , maxConcurrency_(other.maxConcurrency_)
    , stats_(other.stats_) {
}

FilterProcessor& FilterProcessor::operator=(FilterProcessor&& other) noexcept {
    if (this != &other) {
        engine_ = other.engine_;
        initialized_ = other.initialized_.load();
        filterFactories_ = std::move(other.filterFactories_);
        previewActive_ = other.previewActive_.load();
        memoryBudget_ = other.memoryBudget_;
        preferGPU_ = other.preferGPU_;
        maxConcurrency_ = other.maxConcurrency_;
        stats_ = other.stats_;
    }
    return *this;
}

bool FilterProcessor::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!createUniformBuffers()) {
        std::cerr << "[FilterProcessor] Failed to create uniform buffers" << std::endl;
        return false;
    }
    
    registerBuiltInFilters();
    
    initialized_ = true;
    std::cout << "[FilterProcessor] Initialization complete" << std::endl;
    return true;
}

void FilterProcessor::shutdown() {
    if (!initialized_) {
        return;
    }
    
    endPreview();
    clearCache();
    destroyResources();
    
    initialized_ = false;
    std::cout << "[FilterProcessor] Shutdown complete" << std::endl;
}

void FilterProcessor::registerFilter(const std::string& name, 
                                    std::function<std::unique_ptr<Filter>()> factory) {
    std::lock_guard<std::mutex> lock(factoriesMutex_);
    filterFactories_[name] = factory;
}

void FilterProcessor::unregisterFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(factoriesMutex_);
    filterFactories_.erase(name);
}

std::vector<std::string> FilterProcessor::getAvailableFilters() const {
    std::lock_guard<std::mutex> lock(factoriesMutex_);
    std::vector<std::string> names;
    names.reserve(filterFactories_.size());
    
    for (const auto& [name, factory] : filterFactories_) {
        names.push_back(name);
    }
    
    return names;
}

std::unique_ptr<Filter> FilterProcessor::createFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(factoriesMutex_);
    auto it = filterFactories_.find(name);
    if (it != filterFactories_.end()) {
        return it->second();
    }
    return nullptr;
}

bool FilterProcessor::applyFilter(Filter* filter, const Image& input, Image& output) {
    if (!filter || !initialized_) {
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    bool success = false;
    if (preferGPU_ && filter->isGPUAccelerated()) {
        // Try GPU first
        // Would implement GPU path
        success = false; // Fall back to CPU for now
    }
    
    if (!success) {
        success = filter->apply(input, output);
    }
    
    if (success) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.filtersApplied++;
        stats_.pixelsProcessed += input.width * input.height;
        stats_.processingTime += duration;
    }
    
    return success;
}

bool FilterProcessor::applyFilterGPU(Filter* filter, 
                                   Rendering::ResourceId inputTexture,
                                   Rendering::ResourceId outputTexture,
                                   const std::array<uint32_t, 2>& size) {
    if (!filter || !initialized_) {
        return false;
    }
    
    return filter->applyGPU(inputTexture, outputTexture, size);
}

bool FilterProcessor::applyFilterChain(const FilterChain& chain, const Image& input, Image& output) {
    if (!initialized_) {
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    bool success = chain.apply(input, output);
    
    if (success) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.chainsApplied++;
        stats_.pixelsProcessed += input.width * input.height;
        stats_.processingTime += duration;
        stats_.averageComplexityScore = chain.getTotalComplexityScore();
    }
    
    return success;
}

bool FilterProcessor::applyFilterChainGPU(const FilterChain& chain,
                                        Rendering::ResourceId inputTexture,
                                        Rendering::ResourceId outputTexture,
                                        const std::array<uint32_t, 2>& size) {
    return chain.applyGPU(inputTexture, outputTexture, size);
}

void FilterProcessor::processBatch(const std::vector<BatchJob>& jobs,
                                 std::function<void(size_t, bool, const std::string&)> progressCallback) {
    if (!initialized_) {
        return;
    }
    
    std::cout << "[FilterProcessor] Processing batch of " << jobs.size() << " jobs" << std::endl;
    
    // Process jobs in parallel
    const size_t numThreads = std::min(maxConcurrency_, static_cast<uint32_t>(jobs.size()));
    std::vector<std::future<void>> futures;
    
    for (size_t threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        futures.push_back(std::async(std::launch::async, [&, threadIndex]() {
            for (size_t jobIndex = threadIndex; jobIndex < jobs.size(); jobIndex += numThreads) {
                const auto& job = jobs[jobIndex];
                
                try {
                    // Load input image (simplified)
                    Image input(1024, 1024); // Dummy size
                    
                    // Apply filter chain
                    Image output(input.width, input.height);
                    bool success = applyFilterChain(job.filterChain, input, output);
                    
                    if (progressCallback) {
                        progressCallback(jobIndex, success, success ? "" : "Filter application failed");
                    }
                    
                } catch (const std::exception& e) {
                    if (progressCallback) {
                        progressCallback(jobIndex, false, e.what());
                    }
                }
            }
        }));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    std::cout << "[FilterProcessor] Batch processing complete" << std::endl;
}

void FilterProcessor::beginPreview() {
    previewActive_ = true;
    
    // Create preview texture
    previewTexture_ = engine_.create_texture_2d(
        previewSize_[0], previewSize_[1], 
        Rendering::TextureFormat::RGBA8Unorm,
        Rendering::TextureUsage::RenderTarget | Rendering::TextureUsage::Sampled
    );
}

void FilterProcessor::updatePreviewFilter(Filter* filter) {
    if (!previewActive_ || !filter) {
        return;
    }
    
    // Create single-filter chain
    FilterChain chain;
    chain.addFilter(filter->clone());
    updatePreviewChain(chain);
}

void FilterProcessor::updatePreviewChain(const FilterChain& chain) {
    if (!previewActive_) {
        return;
    }
    
    previewChain_ = std::make_unique<FilterChain>(chain);
    
    // Apply to preview (would use actual preview input)
    Image previewInput(previewSize_[0], previewSize_[1]);
    Image previewOutput(previewSize_[0], previewSize_[1]);
    
    applyFilterChain(*previewChain_, previewInput, previewOutput);
}

void FilterProcessor::endPreview() {
    if (!previewActive_) {
        return;
    }
    
    if (previewTexture_ != 0) {
        engine_.destroy_texture(previewTexture_);
        previewTexture_ = 0;
    }
    
    previewChain_.reset();
    previewActive_ = false;
}

size_t FilterProcessor::getMemoryUsage() const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    size_t totalMemory = 0;
    for (const auto& [hash, entry] : textureCache_) {
        totalMemory += entry.memorySize;
    }
    
    return totalMemory;
}

void FilterProcessor::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    
    for (const auto& [hash, entry] : textureCache_) {
        if (entry.textureId != 0) {
            engine_.destroy_texture(entry.textureId);
        }
    }
    
    textureCache_.clear();
    std::cout << "[FilterProcessor] Cache cleared" << std::endl;
}

FilterProcessor::FilterProcessorStats FilterProcessor::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void FilterProcessor::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = FilterProcessorStats{};
}

bool FilterProcessor::createUniformBuffers() {
    // Create uniform buffer for filter parameters
    filterUniformBuffer_ = engine_.create_buffer(
        1024, // 1KB for filter parameters
        Rendering::BufferUsage::Uniform | Rendering::BufferUsage::Dynamic
    );
    
    return filterUniformBuffer_ != 0;
}

void FilterProcessor::destroyResources() {
    if (filterUniformBuffer_ != 0) {
        engine_.destroy_buffer(filterUniformBuffer_);
        filterUniformBuffer_ = 0;
    }
}

void FilterProcessor::registerBuiltInFilters() {
    registerFilter("GaussianBlur", []() { return std::make_unique<GaussianBlurFilter>(); });
    registerFilter("UnsharpMask", []() { return std::make_unique<UnsharpMaskFilter>(); });
    registerFilter("EdgeDetection", []() { return std::make_unique<EdgeDetectionFilter>(); });
    
    std::cout << "[FilterProcessor] Built-in filters registered" << std::endl;
}

// FilterUtils implementation
namespace FilterUtils {
    std::vector<float> createGaussianKernel(float sigma, int& kernelSize) {
        kernelSize = static_cast<int>(std::ceil(sigma * 3.0f)) * 2 + 1;
        std::vector<float> kernel(kernelSize);
        
        float sum = 0.0f;
        int center = kernelSize / 2;
        
        for (int i = 0; i < kernelSize; ++i) {
            float x = i - center;
            kernel[i] = std::exp(-(x * x) / (2 * sigma * sigma));
            sum += kernel[i];
        }
        
        for (float& weight : kernel) {
            weight /= sum;
        }
        
        return kernel;
    }
    
    std::array<float, 4> sampleBilinear(const Image& image, float x, float y) {
        return image.sampleBilinear(x, y);
    }
    
    std::array<float, 3> RGBtoHSV(const std::array<float, 3>& rgb) {
        float max_val = std::max({rgb[0], rgb[1], rgb[2]});
        float min_val = std::min({rgb[0], rgb[1], rgb[2]});
        float delta = max_val - min_val;
        
        float h = 0, s = 0, v = max_val;
        
        if (delta > 0) {
            s = delta / max_val;
            
            if (max_val == rgb[0]) {
                h = 60 * (std::fmod((rgb[1] - rgb[2]) / delta, 6.0f));
            } else if (max_val == rgb[1]) {
                h = 60 * ((rgb[2] - rgb[0]) / delta + 2);
            } else {
                h = 60 * ((rgb[0] - rgb[1]) / delta + 4);
            }
            
            if (h < 0) h += 360;
        }
        
        return {h, s, v};
    }
    
    std::array<float, 3> HSVtoRGB(const std::array<float, 3>& hsv) {
        float h = hsv[0], s = hsv[1], v = hsv[2];
        
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        
        std::array<float, 3> rgb;
        if (h >= 0 && h < 60) {
            rgb = {c, x, 0};
        } else if (h >= 60 && h < 120) {
            rgb = {x, c, 0};
        } else if (h >= 120 && h < 180) {
            rgb = {0, c, x};
        } else if (h >= 180 && h < 240) {
            rgb = {0, x, c};
        } else if (h >= 240 && h < 300) {
            rgb = {x, 0, c};
        } else {
            rgb = {c, 0, x};
        }
        
        return {rgb[0] + m, rgb[1] + m, rgb[2] + m};
    }
    
    bool shouldUseGPU(const std::array<uint32_t, 2>& imageSize, float complexityScore) {
        size_t pixelCount = imageSize[0] * imageSize[1];
        return pixelCount > 1024 * 1024 && complexityScore > 1.5f; // Use GPU for large, complex filters
    }
    
    size_t estimateFilterMemoryUsage(const std::array<uint32_t, 2>& imageSize, float complexityScore) {
        size_t baseMemory = imageSize[0] * imageSize[1] * 4 * sizeof(float);
        return static_cast<size_t>(baseMemory * (1 + complexityScore));
    }
}

} // namespace QuantumCanvas::Raster