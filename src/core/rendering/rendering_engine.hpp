#pragma once

#include <memory>
#include <vector>
#include <array>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>

// Forward declare WGPU types
struct WGPUDevice;
struct WGPUQueue;
struct WGPUSwapChain;
struct WGPURenderPipeline;
struct WGPUComputePipeline;
struct WGPUTexture;
struct WGPUTextureView;
struct WGPUBuffer;
struct WGPUBindGroup;
struct WGPUSurface;
struct WGPUAdapter;

namespace QuantumCanvas::Rendering {

// Forward declarations
class RenderCommand;
class IRenderResource;
class RenderGraph;
class ComputedPipeline;
class ShaderCompiler;

// Type definitions
using ShaderHash = uint64_t;
using ResourceId = uint64_t;
using PipelineId = uint64_t;

// Render configuration
struct RenderConfig {
    bool enableMSAA = true;
    uint32_t msaaSamples = 4;
    bool enableHDR = true;
    bool enableGPUMemoryProfiling = true;
    float targetFPS = 120.0f;
    bool enableVSync = false;
    uint32_t maxFramesInFlight = 2;
    
    // GPU preferences
    bool preferDiscreteGPU = true;
    bool allowSoftwareRenderer = false;
    size_t maxGPUMemoryMB = 2048;
    
    // Quality settings
    enum class QualityLevel {
        Low,
        Medium,
        High,
        Ultra
    };
    QualityLevel qualityLevel = QualityLevel::High;
};

// Draw call information
struct DrawCall {
    enum class Type {
        Triangles,
        Lines,
        Points,
        TriangleStrip,
        LineStrip
    };
    
    Type type = Type::Triangles;
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t firstVertex = 0;
    uint32_t firstInstance = 0;
    
    ResourceId vertexBufferId = 0;
    ResourceId indexBufferId = 0;
    PipelineId pipelineId = 0;
    
    std::vector<ResourceId> textures;
    std::vector<ResourceId> uniformBuffers;
    
    // Render state
    bool depthTest = true;
    bool depthWrite = true;
    bool cullFace = true;
    bool blendEnabled = false;
    
    // Scissor and viewport
    std::array<int32_t, 4> scissorRect = {0, 0, 0, 0}; // x, y, w, h
    std::array<float, 4> viewport = {0.0f, 0.0f, 1.0f, 1.0f}; // x, y, w, h
};

// Render statistics
struct RenderStats {
    uint32_t drawCallCount = 0;
    uint32_t triangleCount = 0;
    uint64_t vertexCount = 0;
    size_t gpuMemoryUsed = 0;
    size_t textureMemoryUsed = 0;
    std::chrono::microseconds frameTime{0};
    std::chrono::microseconds gpuTime{0};
    float fps = 0.0f;
    uint32_t batchedDrawCalls = 0;
    uint32_t stateChanges = 0;
};

// Main Rendering Engine
class RenderingEngine final {
public:
    explicit RenderingEngine(const RenderConfig& config = {});
    ~RenderingEngine();
    
    // Disable copy, enable move
    RenderingEngine(const RenderingEngine&) = delete;
    RenderingEngine& operator=(const RenderingEngine&) = delete;
    RenderingEngine(RenderingEngine&&) noexcept;
    RenderingEngine& operator=(RenderingEngine&&) noexcept;
    
    // Initialization
    bool initialize(void* nativeWindow);
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // Frame management
    void begin_frame();
    void end_frame();
    void present();
    
    // Command submission (thread-safe)
    void submit_draw_call(const DrawCall& call);
    void submit_compute(const ComputeDispatch& dispatch);
    
    // Resource creation
    ResourceId create_buffer(size_t size, BufferUsage usage, const void* data = nullptr);
    ResourceId create_texture(const TextureDescriptor& desc, const void* data = nullptr);
    ResourceId create_sampler(const SamplerDescriptor& desc);
    PipelineId create_render_pipeline(const RenderPipelineDescriptor& desc);
    PipelineId create_compute_pipeline(const ComputePipelineDescriptor& desc);
    
    // Resource management
    void destroy_resource(ResourceId id);
    void update_buffer(ResourceId id, size_t offset, size_t size, const void* data);
    void* map_buffer(ResourceId id, size_t offset, size_t size);
    void unmap_buffer(ResourceId id);
    
    // Render graph for optimization
    void set_render_graph(std::unique_ptr<RenderGraph> graph);
    void execute_render_graph();
    
    // Advanced features
    void setup_ray_tracing(const RTConfig& config);
    void enable_variable_rate_shading(const VRSConfig& config);
    void enable_mesh_shaders(bool enable);
    
    // Performance and debugging
    RenderStats get_stats() const;
    void reset_stats();
    void capture_frame(const std::string& filename);
    void enable_gpu_debugging(bool enable);
    
    // Configuration
    void update_config(const RenderConfig& config);
    const RenderConfig& get_config() const { return config_; }
    
    // Device capabilities
    struct DeviceCapabilities {
        std::string deviceName;
        uint32_t maxTextureSize;
        uint32_t maxTextureArrayLayers;
        uint32_t maxBindGroups;
        uint32_t maxUniformBufferSize;
        uint32_t maxStorageBufferSize;
        uint32_t maxVertexBuffers;
        uint32_t maxVertexAttributes;
        uint32_t maxComputeWorkgroupSize[3];
        bool supportsRayTracing;
        bool supportsMeshShaders;
        bool supportsVariableRateShading;
        bool supportsBindlessResources;
    };
    
    DeviceCapabilities get_device_capabilities() const;
    
private:
    // WGPU handles
    WGPUDevice* device_ = nullptr;
    WGPUQueue* queue_ = nullptr;
    WGPUSwapChain* swapChain_ = nullptr;
    WGPUSurface* surface_ = nullptr;
    WGPUAdapter* adapter_ = nullptr;
    
    // Current frame state
    WGPUTextureView* currentTextureView_ = nullptr;
    uint32_t currentFrameIndex_ = 0;
    
    // Command buffers (double-buffered)
    struct CommandBuffer {
        std::vector<RenderCommand> commands;
        std::vector<std::unique_ptr<IRenderResource>> transientResources;
        std::atomic<bool> isReady{false};
        std::mutex mutex;
    };
    std::array<CommandBuffer, 2> commandBuffers_;
    std::atomic<size_t> currentBuffer_{0};
    
    // Pipeline cache
    std::unordered_map<ShaderHash, std::unique_ptr<ComputedPipeline>> pipelineCache_;
    std::mutex pipelineCacheMutex_;
    
    // Resource management
    std::unordered_map<ResourceId, std::unique_ptr<IRenderResource>> resources_;
    std::mutex resourcesMutex_;
    std::atomic<ResourceId> nextResourceId_{1};
    
    // Shader compiler
    std::unique_ptr<ShaderCompiler> shaderCompiler_;
    
    // Render graph
    std::unique_ptr<RenderGraph> renderGraph_;
    
    // Configuration
    RenderConfig config_;
    std::atomic<bool> initialized_{false};
    
    // Statistics
    mutable std::mutex statsMutex_;
    RenderStats stats_;
    std::chrono::high_resolution_clock::time_point frameStartTime_;
    
    // Internal methods
    bool create_device();
    bool create_swap_chain(void* nativeWindow);
    void destroy_device();
    void process_command_buffer();
    void optimize_draw_calls();
    void update_statistics();
    ShaderHash compute_shader_hash(const ShaderDescriptor& desc) const;
};

// Buffer usage flags
enum class BufferUsage : uint32_t {
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    CopySrc = 1 << 4,
    CopyDst = 1 << 5,
    Indirect = 1 << 6,
    QueryResolve = 1 << 7
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// Texture descriptor
struct TextureDescriptor {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    
    enum class Dimension {
        D1,
        D2,
        D3
    };
    Dimension dimension = Dimension::D2;
    
    enum class Format {
        RGBA8Unorm,
        RGBA8Snorm,
        RGBA16Float,
        RGBA32Float,
        BGRA8Unorm,
        Depth24PlusStencil8,
        Depth32Float,
        R8Unorm,
        R16Float,
        R32Float
    };
    Format format = Format::RGBA8Unorm;
    
    enum class Usage : uint32_t {
        CopySrc = 1 << 0,
        CopyDst = 1 << 1,
        TextureBinding = 1 << 2,
        StorageBinding = 1 << 3,
        RenderAttachment = 1 << 4
    };
    uint32_t usage = static_cast<uint32_t>(Usage::TextureBinding);
};

// Sampler descriptor
struct SamplerDescriptor {
    enum class FilterMode {
        Nearest,
        Linear
    };
    FilterMode minFilter = FilterMode::Linear;
    FilterMode magFilter = FilterMode::Linear;
    FilterMode mipmapFilter = FilterMode::Linear;
    
    enum class AddressMode {
        ClampToEdge,
        Repeat,
        MirrorRepeat,
        ClampToBorder
    };
    AddressMode addressModeU = AddressMode::ClampToEdge;
    AddressMode addressModeV = AddressMode::ClampToEdge;
    AddressMode addressModeW = AddressMode::ClampToEdge;
    
    float lodMinClamp = 0.0f;
    float lodMaxClamp = 1000.0f;
    
    enum class CompareFunction {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };
    CompareFunction compare = CompareFunction::Always;
    
    uint32_t maxAnisotropy = 1;
};

// Compute dispatch parameters
struct ComputeDispatch {
    PipelineId pipelineId = 0;
    uint32_t workgroupsX = 1;
    uint32_t workgroupsY = 1;
    uint32_t workgroupsZ = 1;
    std::vector<ResourceId> buffers;
    std::vector<ResourceId> textures;
};

// Ray tracing configuration
struct RTConfig {
    bool enableRayTracing = true;
    uint32_t maxRayRecursionDepth = 1;
    size_t accelerationStructureSize = 256 * 1024 * 1024; // 256 MB
};

// Variable rate shading configuration
struct VRSConfig {
    bool enableVRS = true;
    uint32_t tileSize = 8; // 8x8 pixel tiles
    
    enum class ShadingRate {
        Rate1x1,  // Full resolution
        Rate1x2,  // Half resolution horizontally
        Rate2x1,  // Half resolution vertically
        Rate2x2,  // Quarter resolution
        Rate2x4,
        Rate4x2,
        Rate4x4   // 1/16 resolution
    };
    ShadingRate defaultRate = ShadingRate::Rate1x1;
};

} // namespace QuantumCanvas::Rendering