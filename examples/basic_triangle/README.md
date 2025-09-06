# Basic Triangle Example

This example demonstrates the fundamental WebGPU rendering capabilities of QuantumCanvas Studio by displaying a simple red triangle.

## What it demonstrates

- WebGPU device initialization and context creation
- Cross-platform window management using GLFW
- Basic vertex and fragment shader compilation
- Simple render pipeline setup
- Frame rendering loop with proper resource management

## Building

### Prerequisites

1. **WGPU-Native**: Download from [wgpu-native releases](https://github.com/gfx-rs/wgpu-native/releases)
   - Extract to `../../third_party/wgpu-native/`
   - Ensure the following structure:
     ```
     third_party/wgpu-native/
     ├── include/webgpu/webgpu.h
     ├── lib/wgpu_native.lib (Windows) or libwgpu_native.a (Linux/macOS)
     └── bin/wgpu_native.dll (Windows only)
     ```

2. **GLFW**: Install via package manager or vcpkg
   - Windows (vcpkg): `vcpkg install glfw3:x64-windows`
   - macOS (brew): `brew install glfw`
   - Ubuntu/Debian: `sudo apt install libglfw3-dev`

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run
./basic_triangle        # Linux/macOS
basic_triangle.exe      # Windows
```

## Expected Output

- A window titled "QuantumCanvas Studio - Basic Triangle"
- Black background with a red triangle in the center
- Console output showing successful WebGPU initialization

## Technical Details

- **Shaders**: Written in WGSL (WebGPU Shading Language)
- **Rendering**: Single draw call with 3 vertices (no vertex buffer)
- **Format**: BGRA8 framebuffer with Fifo presentation mode
- **Platform**: Cross-platform (Windows, macOS, Linux)

This example serves as the foundation for more complex rendering examples and verifies that the WebGPU integration is working correctly.