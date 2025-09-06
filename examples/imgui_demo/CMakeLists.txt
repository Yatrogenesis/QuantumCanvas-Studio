cmake_minimum_required(VERSION 3.20)

project(QuantumCanvas-ImGuiDemo
    VERSION 1.0.0
    LANGUAGES CXX C
    DESCRIPTION "ImGui Integration Demo for QuantumCanvas Studio"
)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find packages
find_package(PkgConfig REQUIRED)

# Try to find webgpu-native and GLFW using different methods
find_path(WEBGPU_INCLUDE_DIR
    NAMES webgpu/webgpu.h
    PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/wgpu-native/include
        /usr/local/include
        /usr/include
        C:/vcpkg/installed/x64-windows/include
)

find_library(WEBGPU_LIBRARY
    NAMES wgpu_native webgpu
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/wgpu-native/lib
        /usr/local/lib
        /usr/lib
        C:/vcpkg/installed/x64-windows/lib
)

# GLFW
find_package(glfw3 REQUIRED)
if(NOT TARGET glfw)
    find_library(GLFW_LIBRARY
        NAMES glfw glfw3
        PATHS
            /usr/local/lib
            /usr/lib
            C:/vcpkg/installed/x64-windows/lib
    )
endif()

# ImGui - try to find it or use local copy
find_path(IMGUI_INCLUDE_DIR
    NAMES imgui.h
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui
        /usr/local/include/imgui
        /usr/include/imgui
        C:/vcpkg/installed/x64-windows/include/imgui
)

# ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_INCLUDE_DIR}/imgui.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_demo.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_draw.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_tables.cpp
    ${IMGUI_INCLUDE_DIR}/imgui_widgets.cpp
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_INCLUDE_DIR}/backends/imgui_impl_wgpu.cpp
)

# Check if ImGui sources exist
foreach(IMGUI_SOURCE ${IMGUI_SOURCES})
    if(NOT EXISTS ${IMGUI_SOURCE})
        message(WARNING "ImGui source not found: ${IMGUI_SOURCE}")
        set(IMGUI_FOUND FALSE)
    endif()
endforeach()

# Create ImGui library if sources found
if(IMGUI_FOUND)
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC 
        ${IMGUI_INCLUDE_DIR}
        ${IMGUI_INCLUDE_DIR}/backends
    )
    
    if(TARGET glfw)
        target_link_libraries(imgui PUBLIC glfw)
    elseif(GLFW_LIBRARY)
        target_link_libraries(imgui PUBLIC ${GLFW_LIBRARY})
    endif()
    
    if(WEBGPU_INCLUDE_DIR)
        target_include_directories(imgui PUBLIC ${WEBGPU_INCLUDE_DIR})
    endif()
endif()

# Create executable
add_executable(imgui_demo main.cpp)

# Include directories
if(WEBGPU_INCLUDE_DIR)
    target_include_directories(imgui_demo PRIVATE ${WEBGPU_INCLUDE_DIR})
    message(STATUS "WebGPU include directory: ${WEBGPU_INCLUDE_DIR}")
else()
    message(WARNING "WebGPU headers not found. Download wgpu-native from https://github.com/gfx-rs/wgpu-native")
endif()

if(IMGUI_INCLUDE_DIR)
    target_include_directories(imgui_demo PRIVATE 
        ${IMGUI_INCLUDE_DIR}
        ${IMGUI_INCLUDE_DIR}/backends
    )
    message(STATUS "ImGui include directory: ${IMGUI_INCLUDE_DIR}")
else()
    message(WARNING "ImGui headers not found. Download ImGui from https://github.com/ocornut/imgui")
endif()

# Link libraries
if(TARGET glfw)
    target_link_libraries(imgui_demo PRIVATE glfw)
elseif(GLFW_LIBRARY)
    target_link_libraries(imgui_demo PRIVATE ${GLFW_LIBRARY})
else()
    message(FATAL_ERROR "GLFW not found. Please install GLFW.")
endif()

if(TARGET imgui)
    target_link_libraries(imgui_demo PRIVATE imgui)
else()
    message(WARNING "ImGui library not available. Cannot build demo.")
endif()

if(WEBGPU_LIBRARY)
    target_link_libraries(imgui_demo PRIVATE ${WEBGPU_LIBRARY})
    message(STATUS "WebGPU library: ${WEBGPU_LIBRARY}")
else()
    message(WARNING "WebGPU library not found. Download wgpu-native from https://github.com/gfx-rs/wgpu-native")
endif()

# Platform-specific linking
if(WIN32)
    # Windows specific libraries
    target_link_libraries(imgui_demo PRIVATE ws2_32 userenv bcrypt ntdll)
    
    # Copy WebGPU DLL if it exists
    find_file(WEBGPU_DLL
        NAMES wgpu_native.dll
        PATHS 
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/wgpu-native/bin
            C:/vcpkg/installed/x64-windows/bin
    )
    
    if(WEBGPU_DLL)
        add_custom_command(TARGET imgui_demo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${WEBGPU_DLL}
            $<TARGET_FILE_DIR:imgui_demo>
        )
    endif()
    
elseif(APPLE)
    # macOS specific linking
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(imgui_demo PRIVATE ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
    
elseif(UNIX)
    # Linux specific linking
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED x11)
    target_link_libraries(imgui_demo PRIVATE ${X11_LIBRARIES} dl pthread)
    target_include_directories(imgui_demo PRIVATE ${X11_INCLUDE_DIRS})
endif()

# Compiler flags
if(MSVC)
    target_compile_options(imgui_demo PRIVATE /W4 /permissive-)
else()
    target_compile_options(imgui_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(imgui_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT_NAME "imgui_demo"
)

# Print build info
message(STATUS "")
message(STATUS "ImGui Demo Configuration:")
message(STATUS "=========================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "WebGPU include: ${WEBGPU_INCLUDE_DIR}")
message(STATUS "WebGPU library: ${WEBGPU_LIBRARY}")
message(STATUS "ImGui include: ${IMGUI_INCLUDE_DIR}")
message(STATUS "GLFW found: ${glfw3_FOUND}")
message(STATUS "")

# Installation
install(TARGETS imgui_demo
    RUNTIME DESTINATION examples/imgui_demo
)

# Create setup script for dependencies
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/setup_deps.txt
    "Required Dependencies for ImGui Demo:\n"
    "=====================================\n"
    "\n"
    "1. WebGPU Native:\n"
    "   Download: https://github.com/gfx-rs/wgpu-native/releases\n"
    "   Extract to: ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/wgpu-native/\n"
    "\n"
    "2. ImGui:\n"
    "   Download: https://github.com/ocornut/imgui/releases\n"
    "   Extract to: ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/imgui/\n"
    "\n"
    "3. GLFW:\n"
    "   - Windows (vcpkg): vcpkg install glfw3:x64-windows\n"
    "   - macOS (brew): brew install glfw\n"
    "   - Ubuntu/Debian: sudo apt install libglfw3-dev\n"
    "\n"
    "Directory structure should be:\n"
    "  third_party/\n"
    "    wgpu-native/\n"
    "      include/webgpu/webgpu.h\n"
    "      lib/wgpu_native.lib/.a\n"
    "      bin/wgpu_native.dll (Windows)\n"
    "    imgui/\n"
    "      imgui.h, imgui.cpp, etc.\n"
    "      backends/imgui_impl_glfw.cpp\n"
    "      backends/imgui_impl_wgpu.cpp\n"
)