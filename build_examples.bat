@echo off
echo QuantumCanvas Studio - Example Build Script
echo ==========================================

REM Create build directories
if not exist "examples\basic_triangle\build" mkdir "examples\basic_triangle\build"
if not exist "examples\imgui_demo\build" mkdir "examples\imgui_demo\build"
if not exist "artifacts" mkdir "artifacts"

REM Check for dependencies
echo.
echo Checking dependencies...

REM Check for GLFW
where glfw3.dll >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo WARNING: GLFW not found in PATH. Install via vcpkg: vcpkg install glfw3:x64-windows
)

REM Check for WebGPU Native
if not exist "third_party\wgpu-native\include\webgpu\webgpu.h" (
    echo WARNING: WebGPU Native not found. Download from:
    echo https://github.com/gfx-rs/wgpu-native/releases
    echo Extract to: third_party\wgpu-native\
)

if not exist "third_party\imgui\imgui.h" (
    echo WARNING: ImGui not found. Download from:
    echo https://github.com/ocornut/imgui/releases
    echo Extract to: third_party\imgui\
)

echo.
echo Building basic_triangle example...
cd examples\basic_triangle

cmake -B build -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo FAILED: CMake configuration failed for basic_triangle
    cd ..\..
    goto :imgui_demo
)

cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo FAILED: Build failed for basic_triangle
    cd ..\..
    goto :imgui_demo
) else (
    echo SUCCESS: basic_triangle built successfully
    copy build\Release\basic_triangle.exe ..\..\artifacts\ 2>nul || copy build\basic_triangle.exe ..\..\artifacts\ 2>nul
)

cd ..\..

:imgui_demo
echo.
echo Building imgui_demo example...
cd examples\imgui_demo

cmake -B build -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo FAILED: CMake configuration failed for imgui_demo
    echo This is expected if dependencies are missing
    cd ..\..
    goto :complete
)

cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo FAILED: Build failed for imgui_demo
    echo This is expected if ImGui dependencies are missing
    cd ..\..
    goto :complete
) else (
    echo SUCCESS: imgui_demo built successfully
    copy build\Release\imgui_demo.exe ..\..\artifacts\ 2>nul || copy build\imgui_demo.exe ..\..\artifacts\ 2>nul
)

cd ..\..

:complete
echo.
echo Build Summary:
echo ==============
dir artifacts
echo.
echo To run examples:
echo - artifacts\basic_triangle.exe (if built)
echo - artifacts\imgui_demo.exe (if built, requires wgpu_native.dll)
echo.
echo For full functionality, ensure all dependencies are installed.
echo See examples\*/README.md for detailed setup instructions.

pause