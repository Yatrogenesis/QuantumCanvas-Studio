#!/bin/bash

echo "QuantumCanvas Studio - Example Build Script"
echo "=========================================="

# Create build directories
mkdir -p examples/basic_triangle/build
mkdir -p examples/imgui_demo/build
mkdir -p artifacts

# Check for dependencies
echo ""
echo "Checking dependencies..."

# Check for GLFW
if ! pkg-config --exists glfw3; then
    echo "WARNING: GLFW not found. Install with:"
    echo "  Ubuntu/Debian: sudo apt install libglfw3-dev"
    echo "  macOS: brew install glfw"
fi

# Check for WebGPU Native
if [ ! -f "third_party/wgpu-native/include/webgpu/webgpu.h" ]; then
    echo "WARNING: WebGPU Native not found. Download from:"
    echo "  https://github.com/gfx-rs/wgpu-native/releases"
    echo "  Extract to: third_party/wgpu-native/"
fi

if [ ! -f "third_party/imgui/imgui.h" ]; then
    echo "WARNING: ImGui not found. Download from:"
    echo "  https://github.com/ocornut/imgui/releases"
    echo "  Extract to: third_party/imgui/"
fi

echo ""
echo "Building basic_triangle example..."
cd examples/basic_triangle

cmake -B build -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "FAILED: CMake configuration failed for basic_triangle"
    cd ../..
    exit 1
fi

cmake --build build --config Release
if [ $? -ne 0 ]; then
    echo "FAILED: Build failed for basic_triangle"
    cd ../..
else
    echo "SUCCESS: basic_triangle built successfully"
    cp build/basic_triangle ../../artifacts/ 2>/dev/null || true
fi

cd ../..

echo ""
echo "Building imgui_demo example..."
cd examples/imgui_demo

cmake -B build -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "FAILED: CMake configuration failed for imgui_demo"
    echo "This is expected if dependencies are missing"
    cd ../..
else
    cmake --build build --config Release
    if [ $? -ne 0 ]; then
        echo "FAILED: Build failed for imgui_demo"
        echo "This is expected if ImGui dependencies are missing"
        cd ../..
    else
        echo "SUCCESS: imgui_demo built successfully"
        cp build/imgui_demo ../../artifacts/ 2>/dev/null || true
        cd ../..
    fi
fi

echo ""
echo "Build Summary:"
echo "=============="
ls -la artifacts/
echo ""
echo "To run examples:"
echo "  ./artifacts/basic_triangle (if built)"
echo "  ./artifacts/imgui_demo (if built, requires WebGPU libs)"
echo ""
echo "For full functionality, ensure all dependencies are installed."
echo "See examples/*/README.md for detailed setup instructions."