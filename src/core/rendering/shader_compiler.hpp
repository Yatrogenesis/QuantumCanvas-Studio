#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <filesystem>
#include <functional>

// Forward declare WGPU types
struct WGPUDevice;
struct WGPUShaderModule;

namespace QuantumCanvas::Rendering {

// Shader types
enum class ShaderStage {
    Vertex,
    Fragment,
    Compute
};

// Shader language support
enum class ShaderLanguage {
    WGSL,     // WebGPU Shading Language
    HLSL,     // High Level Shading Language
    GLSL,     // OpenGL Shading Language
    SPIRV     // SPIR-V bytecode
};

// Shader compilation result
struct ShaderCompilationResult {
    bool success = false;
    std::string error_message;
    std::vector<uint8_t> bytecode;
    std::string preprocessed_source;
    
    // Reflection data
    struct BindingInfo {
        uint32_t binding;
        uint32_t group;
        std::string name;
        std::string type;
        bool is_buffer = false;
        bool is_texture = false;
        bool is_sampler = false;
    };
    std::vector<BindingInfo> bindings;
    
    // Vertex attributes (for vertex shaders)
    struct AttributeInfo {
        uint32_t location;
        std::string name;
        std::string type;
        size_t offset;
        size_t size;
    };
    std::vector<AttributeInfo> attributes;
};

// Shader descriptor
struct ShaderDescriptor {
    ShaderStage stage;
    ShaderLanguage language = ShaderLanguage::WGSL;
    std::string source;
    std::string entry_point = "main";
    std::vector<std::string> defines;
    std::vector<std::filesystem::path> include_paths;
    
    // Optimization settings
    bool optimize = true;
    bool debug_info = false;
    bool warnings_as_errors = true;
    
    // Hash for caching
    mutable uint64_t hash = 0;
    uint64_t compute_hash() const;
};

// Compiled shader resource
class CompiledShader {
public:
    explicit CompiledShader(WGPUShaderModule* module, const ShaderCompilationResult& result);
    ~CompiledShader();
    
    // Disable copy, enable move
    CompiledShader(const CompiledShader&) = delete;
    CompiledShader& operator=(const CompiledShader&) = delete;
    CompiledShader(CompiledShader&& other) noexcept;
    CompiledShader& operator=(CompiledShader&& other) noexcept;
    
    WGPUShaderModule* module() const { return module_; }
    const ShaderCompilationResult& compilation_result() const { return result_; }
    
    // Reflection queries
    const std::vector<ShaderCompilationResult::BindingInfo>& bindings() const { 
        return result_.bindings; 
    }
    const std::vector<ShaderCompilationResult::AttributeInfo>& attributes() const { 
        return result_.attributes; 
    }
    
private:
    WGPUShaderModule* module_;
    ShaderCompilationResult result_;
};

// Shader pipeline cache entry
struct PipelineShaders {
    std::shared_ptr<CompiledShader> vertex;
    std::shared_ptr<CompiledShader> fragment;
    std::shared_ptr<CompiledShader> compute;
    
    uint64_t hash() const;
};

// Main shader compiler
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();
    
    // Initialization
    bool initialize(WGPUDevice* device);
    void shutdown();
    bool is_initialized() const { return device_ != nullptr; }
    
    // Compilation
    std::shared_ptr<CompiledShader> compile_shader(const ShaderDescriptor& desc);
    std::shared_ptr<CompiledShader> compile_shader_from_file(const std::filesystem::path& path,
                                                           ShaderStage stage,
                                                           const std::string& entry_point = "main");
    
    // Batch compilation
    std::vector<std::shared_ptr<CompiledShader>> compile_shaders(
        const std::vector<ShaderDescriptor>& descriptors);
    
    // Hot reload support
    void watch_shader_files(bool enable);
    void set_shader_reload_callback(std::function<void(const std::filesystem::path&)> callback);
    void reload_shader_if_changed(const std::filesystem::path& path);
    
    // Pipeline creation helpers
    PipelineShaders create_render_pipeline_shaders(const std::filesystem::path& vertex_path,
                                                   const std::filesystem::path& fragment_path);
    std::shared_ptr<CompiledShader> create_compute_shader(const std::filesystem::path& compute_path);
    
    // Cross-compilation (HLSL/GLSL to WGSL)
    ShaderCompilationResult cross_compile(const std::string& source,
                                         ShaderLanguage from,
                                         ShaderLanguage to,
                                         ShaderStage stage);
    
    // Cache management
    void clear_cache();
    void set_cache_directory(const std::filesystem::path& path);
    size_t get_cache_size() const;
    void save_cache_to_disk();
    void load_cache_from_disk();
    
    // Preprocessing
    std::string preprocess_shader(const std::string& source,
                                 const std::vector<std::string>& defines = {},
                                 const std::vector<std::filesystem::path>& include_paths = {});
    
    // Built-in shaders
    std::shared_ptr<CompiledShader> get_fullscreen_vertex_shader();
    std::shared_ptr<CompiledShader> get_blit_fragment_shader();
    std::shared_ptr<CompiledShader> get_clear_compute_shader();
    
    // Error handling
    using ErrorCallback = std::function<void(const std::string& error, const ShaderDescriptor& desc)>;
    void set_error_callback(ErrorCallback callback) { error_callback_ = callback; }
    
    // Statistics
    struct CompilerStats {
        size_t shaders_compiled = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t compilation_errors = 0;
        std::chrono::milliseconds total_compilation_time{0};
        std::chrono::milliseconds average_compilation_time{0};
    };
    
    CompilerStats get_stats() const;
    void reset_stats();
    
private:
    WGPUDevice* device_ = nullptr;
    
    // Shader cache
    mutable std::mutex cache_mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<CompiledShader>> shader_cache_;
    std::filesystem::path cache_directory_;
    
    // Built-in shaders
    mutable std::mutex builtin_mutex_;
    std::shared_ptr<CompiledShader> fullscreen_vertex_shader_;
    std::shared_ptr<CompiledShader> blit_fragment_shader_;
    std::shared_ptr<CompiledShader> clear_compute_shader_;
    
    // File watching
    struct FileWatcher;
    std::unique_ptr<FileWatcher> file_watcher_;
    std::function<void(const std::filesystem::path&)> reload_callback_;
    
    // Error handling
    ErrorCallback error_callback_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    CompilerStats stats_;
    
    // Internal methods
    ShaderCompilationResult compile_wgsl(const ShaderDescriptor& desc);
    ShaderCompilationResult compile_hlsl(const ShaderDescriptor& desc);
    ShaderCompilationResult compile_glsl(const ShaderDescriptor& desc);
    ShaderCompilationResult compile_spirv(const ShaderDescriptor& desc);
    
    WGPUShaderModule* create_shader_module(const std::vector<uint8_t>& bytecode);
    ShaderCompilationResult reflect_shader(const std::vector<uint8_t>& spirv);
    
    void create_builtin_shaders();
    void load_builtin_shader(const std::string& source, 
                           ShaderStage stage,
                           std::shared_ptr<CompiledShader>& target);
    
    void handle_compilation_error(const std::string& error, const ShaderDescriptor& desc);
    uint64_t hash_descriptor(const ShaderDescriptor& desc) const;
    
    // Cross-compilation support
    std::unique_ptr<class SPIRVCross> spirv_cross_;
    std::unique_ptr<class DXCCompiler> dxc_compiler_;
    std::unique_ptr<class GLSLangValidator> glslang_;
};

// Utility functions
std::string shader_stage_to_string(ShaderStage stage);
std::string shader_language_to_string(ShaderLanguage language);
ShaderStage shader_stage_from_extension(const std::filesystem::path& path);

// Built-in shader sources
namespace BuiltinShaders {
    extern const char* FULLSCREEN_VERTEX;
    extern const char* BLIT_FRAGMENT;
    extern const char* CLEAR_COMPUTE;
    extern const char* VECTOR_VERTEX;
    extern const char* VECTOR_FRAGMENT;
    extern const char* RASTER_VERTEX;
    extern const char* RASTER_FRAGMENT;
}

} // namespace QuantumCanvas::Rendering