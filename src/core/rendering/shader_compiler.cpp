#include "shader_compiler.hpp"
#include "wgpu_wrapper.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>
#include <cassert>

// Platform-specific includes for file watching
#ifdef _WIN32
    #include <windows.h>
    #include <fileapi.h>
#elif defined(__linux__)
    #include <sys/inotify.h>
    #include <unistd.h>
#elif defined(__APPLE__)
    #include <CoreServices/CoreServices.h>
#endif

namespace QuantumCanvas::Rendering {

// Built-in shader sources
namespace BuiltinShaders {
    const char* FULLSCREEN_VERTEX = R"(
@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> @builtin(position) vec4<f32> {
    // Generate fullscreen triangle
    let x = f32((vertex_index << 1u) & 2u) - 1.0;
    let y = f32(vertex_index & 2u) - 1.0;
    return vec4<f32>(x, y, 0.0, 1.0);
}
)";

    const char* BLIT_FRAGMENT = R"(
@group(0) @binding(0) var source_texture: texture_2d<f32>;
@group(0) @binding(1) var source_sampler: sampler;

@fragment
fn fs_main(@builtin(position) pos: vec4<f32>) -> @location(0) vec4<f32> {
    let tex_coords = pos.xy / vec2<f32>(textureDimensions(source_texture));
    return textureSample(source_texture, source_sampler, tex_coords);
}
)";

    const char* CLEAR_COMPUTE = R"(
@group(0) @binding(0) var target_texture: texture_storage_2d<rgba8unorm, write>;

struct ClearParams {
    clear_color: vec4<f32>,
    dimensions: vec2<u32>,
};

@group(0) @binding(1) var<uniform> params: ClearParams;

@compute @workgroup_size(8, 8, 1)
fn cs_main(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= params.dimensions.x || id.y >= params.dimensions.y) {
        return;
    }
    textureStore(target_texture, vec2<i32>(i32(id.x), i32(id.y)), params.clear_color);
}
)";

    const char* VECTOR_VERTEX = R"(
struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) tex_coords: vec2<f32>,
    @location(2) color: vec4<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) tex_coords: vec2<f32>,
    @location(1) color: vec4<f32>,
};

struct Transform {
    matrix: mat4x4<f32>,
    viewport: vec4<f32>,
};

@group(0) @binding(0) var<uniform> transform: Transform;

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = transform.matrix * vec4<f32>(input.position, 0.0, 1.0);
    output.tex_coords = input.tex_coords;
    output.color = input.color;
    return output;
}
)";

    const char* VECTOR_FRAGMENT = R"(
struct FragmentInput {
    @location(0) tex_coords: vec2<f32>,
    @location(1) color: vec4<f32>,
};

@group(1) @binding(0) var gradient_texture: texture_2d<f32>;
@group(1) @binding(1) var gradient_sampler: sampler;

@fragment
fn fs_main(input: FragmentInput) -> @location(0) vec4<f32> {
    let gradient_color = textureSample(gradient_texture, gradient_sampler, input.tex_coords);
    return input.color * gradient_color;
}
)";

    const char* RASTER_VERTEX = R"(
struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) tex_coords: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) tex_coords: vec2<f32>,
};

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = vec4<f32>(input.position * 2.0 - 1.0, 0.0, 1.0);
    output.tex_coords = input.tex_coords;
    return output;
}
)";

    const char* RASTER_FRAGMENT = R"(
struct FragmentInput {
    @location(0) tex_coords: vec2<f32>,
};

struct BrushParams {
    color: vec4<f32>,
    size: f32,
    hardness: f32,
    opacity: f32,
    flow: f32,
};

@group(0) @binding(0) var brush_texture: texture_2d<f32>;
@group(0) @binding(1) var brush_sampler: sampler;
@group(0) @binding(2) var<uniform> params: BrushParams;

@fragment
fn fs_main(input: FragmentInput) -> @location(0) vec4<f32> {
    let brush_alpha = textureSample(brush_texture, brush_sampler, input.tex_coords).r;
    let falloff = smoothstep(0.0, params.hardness, brush_alpha);
    let final_alpha = falloff * params.opacity * params.flow;
    return vec4<f32>(params.color.rgb, final_alpha);
}
)";
}

// File watcher implementation
struct ShaderCompiler::FileWatcher {
    bool enabled = false;
    std::unordered_map<std::string, std::filesystem::file_time_type> watched_files;
    std::mutex files_mutex;
    
    #ifdef _WIN32
        HANDLE directory_handle = INVALID_HANDLE_VALUE;
    #elif defined(__linux__)
        int inotify_fd = -1;
        std::unordered_map<int, std::string> watch_descriptors;
    #elif defined(__APPLE__)
        FSEventStreamRef event_stream = nullptr;
    #endif
    
    ~FileWatcher() {
        #ifdef _WIN32
            if (directory_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(directory_handle);
            }
        #elif defined(__linux__)
            if (inotify_fd != -1) {
                close(inotify_fd);
            }
        #elif defined(__APPLE__)
            if (event_stream) {
                FSEventStreamStop(event_stream);
                FSEventStreamInvalidate(event_stream);
                FSEventStreamRelease(event_stream);
            }
        #endif
    }
};

// ShaderDescriptor implementation
uint64_t ShaderDescriptor::compute_hash() const {
    if (hash != 0) {
        return hash;
    }
    
    std::hash<std::string> string_hasher;
    hash = string_hasher(source);
    hash ^= string_hasher(entry_point) << 1;
    hash ^= static_cast<uint64_t>(stage) << 2;
    hash ^= static_cast<uint64_t>(language) << 3;
    hash ^= static_cast<uint64_t>(optimize) << 4;
    hash ^= static_cast<uint64_t>(debug_info) << 5;
    
    for (const auto& define : defines) {
        hash ^= string_hasher(define) << 6;
    }
    
    return hash;
}

// CompiledShader implementation
CompiledShader::CompiledShader(WGPUShaderModule* module, const ShaderCompilationResult& result)
    : module_(module), result_(result) {
}

CompiledShader::~CompiledShader() {
    if (module_) {
        wgpuShaderModuleRelease(module_);
    }
}

CompiledShader::CompiledShader(CompiledShader&& other) noexcept
    : module_(std::exchange(other.module_, nullptr))
    , result_(std::move(other.result_)) {
}

CompiledShader& CompiledShader::operator=(CompiledShader&& other) noexcept {
    if (this != &other) {
        if (module_) {
            wgpuShaderModuleRelease(module_);
        }
        module_ = std::exchange(other.module_, nullptr);
        result_ = std::move(other.result_);
    }
    return *this;
}

// PipelineShaders implementation
uint64_t PipelineShaders::hash() const {
    uint64_t h = 0;
    if (vertex) {
        h ^= reinterpret_cast<uintptr_t>(vertex.get());
    }
    if (fragment) {
        h ^= reinterpret_cast<uintptr_t>(fragment.get()) << 1;
    }
    if (compute) {
        h ^= reinterpret_cast<uintptr_t>(compute.get()) << 2;
    }
    return h;
}

// ShaderCompiler implementation
ShaderCompiler::ShaderCompiler() 
    : file_watcher_(std::make_unique<FileWatcher>()) {
    
    // Initialize statistics
    stats_ = CompilerStats{};
    
    std::cout << "[ShaderCompiler] Initialized" << std::endl;
}

ShaderCompiler::~ShaderCompiler() {
    shutdown();
}

bool ShaderCompiler::initialize(WGPUDevice* device) {
    if (device_ != nullptr) {
        return true; // Already initialized
    }
    
    device_ = device;
    
    if (!device_) {
        std::cerr << "[ShaderCompiler] Invalid device provided" << std::endl;
        return false;
    }
    
    // Create built-in shaders
    create_builtin_shaders();
    
    // Set default cache directory
    cache_directory_ = std::filesystem::current_path() / "shader_cache";
    std::filesystem::create_directories(cache_directory_);
    
    // Load cache from disk
    load_cache_from_disk();
    
    std::cout << "[ShaderCompiler] Initialized with device" << std::endl;
    return true;
}

void ShaderCompiler::shutdown() {
    if (device_ == nullptr) {
        return;
    }
    
    // Save cache to disk
    save_cache_to_disk();
    
    // Clear cache
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        shader_cache_.clear();
    }
    
    // Clear built-in shaders
    {
        std::lock_guard<std::mutex> lock(builtin_mutex_);
        fullscreen_vertex_shader_.reset();
        blit_fragment_shader_.reset();
        clear_compute_shader_.reset();
    }
    
    device_ = nullptr;
    std::cout << "[ShaderCompiler] Shutdown complete" << std::endl;
}

std::shared_ptr<CompiledShader> ShaderCompiler::compile_shader(const ShaderDescriptor& desc) {
    if (!device_) {
        std::cerr << "[ShaderCompiler] Not initialized" << std::endl;
        return nullptr;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Check cache first
    uint64_t hash = desc.compute_hash();
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = shader_cache_.find(hash);
        if (it != shader_cache_.end()) {
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.cache_hits++;
            }
            return it->second;
        }
    }
    
    // Compile shader
    ShaderCompilationResult result;
    
    try {
        switch (desc.language) {
            case ShaderLanguage::WGSL:
                result = compile_wgsl(desc);
                break;
            case ShaderLanguage::HLSL:
                result = compile_hlsl(desc);
                break;
            case ShaderLanguage::GLSL:
                result = compile_glsl(desc);
                break;
            case ShaderLanguage::SPIRV:
                result = compile_spirv(desc);
                break;
            default:
                result.success = false;
                result.error_message = "Unsupported shader language";
                break;
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    if (!result.success) {
        handle_compilation_error(result.error_message, desc);
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.compilation_errors++;
        return nullptr;
    }
    
    // Create shader module
    WGPUShaderModule* module = create_shader_module(result.bytecode);
    if (!module) {
        handle_compilation_error("Failed to create shader module", desc);
        return nullptr;
    }
    
    // Create compiled shader
    auto compiled_shader = std::make_shared<CompiledShader>(module, result);
    
    // Add to cache
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        shader_cache_[hash] = compiled_shader;
    }
    
    // Update statistics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.shaders_compiled++;
        stats_.cache_misses++;
        stats_.total_compilation_time += duration;
        if (stats_.shaders_compiled > 0) {
            stats_.average_compilation_time = 
                std::chrono::milliseconds(stats_.total_compilation_time.count() / stats_.shaders_compiled);
        }
    }
    
    std::cout << "[ShaderCompiler] Compiled " << shader_stage_to_string(desc.stage) 
              << " shader in " << duration.count() << "ms" << std::endl;
    
    return compiled_shader;
}

std::shared_ptr<CompiledShader> ShaderCompiler::compile_shader_from_file(
    const std::filesystem::path& path, ShaderStage stage, const std::string& entry_point) {
    
    if (!std::filesystem::exists(path)) {
        std::cerr << "[ShaderCompiler] Shader file not found: " << path << std::endl;
        return nullptr;
    }
    
    // Read file
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ShaderCompiler] Failed to open shader file: " << path << std::endl;
        return nullptr;
    }
    
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Determine language from extension
    ShaderLanguage language = ShaderLanguage::WGSL;
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".wgsl") {
        language = ShaderLanguage::WGSL;
    } else if (ext == ".hlsl") {
        language = ShaderLanguage::HLSL;
    } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp") {
        language = ShaderLanguage::GLSL;
    } else if (ext == ".spv") {
        language = ShaderLanguage::SPIRV;
    }
    
    ShaderDescriptor desc;
    desc.stage = stage;
    desc.language = language;
    desc.source = source;
    desc.entry_point = entry_point;
    
    return compile_shader(desc);
}

std::vector<std::shared_ptr<CompiledShader>> ShaderCompiler::compile_shaders(
    const std::vector<ShaderDescriptor>& descriptors) {
    
    std::vector<std::shared_ptr<CompiledShader>> results;
    results.reserve(descriptors.size());
    
    for (const auto& desc : descriptors) {
        auto compiled = compile_shader(desc);
        results.push_back(compiled);
    }
    
    return results;
}

void ShaderCompiler::watch_shader_files(bool enable) {
    file_watcher_->enabled = enable;
    std::cout << "[ShaderCompiler] File watching " << (enable ? "enabled" : "disabled") << std::endl;
}

void ShaderCompiler::set_shader_reload_callback(std::function<void(const std::filesystem::path&)> callback) {
    reload_callback_ = callback;
}

void ShaderCompiler::reload_shader_if_changed(const std::filesystem::path& path) {
    if (!file_watcher_->enabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(file_watcher_->files_mutex);
    
    auto last_write_time = std::filesystem::last_write_time(path);
    auto& watched_time = file_watcher_->watched_files[path.string()];
    
    if (watched_time != last_write_time) {
        watched_time = last_write_time;
        if (reload_callback_) {
            reload_callback_(path);
        }
    }
}

PipelineShaders ShaderCompiler::create_render_pipeline_shaders(
    const std::filesystem::path& vertex_path, const std::filesystem::path& fragment_path) {
    
    PipelineShaders shaders;
    shaders.vertex = compile_shader_from_file(vertex_path, ShaderStage::Vertex);
    shaders.fragment = compile_shader_from_file(fragment_path, ShaderStage::Fragment);
    
    return shaders;
}

std::shared_ptr<CompiledShader> ShaderCompiler::create_compute_shader(
    const std::filesystem::path& compute_path) {
    return compile_shader_from_file(compute_path, ShaderStage::Compute);
}

ShaderCompilationResult ShaderCompiler::cross_compile(
    const std::string& source, ShaderLanguage from, ShaderLanguage to, ShaderStage stage) {
    
    // For now, return error - cross-compilation would require SPIRV-Cross integration
    ShaderCompilationResult result;
    result.success = false;
    result.error_message = "Cross-compilation not yet implemented";
    return result;
}

void ShaderCompiler::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    shader_cache_.clear();
    std::cout << "[ShaderCompiler] Cache cleared" << std::endl;
}

void ShaderCompiler::set_cache_directory(const std::filesystem::path& path) {
    cache_directory_ = path;
    std::filesystem::create_directories(cache_directory_);
}

size_t ShaderCompiler::get_cache_size() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return shader_cache_.size();
}

void ShaderCompiler::save_cache_to_disk() {
    // Cache persistence would be implemented here
    std::cout << "[ShaderCompiler] Saving cache to disk (not yet implemented)" << std::endl;
}

void ShaderCompiler::load_cache_from_disk() {
    // Cache loading would be implemented here
    std::cout << "[ShaderCompiler] Loading cache from disk (not yet implemented)" << std::endl;
}

std::string ShaderCompiler::preprocess_shader(
    const std::string& source, const std::vector<std::string>& defines, 
    const std::vector<std::filesystem::path>& include_paths) {
    
    std::string result = source;
    
    // Simple preprocessing - add defines at the top
    std::string defines_block;
    for (const auto& define : defines) {
        defines_block += "#define " + define + "\n";
    }
    
    if (!defines_block.empty()) {
        result = defines_block + "\n" + result;
    }
    
    // TODO: Implement #include resolution
    
    return result;
}

std::shared_ptr<CompiledShader> ShaderCompiler::get_fullscreen_vertex_shader() {
    std::lock_guard<std::mutex> lock(builtin_mutex_);
    return fullscreen_vertex_shader_;
}

std::shared_ptr<CompiledShader> ShaderCompiler::get_blit_fragment_shader() {
    std::lock_guard<std::mutex> lock(builtin_mutex_);
    return blit_fragment_shader_;
}

std::shared_ptr<CompiledShader> ShaderCompiler::get_clear_compute_shader() {
    std::lock_guard<std::mutex> lock(builtin_mutex_);
    return clear_compute_shader_;
}

ShaderCompiler::CompilerStats ShaderCompiler::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void ShaderCompiler::reset_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = CompilerStats{};
}

// Private implementation methods

ShaderCompilationResult ShaderCompiler::compile_wgsl(const ShaderDescriptor& desc) {
    ShaderCompilationResult result;
    
    // Preprocess source
    result.preprocessed_source = preprocess_shader(desc.source, desc.defines, desc.include_paths);
    
    // For WGSL, we can pass the source directly to WebGPU
    result.bytecode = std::vector<uint8_t>(
        result.preprocessed_source.begin(), 
        result.preprocessed_source.end()
    );
    
    // Simple validation - check for basic WGSL structure
    if (result.preprocessed_source.find("@vertex") != std::string::npos ||
        result.preprocessed_source.find("@fragment") != std::string::npos ||
        result.preprocessed_source.find("@compute") != std::string::npos) {
        result.success = true;
    } else {
        result.success = false;
        result.error_message = "Invalid WGSL shader - missing stage attribute";
    }
    
    return result;
}

ShaderCompilationResult ShaderCompiler::compile_hlsl(const ShaderDescriptor& desc) {
    ShaderCompilationResult result;
    result.success = false;
    result.error_message = "HLSL compilation not yet implemented";
    return result;
}

ShaderCompilationResult ShaderCompiler::compile_glsl(const ShaderDescriptor& desc) {
    ShaderCompilationResult result;
    result.success = false;
    result.error_message = "GLSL compilation not yet implemented";
    return result;
}

ShaderCompilationResult ShaderCompiler::compile_spirv(const ShaderDescriptor& desc) {
    ShaderCompilationResult result;
    result.success = false;
    result.error_message = "SPIRV compilation not yet implemented";
    return result;
}

WGPUShaderModule* ShaderCompiler::create_shader_module(const std::vector<uint8_t>& bytecode) {
    if (!device_ || bytecode.empty()) {
        return nullptr;
    }
    
    WGPUShaderModuleWGSLDescriptor wgsl_desc = {};
    wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    wgsl_desc.code = reinterpret_cast<const char*>(bytecode.data());
    
    WGPUShaderModuleDescriptor desc = {};
    desc.nextInChain = &wgsl_desc.chain;
    desc.label = "Compiled Shader";
    
    return wgpuDeviceCreateShaderModule(device_, &desc);
}

ShaderCompilationResult ShaderCompiler::reflect_shader(const std::vector<uint8_t>& spirv) {
    ShaderCompilationResult result;
    // Shader reflection would be implemented here using SPIRV-Cross
    result.success = true;
    return result;
}

void ShaderCompiler::create_builtin_shaders() {
    std::lock_guard<std::mutex> lock(builtin_mutex_);
    
    load_builtin_shader(BuiltinShaders::FULLSCREEN_VERTEX, ShaderStage::Vertex, fullscreen_vertex_shader_);
    load_builtin_shader(BuiltinShaders::BLIT_FRAGMENT, ShaderStage::Fragment, blit_fragment_shader_);
    load_builtin_shader(BuiltinShaders::CLEAR_COMPUTE, ShaderStage::Compute, clear_compute_shader_);
    
    std::cout << "[ShaderCompiler] Built-in shaders created" << std::endl;
}

void ShaderCompiler::load_builtin_shader(const std::string& source, ShaderStage stage,
                                        std::shared_ptr<CompiledShader>& target) {
    ShaderDescriptor desc;
    desc.stage = stage;
    desc.language = ShaderLanguage::WGSL;
    desc.source = source;
    desc.entry_point = stage == ShaderStage::Compute ? "cs_main" : 
                      (stage == ShaderStage::Vertex ? "vs_main" : "fs_main");
    
    target = compile_shader(desc);
}

void ShaderCompiler::handle_compilation_error(const std::string& error, const ShaderDescriptor& desc) {
    std::cerr << "[ShaderCompiler] Compilation error in " 
              << shader_stage_to_string(desc.stage) << " shader: " << error << std::endl;
    
    if (error_callback_) {
        error_callback_(error, desc);
    }
}

uint64_t ShaderCompiler::hash_descriptor(const ShaderDescriptor& desc) const {
    return desc.compute_hash();
}

// Utility functions
std::string shader_stage_to_string(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex: return "vertex";
        case ShaderStage::Fragment: return "fragment";
        case ShaderStage::Compute: return "compute";
        default: return "unknown";
    }
}

std::string shader_language_to_string(ShaderLanguage language) {
    switch (language) {
        case ShaderLanguage::WGSL: return "WGSL";
        case ShaderLanguage::HLSL: return "HLSL";
        case ShaderLanguage::GLSL: return "GLSL";
        case ShaderLanguage::SPIRV: return "SPIRV";
        default: return "unknown";
    }
}

ShaderStage shader_stage_from_extension(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".vert") return ShaderStage::Vertex;
    if (ext == ".frag") return ShaderStage::Fragment;
    if (ext == ".comp") return ShaderStage::Compute;
    
    // Default to vertex for unknown extensions
    return ShaderStage::Vertex;
}

} // namespace QuantumCanvas::Rendering