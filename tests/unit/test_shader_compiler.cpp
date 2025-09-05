#include <gtest/gtest.h>
#include "../../src/core/rendering/shader_compiler.hpp"
#include <memory>
#include <fstream>
#include <filesystem>

using namespace QuantumCanvas::Rendering;

class ShaderCompilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        compiler = std::make_unique<ShaderCompiler>();
        // Mock device - in real tests this would be a real WGPU device
        mock_device = reinterpret_cast<WGPUDevice*>(0x1234); // Mock pointer
        
        ASSERT_TRUE(compiler->initialize(mock_device));
    }

    void TearDown() override {
        compiler->shutdown();
        compiler.reset();
    }

    std::unique_ptr<ShaderCompiler> compiler;
    WGPUDevice* mock_device;
    
    // Helper to create temporary shader files
    std::filesystem::path createTempShaderFile(const std::string& content, const std::string& extension) {
        static int counter = 0;
        std::filesystem::path temp_path = std::filesystem::temp_directory_path();
        std::filesystem::path shader_path = temp_path / ("test_shader_" + std::to_string(++counter) + extension);
        
        std::ofstream file(shader_path);
        file << content;
        file.close();
        
        return shader_path;
    }
};

TEST_F(ShaderCompilerTest, Initialization) {
    EXPECT_TRUE(compiler->is_initialized());
    
    auto stats = compiler->get_stats();
    EXPECT_EQ(stats.shaders_compiled, 0);
    EXPECT_EQ(stats.cache_hits, 0);
    EXPECT_EQ(stats.cache_misses, 0);
}

TEST_F(ShaderCompilerTest, BasicWGSLCompilation) {
    const std::string vertex_source = R"(
@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> @builtin(position) vec4<f32> {
    let x = f32((vertex_index << 1u) & 2u) - 1.0;
    let y = f32(vertex_index & 2u) - 1.0;
    return vec4<f32>(x, y, 0.0, 1.0);
}
)";

    ShaderDescriptor desc;
    desc.stage = ShaderStage::Vertex;
    desc.language = ShaderLanguage::WGSL;
    desc.source = vertex_source;
    desc.entry_point = "vs_main";

    auto compiled_shader = compiler->compile_shader(desc);
    ASSERT_NE(compiled_shader, nullptr);
    EXPECT_NE(compiled_shader->module(), nullptr);
    
    const auto& result = compiled_shader->compilation_result();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_FALSE(result.bytecode.empty());
}

TEST_F(ShaderCompilerTest, FragmentShaderCompilation) {
    const std::string fragment_source = R"(
@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}
)";

    ShaderDescriptor desc;
    desc.stage = ShaderStage::Fragment;
    desc.language = ShaderLanguage::WGSL;
    desc.source = fragment_source;
    desc.entry_point = "fs_main";

    auto compiled_shader = compiler->compile_shader(desc);
    ASSERT_NE(compiled_shader, nullptr);
    EXPECT_TRUE(compiled_shader->compilation_result().success);
}

TEST_F(ShaderCompilerTest, ComputeShaderCompilation) {
    const std::string compute_source = R"(
@group(0) @binding(0) var<storage, read_write> data: array<f32>;

@compute @workgroup_size(64)
fn cs_main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let index = global_id.x;
    if (index >= arrayLength(&data)) {
        return;
    }
    data[index] = data[index] * 2.0;
}
)";

    ShaderDescriptor desc;
    desc.stage = ShaderStage::Compute;
    desc.language = ShaderLanguage::WGSL;
    desc.source = compute_source;
    desc.entry_point = "cs_main";

    auto compiled_shader = compiler->compile_shader(desc);
    ASSERT_NE(compiled_shader, nullptr);
    EXPECT_TRUE(compiled_shader->compilation_result().success);
}

TEST_F(ShaderCompilerTest, InvalidShaderCompilation) {
    const std::string invalid_source = R"(
@vertex
fn vs_main() -> @builtin(position) vec4<f32> {
    return invalid_function_call();
}
)";

    ShaderDescriptor desc;
    desc.stage = ShaderStage::Vertex;
    desc.language = ShaderLanguage::WGSL;
    desc.source = invalid_source;
    desc.entry_point = "vs_main";

    auto compiled_shader = compiler->compile_shader(desc);
    EXPECT_EQ(compiled_shader, nullptr);
    
    auto stats = compiler->get_stats();
    EXPECT_GT(stats.compilation_errors, 0);
}

TEST_F(ShaderCompilerTest, ShaderCaching) {
    const std::string vertex_source = R"(
@vertex
fn vs_main() -> @builtin(position) vec4<f32> {
    return vec4<f32>(0.0, 0.0, 0.0, 1.0);
}
)";

    ShaderDescriptor desc;
    desc.stage = ShaderStage::Vertex;
    desc.language = ShaderLanguage::WGSL;
    desc.source = vertex_source;

    // First compilation - cache miss
    auto shader1 = compiler->compile_shader(desc);
    ASSERT_NE(shader1, nullptr);
    
    auto stats_after_first = compiler->get_stats();
    EXPECT_EQ(stats_after_first.cache_misses, 1);
    EXPECT_EQ(stats_after_first.cache_hits, 0);

    // Second compilation with same descriptor - cache hit
    auto shader2 = compiler->compile_shader(desc);
    ASSERT_NE(shader2, nullptr);
    
    auto stats_after_second = compiler->get_stats();
    EXPECT_EQ(stats_after_second.cache_hits, 1);
    
    // Verify same shader object
    EXPECT_EQ(shader1.get(), shader2.get());
}

TEST_F(ShaderCompilerTest, FileCompilation) {
    const std::string shader_content = R"(
@vertex
fn vs_main(@location(0) position: vec2<f32>) -> @builtin(position) vec4<f32> {
    return vec4<f32>(position, 0.0, 1.0);
}
)";
    
    auto shader_file = createTempShaderFile(shader_content, ".wgsl");
    
    auto compiled_shader = compiler->compile_shader_from_file(
        shader_file, ShaderStage::Vertex, "vs_main");
    
    ASSERT_NE(compiled_shader, nullptr);
    EXPECT_TRUE(compiled_shader->compilation_result().success);
    
    // Cleanup
    std::filesystem::remove(shader_file);
}

TEST_F(ShaderCompilerTest, BatchCompilation) {
    std::vector<ShaderDescriptor> descriptors;
    
    // Vertex shader
    ShaderDescriptor vertex_desc;
    vertex_desc.stage = ShaderStage::Vertex;
    vertex_desc.language = ShaderLanguage::WGSL;
    vertex_desc.source = R"(
@vertex
fn vs_main() -> @builtin(position) vec4<f32> {
    return vec4<f32>(0.0);
}
)";
    descriptors.push_back(vertex_desc);
    
    // Fragment shader
    ShaderDescriptor fragment_desc;
    fragment_desc.stage = ShaderStage::Fragment;
    fragment_desc.language = ShaderLanguage::WGSL;
    fragment_desc.source = R"(
@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0);
}
)";
    descriptors.push_back(fragment_desc);
    
    auto compiled_shaders = compiler->compile_shaders(descriptors);
    
    ASSERT_EQ(compiled_shaders.size(), 2);
    ASSERT_NE(compiled_shaders[0], nullptr);
    ASSERT_NE(compiled_shaders[1], nullptr);
    EXPECT_TRUE(compiled_shaders[0]->compilation_result().success);
    EXPECT_TRUE(compiled_shaders[1]->compilation_result().success);
}

TEST_F(ShaderCompilerTest, PipelineCreation) {
    // Create temporary shader files
    const std::string vertex_content = R"(
@vertex
fn vs_main(@location(0) pos: vec2<f32>) -> @builtin(position) vec4<f32> {
    return vec4<f32>(pos, 0.0, 1.0);
}
)";
    
    const std::string fragment_content = R"(
@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}
)";
    
    auto vertex_file = createTempShaderFile(vertex_content, ".wgsl");
    auto fragment_file = createTempShaderFile(fragment_content, ".wgsl");
    
    auto pipeline_shaders = compiler->create_render_pipeline_shaders(vertex_file, fragment_file);
    
    ASSERT_NE(pipeline_shaders.vertex, nullptr);
    ASSERT_NE(pipeline_shaders.fragment, nullptr);
    EXPECT_EQ(pipeline_shaders.compute, nullptr);
    
    // Cleanup
    std::filesystem::remove(vertex_file);
    std::filesystem::remove(fragment_file);
}

TEST_F(ShaderCompilerTest, ComputePipelineCreation) {
    const std::string compute_content = R"(
@compute @workgroup_size(1)
fn cs_main() {
    // Empty compute shader
}
)";
    
    auto compute_file = createTempShaderFile(compute_content, ".wgsl");
    
    auto compute_shader = compiler->create_compute_shader(compute_file);
    ASSERT_NE(compute_shader, nullptr);
    EXPECT_TRUE(compute_shader->compilation_result().success);
    
    std::filesystem::remove(compute_file);
}

TEST_F(ShaderCompilerTest, BuiltinShaders) {
    auto fullscreen_vs = compiler->get_fullscreen_vertex_shader();
    ASSERT_NE(fullscreen_vs, nullptr);
    EXPECT_TRUE(fullscreen_vs->compilation_result().success);
    
    auto blit_fs = compiler->get_blit_fragment_shader();
    ASSERT_NE(blit_fs, nullptr);
    EXPECT_TRUE(blit_fs->compilation_result().success);
    
    auto clear_cs = compiler->get_clear_compute_shader();
    ASSERT_NE(clear_cs, nullptr);
    EXPECT_TRUE(clear_cs->compilation_result().success);
}

TEST_F(ShaderCompilerTest, PreprocessorDefines) {
    const std::string shader_source = R"(
@vertex
fn vs_main() -> @builtin(position) vec4<f32> {
#ifdef USE_COLOR
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
#else
    return vec4<f32>(0.0, 1.0, 0.0, 1.0);
#endif
}
)";
    
    ShaderDescriptor desc;
    desc.stage = ShaderStage::Vertex;
    desc.language = ShaderLanguage::WGSL;
    desc.source = shader_source;
    desc.defines.push_back("USE_COLOR");
    
    auto processed = compiler->preprocess_shader(desc.source, desc.defines);
    EXPECT_NE(processed.find("#define USE_COLOR"), std::string::npos);
    EXPECT_NE(processed.find(shader_source), std::string::npos);
}

TEST_F(ShaderCompilerTest, ErrorCallback) {
    std::string captured_error;
    ShaderDescriptor captured_desc;
    bool callback_called = false;
    
    compiler->set_error_callback([&](const std::string& error, const ShaderDescriptor& desc) {
        captured_error = error;
        captured_desc = desc;
        callback_called = true;
    });
    
    ShaderDescriptor desc;
    desc.stage = ShaderStage::Vertex;
    desc.language = ShaderLanguage::WGSL;
    desc.source = "invalid shader code";
    
    auto result = compiler->compile_shader(desc);
    EXPECT_EQ(result, nullptr);
    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(captured_error.empty());
}

TEST_F(ShaderCompilerTest, CacheManagement) {
    // Test cache clearing
    ShaderDescriptor desc;
    desc.stage = ShaderStage::Vertex;
    desc.language = ShaderLanguage::WGSL;
    desc.source = "@vertex fn vs_main() -> @builtin(position) vec4<f32> { return vec4<f32>(0.0); }";
    
    auto shader1 = compiler->compile_shader(desc);
    ASSERT_NE(shader1, nullptr);
    
    EXPECT_GT(compiler->get_cache_size(), 0);
    
    compiler->clear_cache();
    EXPECT_EQ(compiler->get_cache_size(), 0);
    
    // After clearing cache, compilation should work but be a cache miss
    auto shader2 = compiler->compile_shader(desc);
    ASSERT_NE(shader2, nullptr);
    
    // Should be different instances due to cache clear
    EXPECT_NE(shader1.get(), shader2.get());
}

TEST_F(ShaderCompilerTest, StatisticsTracking) {
    compiler->reset_stats();
    auto initial_stats = compiler->get_stats();
    EXPECT_EQ(initial_stats.shaders_compiled, 0);
    EXPECT_EQ(initial_stats.cache_hits, 0);
    EXPECT_EQ(initial_stats.cache_misses, 0);
    
    ShaderDescriptor desc;
    desc.stage = ShaderStage::Vertex;
    desc.language = ShaderLanguage::WGSL;
    desc.source = "@vertex fn vs_main() -> @builtin(position) vec4<f32> { return vec4<f32>(0.0); }";
    
    // First compilation
    auto shader1 = compiler->compile_shader(desc);
    ASSERT_NE(shader1, nullptr);
    
    auto stats1 = compiler->get_stats();
    EXPECT_EQ(stats1.shaders_compiled, 1);
    EXPECT_EQ(stats1.cache_misses, 1);
    EXPECT_EQ(stats1.cache_hits, 0);
    EXPECT_GT(stats1.total_compilation_time.count(), 0);
    
    // Second compilation (cache hit)
    auto shader2 = compiler->compile_shader(desc);
    ASSERT_NE(shader2, nullptr);
    
    auto stats2 = compiler->get_stats();
    EXPECT_EQ(stats2.shaders_compiled, 1); // Still 1 because it was cached
    EXPECT_EQ(stats2.cache_hits, 1);
    EXPECT_EQ(stats2.cache_misses, 1);
}

TEST_F(ShaderCompilerTest, UnsupportedLanguages) {
    ShaderDescriptor hlsl_desc;
    hlsl_desc.stage = ShaderStage::Vertex;
    hlsl_desc.language = ShaderLanguage::HLSL;
    hlsl_desc.source = "float4 vs_main() : SV_POSITION { return float4(0,0,0,1); }";
    
    auto hlsl_shader = compiler->compile_shader(hlsl_desc);
    EXPECT_EQ(hlsl_shader, nullptr); // Not implemented yet
    
    ShaderDescriptor glsl_desc;
    glsl_desc.stage = ShaderStage::Vertex;
    glsl_desc.language = ShaderLanguage::GLSL;
    glsl_desc.source = "#version 450\nvoid main() { gl_Position = vec4(0); }";
    
    auto glsl_shader = compiler->compile_shader(glsl_desc);
    EXPECT_EQ(glsl_shader, nullptr); // Not implemented yet
}

TEST_F(ShaderCompilerTest, FileWatching) {
    // Test file watching functionality
    compiler->watch_shader_files(true);
    
    bool callback_called = false;
    std::filesystem::path callback_path;
    
    compiler->set_shader_reload_callback([&](const std::filesystem::path& path) {
        callback_called = true;
        callback_path = path;
    });
    
    // Create a temporary file
    auto temp_file = createTempShaderFile("@vertex fn vs_main() -> @builtin(position) vec4<f32> { return vec4<f32>(0.0); }", ".wgsl");
    
    // Simulate file change by calling reload check
    compiler->reload_shader_if_changed(temp_file);
    
    // Modify file and check again
    {
        std::ofstream file(temp_file);
        file << "@vertex fn vs_main() -> @builtin(position) vec4<f32> { return vec4<f32>(1.0); }";
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Ensure different timestamp
    compiler->reload_shader_if_changed(temp_file);
    
    compiler->watch_shader_files(false);
    std::filesystem::remove(temp_file);
}

// Performance test
TEST_F(ShaderCompilerTest, CompilationPerformance) {
    const std::string simple_shader = "@vertex fn vs_main() -> @builtin(position) vec4<f32> { return vec4<f32>(0.0); }";
    
    const int num_compilations = 100;
    std::vector<std::shared_ptr<CompiledShader>> shaders;
    shaders.reserve(num_compilations);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_compilations; ++i) {
        ShaderDescriptor desc;
        desc.stage = ShaderStage::Vertex;
        desc.language = ShaderLanguage::WGSL;
        desc.source = simple_shader + " // variant " + std::to_string(i);
        
        auto shader = compiler->compile_shader(desc);
        if (shader) {
            shaders.push_back(shader);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(shaders.size(), num_compilations);
    
    double avg_time = static_cast<double>(duration.count()) / num_compilations;
    std::cout << "Average shader compilation time: " << avg_time << "ms" << std::endl;
    
    // Should compile reasonably quickly
    EXPECT_LT(avg_time, 100.0); // Less than 100ms per shader on average
}