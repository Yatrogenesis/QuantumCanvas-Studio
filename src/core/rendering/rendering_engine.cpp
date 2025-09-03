#include "rendering_engine.hpp"
#include "render_command.hpp"
#include "render_resource.hpp"
#include "render_graph.hpp"
#include "shader_compiler.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>

// WGPU includes - would be actual WGPU headers in real implementation
#include "wgpu_wrapper.hpp"

namespace QuantumCanvas::Rendering {

RenderingEngine::RenderingEngine(const RenderConfig& config)
    : config_(config) {
    
    // Initialize shader compiler
    shaderCompiler_ = std::make_unique<ShaderCompiler>();
    
    // Initialize command buffers
    for (auto& buffer : commandBuffers_) {
        buffer.commands.reserve(1024);
        buffer.transientResources.reserve(256);
    }
}

RenderingEngine::~RenderingEngine() {
    shutdown();
}

RenderingEngine::RenderingEngine(RenderingEngine&& other) noexcept 
    : device_(std::exchange(other.device_, nullptr))
    , queue_(std::exchange(other.queue_, nullptr))
    , swapChain_(std::exchange(other.swapChain_, nullptr))
    , surface_(std::exchange(other.surface_, nullptr))
    , adapter_(std::exchange(other.adapter_, nullptr))
    , currentTextureView_(std::exchange(other.currentTextureView_, nullptr))
    , currentFrameIndex_(other.currentFrameIndex_)
    , commandBuffers_(std::move(other.commandBuffers_))
    , currentBuffer_(other.currentBuffer_.load())
    , pipelineCache_(std::move(other.pipelineCache_))
    , resources_(std::move(other.resources_))
    , nextResourceId_(other.nextResourceId_.load())
    , shaderCompiler_(std::move(other.shaderCompiler_))
    , renderGraph_(std::move(other.renderGraph_))
    , config_(other.config_)
    , initialized_(other.initialized_.load())
    , stats_(other.stats_) {
}

RenderingEngine& RenderingEngine::operator=(RenderingEngine&& other) noexcept {
    if (this != &other) {
        shutdown();
        
        device_ = std::exchange(other.device_, nullptr);
        queue_ = std::exchange(other.queue_, nullptr);
        swapChain_ = std::exchange(other.swapChain_, nullptr);
        surface_ = std::exchange(other.surface_, nullptr);
        adapter_ = std::exchange(other.adapter_, nullptr);
        currentTextureView_ = std::exchange(other.currentTextureView_, nullptr);
        currentFrameIndex_ = other.currentFrameIndex_;
        commandBuffers_ = std::move(other.commandBuffers_);
        currentBuffer_ = other.currentBuffer_.load();
        pipelineCache_ = std::move(other.pipelineCache_);
        resources_ = std::move(other.resources_);
        nextResourceId_ = other.nextResourceId_.load();
        shaderCompiler_ = std::move(other.shaderCompiler_);
        renderGraph_ = std::move(other.renderGraph_);
        config_ = other.config_;
        initialized_ = other.initialized_.load();
        stats_ = other.stats_;
    }
    return *this;
}

bool RenderingEngine::initialize(void* nativeWindow) {
    if (initialized_) {
        return true;
    }
    
    try {
        // Create WGPU device
        if (!create_device()) {
            std::cerr << "Failed to create WGPU device" << std::endl;
            return false;
        }
        
        // Create swap chain
        if (!create_swap_chain(nativeWindow)) {
            std::cerr << "Failed to create swap chain" << std::endl;
            destroy_device();
            return false;
        }
        
        // Initialize shader compiler
        if (!shaderCompiler_->initialize(device_)) {
            std::cerr << "Failed to initialize shader compiler" << std::endl;
            destroy_device();
            return false;
        }
        
        initialized_ = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Rendering engine initialization failed: " << e.what() << std::endl;
        destroy_device();
        return false;
    }
}

void RenderingEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Wait for GPU to finish
    WGPUWrapper::device_poll(device_, true);
    
    // Clear resources
    {
        std::lock_guard<std::mutex> lock(resourcesMutex_);
        resources_.clear();
    }
    
    // Clear pipeline cache
    {
        std::lock_guard<std::mutex> lock(pipelineCacheMutex_);
        pipelineCache_.clear();
    }
    
    // Clear command buffers
    for (auto& buffer : commandBuffers_) {
        std::lock_guard<std::mutex> lock(buffer.mutex);
        buffer.commands.clear();
        buffer.transientResources.clear();
    }
    
    // Shutdown components
    if (shaderCompiler_) {
        shaderCompiler_->shutdown();
    }
    
    destroy_device();
    initialized_ = false;
}

void RenderingEngine::begin_frame() {
    assert(initialized_);
    
    frameStartTime_ = std::chrono::high_resolution_clock::now();
    
    // Get current swap chain texture
    WGPUTextureView textureView = WGPUWrapper::swapchain_get_current_texture_view(swapChain_);
    if (!textureView) {
        // Handle swap chain recreation
        return;
    }
    
    currentTextureView_ = textureView;
    currentFrameIndex_ = (currentFrameIndex_ + 1) % config_.maxFramesInFlight;
    
    // Switch to next command buffer
    size_t nextBuffer = (currentBuffer_ + 1) % 2;
    auto& cmdBuffer = commandBuffers_[nextBuffer];
    
    // Wait for command buffer to be ready
    while (!cmdBuffer.isReady.load()) {
        std::this_thread::yield();
    }
    
    // Clear command buffer
    {
        std::lock_guard<std::mutex> lock(cmdBuffer.mutex);
        cmdBuffer.commands.clear();
        cmdBuffer.transientResources.clear();
        cmdBuffer.isReady = false;
    }
    
    currentBuffer_ = nextBuffer;
}

void RenderingEngine::end_frame() {
    assert(initialized_);
    
    // Process current command buffer
    process_command_buffer();
    
    // Update statistics
    update_statistics();
}

void RenderingEngine::present() {
    assert(initialized_);
    
    // Present the frame
    WGPUWrapper::swapchain_present(swapChain_);
    
    // Release current texture view
    if (currentTextureView_) {
        WGPUWrapper::texture_view_release(currentTextureView_);
        currentTextureView_ = nullptr;
    }
}

void RenderingEngine::submit_draw_call(const DrawCall& call) {
    size_t bufferIndex = currentBuffer_.load();
    auto& cmdBuffer = commandBuffers_[bufferIndex];
    
    std::lock_guard<std::mutex> lock(cmdBuffer.mutex);
    
    // Create render command
    auto renderCmd = std::make_unique<DrawRenderCommand>(call);
    cmdBuffer.commands.push_back(std::move(renderCmd));
    
    // Update statistics
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.drawCallCount++;
        
        if (call.type == DrawCall::Type::Triangles) {
            stats_.triangleCount += call.vertexCount / 3;
        }
        stats_.vertexCount += call.vertexCount;
    }
}

void RenderingEngine::submit_compute(const ComputeDispatch& dispatch) {
    size_t bufferIndex = currentBuffer_.load();
    auto& cmdBuffer = commandBuffers_[bufferIndex];
    
    std::lock_guard<std::mutex> lock(cmdBuffer.mutex);
    
    // Create compute command
    auto computeCmd = std::make_unique<ComputeRenderCommand>(dispatch);
    cmdBuffer.commands.push_back(std::move(computeCmd));
}

ResourceId RenderingEngine::create_buffer(size_t size, BufferUsage usage, const void* data) {
    auto buffer = std::make_unique<BufferResource>();
    
    WGPUBufferDescriptor desc = {};
    desc.size = size;
    desc.usage = static_cast<WGPUBufferUsage>(usage);
    desc.mappedAtCreation = data != nullptr;
    
    buffer->handle = WGPUWrapper::device_create_buffer(device_, &desc);
    buffer->size = size;
    buffer->usage = usage;
    
    if (data) {
        void* mappedData = WGPUWrapper::buffer_get_mapped_range(buffer->handle, 0, size);
        std::memcpy(mappedData, data, size);
        WGPUWrapper::buffer_unmap(buffer->handle);
    }
    
    ResourceId id = nextResourceId_++;
    buffer->id = id;
    
    {
        std::lock_guard<std::mutex> lock(resourcesMutex_);
        resources_[id] = std::move(buffer);
    }
    
    return id;
}

ResourceId RenderingEngine::create_texture(const TextureDescriptor& desc, const void* data) {
    auto texture = std::make_unique<TextureResource>();
    
    WGPUTextureDescriptor wgpuDesc = {};
    wgpuDesc.dimension = static_cast<WGPUTextureDimension>(desc.dimension);
    wgpuDesc.size.width = desc.width;
    wgpuDesc.size.height = desc.height;
    wgpuDesc.size.depthOrArrayLayers = desc.depth;
    wgpuDesc.format = static_cast<WGPUTextureFormat>(desc.format);
    wgpuDesc.mipLevelCount = desc.mipLevelCount;
    wgpuDesc.sampleCount = desc.sampleCount;
    wgpuDesc.usage = desc.usage;
    
    texture->handle = WGPUWrapper::device_create_texture(device_, &wgpuDesc);
    texture->descriptor = desc;
    
    if (data) {
        // Upload texture data
        WGPUImageCopyTexture destination = {};
        destination.texture = texture->handle;
        destination.mipLevel = 0;
        destination.origin = {0, 0, 0};
        
        WGPUTextureDataLayout dataLayout = {};
        dataLayout.offset = 0;
        dataLayout.bytesPerRow = desc.width * 4; // Assuming RGBA8
        dataLayout.rowsPerImage = desc.height;
        
        WGPUExtent3D writeSize = {desc.width, desc.height, desc.depth};
        
        WGPUWrapper::queue_write_texture(queue_, &destination, data, 
                                        desc.width * desc.height * 4, 
                                        &dataLayout, &writeSize);
    }
    
    ResourceId id = nextResourceId_++;
    texture->id = id;
    
    {
        std::lock_guard<std::mutex> lock(resourcesMutex_);
        resources_[id] = std::move(texture);
    }
    
    return id;
}

ResourceId RenderingEngine::create_sampler(const SamplerDescriptor& desc) {
    auto sampler = std::make_unique<SamplerResource>();
    
    WGPUSamplerDescriptor wgpuDesc = {};
    wgpuDesc.addressModeU = static_cast<WGPUAddressMode>(desc.addressModeU);
    wgpuDesc.addressModeV = static_cast<WGPUAddressMode>(desc.addressModeV);
    wgpuDesc.addressModeW = static_cast<WGPUAddressMode>(desc.addressModeW);
    wgpuDesc.magFilter = static_cast<WGPUFilterMode>(desc.magFilter);
    wgpuDesc.minFilter = static_cast<WGPUFilterMode>(desc.minFilter);
    wgpuDesc.mipmapFilter = static_cast<WGPUMipmapFilterMode>(desc.mipmapFilter);
    wgpuDesc.lodMinClamp = desc.lodMinClamp;
    wgpuDesc.lodMaxClamp = desc.lodMaxClamp;
    wgpuDesc.compare = static_cast<WGPUCompareFunction>(desc.compare);
    wgpuDesc.maxAnisotropy = desc.maxAnisotropy;
    
    sampler->handle = WGPUWrapper::device_create_sampler(device_, &wgpuDesc);
    sampler->descriptor = desc;
    
    ResourceId id = nextResourceId_++;
    sampler->id = id;
    
    {
        std::lock_guard<std::mutex> lock(resourcesMutex_);
        resources_[id] = std::move(sampler);
    }
    
    return id;
}

void RenderingEngine::destroy_resource(ResourceId id) {
    std::lock_guard<std::mutex> lock(resourcesMutex_);
    
    auto it = resources_.find(id);
    if (it != resources_.end()) {
        resources_.erase(it);
    }
}

bool RenderingEngine::create_device() {
    // Request adapter
    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.powerPreference = config_.preferDiscreteGPU ? 
        WGPUPowerPreference_HighPerformance : WGPUPowerPreference_LowPower;
    
    adapter_ = WGPUWrapper::instance_request_adapter(&adapterOptions);
    if (!adapter_) {
        return false;
    }
    
    // Request device
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.requiredFeatureCount = 0;
    deviceDesc.requiredLimits = nullptr;
    
    device_ = WGPUWrapper::adapter_request_device(adapter_, &deviceDesc);
    if (!device_) {
        return false;
    }
    
    // Get queue
    queue_ = WGPUWrapper::device_get_queue(device_);
    
    return true;
}

bool RenderingEngine::create_swap_chain(void* nativeWindow) {
    // Create surface
    surface_ = WGPUWrapper::create_surface(nativeWindow);
    if (!surface_) {
        return false;
    }
    
    // Configure surface
    WGPUSwapChainDescriptor swapChainDesc = {};
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.format = WGPUTextureFormat_BGRA8Unorm;
    swapChainDesc.width = 1920;  // Would be actual window size
    swapChainDesc.height = 1080;
    swapChainDesc.presentMode = config_.enableVSync ? 
        WGPUPresentMode_Fifo : WGPUPresentMode_Immediate;
    
    swapChain_ = WGPUWrapper::device_create_swap_chain(device_, surface_, &swapChainDesc);
    
    return swapChain_ != nullptr;
}

void RenderingEngine::destroy_device() {
    if (swapChain_) {
        WGPUWrapper::swap_chain_release(swapChain_);
        swapChain_ = nullptr;
    }
    
    if (surface_) {
        WGPUWrapper::surface_release(surface_);
        surface_ = nullptr;
    }
    
    if (queue_) {
        WGPUWrapper::queue_release(queue_);
        queue_ = nullptr;
    }
    
    if (device_) {
        WGPUWrapper::device_release(device_);
        device_ = nullptr;
    }
    
    if (adapter_) {
        WGPUWrapper::adapter_release(adapter_);
        adapter_ = nullptr;
    }
}

void RenderingEngine::process_command_buffer() {
    size_t bufferIndex = currentBuffer_.load();
    auto& cmdBuffer = commandBuffers_[bufferIndex];
    
    std::lock_guard<std::mutex> lock(cmdBuffer.mutex);
    
    if (cmdBuffer.commands.empty()) {
        cmdBuffer.isReady = true;
        return;
    }
    
    // Create command encoder
    WGPUCommandEncoderDescriptor encoderDesc = {};
    WGPUCommandEncoder encoder = WGPUWrapper::device_create_command_encoder(device_, &encoderDesc);
    
    // Create render pass
    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = currentTextureView_;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = {0.0, 0.0, 0.0, 1.0};
    
    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    
    WGPURenderPassEncoder passEncoder = WGPUWrapper::command_encoder_begin_render_pass(encoder, &renderPassDesc);
    
    // Execute commands
    for (const auto& command : cmdBuffer.commands) {
        command->execute(passEncoder, *this);
    }
    
    // End render pass
    WGPUWrapper::render_pass_encoder_end(passEncoder);
    
    // Finish command buffer
    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    WGPUCommandBuffer commandBuffer = WGPUWrapper::command_encoder_finish(encoder, &cmdBufferDesc);
    
    // Submit to queue
    WGPUWrapper::queue_submit(queue_, 1, &commandBuffer);
    
    // Clean up
    WGPUWrapper::command_buffer_release(commandBuffer);
    WGPUWrapper::render_pass_encoder_release(passEncoder);
    WGPUWrapper::command_encoder_release(encoder);
    
    cmdBuffer.isReady = true;
}

void RenderingEngine::update_statistics() {
    auto now = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(now - frameStartTime_);
    
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_.frameTime = frameDuration;
    
    if (frameDuration.count() > 0) {
        stats_.fps = 1000000.0f / frameDuration.count();
    }
}

RenderStats RenderingEngine::get_stats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void RenderingEngine::reset_stats() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = RenderStats{};
}

} // namespace QuantumCanvas::Rendering