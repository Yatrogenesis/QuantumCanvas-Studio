cmake_minimum_required(VERSION 3.20)

project(QuantumCanvas-BasicTriangle
    VERSION 1.0.0
    LANGUAGES CXX C
    DESCRIPTION "Basic WebGPU Triangle Example for QuantumCanvas Studio"
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

# Create executable
add_executable(basic_triangle main.cpp)

# Include directories
if(WEBGPU_INCLUDE_DIR)
    target_include_directories(basic_triangle PRIVATE ${WEBGPU_INCLUDE_DIR})
    message(STATUS "WebGPU include directory: ${WEBGPU_INCLUDE_DIR}")
else()
    message(WARNING "WebGPU headers not found. Download wgpu-native from https://github.com/gfx-rs/wgpu-native")
endif()

# Link libraries
if(TARGET glfw)
    target_link_libraries(basic_triangle PRIVATE glfw)
elseif(GLFW_LIBRARY)
    target_link_libraries(basic_triangle PRIVATE ${GLFW_LIBRARY})
else()
    message(FATAL_ERROR "GLFW not found. Please install GLFW.")
endif()

if(WEBGPU_LIBRARY)
    target_link_libraries(basic_triangle PRIVATE ${WEBGPU_LIBRARY})
    message(STATUS "WebGPU library: ${WEBGPU_LIBRARY}")
else()
    message(WARNING "WebGPU library not found. Download wgpu-native from https://github.com/gfx-rs/wgpu-native")
endif()

# Platform-specific linking
if(WIN32)
    # Windows specific libraries
    target_link_libraries(basic_triangle PRIVATE ws2_32 userenv bcrypt ntdll)
    
    # Copy WebGPU DLL if it exists
    find_file(WEBGPU_DLL
        NAMES wgpu_native.dll
        PATHS 
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/wgpu-native/bin
            C:/vcpkg/installed/x64-windows/bin
    )
    
    if(WEBGPU_DLL)
        add_custom_command(TARGET basic_triangle POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${WEBGPU_DLL}
            $<TARGET_FILE_DIR:basic_triangle>
        )
    endif()
    
elseif(APPLE)
    # macOS specific linking
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(basic_triangle PRIVATE ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
    
elseif(UNIX)
    # Linux specific linking
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED x11)
    target_link_libraries(basic_triangle PRIVATE ${X11_LIBRARIES} dl pthread)
    target_include_directories(basic_triangle PRIVATE ${X11_INCLUDE_DIRS})
endif()

# Compiler flags
if(MSVC)
    target_compile_options(basic_triangle PRIVATE /W4 /permissive-)
else()
    target_compile_options(basic_triangle PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(basic_triangle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    OUTPUT_NAME "basic_triangle"
)

# Print build info
message(STATUS "")
message(STATUS "Basic Triangle Example Configuration:")
message(STATUS "=====================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "WebGPU include: ${WEBGPU_INCLUDE_DIR}")
message(STATUS "WebGPU library: ${WEBGPU_LIBRARY}")
message(STATUS "GLFW found: ${glfw3_FOUND}")
message(STATUS "")

# Installation
install(TARGETS basic_triangle
    RUNTIME DESTINATION examples/basic_triangle
)

# Create a simple script to download dependencies if missing
if(NOT WEBGPU_LIBRARY OR NOT WEBGPU_INCLUDE_DIR)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/download_deps.txt
        "WebGPU Native not found.\n"
        "Please download from: https://github.com/gfx-rs/wgpu-native/releases\n"
        "Extract to: ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/wgpu-native/\n"
        "\n"
        "Directory structure should be:\n"
        "  third_party/wgpu-native/\n"
        "    include/webgpu/webgpu.h\n"
        "    lib/libwgpu_native.a (Linux/macOS) or wgpu_native.lib (Windows)\n"
        "    bin/wgpu_native.dll (Windows)\n"
    )
endif()