#pragma once

// WGPU Wrapper - Abstracts WGPU C API for easier usage
// This is a simplified wrapper - in real implementation would use actual WGPU headers

#include <cstdint>

// Forward declarations for WGPU types
struct WGPUDevice;
struct WGPUQueue;
struct WGPUSwapChain;
struct WGPUAdapter;
struct WGPUSurface;
struct WGPUTexture;
struct WGPUTextureView;
struct WGPUBuffer;
struct WGPUShaderModule;
struct WGPURenderPipeline;
struct WGPUComputePipeline;
struct WGPUSampler;
struct WGPUCommandEncoder;
struct WGPURenderPassEncoder;
struct WGPUCommandBuffer;
struct WGPUBindGroup;

// Enums
enum WGPUPowerPreference {
    WGPUPowerPreference_LowPower,
    WGPUPowerPreference_HighPerformance
};

enum WGPUTextureUsage {
    WGPUTextureUsage_CopySrc = 1 << 0,
    WGPUTextureUsage_CopyDst = 1 << 1,
    WGPUTextureUsage_TextureBinding = 1 << 2,
    WGPUTextureUsage_StorageBinding = 1 << 3,
    WGPUTextureUsage_RenderAttachment = 1 << 4
};

enum WGPUBufferUsage {
    WGPUBufferUsage_MapRead = 1 << 0,
    WGPUBufferUsage_MapWrite = 1 << 1,
    WGPUBufferUsage_CopySrc = 1 << 2,
    WGPUBufferUsage_CopyDst = 1 << 3,
    WGPUBufferUsage_Index = 1 << 4,
    WGPUBufferUsage_Vertex = 1 << 5,
    WGPUBufferUsage_Uniform = 1 << 6,
    WGPUBufferUsage_Storage = 1 << 7,
    WGPUBufferUsage_Indirect = 1 << 8,
    WGPUBufferUsage_QueryResolve = 1 << 9
};

enum WGPUTextureDimension {
    WGPUTextureDimension_1D,
    WGPUTextureDimension_2D,
    WGPUTextureDimension_3D
};

enum WGPUTextureFormat {
    WGPUTextureFormat_R8Unorm,
    WGPUTextureFormat_R8Snorm,
    WGPUTextureFormat_R8Uint,
    WGPUTextureFormat_R8Sint,
    WGPUTextureFormat_R16Uint,
    WGPUTextureFormat_R16Sint,
    WGPUTextureFormat_R16Float,
    WGPUTextureFormat_RG8Unorm,
    WGPUTextureFormat_RG8Snorm,
    WGPUTextureFormat_RG8Uint,
    WGPUTextureFormat_RG8Sint,
    WGPUTextureFormat_R32Float,
    WGPUTextureFormat_R32Uint,
    WGPUTextureFormat_R32Sint,
    WGPUTextureFormat_RG16Uint,
    WGPUTextureFormat_RG16Sint,
    WGPUTextureFormat_RG16Float,
    WGPUTextureFormat_RGBA8Unorm,
    WGPUTextureFormat_RGBA8UnormSrgb,
    WGPUTextureFormat_RGBA8Snorm,
    WGPUTextureFormat_RGBA8Uint,
    WGPUTextureFormat_RGBA8Sint,
    WGPUTextureFormat_BGRA8Unorm,
    WGPUTextureFormat_BGRA8UnormSrgb,
    WGPUTextureFormat_RGB10A2Unorm,
    WGPUTextureFormat_RG11B10Ufloat,
    WGPUTextureFormat_RGB9E5Ufloat,
    WGPUTextureFormat_RG32Float,
    WGPUTextureFormat_RG32Uint,
    WGPUTextureFormat_RG32Sint,
    WGPUTextureFormat_RGBA16Uint,
    WGPUTextureFormat_RGBA16Sint,
    WGPUTextureFormat_RGBA16Float,
    WGPUTextureFormat_RGBA32Float,
    WGPUTextureFormat_RGBA32Uint,
    WGPUTextureFormat_RGBA32Sint,
    WGPUTextureFormat_Depth32Float,
    WGPUTextureFormat_Depth24Plus,
    WGPUTextureFormat_Depth24PlusStencil8
};

enum WGPUPresentMode {
    WGPUPresentMode_Immediate,
    WGPUPresentMode_Mailbox,
    WGPUPresentMode_Fifo
};

enum WGPULoadOp {
    WGPULoadOp_Clear,
    WGPULoadOp_Load
};

enum WGPUStoreOp {
    WGPUStoreOp_Store,
    WGPUStoreOp_Discard
};

enum WGPUAddressMode {
    WGPUAddressMode_Repeat,
    WGPUAddressMode_MirrorRepeat,
    WGPUAddressMode_ClampToEdge
};

enum WGPUFilterMode {
    WGPUFilterMode_Nearest,
    WGPUFilterMode_Linear
};

enum WGPUMipmapFilterMode {
    WGPUMipmapFilterMode_Nearest,
    WGPUMipmapFilterMode_Linear
};

enum WGPUCompareFunction {
    WGPUCompareFunction_Never,
    WGPUCompareFunction_Less,
    WGPUCompareFunction_Equal,
    WGPUCompareFunction_LessEqual,
    WGPUCompareFunction_Greater,
    WGPUCompareFunction_NotEqual,
    WGPUCompareFunction_GreaterEqual,
    WGPUCompareFunction_Always
};

// Structures
struct WGPUColor {
    double r, g, b, a;
};

struct WGPUExtent3D {
    uint32_t width, height, depthOrArrayLayers;
};

struct WGPUOrigin3D {
    uint32_t x, y, z;
};

struct WGPURequestAdapterOptions {
    WGPUSurface* compatibleSurface = nullptr;
    WGPUPowerPreference powerPreference = WGPUPowerPreference_HighPerformance;
    bool forceFallbackAdapter = false;
};

struct WGPUDeviceDescriptor {
    const char* label = nullptr;
    uint32_t requiredFeatureCount = 0;
    const void* requiredFeatures = nullptr;
    const void* requiredLimits = nullptr;
    void* defaultQueue = nullptr;
};

struct WGPUSwapChainDescriptor {
    const char* label = nullptr;
    uint32_t usage;
    WGPUTextureFormat format;
    uint32_t width, height;
    WGPUPresentMode presentMode;
};

struct WGPUBufferDescriptor {
    const char* label = nullptr;
    uint64_t size;
    uint32_t usage;
    bool mappedAtCreation = false;
};

struct WGPUTextureDescriptor {
    const char* label = nullptr;
    uint32_t usage;
    WGPUTextureDimension dimension;
    WGPUExtent3D size;
    WGPUTextureFormat format;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
};

struct WGPUSamplerDescriptor {
    const char* label = nullptr;
    WGPUAddressMode addressModeU = WGPUAddressMode_ClampToEdge;
    WGPUAddressMode addressModeV = WGPUAddressMode_ClampToEdge;
    WGPUAddressMode addressModeW = WGPUAddressMode_ClampToEdge;
    WGPUFilterMode magFilter = WGPUFilterMode_Linear;
    WGPUFilterMode minFilter = WGPUFilterMode_Linear;
    WGPUMipmapFilterMode mipmapFilter = WGPUMipmapFilterMode_Linear;
    float lodMinClamp = 0.0f;
    float lodMaxClamp = 32.0f;
    WGPUCompareFunction compare = WGPUCompareFunction_Always;
    uint16_t maxAnisotropy = 1;
};

struct WGPUCommandEncoderDescriptor {
    const char* label = nullptr;
};

struct WGPURenderPassColorAttachment {
    WGPUTextureView* view;
    WGPUTextureView* resolveTarget = nullptr;
    WGPULoadOp loadOp;
    WGPUStoreOp storeOp;
    WGPUColor clearValue = {0.0, 0.0, 0.0, 1.0};
};

struct WGPURenderPassDescriptor {
    const char* label = nullptr;
    uint32_t colorAttachmentCount;
    const WGPURenderPassColorAttachment* colorAttachments;
    const void* depthStencilAttachment = nullptr;
    const void* occlusionQuerySet = nullptr;
    uint64_t timestampWriteCount = 0;
    const void* timestampWrites = nullptr;
};

struct WGPUCommandBufferDescriptor {
    const char* label = nullptr;
};

struct WGPUImageCopyTexture {
    WGPUTexture* texture;
    uint32_t mipLevel = 0;
    WGPUOrigin3D origin = {0, 0, 0};
    uint32_t aspect = 0;
};

struct WGPUTextureDataLayout {
    uint64_t offset = 0;
    uint32_t bytesPerRow;
    uint32_t rowsPerImage;
};

// WGPU Wrapper class
class WGPUWrapper {
public:
    // Instance management
    static WGPUAdapter* instance_request_adapter(const WGPURequestAdapterOptions* options);
    static void adapter_release(WGPUAdapter* adapter);
    
    // Device management
    static WGPUDevice* adapter_request_device(WGPUAdapter* adapter, const WGPUDeviceDescriptor* descriptor);
    static void device_release(WGPUDevice* device);
    static WGPUQueue* device_get_queue(WGPUDevice* device);
    static void device_poll(WGPUDevice* device, bool wait);
    
    // Surface and swap chain
    static WGPUSurface* create_surface(void* nativeWindow);
    static void surface_release(WGPUSurface* surface);
    static WGPUSwapChain* device_create_swap_chain(WGPUDevice* device, WGPUSurface* surface, const WGPUSwapChainDescriptor* descriptor);
    static void swap_chain_release(WGPUSwapChain* swapChain);
    static WGPUTextureView* swapchain_get_current_texture_view(WGPUSwapChain* swapChain);
    static void swapchain_present(WGPUSwapChain* swapChain);
    
    // Buffer management
    static WGPUBuffer* device_create_buffer(WGPUDevice* device, const WGPUBufferDescriptor* descriptor);
    static void buffer_release(WGPUBuffer* buffer);
    static void* buffer_get_mapped_range(WGPUBuffer* buffer, size_t offset, size_t size);
    static void buffer_unmap(WGPUBuffer* buffer);
    
    // Texture management
    static WGPUTexture* device_create_texture(WGPUDevice* device, const WGPUTextureDescriptor* descriptor);
    static void texture_release(WGPUTexture* texture);
    static WGPUTextureView* texture_create_view(WGPUTexture* texture, const void* descriptor);
    static void texture_view_release(WGPUTextureView* textureView);
    
    // Sampler management
    static WGPUSampler* device_create_sampler(WGPUDevice* device, const WGPUSamplerDescriptor* descriptor);
    static void sampler_release(WGPUSampler* sampler);
    
    // Command recording
    static WGPUCommandEncoder* device_create_command_encoder(WGPUDevice* device, const WGPUCommandEncoderDescriptor* descriptor);
    static void command_encoder_release(WGPUCommandEncoder* commandEncoder);
    static WGPURenderPassEncoder* command_encoder_begin_render_pass(WGPUCommandEncoder* commandEncoder, const WGPURenderPassDescriptor* descriptor);
    static void render_pass_encoder_release(WGPURenderPassEncoder* renderPassEncoder);
    static void render_pass_encoder_end(WGPURenderPassEncoder* renderPassEncoder);
    static WGPUCommandBuffer* command_encoder_finish(WGPUCommandEncoder* commandEncoder, const WGPUCommandBufferDescriptor* descriptor);
    static void command_buffer_release(WGPUCommandBuffer* commandBuffer);
    
    // Queue operations
    static void queue_release(WGPUQueue* queue);
    static void queue_submit(WGPUQueue* queue, uint32_t commandCount, WGPUCommandBuffer* const* commands);
    static void queue_write_texture(WGPUQueue* queue, const WGPUImageCopyTexture* destination, const void* data, size_t dataSize, const WGPUTextureDataLayout* dataLayout, const WGPUExtent3D* writeSize);
    
private:
    WGPUWrapper() = delete;
};

namespace QuantumCanvas::Rendering {

// WGPU resource wrappers for type safety
class WGPUResource {
public:
    virtual ~WGPUResource() = default;
    virtual void release() = 0;
};

class BufferResource : public WGPUResource {
public:
    WGPUBuffer* handle = nullptr;
    size_t size = 0;
    uint32_t usage = 0;
    
    ~BufferResource() override { release(); }
    void release() override {
        if (handle) {
            WGPUWrapper::buffer_release(handle);
            handle = nullptr;
        }
    }
};

class TextureResource : public WGPUResource {
public:
    WGPUTexture* handle = nullptr;
    WGPUTextureView* view = nullptr;
    TextureDescriptor descriptor;
    
    ~TextureResource() override { release(); }
    void release() override {
        if (view) {
            WGPUWrapper::texture_view_release(view);
            view = nullptr;
        }
        if (handle) {
            WGPUWrapper::texture_release(handle);
            handle = nullptr;
        }
    }
};

class SamplerResource : public WGPUResource {
public:
    WGPUSampler* handle = nullptr;
    SamplerDescriptor descriptor;
    
    ~SamplerResource() override { release(); }
    void release() override {
        if (handle) {
            WGPUWrapper::sampler_release(handle);
            handle = nullptr;
        }
    }
};

} // namespace QuantumCanvas::Rendering