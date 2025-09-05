#include "layer_compositor.hpp"
#include "image.hpp"
#include "../../core/memory/memory_manager.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <stdexcept>

namespace QuantumCanvas::Raster {

// Layer implementation
uint32_t Layer::nextId_ = 1;

Layer::Layer(const std::string& name) 
    : id_(nextId_++), name_(name) {}

Layer::~Layer() = default;

Layer::Layer(Layer&& other) noexcept
    : id_(other.id_)
    , name_(std::move(other.name_))
    , flags_(other.flags_)
    , blendMode_(other.blendMode_)
    , opacity_(other.opacity_)
    , transform_(other.transform_)
    , mask_(std::move(other.mask_))
    , effects_(std::move(other.effects_))
    , dirty_(other.dirty_.load()) {
    other.id_ = 0;
}

Layer& Layer::operator=(Layer&& other) noexcept {
    if (this != &other) {
        id_ = other.id_;
        name_ = std::move(other.name_);
        flags_ = other.flags_;
        blendMode_ = other.blendMode_;
        opacity_ = other.opacity_;
        transform_ = other.transform_;
        mask_ = std::move(other.mask_);
        effects_ = std::move(other.effects_);
        dirty_ = other.dirty_.load();
        other.id_ = 0;
    }
    return *this;
}

void Layer::setVisible(bool visible) {
    if (visible) {
        flags_ |= LayerFlags::Visible;
    } else {
        flags_ &= ~LayerFlags::Visible;
    }
    markDirty();
}

void Layer::setLocked(bool locked) {
    if (locked) {
        flags_ |= LayerFlags::Locked;
    } else {
        flags_ &= ~LayerFlags::Locked;
    }
}

void Layer::removeEffect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
        markDirty();
    }
}

std::vector<uint8_t> Layer::serialize() const {
    // Basic serialization - would need proper format in production
    std::vector<uint8_t> data;
    
    // Serialize ID and name length
    data.resize(sizeof(id_) + sizeof(uint32_t) + name_.size() + sizeof(flags_) + 
                sizeof(blendMode_) + sizeof(opacity_) + sizeof(transform_));
    
    size_t offset = 0;
    std::memcpy(data.data() + offset, &id_, sizeof(id_));
    offset += sizeof(id_);
    
    uint32_t nameLen = static_cast<uint32_t>(name_.size());
    std::memcpy(data.data() + offset, &nameLen, sizeof(nameLen));
    offset += sizeof(nameLen);
    
    std::memcpy(data.data() + offset, name_.data(), name_.size());
    offset += name_.size();
    
    std::memcpy(data.data() + offset, &flags_, sizeof(flags_));
    offset += sizeof(flags_);
    
    std::memcpy(data.data() + offset, &blendMode_, sizeof(blendMode_));
    offset += sizeof(blendMode_);
    
    std::memcpy(data.data() + offset, &opacity_, sizeof(opacity_));
    offset += sizeof(opacity_);
    
    std::memcpy(data.data() + offset, &transform_, sizeof(transform_));
    
    return data;
}

bool Layer::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(id_) + sizeof(uint32_t)) {
        return false;
    }
    
    size_t offset = 0;
    std::memcpy(&id_, data.data() + offset, sizeof(id_));
    offset += sizeof(id_);
    
    uint32_t nameLen;
    std::memcpy(&nameLen, data.data() + offset, sizeof(nameLen));
    offset += sizeof(nameLen);
    
    if (offset + nameLen > data.size()) {
        return false;
    }
    
    name_.assign(reinterpret_cast<const char*>(data.data() + offset), nameLen);
    offset += nameLen;
    
    if (data.size() < offset + sizeof(flags_) + sizeof(blendMode_) + sizeof(opacity_) + sizeof(transform_)) {
        return false;
    }
    
    std::memcpy(&flags_, data.data() + offset, sizeof(flags_));
    offset += sizeof(flags_);
    
    std::memcpy(&blendMode_, data.data() + offset, sizeof(blendMode_));
    offset += sizeof(blendMode_);
    
    std::memcpy(&opacity_, data.data() + offset, sizeof(opacity_));
    offset += sizeof(opacity_);
    
    std::memcpy(&transform_, data.data() + offset, sizeof(transform_));
    
    markDirty();
    return true;
}

// LayerTransform implementation
std::array<float, 9> LayerTransform::getMatrix() const {
    if (usePerspective) {
        // Convert 4x4 perspective to 3x3 affine (simplified)
        return {
            perspectiveMatrix[0], perspectiveMatrix[1], perspectiveMatrix[3],
            perspectiveMatrix[4], perspectiveMatrix[5], perspectiveMatrix[7],
            perspectiveMatrix[12], perspectiveMatrix[13], perspectiveMatrix[15]
        };
    }
    
    // Build affine transformation matrix
    float cos_r = std::cos(rotation);
    float sin_r = std::sin(rotation);
    
    // Translation to anchor point
    float ax = anchor[0];
    float ay = anchor[1];
    
    // Combined transformation matrix
    return {
        scale[0] * cos_r, -scale[0] * sin_r, position[0] + ax * (1 - scale[0] * cos_r) + ay * scale[0] * sin_r,
        scale[1] * sin_r,  scale[1] * cos_r, position[1] + ay * (1 - scale[1] * cos_r) - ax * scale[1] * sin_r,
        0.0f, 0.0f, 1.0f
    };
}

std::array<float, 2> LayerTransform::transformPoint(const std::array<float, 2>& point) const {
    auto matrix = getMatrix();
    return LayerUtils::transformPoint(point, matrix);
}

LayerTransform LayerTransform::inverse() const {
    LayerTransform inv;
    inv.position = {-position[0], -position[1]};
    inv.scale = {1.0f / scale[0], 1.0f / scale[1]};
    inv.rotation = -rotation;
    inv.skew = {-skew[0], -skew[1]};
    inv.anchor = anchor;
    return inv;
}

LayerTransform LayerTransform::compose(const LayerTransform& other) const {
    auto thisMatrix = getMatrix();
    auto otherMatrix = other.getMatrix();
    auto combined = LayerUtils::multiplyMatrices(thisMatrix, otherMatrix);
    
    LayerTransform result;
    // Extract components from combined matrix (simplified)
    result.position = {combined[2], combined[5]};
    result.scale = {std::sqrt(combined[0]*combined[0] + combined[1]*combined[1]),
                   std::sqrt(combined[3]*combined[3] + combined[4]*combined[4])};
    result.rotation = std::atan2(combined[1], combined[0]);
    
    return result;
}

void LayerTransform::reset() {
    position = {0.0f, 0.0f};
    scale = {1.0f, 1.0f};
    rotation = 0.0f;
    skew = {0.0f, 0.0f};
    anchor = {0.5f, 0.5f};
    perspectiveMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    usePerspective = false;
}

bool LayerTransform::operator==(const LayerTransform& other) const {
    return position == other.position &&
           scale == other.scale &&
           std::abs(rotation - other.rotation) < 1e-6f &&
           skew == other.skew &&
           anchor == other.anchor &&
           usePerspective == other.usePerspective;
}

// RasterLayer implementation
RasterLayer::RasterLayer(const std::string& name) 
    : Layer(name) {}

RasterLayer::RasterLayer(std::unique_ptr<Image> image, const std::string& name)
    : Layer(name), image_(std::move(image)) {}

RasterLayer::~RasterLayer() = default;

Rendering::ResourceId RasterLayer::getContentTexture() const {
    if (!image_) return 0;
    
    if (textureDirty_) {
        updateTexture();
    }
    
    return cachedTexture_;
}

std::array<uint32_t, 2> RasterLayer::getContentSize() const {
    if (!image_) return {0, 0};
    return image_->getSize();
}

std::array<float, 4> RasterLayer::getBounds() const {
    if (!image_) return {0.0f, 0.0f, 0.0f, 0.0f};
    
    auto size = image_->getSize();
    return {0.0f, 0.0f, static_cast<float>(size[0]), static_cast<float>(size[1])};
}

void RasterLayer::setImage(std::unique_ptr<Image> image) {
    image_ = std::move(image);
    textureDirty_ = true;
    markDirty();
}

void RasterLayer::resize(const std::array<uint32_t, 2>& newSize) {
    if (!image_) return;
    
    // image_->resize(newSize); // Would need Image::resize implementation
    textureDirty_ = true;
    markDirty();
}

void RasterLayer::clear(const std::array<float, 4>& color) {
    if (!image_) return;
    
    // image_->clear(color); // Would need Image::clear implementation
    textureDirty_ = true;
    markDirty();
}

void RasterLayer::updateTexture() const {
    if (!image_ || !textureDirty_) return;
    
    // Update GPU texture from image data
    // This would interface with RenderingEngine to upload texture
    textureDirty_ = false;
}

std::vector<uint8_t> RasterLayer::serialize() const {
    auto baseData = Layer::serialize();
    
    if (image_) {
        // auto imageData = image_->serialize(); // Would need Image serialization
        // baseData.insert(baseData.end(), imageData.begin(), imageData.end());
    }
    
    return baseData;
}

bool RasterLayer::deserialize(const std::vector<uint8_t>& data) {
    if (!Layer::deserialize(data)) {
        return false;
    }
    
    // Deserialize image data
    textureDirty_ = true;
    return true;
}

// AdjustmentLayer implementation
AdjustmentLayer::AdjustmentLayer(AdjustmentType type, const std::string& name)
    : Layer(name), adjustmentType_(type) {
    initializeDefaultParameters();
}

AdjustmentLayer::~AdjustmentLayer() = default;

Rendering::ResourceId AdjustmentLayer::getContentTexture() const {
    if (parametersDirty_) {
        updateAdjustmentTexture();
    }
    return adjustmentTexture_;
}

std::array<uint32_t, 2> AdjustmentLayer::getContentSize() const {
    // Adjustment layers typically inherit size from document
    return {1024, 1024}; // Default size
}

std::array<float, 4> AdjustmentLayer::getBounds() const {
    // Adjustment layers affect entire document
    return {-1e6f, -1e6f, 1e6f, 1e6f};
}

void AdjustmentLayer::setParameter(const std::string& name, float value) {
    parameters_[name] = value;
    parametersDirty_ = true;
    markDirty();
}

float AdjustmentLayer::getParameter(const std::string& name) const {
    auto it = parameters_.find(name);
    return (it != parameters_.end()) ? it->second : 0.0f;
}

void AdjustmentLayer::updateAdjustmentTexture() const {
    if (!parametersDirty_) return;
    
    // Generate adjustment lookup table or shader parameters
    // This would create appropriate GPU resources
    parametersDirty_ = false;
}

void AdjustmentLayer::initializeDefaultParameters() {
    switch (adjustmentType_) {
        case Brightness:
            parameters_["brightness"] = 0.0f;
            break;
        case Contrast:
            parameters_["contrast"] = 0.0f;
            break;
        case Levels:
            parameters_["input_black"] = 0.0f;
            parameters_["input_white"] = 1.0f;
            parameters_["gamma"] = 1.0f;
            parameters_["output_black"] = 0.0f;
            parameters_["output_white"] = 1.0f;
            break;
        case HueSaturation:
            parameters_["hue"] = 0.0f;
            parameters_["saturation"] = 0.0f;
            parameters_["lightness"] = 0.0f;
            break;
        default:
            break;
    }
}

std::vector<uint8_t> AdjustmentLayer::serialize() const {
    auto baseData = Layer::serialize();
    
    // Serialize adjustment type and parameters
    baseData.resize(baseData.size() + sizeof(adjustmentType_) + sizeof(uint32_t));
    
    size_t offset = baseData.size() - sizeof(adjustmentType_) - sizeof(uint32_t);
    std::memcpy(baseData.data() + offset, &adjustmentType_, sizeof(adjustmentType_));
    offset += sizeof(adjustmentType_);
    
    uint32_t paramCount = static_cast<uint32_t>(parameters_.size());
    std::memcpy(baseData.data() + offset, &paramCount, sizeof(paramCount));
    
    for (const auto& [name, value] : parameters_) {
        uint32_t nameLen = static_cast<uint32_t>(name.size());
        baseData.resize(baseData.size() + sizeof(nameLen) + name.size() + sizeof(value));
        
        std::memcpy(baseData.data() + baseData.size() - sizeof(nameLen) - name.size() - sizeof(value),
                   &nameLen, sizeof(nameLen));
        std::memcpy(baseData.data() + baseData.size() - name.size() - sizeof(value),
                   name.data(), name.size());
        std::memcpy(baseData.data() + baseData.size() - sizeof(value),
                   &value, sizeof(value));
    }
    
    return baseData;
}

bool AdjustmentLayer::deserialize(const std::vector<uint8_t>& data) {
    if (!Layer::deserialize(data)) {
        return false;
    }
    
    // Deserialize adjustment data
    parametersDirty_ = true;
    return true;
}

// LayerGroup implementation
LayerGroup::LayerGroup(const std::string& name)
    : Layer(name) {}

LayerGroup::~LayerGroup() = default;

Rendering::ResourceId LayerGroup::getContentTexture() const {
    if (contentDirty_) {
        updateGroupTexture();
    }
    return groupTexture_;
}

std::array<uint32_t, 2> LayerGroup::getContentSize() const {
    return groupSize_;
}

std::array<float, 4> LayerGroup::getBounds() const {
    if (layers_.empty()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    auto bounds = layers_[0]->getBounds();
    for (size_t i = 1; i < layers_.size(); ++i) {
        bounds = LayerUtils::unionBounds(bounds, layers_[i]->getBounds());
    }
    
    return bounds;
}

void LayerGroup::addLayer(std::unique_ptr<Layer> layer) {
    if (layer) {
        layers_.push_back(std::move(layer));
        contentDirty_ = true;
        markDirty();
    }
}

void LayerGroup::insertLayer(size_t index, std::unique_ptr<Layer> layer) {
    if (layer && index <= layers_.size()) {
        layers_.insert(layers_.begin() + index, std::move(layer));
        contentDirty_ = true;
        markDirty();
    }
}

std::unique_ptr<Layer> LayerGroup::removeLayer(size_t index) {
    if (index < layers_.size()) {
        auto layer = std::move(layers_[index]);
        layers_.erase(layers_.begin() + index);
        contentDirty_ = true;
        markDirty();
        return layer;
    }
    return nullptr;
}

std::unique_ptr<Layer> LayerGroup::removeLayer(uint32_t layerId) {
    auto it = std::find_if(layers_.begin(), layers_.end(),
                          [layerId](const std::unique_ptr<Layer>& layer) {
                              return layer->getId() == layerId;
                          });
    
    if (it != layers_.end()) {
        auto layer = std::move(*it);
        layers_.erase(it);
        contentDirty_ = true;
        markDirty();
        return layer;
    }
    return nullptr;
}

void LayerGroup::moveLayer(size_t fromIndex, size_t toIndex) {
    if (fromIndex < layers_.size() && toIndex < layers_.size() && fromIndex != toIndex) {
        auto layer = std::move(layers_[fromIndex]);
        layers_.erase(layers_.begin() + fromIndex);
        layers_.insert(layers_.begin() + toIndex, std::move(layer));
        contentDirty_ = true;
        markDirty();
    }
}

Layer* LayerGroup::getLayer(size_t index) {
    return (index < layers_.size()) ? layers_[index].get() : nullptr;
}

const Layer* LayerGroup::getLayer(size_t index) const {
    return (index < layers_.size()) ? layers_[index].get() : nullptr;
}

Layer* LayerGroup::findLayer(uint32_t layerId) {
    auto it = std::find_if(layers_.begin(), layers_.end(),
                          [layerId](const std::unique_ptr<Layer>& layer) {
                              return layer->getId() == layerId;
                          });
    return (it != layers_.end()) ? it->get() : nullptr;
}

const Layer* LayerGroup::findLayer(uint32_t layerId) const {
    auto it = std::find_if(layers_.begin(), layers_.end(),
                          [layerId](const std::unique_ptr<Layer>& layer) {
                              return layer->getId() == layerId;
                          });
    return (it != layers_.end()) ? it->get() : nullptr;
}

void LayerGroup::setPassThrough(bool passThrough) {
    if (passThrough) {
        flags_ |= LayerFlags::PassThrough;
    } else {
        flags_ &= ~LayerFlags::PassThrough;
    }
    markDirty();
}

void LayerGroup::setIsolated(bool isolated) {
    if (isolated) {
        flags_ |= LayerFlags::Isolated;
    } else {
        flags_ &= ~LayerFlags::Isolated;
    }
    markDirty();
}

void LayerGroup::updateGroupTexture() const {
    if (!contentDirty_) return;
    
    // Composite all child layers into group texture
    // This would use LayerCompositor to render all layers
    contentDirty_ = false;
}

std::vector<uint8_t> LayerGroup::serialize() const {
    auto baseData = Layer::serialize();
    
    // Serialize layer count
    uint32_t layerCount = static_cast<uint32_t>(layers_.size());
    baseData.resize(baseData.size() + sizeof(layerCount));
    std::memcpy(baseData.data() + baseData.size() - sizeof(layerCount), 
               &layerCount, sizeof(layerCount));
    
    // Serialize each layer
    for (const auto& layer : layers_) {
        auto layerData = layer->serialize();
        baseData.insert(baseData.end(), layerData.begin(), layerData.end());
    }
    
    return baseData;
}

bool LayerGroup::deserialize(const std::vector<uint8_t>& data) {
    if (!Layer::deserialize(data)) {
        return false;
    }
    
    // Deserialize layers
    contentDirty_ = true;
    return true;
}

// LayerCompositor implementation
LayerCompositor::LayerCompositor(Rendering::RenderingEngine& engine)
    : engine_(engine) {}

LayerCompositor::~LayerCompositor() {
    shutdown();
}

LayerCompositor::LayerCompositor(LayerCompositor&& other) noexcept
    : engine_(other.engine_)
    , initialized_(other.initialized_.load())
    , blendPipelines_(std::move(other.blendPipelines_))
    , transformPipelineId_(other.transformPipelineId_)
    , maskPipelineId_(other.maskPipelineId_)
    , effectPipelines_(std::move(other.effectPipelines_))
    , customBlendPipelines_(std::move(other.customBlendPipelines_))
    , blendUniformId_(other.blendUniformId_)
    , transformUniformId_(other.transformUniformId_)
    , effectUniformId_(other.effectUniformId_)
    , multisampleCount_(other.multisampleCount_)
    , colorSpace_(other.colorSpace_)
    , clippingMask_(other.clippingMask_)
    , hasClippingMask_(other.hasClippingMask_)
    , stats_(other.stats_) {
    
    other.initialized_ = false;
    other.transformPipelineId_ = 0;
    other.maskPipelineId_ = 0;
    other.blendUniformId_ = 0;
    other.transformUniformId_ = 0;
    other.effectUniformId_ = 0;
    other.clippingMask_ = 0;
    other.hasClippingMask_ = false;
}

LayerCompositor& LayerCompositor::operator=(LayerCompositor&& other) noexcept {
    if (this != &other) {
        shutdown();
        
        initialized_ = other.initialized_.load();
        blendPipelines_ = std::move(other.blendPipelines_);
        transformPipelineId_ = other.transformPipelineId_;
        maskPipelineId_ = other.maskPipelineId_;
        effectPipelines_ = std::move(other.effectPipelines_);
        customBlendPipelines_ = std::move(other.customBlendPipelines_);
        blendUniformId_ = other.blendUniformId_;
        transformUniformId_ = other.transformUniformId_;
        effectUniformId_ = other.effectUniformId_;
        multisampleCount_ = other.multisampleCount_;
        colorSpace_ = other.colorSpace_;
        clippingMask_ = other.clippingMask_;
        hasClippingMask_ = other.hasClippingMask_;
        stats_ = other.stats_;
        
        other.initialized_ = false;
        other.transformPipelineId_ = 0;
        other.maskPipelineId_ = 0;
        other.blendUniformId_ = 0;
        other.transformUniformId_ = 0;
        other.effectUniformId_ = 0;
        other.clippingMask_ = 0;
        other.hasClippingMask_ = false;
    }
    return *this;
}

bool LayerCompositor::initialize() {
    if (initialized_) return true;
    
    try {
        if (!createBlendPipelines()) {
            return false;
        }
        
        if (!createEffectPipelines()) {
            return false;
        }
        
        if (!createUniformBuffers()) {
            return false;
        }
        
        initialized_ = true;
        return true;
    }
    catch (const std::exception&) {
        shutdown();
        return false;
    }
}

void LayerCompositor::shutdown() {
    if (!initialized_) return;
    
    destroyResources();
    initialized_ = false;
}

void LayerCompositor::compositeToTarget(const std::vector<Layer*>& layers,
                                       Rendering::ResourceId targetTexture,
                                       const std::array<uint32_t, 2>& targetSize,
                                       const std::array<float, 4>& bounds) {
    if (!initialized_ || layers.empty()) return;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.layersComposited = 0;
    stats_.blendOperations = 0;
    stats_.transformOperations = 0;
    stats_.pixelsProcessed = static_cast<uint64_t>(targetSize[0]) * targetSize[1];
    
    // Set render target
    engine_.setRenderTarget(targetTexture, targetSize);
    
    // Clear target if needed
    engine_.clearRenderTarget({0.0f, 0.0f, 0.0f, 0.0f});
    
    // Calculate effective bounds
    auto effectiveBounds = bounds;
    if (bounds[0] == bounds[2] && bounds[1] == bounds[3]) {
        effectiveBounds = calculateLayerBounds(layers);
    }
    
    // Composite each visible layer
    for (Layer* layer : layers) {
        if (!layer || !layer->isVisible()) continue;
        
        if (!layerIntersectsBounds(layer, effectiveBounds)) continue;
        
        compositeLayer(layer, targetTexture, targetSize, effectiveBounds);
        stats_.layersComposited++;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    stats_.compositionTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
}

Rendering::ResourceId LayerCompositor::compositeToTexture(const std::vector<Layer*>& layers,
                                                        const std::array<uint32_t, 2>& size,
                                                        const std::array<float, 4>& bounds) {
    if (!initialized_ || layers.empty()) return 0;
    
    // Create temporary render target
    auto targetTexture = engine_.createTexture(size[0], size[1], 
                                              Rendering::PixelFormat::RGBA8,
                                              Rendering::TextureUsage::RenderTarget);
    
    if (targetTexture == 0) return 0;
    
    compositeToTarget(layers, targetTexture, size, bounds);
    return targetTexture;
}

void LayerCompositor::compositeLayer(Layer* layer,
                                    Rendering::ResourceId targetTexture,
                                    const std::array<uint32_t, 2>& targetSize,
                                    const std::array<float, 4>& bounds) {
    if (!layer || !layer->isVisible()) return;
    
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    // Get layer content
    auto contentTexture = layer->getContentTexture();
    if (contentTexture == 0) return;
    
    // Apply layer transform
    applyLayerTransform(layer, targetSize);
    stats_.transformOperations++;
    
    // Apply layer mask if present
    if (layer->hasMask()) {
        applyLayerMask(layer, contentTexture);
    }
    
    // Apply layer effects
    auto processedTexture = contentTexture;
    const auto& effects = layer->getEffects();
    for (const auto& effect : effects) {
        if (effect.enabled) {
            processedTexture = applyLayerEffects(layer, processedTexture, layer->getContentSize());
            stats_.effectsApplied++;
        }
    }
    
    // Blend with target
    blendTextures(targetTexture, processedTexture, targetTexture,
                 layer->getBlendMode(), layer->getOpacity(), targetSize);
    stats_.blendOperations++;
}

void LayerCompositor::blendTextures(Rendering::ResourceId baseTexture,
                                   Rendering::ResourceId overlayTexture,
                                   Rendering::ResourceId targetTexture,
                                   BlendMode blendMode,
                                   float opacity,
                                   const std::array<uint32_t, 2>& size) {
    if (!initialized_) return;
    
    auto pipeline = getBlendPipeline(blendMode);
    if (pipeline == 0) return;
    
    // Update blend uniforms
    updateBlendUniforms(blendMode, opacity);
    
    // Set pipeline and textures
    engine_.setPipeline(pipeline);
    engine_.setTexture(0, baseTexture);
    engine_.setTexture(1, overlayTexture);
    engine_.setUniformBuffer(0, blendUniformId_);
    
    // Draw fullscreen quad
    engine_.drawFullscreenQuad();
}

void LayerCompositor::registerCustomBlendMode(BlendMode mode, const std::string& shaderCode) {
    if (mode < BlendMode::Custom) return;
    
    // Create custom pipeline with provided shader code
    // This would compile and store the custom blend shader
}

void LayerCompositor::unregisterCustomBlendMode(BlendMode mode) {
    auto it = customBlendPipelines_.find(mode);
    if (it != customBlendPipelines_.end()) {
        engine_.destroyPipeline(it->second);
        customBlendPipelines_.erase(it);
    }
}

Rendering::ResourceId LayerCompositor::applyLayerEffects(Layer* layer,
                                                       Rendering::ResourceId sourceTexture,
                                                       const std::array<uint32_t, 2>& size) {
    // Apply each effect in sequence
    auto currentTexture = sourceTexture;
    
    const auto& effects = layer->getEffects();
    for (const auto& effect : effects) {
        if (!effect.enabled) continue;
        
        auto it = effectPipelines_.find(effect.type);
        if (it == effectPipelines_.end()) continue;
        
        // Create temporary texture for effect output
        auto effectTexture = engine_.createTexture(size[0], size[1],
                                                  Rendering::PixelFormat::RGBA8,
                                                  Rendering::TextureUsage::RenderTarget);
        
        updateEffectUniforms(effect);
        
        engine_.setPipeline(it->second);
        engine_.setTexture(0, currentTexture);
        engine_.setUniformBuffer(0, effectUniformId_);
        engine_.setRenderTarget(effectTexture, size);
        engine_.drawFullscreenQuad();
        
        // Clean up previous temporary texture
        if (currentTexture != sourceTexture) {
            engine_.destroyTexture(currentTexture);
        }
        
        currentTexture = effectTexture;
    }
    
    return currentTexture;
}

void LayerCompositor::setClippingMask(Rendering::ResourceId maskTexture) {
    clippingMask_ = maskTexture;
    hasClippingMask_ = true;
}

void LayerCompositor::clearClippingMask() {
    clippingMask_ = 0;
    hasClippingMask_ = false;
}

LayerCompositor::CompositionStats LayerCompositor::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void LayerCompositor::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = CompositionStats{};
}

bool LayerCompositor::createBlendPipelines() {
    // Create pipelines for each blend mode
    const std::array<BlendMode, 25> blendModes = {
        BlendMode::Normal, BlendMode::Dissolve,
        BlendMode::Darken, BlendMode::Multiply, BlendMode::ColorBurn, BlendMode::LinearBurn, BlendMode::DarkerColor,
        BlendMode::Lighten, BlendMode::Screen, BlendMode::ColorDodge, BlendMode::LinearDodge, BlendMode::LighterColor,
        BlendMode::Overlay, BlendMode::SoftLight, BlendMode::HardLight, BlendMode::VividLight,
        BlendMode::LinearLight, BlendMode::PinLight, BlendMode::HardMix,
        BlendMode::Difference, BlendMode::Exclusion, BlendMode::Subtract, BlendMode::Divide,
        BlendMode::Hue, BlendMode::Saturation
    };
    
    for (auto mode : blendModes) {
        auto shaderCode = getBlendModeShaderCode(mode);
        auto pipelineId = engine_.createPipeline(shaderCode);
        if (pipelineId == 0) {
            return false;
        }
        blendPipelines_[mode] = pipelineId;
    }
    
    return true;
}

bool LayerCompositor::createEffectPipelines() {
    // Create pipelines for layer effects
    const std::array<LayerEffect::Type, 11> effectTypes = {
        LayerEffect::DropShadow, LayerEffect::InnerShadow,
        LayerEffect::OuterGlow, LayerEffect::InnerGlow,
        LayerEffect::Bevel, LayerEffect::Emboss,
        LayerEffect::Stroke, LayerEffect::ColorOverlay,
        LayerEffect::GradientOverlay, LayerEffect::PatternOverlay,
        LayerEffect::Satin
    };
    
    for (auto type : effectTypes) {
        // Create effect-specific pipeline
        auto pipelineId = engine_.createPipeline("effect_shader");
        if (pipelineId == 0) {
            return false;
        }
        effectPipelines_[type] = pipelineId;
    }
    
    return true;
}

bool LayerCompositor::createUniformBuffers() {
    // Create uniform buffers for different purposes
    blendUniformId_ = engine_.createBuffer(256, Rendering::BufferUsage::Uniform);
    transformUniformId_ = engine_.createBuffer(256, Rendering::BufferUsage::Uniform);
    effectUniformId_ = engine_.createBuffer(512, Rendering::BufferUsage::Uniform);
    
    return blendUniformId_ != 0 && transformUniformId_ != 0 && effectUniformId_ != 0;
}

void LayerCompositor::destroyResources() {
    // Destroy all pipelines
    for (auto& [mode, pipeline] : blendPipelines_) {
        engine_.destroyPipeline(pipeline);
    }
    blendPipelines_.clear();
    
    for (auto& [type, pipeline] : effectPipelines_) {
        engine_.destroyPipeline(pipeline);
    }
    effectPipelines_.clear();
    
    for (auto& [mode, pipeline] : customBlendPipelines_) {
        engine_.destroyPipeline(pipeline);
    }
    customBlendPipelines_.clear();
    
    // Destroy uniform buffers
    if (blendUniformId_ != 0) {
        engine_.destroyBuffer(blendUniformId_);
        blendUniformId_ = 0;
    }
    if (transformUniformId_ != 0) {
        engine_.destroyBuffer(transformUniformId_);
        transformUniformId_ = 0;
    }
    if (effectUniformId_ != 0) {
        engine_.destroyBuffer(effectUniformId_);
        effectUniformId_ = 0;
    }
}

void LayerCompositor::applyLayerTransform(Layer* layer, const std::array<uint32_t, 2>& targetSize) {
    if (!layer) return;
    
    const auto& transform = layer->getTransform();
    updateTransformUniforms(transform, targetSize);
    
    // Apply transform pipeline if needed
    if (transformPipelineId_ != 0) {
        engine_.setPipeline(transformPipelineId_);
        engine_.setUniformBuffer(0, transformUniformId_);
    }
}

void LayerCompositor::applyLayerMask(Layer* layer, Rendering::ResourceId sourceTexture) {
    if (!layer || !layer->hasMask()) return;
    
    const auto* mask = layer->getMask();
    if (!mask || !mask->enabled || mask->maskTexture == 0) return;
    
    if (maskPipelineId_ != 0) {
        engine_.setPipeline(maskPipelineId_);
        engine_.setTexture(0, sourceTexture);
        engine_.setTexture(1, mask->maskTexture);
    }
}

void LayerCompositor::applyLayerEffect(const LayerEffect& effect,
                                      Rendering::ResourceId sourceTexture,
                                      Rendering::ResourceId targetTexture,
                                      const std::array<uint32_t, 2>& size) {
    auto it = effectPipelines_.find(effect.type);
    if (it == effectPipelines_.end()) return;
    
    updateEffectUniforms(effect);
    
    engine_.setPipeline(it->second);
    engine_.setTexture(0, sourceTexture);
    engine_.setUniformBuffer(0, effectUniformId_);
    engine_.setRenderTarget(targetTexture, size);
    engine_.drawFullscreenQuad();
}

void LayerCompositor::updateBlendUniforms(BlendMode mode, float opacity) {
    struct BlendUniforms {
        uint32_t blendMode;
        float opacity;
        float padding[2];
    };
    
    BlendUniforms uniforms{};
    uniforms.blendMode = static_cast<uint32_t>(mode);
    uniforms.opacity = opacity;
    
    engine_.updateBuffer(blendUniformId_, &uniforms, sizeof(uniforms));
}

void LayerCompositor::updateTransformUniforms(const LayerTransform& transform,
                                             const std::array<uint32_t, 2>& targetSize) {
    struct TransformUniforms {
        std::array<float, 16> matrix;
        std::array<float, 2> targetSize;
        float padding[2];
    };
    
    TransformUniforms uniforms{};
    auto matrix3x3 = transform.getMatrix();
    
    // Convert 3x3 to 4x4 matrix
    uniforms.matrix = {
        matrix3x3[0], matrix3x3[1], 0.0f, matrix3x3[2],
        matrix3x3[3], matrix3x3[4], 0.0f, matrix3x3[5],
        0.0f, 0.0f, 1.0f, 0.0f,
        matrix3x3[6], matrix3x3[7], 0.0f, matrix3x3[8]
    };
    
    uniforms.targetSize = {static_cast<float>(targetSize[0]), static_cast<float>(targetSize[1])};
    
    engine_.updateBuffer(transformUniformId_, &uniforms, sizeof(uniforms));
}

void LayerCompositor::updateEffectUniforms(const LayerEffect& effect) {
    struct EffectUniforms {
        uint32_t effectType;
        uint32_t blendMode;
        float opacity;
        float parameters[32]; // Generic parameter storage
    };
    
    EffectUniforms uniforms{};
    uniforms.effectType = static_cast<uint32_t>(effect.type);
    uniforms.blendMode = static_cast<uint32_t>(effect.blendMode);
    uniforms.opacity = effect.opacity;
    
    // Copy float parameters
    size_t paramIndex = 0;
    for (const auto& [name, value] : effect.floatParams) {
        if (paramIndex < 32) {
            uniforms.parameters[paramIndex++] = value;
        }
    }
    
    engine_.updateBuffer(effectUniformId_, &uniforms, sizeof(uniforms));
}

Rendering::PipelineId LayerCompositor::getBlendPipeline(BlendMode mode) {
    auto it = blendPipelines_.find(mode);
    if (it != blendPipelines_.end()) {
        return it->second;
    }
    
    // Check custom blend modes
    auto customIt = customBlendPipelines_.find(mode);
    if (customIt != customBlendPipelines_.end()) {
        return customIt->second;
    }
    
    // Fallback to Normal blend mode
    auto normalIt = blendPipelines_.find(BlendMode::Normal);
    return (normalIt != blendPipelines_.end()) ? normalIt->second : 0;
}

std::array<float, 4> LayerCompositor::calculateLayerBounds(const std::vector<Layer*>& layers) {
    if (layers.empty()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    auto bounds = layers[0]->getBounds();
    for (size_t i = 1; i < layers.size(); ++i) {
        if (layers[i] && layers[i]->isVisible()) {
            bounds = LayerUtils::unionBounds(bounds, layers[i]->getBounds());
        }
    }
    
    return bounds;
}

bool LayerCompositor::layerIntersectsBounds(Layer* layer, const std::array<float, 4>& bounds) {
    if (!layer) return false;
    
    auto layerBounds = layer->getBounds();
    return LayerUtils::boundsOverlap(layerBounds, bounds);
}

std::string LayerCompositor::getBlendModeShaderCode(BlendMode mode) {
    switch (mode) {
        case BlendMode::Normal:
            return R"(
                vec4 blend(vec4 base, vec4 overlay, float opacity) {
                    return mix(base, overlay, overlay.a * opacity);
                }
            )";
        
        case BlendMode::Multiply:
            return R"(
                vec4 blend(vec4 base, vec4 overlay, float opacity) {
                    vec3 result = base.rgb * overlay.rgb;
                    return vec4(mix(base.rgb, result, overlay.a * opacity), base.a);
                }
            )";
        
        case BlendMode::Screen:
            return R"(
                vec4 blend(vec4 base, vec4 overlay, float opacity) {
                    vec3 result = 1.0 - (1.0 - base.rgb) * (1.0 - overlay.rgb);
                    return vec4(mix(base.rgb, result, overlay.a * opacity), base.a);
                }
            )";
        
        case BlendMode::Overlay:
            return R"(
                vec4 blend(vec4 base, vec4 overlay, float opacity) {
                    vec3 result;
                    result.r = base.r < 0.5 ? 2.0 * base.r * overlay.r : 1.0 - 2.0 * (1.0 - base.r) * (1.0 - overlay.r);
                    result.g = base.g < 0.5 ? 2.0 * base.g * overlay.g : 1.0 - 2.0 * (1.0 - base.g) * (1.0 - overlay.g);
                    result.b = base.b < 0.5 ? 2.0 * base.b * overlay.b : 1.0 - 2.0 * (1.0 - base.b) * (1.0 - overlay.b);
                    return vec4(mix(base.rgb, result, overlay.a * opacity), base.a);
                }
            )";
        
        default:
            return getBlendModeShaderCode(BlendMode::Normal);
    }
}

// LayerUtils implementation
namespace LayerUtils {

std::array<float, 3> RGBtoHSV(const std::array<float, 3>& rgb) {
    float r = rgb[0], g = rgb[1], b = rgb[2];
    float max_val = std::max({r, g, b});
    float min_val = std::min({r, g, b});
    float delta = max_val - min_val;
    
    std::array<float, 3> hsv;
    
    // Hue
    if (delta == 0.0f) {
        hsv[0] = 0.0f;
    } else if (max_val == r) {
        hsv[0] = 60.0f * std::fmod((g - b) / delta, 6.0f);
    } else if (max_val == g) {
        hsv[0] = 60.0f * ((b - r) / delta + 2.0f);
    } else {
        hsv[0] = 60.0f * ((r - g) / delta + 4.0f);
    }
    
    if (hsv[0] < 0.0f) hsv[0] += 360.0f;
    
    // Saturation
    hsv[1] = (max_val == 0.0f) ? 0.0f : delta / max_val;
    
    // Value
    hsv[2] = max_val;
    
    return hsv;
}

std::array<float, 3> HSVtoRGB(const std::array<float, 3>& hsv) {
    float h = hsv[0], s = hsv[1], v = hsv[2];
    
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    std::array<float, 3> rgb;
    
    if (h >= 0.0f && h < 60.0f) {
        rgb = {c, x, 0.0f};
    } else if (h >= 60.0f && h < 120.0f) {
        rgb = {x, c, 0.0f};
    } else if (h >= 120.0f && h < 180.0f) {
        rgb = {0.0f, c, x};
    } else if (h >= 180.0f && h < 240.0f) {
        rgb = {0.0f, x, c};
    } else if (h >= 240.0f && h < 300.0f) {
        rgb = {x, 0.0f, c};
    } else {
        rgb = {c, 0.0f, x};
    }
    
    rgb[0] += m;
    rgb[1] += m;
    rgb[2] += m;
    
    return rgb;
}

std::array<float, 3> RGBtoLAB(const std::array<float, 3>& rgb) {
    // Convert RGB to XYZ first
    float r = rgb[0] > 0.04045f ? std::pow((rgb[0] + 0.055f) / 1.055f, 2.4f) : rgb[0] / 12.92f;
    float g = rgb[1] > 0.04045f ? std::pow((rgb[1] + 0.055f) / 1.055f, 2.4f) : rgb[1] / 12.92f;
    float b = rgb[2] > 0.04045f ? std::pow((rgb[2] + 0.055f) / 1.055f, 2.4f) : rgb[2] / 12.92f;
    
    r *= 100.0f;
    g *= 100.0f;
    b *= 100.0f;
    
    // Observer. = 2°, Illuminant = D65
    float x = r * 0.4124f + g * 0.3576f + b * 0.1805f;
    float y = r * 0.2126f + g * 0.7152f + b * 0.0722f;
    float z = r * 0.0193f + g * 0.1192f + b * 0.9505f;
    
    x /= 95.047f;  // Observer= 2°, Illuminant= D65
    y /= 100.000f;
    z /= 108.883f;
    
    x = x > 0.008856f ? std::pow(x, 1.0f/3.0f) : (7.787f * x + 16.0f/116.0f);
    y = y > 0.008856f ? std::pow(y, 1.0f/3.0f) : (7.787f * y + 16.0f/116.0f);
    z = z > 0.008856f ? std::pow(z, 1.0f/3.0f) : (7.787f * z + 16.0f/116.0f);
    
    return {116.0f * y - 16.0f, 500.0f * (x - y), 200.0f * (y - z)};
}

std::array<float, 3> LABtoRGB(const std::array<float, 3>& lab) {
    float y = (lab[0] + 16.0f) / 116.0f;
    float x = lab[1] / 500.0f + y;
    float z = y - lab[2] / 200.0f;
    
    x = x * x * x > 0.008856f ? x * x * x : (x - 16.0f/116.0f) / 7.787f;
    y = y * y * y > 0.008856f ? y * y * y : (y - 16.0f/116.0f) / 7.787f;
    z = z * z * z > 0.008856f ? z * z * z : (z - 16.0f/116.0f) / 7.787f;
    
    x *= 95.047f;   // Observer= 2°, Illuminant= D65
    y *= 100.000f;
    z *= 108.883f;
    
    x /= 100.0f;
    y /= 100.0f;
    z /= 100.0f;
    
    float r = x *  3.2406f + y * -1.5372f + z * -0.4986f;
    float g = x * -0.9689f + y *  1.8758f + z *  0.0415f;
    float b = x *  0.0557f + y * -0.2040f + z *  1.0570f;
    
    r = r > 0.0031308f ? 1.055f * std::pow(r, 1.0f/2.4f) - 0.055f : 12.92f * r;
    g = g > 0.0031308f ? 1.055f * std::pow(g, 1.0f/2.4f) - 0.055f : 12.92f * g;
    b = b > 0.0031308f ? 1.055f * std::pow(b, 1.0f/2.4f) - 0.055f : 12.92f * b;
    
    return {std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f), std::clamp(b, 0.0f, 1.0f)};
}

std::array<float, 4> blendNormal(const std::array<float, 4>& base,
                                const std::array<float, 4>& overlay,
                                float opacity) {
    float alpha = overlay[3] * opacity;
    return {
        base[0] * (1.0f - alpha) + overlay[0] * alpha,
        base[1] * (1.0f - alpha) + overlay[1] * alpha,
        base[2] * (1.0f - alpha) + overlay[2] * alpha,
        base[3]
    };
}

std::array<float, 4> blendMultiply(const std::array<float, 4>& base,
                                  const std::array<float, 4>& overlay,
                                  float opacity) {
    float alpha = overlay[3] * opacity;
    return {
        base[0] * (1.0f - alpha) + (base[0] * overlay[0]) * alpha,
        base[1] * (1.0f - alpha) + (base[1] * overlay[1]) * alpha,
        base[2] * (1.0f - alpha) + (base[2] * overlay[2]) * alpha,
        base[3]
    };
}

std::array<float, 4> blendScreen(const std::array<float, 4>& base,
                                const std::array<float, 4>& overlay,
                                float opacity) {
    float alpha = overlay[3] * opacity;
    return {
        base[0] * (1.0f - alpha) + (1.0f - (1.0f - base[0]) * (1.0f - overlay[0])) * alpha,
        base[1] * (1.0f - alpha) + (1.0f - (1.0f - base[1]) * (1.0f - overlay[1])) * alpha,
        base[2] * (1.0f - alpha) + (1.0f - (1.0f - base[2]) * (1.0f - overlay[2])) * alpha,
        base[3]
    };
}

std::array<float, 4> blendOverlay(const std::array<float, 4>& base,
                                 const std::array<float, 4>& overlay,
                                 float opacity) {
    auto overlayChannel = [](float base, float overlay) {
        return base < 0.5f ? 2.0f * base * overlay : 1.0f - 2.0f * (1.0f - base) * (1.0f - overlay);
    };
    
    float alpha = overlay[3] * opacity;
    return {
        base[0] * (1.0f - alpha) + overlayChannel(base[0], overlay[0]) * alpha,
        base[1] * (1.0f - alpha) + overlayChannel(base[1], overlay[1]) * alpha,
        base[2] * (1.0f - alpha) + overlayChannel(base[2], overlay[2]) * alpha,
        base[3]
    };
}

std::array<float, 4> blendSoftLight(const std::array<float, 4>& base,
                                   const std::array<float, 4>& overlay,
                                   float opacity) {
    auto softLightChannel = [](float base, float overlay) {
        if (overlay <= 0.5f) {
            return base - (1.0f - 2.0f * overlay) * base * (1.0f - base);
        } else {
            float d = base <= 0.25f ? ((16.0f * base - 12.0f) * base + 4.0f) * base
                                   : std::sqrt(base);
            return base + (2.0f * overlay - 1.0f) * (d - base);
        }
    };
    
    float alpha = overlay[3] * opacity;
    return {
        base[0] * (1.0f - alpha) + softLightChannel(base[0], overlay[0]) * alpha,
        base[1] * (1.0f - alpha) + softLightChannel(base[1], overlay[1]) * alpha,
        base[2] * (1.0f - alpha) + softLightChannel(base[2], overlay[2]) * alpha,
        base[3]
    };
}

std::array<float, 9> multiplyMatrices(const std::array<float, 9>& a, const std::array<float, 9>& b) {
    return {
        a[0]*b[0] + a[1]*b[3] + a[2]*b[6],  a[0]*b[1] + a[1]*b[4] + a[2]*b[7],  a[0]*b[2] + a[1]*b[5] + a[2]*b[8],
        a[3]*b[0] + a[4]*b[3] + a[5]*b[6],  a[3]*b[1] + a[4]*b[4] + a[5]*b[7],  a[3]*b[2] + a[4]*b[5] + a[5]*b[8],
        a[6]*b[0] + a[7]*b[3] + a[8]*b[6],  a[6]*b[1] + a[7]*b[4] + a[8]*b[7],  a[6]*b[2] + a[7]*b[5] + a[8]*b[8]
    };
}

std::array<float, 9> invertMatrix(const std::array<float, 9>& matrix) {
    float det = matrix[0] * (matrix[4]*matrix[8] - matrix[5]*matrix[7]) -
                matrix[1] * (matrix[3]*matrix[8] - matrix[5]*matrix[6]) +
                matrix[2] * (matrix[3]*matrix[7] - matrix[4]*matrix[6]);
    
    if (std::abs(det) < 1e-6f) {
        // Return identity matrix if not invertible
        return {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    }
    
    float invDet = 1.0f / det;
    
    return {
        (matrix[4]*matrix[8] - matrix[5]*matrix[7]) * invDet,
        (matrix[2]*matrix[7] - matrix[1]*matrix[8]) * invDet,
        (matrix[1]*matrix[5] - matrix[2]*matrix[4]) * invDet,
        (matrix[5]*matrix[6] - matrix[3]*matrix[8]) * invDet,
        (matrix[0]*matrix[8] - matrix[2]*matrix[6]) * invDet,
        (matrix[2]*matrix[3] - matrix[0]*matrix[5]) * invDet,
        (matrix[3]*matrix[7] - matrix[4]*matrix[6]) * invDet,
        (matrix[1]*matrix[6] - matrix[0]*matrix[7]) * invDet,
        (matrix[0]*matrix[4] - matrix[1]*matrix[3]) * invDet
    };
}

std::array<float, 2> transformPoint(const std::array<float, 2>& point, const std::array<float, 9>& matrix) {
    return {
        matrix[0] * point[0] + matrix[1] * point[1] + matrix[2],
        matrix[3] * point[0] + matrix[4] * point[1] + matrix[5]
    };
}

std::array<float, 4> transformBounds(const std::array<float, 4>& bounds, const std::array<float, 9>& matrix) {
    // Transform all four corners
    std::array<std::array<float, 2>, 4> corners = {{
        {bounds[0], bounds[1]},  // min_x, min_y
        {bounds[2], bounds[1]},  // max_x, min_y
        {bounds[2], bounds[3]},  // max_x, max_y
        {bounds[0], bounds[3]}   // min_x, max_y
    }};
    
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    
    for (const auto& corner : corners) {
        auto transformed = transformPoint(corner, matrix);
        minX = std::min(minX, transformed[0]);
        minY = std::min(minY, transformed[1]);
        maxX = std::max(maxX, transformed[0]);
        maxY = std::max(maxY, transformed[1]);
    }
    
    return {minX, minY, maxX, maxY};
}

std::array<float, 4> unionBounds(const std::array<float, 4>& a, const std::array<float, 4>& b) {
    return {
        std::min(a[0], b[0]),  // min_x
        std::min(a[1], b[1]),  // min_y
        std::max(a[2], b[2]),  // max_x
        std::max(a[3], b[3])   // max_y
    };
}

std::array<float, 4> intersectBounds(const std::array<float, 4>& a, const std::array<float, 4>& b) {
    float minX = std::max(a[0], b[0]);
    float minY = std::max(a[1], b[1]);
    float maxX = std::min(a[2], b[2]);
    float maxY = std::min(a[3], b[3]);
    
    if (minX >= maxX || minY >= maxY) {
        // No intersection
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    return {minX, minY, maxX, maxY};
}

bool boundsOverlap(const std::array<float, 4>& a, const std::array<float, 4>& b) {
    return !(a[2] <= b[0] || b[2] <= a[0] || a[3] <= b[1] || b[3] <= a[1]);
}

} // namespace LayerUtils

} // namespace QuantumCanvas::Raster