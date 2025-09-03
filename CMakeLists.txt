cmake_minimum_required(VERSION 3.25)

project(QuantumCanvas-Studio
    VERSION 1.0.0
    LANGUAGES CXX C
    DESCRIPTION "Professional Creative Suite - Enterprise Grade CAD/Design Software"
    HOMEPAGE_URL "https://github.com/username/quantumcanvas-studio"
)

# =============================================================================
# Project Configuration
# =============================================================================

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Type Configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# =============================================================================
# Options and Features
# =============================================================================

option(QCS_ENABLE_AI_FEATURES "Enable AI-powered design features" ON)
option(QCS_ENABLE_CLOUD_SYNC "Enable cloud synchronization" ON)
option(QCS_ENABLE_COLLABORATION "Enable real-time collaboration" ON)
option(QCS_ENABLE_PLUGIN_SYSTEM "Enable plugin architecture" ON)
option(QCS_ENABLE_CAD_MODULE "Enable CAD functionality" ON)
option(QCS_ENABLE_VECTOR_MODULE "Enable vector graphics" ON)
option(QCS_ENABLE_RASTER_MODULE "Enable raster graphics" ON)
option(QCS_BUILD_TESTS "Build test suite" ON)
option(QCS_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(QCS_ENABLE_PROFILING "Enable built-in profiler" OFF)

# Platform Detection
if(WIN32)
    set(QCS_PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(QCS_PLATFORM_MACOS TRUE)
elseif(UNIX)
    set(QCS_PLATFORM_LINUX TRUE)
endif()

# =============================================================================
# Compiler Configuration
# =============================================================================

# Compiler-specific optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # MSVC optimizations
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /GL /arch:AVX2")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
        add_compile_definitions(NDEBUG)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        # GCC/Clang optimizations
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -flto")
        add_compile_definitions(NDEBUG)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Od /Zi /RTC1")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fsanitize=address")
    endif()
    add_compile_definitions(_DEBUG QCS_DEBUG_BUILD)
endif()

# Warning levels
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# =============================================================================
# Dependencies Management
# =============================================================================

# Find required packages
find_package(PkgConfig REQUIRED)

# Core dependencies
find_package(Eigen3 REQUIRED)
find_package(Freetype REQUIRED)

# Graphics dependencies
if(QCS_PLATFORM_WINDOWS)
    find_library(D3D12_LIBRARY d3d12)
    find_library(DXGI_LIBRARY dxgi)
elseif(QCS_PLATFORM_MACOS)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
elseif(QCS_PLATFORM_LINUX)
    find_package(Vulkan REQUIRED)
    pkg_check_modules(GTK REQUIRED gtk+-3.0)
endif()

# AI/ML dependencies (optional)
if(QCS_ENABLE_AI_FEATURES)
    find_package(OpenCV REQUIRED)
    # ONNX Runtime will be included as submodule
endif()

# Networking dependencies
if(QCS_ENABLE_CLOUD_SYNC OR QCS_ENABLE_COLLABORATION)
    find_package(Protobuf REQUIRED)
    # WebRTC will be included as submodule
endif()

# =============================================================================
# Third-party Libraries (Submodules)
# =============================================================================

# Add third-party directory
add_subdirectory(third_party)

# Core libraries
set(SKIA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/skia")
set(WGPU_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/wgpu-native")

# CAD libraries
if(QCS_ENABLE_CAD_MODULE)
    set(OPENCASCADE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/opencascade")
    set(CGAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cgal")
endif()

# =============================================================================
# Project Structure
# =============================================================================

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
)

# Source directories
add_subdirectory(src/core)
add_subdirectory(src/ui)
add_subdirectory(src/rendering)
add_subdirectory(src/input)

# Optional modules
if(QCS_ENABLE_CAD_MODULE)
    add_subdirectory(src/modules/cad)
endif()

if(QCS_ENABLE_VECTOR_MODULE)
    add_subdirectory(src/modules/vector)
endif()

if(QCS_ENABLE_RASTER_MODULE)
    add_subdirectory(src/modules/raster)
endif()

if(QCS_ENABLE_AI_FEATURES)
    add_subdirectory(src/ai)
endif()

if(QCS_ENABLE_PLUGIN_SYSTEM)
    add_subdirectory(src/plugins)
endif()

# Platform-specific code
if(QCS_PLATFORM_WINDOWS)
    add_subdirectory(src/platform/windows)
elseif(QCS_PLATFORM_MACOS)
    add_subdirectory(src/platform/macos)
elseif(QCS_PLATFORM_LINUX)
    add_subdirectory(src/platform/linux)
endif()

# =============================================================================
# Main Executable
# =============================================================================

set(QCS_SOURCES
    src/main.cpp
    src/application.cpp
    src/application.hpp
)

add_executable(QuantumCanvas-Studio ${QCS_SOURCES})

# Link libraries
target_link_libraries(QuantumCanvas-Studio
    PRIVATE
        qcs_core
        qcs_ui
        qcs_rendering
        qcs_input
        ${FREETYPE_LIBRARIES}
)

# Platform-specific linking
if(QCS_PLATFORM_WINDOWS)
    target_link_libraries(QuantumCanvas-Studio PRIVATE
        ${D3D12_LIBRARY}
        ${DXGI_LIBRARY}
        qcs_platform_windows
    )
elseif(QCS_PLATFORM_MACOS)
    target_link_libraries(QuantumCanvas-Studio PRIVATE
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
        qcs_platform_macos
    )
elseif(QCS_PLATFORM_LINUX)
    target_link_libraries(QuantumCanvas-Studio PRIVATE
        ${Vulkan_LIBRARIES}
        ${GTK_LIBRARIES}
        qcs_platform_linux
    )
endif()

# Optional module linking
if(QCS_ENABLE_CAD_MODULE)
    target_link_libraries(QuantumCanvas-Studio PRIVATE qcs_cad)
endif()

if(QCS_ENABLE_VECTOR_MODULE)
    target_link_libraries(QuantumCanvas-Studio PRIVATE qcs_vector)
endif()

if(QCS_ENABLE_RASTER_MODULE)
    target_link_libraries(QuantumCanvas-Studio PRIVATE qcs_raster)
endif()

if(QCS_ENABLE_AI_FEATURES)
    target_link_libraries(QuantumCanvas-Studio PRIVATE 
        qcs_ai 
        ${OpenCV_LIBS}
    )
endif()

# =============================================================================
# Testing Configuration
# =============================================================================

if(QCS_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()

# =============================================================================
# Benchmarks Configuration
# =============================================================================

if(QCS_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)
    add_subdirectory(benchmarks)
endif()

# =============================================================================
# Installation Configuration
# =============================================================================

# Install executable
install(TARGETS QuantumCanvas-Studio
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install assets
install(DIRECTORY assets/
    DESTINATION share/quantumcanvas-studio/assets
    PATTERN "*.tmp" EXCLUDE
)

# Install documentation
install(FILES README.md LICENSE CONTRIBUTING.md
    DESTINATION share/doc/quantumcanvas-studio
)

# Install desktop files (Linux)
if(QCS_PLATFORM_LINUX)
    install(FILES deployment/linux/quantumcanvas-studio.desktop
        DESTINATION share/applications
    )
    install(FILES assets/icons/quantumcanvas-studio.svg
        DESTINATION share/icons/hicolor/scalable/apps
    )
endif()

# =============================================================================
# Package Configuration
# =============================================================================

# CPack configuration
set(CPACK_PACKAGE_NAME "QuantumCanvas Studio")
set(CPACK_PACKAGE_VENDOR "QuantumCanvas Team")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional Creative Suite")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# Platform-specific packaging
if(QCS_PLATFORM_WINDOWS)
    set(CPACK_GENERATOR "NSIS;WIX")
    set(CPACK_NSIS_DISPLAY_NAME "QuantumCanvas Studio")
    set(CPACK_NSIS_PACKAGE_NAME "QuantumCanvas Studio")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/quantumcanvas.ico")
elseif(QCS_PLATFORM_MACOS)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/dmg-background.png")
elseif(QCS_PLATFORM_LINUX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "QuantumCanvas Team")
    set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Graphics")
endif()

include(CPack)

# =============================================================================
# Development Helpers
# =============================================================================

# Clang-format target
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
        COMMENT "Running clang-format on source files"
    )
endif()

# Documentation generation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# =============================================================================
# Status Summary
# =============================================================================

message(STATUS "")
message(STATUS "QuantumCanvas Studio Configuration Summary:")
message(STATUS "==========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  AI Features: ${QCS_ENABLE_AI_FEATURES}")
message(STATUS "  Cloud Sync: ${QCS_ENABLE_CLOUD_SYNC}")
message(STATUS "  Collaboration: ${QCS_ENABLE_COLLABORATION}")
message(STATUS "  Plugin System: ${QCS_ENABLE_PLUGIN_SYSTEM}")
message(STATUS "  CAD Module: ${QCS_ENABLE_CAD_MODULE}")
message(STATUS "  Vector Module: ${QCS_ENABLE_VECTOR_MODULE}")
message(STATUS "  Raster Module: ${QCS_ENABLE_RASTER_MODULE}")
message(STATUS "")
message(STATUS "Development:")
message(STATUS "  Build Tests: ${QCS_BUILD_TESTS}")
message(STATUS "  Build Benchmarks: ${QCS_BUILD_BENCHMARKS}")
message(STATUS "  Enable Profiling: ${QCS_ENABLE_PROFILING}")
message(STATUS "")
message(STATUS "Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")