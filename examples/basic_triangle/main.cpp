/*
 * Copyright (c) 2024 Francisco Molina (QuantumCanvas Studio)
 * Licensed under Dual License Agreement - See LICENSE file for details
 * 
 * ATTRIBUTION REQUIRED: This software must include attribution to Francisco Molina
 * COMMERCIAL USE: Requires separate license and royalties - contact pako.molina@gmail.com
 * 
 * Project: https://github.com/Yatrogenesis/QuantumCanvas-Studio
 * Author: Francisco Molina <pako.molina@gmail.com>
 */

#include <iostream>
#include <vector>
#include <memory>
#include <cassert>

// Include WGPU-native headers
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

// Simple triangle vertex shader
const char* vertex_shader_source = R"(
@vertex fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> @builtin(position) vec4f {
    var pos = array<vec2f, 3>(
        vec2f( 0.0,  0.5),
        vec2f(-0.5, -0.5),
        vec2f( 0.5, -0.5)
    );
    return vec4f(pos[vertexIndex], 0.0, 1.0);
}
)";

// Simple triangle fragment shader
const char* fragment_shader_source = R"(
@fragment fn fs_main() -> @location(0) vec4f {
    return vec4f(1.0, 0.0, 0.0, 1.0); // Red color
}
)";

class TriangleExample {
private:
    GLFWwindow* window = nullptr;
    WGPUInstance instance = nullptr;
    WGPUSurface surface = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    WGPUSwapChain swapChain = nullptr;
    WGPURenderPipeline renderPipeline = nullptr;
    
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    
public:
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Create window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(windowWidth, windowHeight, "QuantumCanvas Studio - Basic Triangle", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        // Initialize WebGPU
        if (!initWebGPU()) {
            cleanup();
            return false;
        }
        
        // Create render pipeline
        if (!createRenderPipeline()) {
            cleanup();
            return false;
        }
        
        std::cout << "QuantumCanvas Studio - Basic Triangle Example Initialized Successfully!" << std::endl;
        std::cout << "- WebGPU Device: " << (device ? "Created" : "Failed") << std::endl;
        std::cout << "- Swap Chain: " << (swapChain ? "Created" : "Failed") << std::endl;
        std::cout << "- Render Pipeline: " << (renderPipeline ? "Created" : "Failed") << std::endl;
        
        return true;
    }
    
    void run() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            render();
        }
    }
    
    void cleanup() {
        if (renderPipeline) wgpuRenderPipelineRelease(renderPipeline);
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
        WGPUSwapChainDescriptor swapChainDesc = {};
        swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
        swapChainDesc.format = WGPUTextureFormat_BGRA8Unorm;
        swapChainDesc.width = windowWidth;
        swapChainDesc.height = windowHeight;
        swapChainDesc.presentMode = WGPUPresentMode_Fifo;
        
        swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
        if (!swapChain) {
            std::cerr << "Failed to create swap chain" << std::endl;
            return false;
        }
        
        return true;
    }
    
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
    
    bool createRenderPipeline() {
        // Create shader modules
        WGPUShaderModuleWGSLDescriptor vertexShaderDesc = {};
        vertexShaderDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        vertexShaderDesc.code = vertex_shader_source;
        
        WGPUShaderModuleDescriptor vertexModuleDesc = {};
        vertexModuleDesc.nextInChain = reinterpret_cast<const WGPUChainedStruct*>(&vertexShaderDesc);
        
        WGPUShaderModule vertexShader = wgpuDeviceCreateShaderModule(device, &vertexModuleDesc);
        
        WGPUShaderModuleWGSLDescriptor fragmentShaderDesc = {};
        fragmentShaderDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        fragmentShaderDesc.code = fragment_shader_source;
        
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
        
        renderPipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
        
        // Clean up shaders
        wgpuShaderModuleRelease(vertexShader);
        wgpuShaderModuleRelease(fragmentShader);
        
        return renderPipeline != nullptr;
    }
    
    void render() {
        WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
        if (!nextTexture) {
            std::cerr << "Failed to get current texture view" << std::endl;
            return;
        }
        
        WGPUCommandEncoderDescriptor commandEncoderDesc = {};
        WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);
        
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = nextTexture;
        colorAttachment.loadOp = WGPULoadOp_Clear;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = {0.0, 0.0, 0.0, 1.0}; // Black background
        
        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;
        
        WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDesc);
        wgpuRenderPassEncoderSetPipeline(renderPassEncoder, renderPipeline);
        wgpuRenderPassEncoderDraw(renderPassEncoder, 3, 1, 0, 0);
        wgpuRenderPassEncoderEnd(renderPassEncoder);
        
        WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
        WGPUCommandBuffer commands = wgpuCommandEncoderFinish(commandEncoder, &cmdBufferDescriptor);
        wgpuQueueSubmit(queue, 1, &commands);
        
        wgpuSwapChainPresent(swapChain);
        
        wgpuCommandBufferRelease(commands);
        wgpuRenderPassEncoderRelease(renderPassEncoder);
        wgpuCommandEncoderRelease(commandEncoder);
        wgpuTextureViewRelease(nextTexture);
    }
    
    // Synchronous adapter request helper
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
    
    // Synchronous device request helper
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
    std::cout << "QuantumCanvas Studio - Basic WebGPU Triangle Example" << std::endl;
    std::cout << "====================================================" << std::endl;
    std::cout << "This example demonstrates:" << std::endl;
    std::cout << "- WebGPU initialization and setup" << std::endl;
    std::cout << "- Cross-platform window creation with GLFW" << std::endl;
    std::cout << "- Basic vertex/fragment shader pipeline" << std::endl;
    std::cout << "- Rendering a simple red triangle" << std::endl;
    std::cout << std::endl;
    
    TriangleExample example;
    
    if (!example.initialize()) {
        std::cerr << "Failed to initialize triangle example" << std::endl;
        return 1;
    }
    
    std::cout << "Window created successfully. Press ESC or close window to exit." << std::endl;
    example.run();
    example.cleanup();
    
    std::cout << "Example completed successfully!" << std::endl;
    return 0;
}