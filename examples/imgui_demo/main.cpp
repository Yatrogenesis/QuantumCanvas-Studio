#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include <chrono>

// Include WGPU-native and GLFW headers
#ifdef _WIN32
#include <windows.h>
#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__APPLE__)
#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#endif

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>

// Simple vertex shader for background
const char* background_vertex_shader = R"(
@vertex fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> @builtin(position) vec4f {
    var pos = array<vec2f, 6>(
        vec2f(-1.0, -1.0),
        vec2f( 1.0, -1.0),
        vec2f( 1.0,  1.0),
        vec2f(-1.0, -1.0),
        vec2f( 1.0,  1.0),
        vec2f(-1.0,  1.0)
    );
    return vec4f(pos[vertexIndex], 0.0, 1.0);
}
)";

// Simple fragment shader for background gradient
const char* background_fragment_shader = R"(
@fragment fn fs_main(@builtin(position) position: vec4f) -> @location(0) vec4f {
    let uv = position.xy / 800.0; // Normalized screen coordinates
    let gradient = mix(vec3f(0.1, 0.2, 0.4), vec3f(0.2, 0.3, 0.6), uv.y);
    return vec4f(gradient, 1.0);
}
)";

class ImGuiDemo {
private:
    GLFWwindow* window = nullptr;
    WGPUInstance instance = nullptr;
    WGPUSurface surface = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    WGPUSwapChain swapChain = nullptr;
    WGPURenderPipeline backgroundPipeline = nullptr;
    
    uint32_t windowWidth = 1200;
    uint32_t windowHeight = 800;
    
    // Demo state
    bool show_demo_window = true;
    bool show_metrics_window = true;
    bool show_about_window = false;
    float clear_color[3] = {0.1f, 0.2f, 0.4f};
    
    // Performance metrics
    std::chrono::high_resolution_clock::time_point last_time;
    float fps = 0.0f;
    int frame_count = 0;
    
public:
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        
        window = glfwCreateWindow(windowWidth, windowHeight, "QuantumCanvas Studio - ImGui Demo", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        // Set resize callback
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
            auto* demo = static_cast<ImGuiDemo*>(glfwGetWindowUserPointer(window));
            demo->onResize(width, height);
        });
        
        // Initialize WebGPU
        if (!initWebGPU()) {
            cleanup();
            return false;
        }
        
        // Initialize ImGui
        if (!initImGui()) {
            cleanup();
            return false;
        }
        
        // Create background pipeline
        if (!createBackgroundPipeline()) {
            cleanup();
            return false;
        }
        
        last_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "QuantumCanvas Studio - ImGui Demo Initialized Successfully!" << std::endl;
        std::cout << "- WebGPU Device: " << (device ? "✓" : "✗") << std::endl;
        std::cout << "- Swap Chain: " << (swapChain ? "✓" : "✗") << std::endl;
        std::cout << "- ImGui: " << (ImGui::GetCurrentContext() ? "✓" : "✗") << std::endl;
        std::cout << "- Background Pipeline: " << (backgroundPipeline ? "✓" : "✗") << std::endl;
        
        return true;
    }
    
    void run() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            updateFPS();
            render();
        }
    }
    
    void cleanup() {
        if (backgroundPipeline) wgpuRenderPipelineRelease(backgroundPipeline);
        
        // Cleanup ImGui
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        // Cleanup WebGPU
        if (swapChain) wgpuSwapChainRelease(swapChain);
        if (queue) wgpuQueueRelease(queue);
        if (device) wgpuDeviceRelease(device);
        if (adapter) wgpuAdapterRelease(adapter);
        if (surface) wgpuSurfaceRelease(surface);
        if (instance) wgpuInstanceRelease(instance);
        
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }
    
private:
    bool initWebGPU() {
        // Create WebGPU instance
        WGPUInstanceDescriptor instanceDesc = {};
        instance = wgpuCreateInstance(&instanceDesc);
        if (!instance) {
            std::cerr << "Failed to create WebGPU instance" << std::endl;
            return false;
        }
        
        // Create surface
        surface = createSurface();
        if (!surface) {
            std::cerr << "Failed to create WebGPU surface" << std::endl;
            return false;
        }
        
        // Request adapter
        WGPURequestAdapterOptions adapterOpts = {};
        adapterOpts.compatibleSurface = surface;
        adapterOpts.powerPreference = WGPUPowerPreference_HighPerformance;
        
        adapter = requestAdapterSync(instance, &adapterOpts);
        if (!adapter) {
            std::cerr << "Failed to get WebGPU adapter" << std::endl;
            return false;
        }
        
        // Request device
        WGPUDeviceDescriptor deviceDesc = {};
        device = requestDeviceSync(adapter, &deviceDesc);
        if (!device) {
            std::cerr << "Failed to get WebGPU device" << std::endl;
            return false;
        }
        
        // Get queue
        queue = wgpuDeviceGetQueue(device);
        
        // Create swap chain
        createSwapChain();
        
        return true;
    }
    
    bool initImGui() {
        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        
        // Setup style
        ImGui::StyleColorsDark();
        
        // Customize style for QuantumCanvas
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 6.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 4.0f;
        
        // Custom colors
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.13f, 0.95f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.3f, 0.5f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.2f, 0.3f, 0.5f, 0.8f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.35f, 0.6f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.4f, 0.7f, 1.0f);
        
        // Setup Platform/Renderer backends
        if (!ImGui_ImplGlfw_InitForOther(window, true)) {
            std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl;
            return false;
        }
        
        if (!ImGui_ImplWGPU_Init(device, 3, WGPUTextureFormat_BGRA8Unorm, WGPUTextureFormat_Undefined)) {
            std::cerr << "Failed to initialize ImGui WGPU backend" << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool createBackgroundPipeline() {
        // Create shader modules
        WGPUShaderModuleWGSLDescriptor vertexShaderDesc = {};
        vertexShaderDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        vertexShaderDesc.code = background_vertex_shader;
        
        WGPUShaderModuleDescriptor vertexModuleDesc = {};
        vertexModuleDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&vertexShaderDesc);
        
        WGPUShaderModule vertexShader = wgpuDeviceCreateShaderModule(device, &vertexModuleDesc);
        
        WGPUShaderModuleWGSLDescriptor fragmentShaderDesc = {};
        fragmentShaderDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        fragmentShaderDesc.code = background_fragment_shader;
        
        WGPUShaderModuleDescriptor fragmentModuleDesc = {};
        fragmentModuleDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&fragmentShaderDesc);
        
        WGPUShaderModule fragmentShader = wgpuDeviceCreateShaderModule(device, &fragmentModuleDesc);
        
        // Create render pipeline
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = WGPUTextureFormat_BGRA8Unorm;
        colorTarget.writeMask = WGPUColorWriteMask_All;
        
        WGPUFragmentState fragmentState = {};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = "fs_main";
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;
        
        WGPUVertexState vertexState = {};
        vertexState.module = vertexShader;
        vertexState.entryPoint = "vs_main";
        
        WGPURenderPipelineDescriptor pipelineDesc = {};
        pipelineDesc.vertex = vertexState;
        pipelineDesc.fragment = &fragmentState;
        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDesc.primitive.cullMode = WGPUCullMode_None;
        
        WGPUMultisampleState multisample = {};
        multisample.count = 1;
        multisample.mask = 0xFFFFFFFF;
        multisample.alphaToCoverageEnabled = false;
        pipelineDesc.multisample = multisample;
        
        backgroundPipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
        
        // Clean up shaders
        wgpuShaderModuleRelease(vertexShader);
        wgpuShaderModuleRelease(fragmentShader);
        
        return backgroundPipeline != nullptr;
    }
    
    void createSwapChain() {
        if (swapChain) {
            wgpuSwapChainRelease(swapChain);
        }
        
        WGPUSwapChainDescriptor swapChainDesc = {};
        swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
        swapChainDesc.format = WGPUTextureFormat_BGRA8Unorm;
        swapChainDesc.width = windowWidth;
        swapChainDesc.height = windowHeight;
        swapChainDesc.presentMode = WGPUPresentMode_Fifo;
        
        swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
    }
    
    void onResize(int width, int height) {
        windowWidth = width;
        windowHeight = height;
        createSwapChain();
    }
    
    void updateFPS() {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_time);
        
        frame_count++;
        if (delta.count() >= 1000000) { // Update every second
            fps = frame_count / (delta.count() / 1000000.0f);
            frame_count = 0;
            last_time = current_time;
        }
    }
    
    void render() {
        WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
        if (!nextTexture) {
            return;
        }
        
        // Start ImGui frame
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Create docking space
        createDockSpace();
        
        // Render ImGui windows
        renderImGuiContent();
        
        // Render
        ImGui::Render();
        
        WGPUCommandEncoderDescriptor encoderDesc = {};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
        
        // Render pass
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = nextTexture;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {clear_color[0], clear_color[1], clear_color[2], 1.0f};
        
        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        
        // Draw background
        wgpuRenderPassEncoderSetPipeline(renderPass, backgroundPipeline);
        wgpuRenderPassEncoderDraw(renderPass, 6, 1, 0, 0);
        
        // Draw ImGui
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
        
        wgpuRenderPassEncoderEnd(renderPass);
        
        WGPUCommandBufferDescriptor cmdBufferDesc = {};
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
        wgpuQueueSubmit(queue, 1, &commands);
        
        wgpuSwapChainPresent(swapChain);
        
        // Cleanup
        wgpuCommandBufferRelease(commands);
        wgpuRenderPassEncoderRelease(renderPass);
        wgpuCommandEncoderRelease(encoder);
        wgpuTextureViewRelease(nextTexture);
    }
    
    void createDockSpace() {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);
        
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
                ImGui::MenuItem("Metrics Window", nullptr, &show_metrics_window);
                ImGui::MenuItem("About", nullptr, &show_about_window);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        // Submit the DockSpace
        ImGuiID dockspace_id = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        
        ImGui::End();
    }
    
    void renderImGuiContent() {
        // Demo window
        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }
        
        // Metrics window
        if (show_metrics_window) {
            ImGui::Begin("Performance Metrics", &show_metrics_window);
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Frame Time: %.3f ms", 1000.0f / fps);
            ImGui::Separator();
            ImGui::ColorEdit3("Clear Color", clear_color);
            ImGui::End();
        }
        
        // About window
        if (show_about_window) {
            ImGui::Begin("About QuantumCanvas Studio", &show_about_window);
            ImGui::Text("QuantumCanvas Studio");
            ImGui::Separator();
            ImGui::Text("A professional creative software suite built with:");
            ImGui::BulletText("WebGPU for GPU acceleration");
            ImGui::BulletText("ImGui for user interface");
            ImGui::BulletText("GLFW for cross-platform windowing");
            ImGui::BulletText("Modern C++20");
            ImGui::Separator();
            ImGui::Text("This demo showcases:");
            ImGui::BulletText("WebGPU rendering pipeline");
            ImGui::BulletText("ImGui integration");
            ImGui::BulletText("Performance metrics");
            ImGui::BulletText("Dockable interface");
            if (ImGui::Button("Close")) {
                show_about_window = false;
            }
            ImGui::End();
        }
        
        // Main canvas area (placeholder)
        ImGui::Begin("Canvas");
        ImGui::Text("Main Canvas Area");
        ImGui::Text("This is where the drawing/editing would happen");
        ImGui::Button("Sample Tool", ImVec2(100, 30));
        ImGui::SameLine();
        ImGui::Button("Another Tool", ImVec2(100, 30));
        ImGui::End();
        
        // Tools panel
        ImGui::Begin("Tools");
        ImGui::Text("Tool Palette");
        ImGui::Separator();
        const char* tools[] = {"Select", "Brush", "Pencil", "Eraser", "Shape", "Text"};
        static int selected_tool = 0;
        for (int i = 0; i < 6; i++) {
            if (ImGui::Selectable(tools[i], selected_tool == i)) {
                selected_tool = i;
            }
        }
        ImGui::End();
        
        // Properties panel
        ImGui::Begin("Properties");
        ImGui::Text("Tool Properties");
        ImGui::Separator();
        static float brush_size = 10.0f;
        static float opacity = 1.0f;
        ImGui::SliderFloat("Size", &brush_size, 1.0f, 100.0f);
        ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f);
        static ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImGui::ColorEdit3("Color", (float*)&color);
        ImGui::End();
    }
    
    // Helper methods for WebGPU synchronous operations
    WGPUSurface createSurface() {
#ifdef _WIN32
        WGPUSurfaceDescriptorFromWindowsHWND surfaceDesc = {};
        surfaceDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
        surfaceDesc.hinstance = GetModuleHandle(nullptr);
        surfaceDesc.hwnd = glfwGetWin32Window(window);
        
        WGPUSurfaceDescriptor descriptor = {};
        descriptor.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceDesc);
        
        return wgpuInstanceCreateSurface(instance, &descriptor);
        
#elif defined(__APPLE__)
        WGPUSurfaceDescriptorFromMetalLayer surfaceDesc = {};
        surfaceDesc.chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;
        surfaceDesc.layer = glfwGetCocoaWindow(window);
        
        WGPUSurfaceDescriptor descriptor = {};
        descriptor.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceDesc);
        
        return wgpuInstanceCreateSurface(instance, &descriptor);
        
#elif defined(__linux__)
        WGPUSurfaceDescriptorFromXlibWindow surfaceDesc = {};
        surfaceDesc.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
        surfaceDesc.display = glfwGetX11Display();
        surfaceDesc.window = glfwGetX11Window(window);
        
        WGPUSurfaceDescriptor descriptor = {};
        descriptor.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&surfaceDesc);
        
        return wgpuInstanceCreateSurface(instance, &descriptor);
#endif
        return nullptr;
    }
    
    WGPUAdapter requestAdapterSync(WGPUInstance instance, const WGPURequestAdapterOptions* options) {
        struct AdapterRequestData {
            WGPUAdapter adapter = nullptr;
            bool done = false;
        } data;
        
        auto callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* userdata) {
            auto* data = static_cast<AdapterRequestData*>(userdata);
            if (status == WGPURequestAdapterStatus_Success) {
                data->adapter = adapter;
            } else {
                std::cerr << "Failed to get adapter: " << message << std::endl;
            }
            data->done = true;
        };
        
        wgpuInstanceRequestAdapter(instance, options, callback, &data);
        
        while (!data.done) {
            wgpuInstanceProcessEvents(instance);
        }
        
        return data.adapter;
    }
    
    WGPUDevice requestDeviceSync(WGPUAdapter adapter, const WGPUDeviceDescriptor* descriptor) {
        struct DeviceRequestData {
            WGPUDevice device = nullptr;
            bool done = false;
        } data;
        
        auto callback = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata) {
            auto* data = static_cast<DeviceRequestData*>(userdata);
            if (status == WGPURequestDeviceStatus_Success) {
                data->device = device;
            } else {
                std::cerr << "Failed to get device: " << message << std::endl;
            }
            data->done = true;
        };
        
        wgpuAdapterRequestDevice(adapter, descriptor, callback, &data);
        
        while (!data.done) {
            wgpuInstanceProcessEvents(instance);
        }
        
        return data.device;
    }
};

int main() {
    std::cout << "QuantumCanvas Studio - ImGui Integration Demo" << std::endl;
    std::cout << "===============================================" << std::endl;
    std::cout << "This demo showcases:" << std::endl;
    std::cout << "- Professional UI with ImGui integration" << std::endl;
    std::cout << "- Dockable windows and panels" << std::endl;
    std::cout << "- WebGPU background rendering" << std::endl;
    std::cout << "- Real-time performance metrics" << std::endl;
    std::cout << "- Professional application layout" << std::endl;
    std::cout << std::endl;
    
    ImGuiDemo demo;
    
    if (!demo.initialize()) {
        std::cerr << "Failed to initialize ImGui demo" << std::endl;
        return 1;
    }
    
    std::cout << "Demo initialized successfully!" << std::endl;
    std::cout << "- Use 'View' menu to toggle different panels" << std::endl;
    std::cout << "- Try docking windows by dragging their title bars" << std::endl;
    std::cout << "- Adjust clear color in Performance Metrics panel" << std::endl;
    std::cout << std::endl;
    
    demo.run();
    demo.cleanup();
    
    std::cout << "Demo completed successfully!" << std::endl;
    return 0;
}