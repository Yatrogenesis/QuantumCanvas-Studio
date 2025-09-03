# QuantumCanvas Studio - Raster Graphics Module
# High-performance raster graphics engine with GPU acceleration
# Part of MÓDULO 3: RASTER GRAPHICS ENGINE

cmake_minimum_required(VERSION 3.20)

# Module configuration
set(RASTER_MODULE_NAME "QuantumCanvasRaster")
set(RASTER_MODULE_VERSION "1.0.0")

# Module sources
set(RASTER_SOURCES
    # Core raster headers
    brush_engine.hpp
    layer_compositor.hpp
    filter_processor.hpp
    color_manager.hpp
    
    # Implementation files (to be created)
    brush_engine.cpp
    layer_compositor.cpp  
    filter_processor.cpp
    color_manager.cpp
    
    # Additional components
    raster_image.hpp
    raster_image.cpp
    paint_medium.hpp
    paint_medium.cpp
)

# Create the raster module library
add_library(${RASTER_MODULE_NAME} STATIC ${RASTER_SOURCES})

# Set C++20 standard
target_compile_features(${RASTER_MODULE_NAME} PRIVATE cxx_std_20)

# Include directories
target_include_directories(${RASTER_MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

# Link dependencies
target_link_libraries(${RASTER_MODULE_NAME}
    PUBLIC
        QuantumCanvasCore           # Core engine components
        QuantumCanvasRendering      # Rendering engine
        QuantumCanvasMath          # Math utilities
    PRIVATE
        ${WGPU_LIBRARIES}          # WebGPU for GPU acceleration
        ${FREETYPE_LIBRARIES}      # Font rendering for text layers
        ${PNG_LIBRARIES}           # PNG support
        ${JPEG_LIBRARIES}          # JPEG support
        ${TIFF_LIBRARIES}          # TIFF support
)

# Compiler definitions
target_compile_definitions(${RASTER_MODULE_NAME}
    PRIVATE
        QUANTUM_CANVAS_RASTER_EXPORTS
        QUANTUM_CANVAS_VERSION_MAJOR=1
        QUANTUM_CANVAS_VERSION_MINOR=0
        QUANTUM_CANVAS_VERSION_PATCH=0
)

# Platform-specific configurations
if(WIN32)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        QUANTUM_CANVAS_PLATFORM_WINDOWS
    )
    target_link_libraries(${RASTER_MODULE_NAME} PRIVATE
        d3d12       # DirectX 12 support
        dxgi        # DirectX Graphics Infrastructure
    )
elseif(APPLE)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE
        QUANTUM_CANVAS_PLATFORM_MACOS
    )
    target_link_libraries(${RASTER_MODULE_NAME} PRIVATE
        "-framework Metal"
        "-framework CoreGraphics"
        "-framework Foundation"
    )
elseif(UNIX)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE
        QUANTUM_CANVAS_PLATFORM_LINUX
    )
    target_link_libraries(${RASTER_MODULE_NAME} PRIVATE
        X11
        vulkan
    )
endif()

# Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(${RASTER_MODULE_NAME} PRIVATE
            /O2           # Maximum optimization
            /Ob2          # Inline function expansion
            /Ot           # Favor speed over size
            /arch:AVX2    # Enable AVX2 instructions
            /fp:fast      # Fast floating-point model
        )
    else()
        target_compile_options(${RASTER_MODULE_NAME} PRIVATE
            -O3           # Maximum optimization
            -march=native # Use native CPU instructions
            -ffast-math   # Fast math optimizations
            -flto         # Link-time optimization
        )
    endif()
endif()

# Debug configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE
        QUANTUM_CANVAS_DEBUG
        QUANTUM_CANVAS_ENABLE_PROFILING
        QUANTUM_CANVAS_ENABLE_VALIDATION
    )
endif()

# Features configuration
option(QUANTUM_CANVAS_ENABLE_FLUID_SIMULATION "Enable fluid simulation for paint media" ON)
option(QUANTUM_CANVAS_ENABLE_GPU_BRUSH "Enable GPU-accelerated brush rendering" ON)
option(QUANTUM_CANVAS_ENABLE_LAYER_EFFECTS "Enable layer effects and filters" ON)
option(QUANTUM_CANVAS_ENABLE_COLOR_MANAGEMENT "Enable professional color management" ON)

if(QUANTUM_CANVAS_ENABLE_FLUID_SIMULATION)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_ENABLE_FLUID_SIMULATION)
endif()

if(QUANTUM_CANVAS_ENABLE_GPU_BRUSH)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_ENABLE_GPU_BRUSH)
endif()

if(QUANTUM_CANVAS_ENABLE_LAYER_EFFECTS)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_ENABLE_LAYER_EFFECTS)
endif()

if(QUANTUM_CANVAS_ENABLE_COLOR_MANAGEMENT)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_ENABLE_COLOR_MANAGEMENT)
    target_link_libraries(${RASTER_MODULE_NAME} PRIVATE
        ${LCMS2_LIBRARIES}  # Little CMS for color management
    )
endif()

# Performance profiling
option(QUANTUM_CANVAS_ENABLE_RASTER_PROFILING "Enable detailed raster performance profiling" OFF)
if(QUANTUM_CANVAS_ENABLE_RASTER_PROFILING)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_ENABLE_RASTER_PROFILING)
endif()

# Memory management options
option(QUANTUM_CANVAS_ENABLE_MEMORY_POOLS "Use custom memory pools for raster operations" ON)
if(QUANTUM_CANVAS_ENABLE_MEMORY_POOLS)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_ENABLE_MEMORY_POOLS)
endif()

# SIMD optimizations
option(QUANTUM_CANVAS_ENABLE_SIMD "Enable SIMD optimizations for image processing" ON)
if(QUANTUM_CANVAS_ENABLE_SIMD)
    target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_ENABLE_SIMD)
    
    # Check for specific SIMD support
    include(CheckCXXCompilerFlag)
    
    check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)
    if(COMPILER_SUPPORTS_SSE42)
        target_compile_options(${RASTER_MODULE_NAME} PRIVATE -msse4.2)
        target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_HAVE_SSE42)
    endif()
    
    check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
    if(COMPILER_SUPPORTS_AVX2)
        target_compile_options(${RASTER_MODULE_NAME} PRIVATE -mavx2)
        target_compile_definitions(${RASTER_MODULE_NAME} PRIVATE QUANTUM_CANVAS_HAVE_AVX2)
    endif()
endif()

# Threading support
find_package(Threads REQUIRED)
target_link_libraries(${RASTER_MODULE_NAME} PRIVATE Threads::Threads)

# GPU compute shaders
set(RASTER_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
file(GLOB RASTER_SHADERS
    "${RASTER_SHADER_DIR}/*.wgsl"
    "${RASTER_SHADER_DIR}/*.hlsl"
    "${RASTER_SHADER_DIR}/*.glsl"
)

# Create custom target for shader compilation
if(RASTER_SHADERS)
    add_custom_target(${RASTER_MODULE_NAME}_Shaders
        SOURCES ${RASTER_SHADERS}
        COMMENT "Raster module shaders"
    )
    add_dependencies(${RASTER_MODULE_NAME} ${RASTER_MODULE_NAME}_Shaders)
    
    # Copy shaders to output directory
    foreach(shader ${RASTER_SHADERS})
        get_filename_component(shader_name ${shader} NAME)
        configure_file(${shader} "${CMAKE_BINARY_DIR}/shaders/raster/${shader_name}" COPYONLY)
    endforeach()
endif()

# Unit tests
option(QUANTUM_CANVAS_BUILD_RASTER_TESTS "Build raster module unit tests" ON)
if(QUANTUM_CANVAS_BUILD_RASTER_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Benchmarks
option(QUANTUM_CANVAS_BUILD_RASTER_BENCHMARKS "Build raster module benchmarks" OFF)
if(QUANTUM_CANVAS_BUILD_RASTER_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Documentation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME "QuantumCanvas Raster Module")
    set(DOXYGEN_PROJECT_VERSION ${RASTER_MODULE_VERSION})
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs/raster")
    set(DOXYGEN_INPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    
    doxygen_add_docs(${RASTER_MODULE_NAME}_docs
        ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating raster module documentation"
    )
endif()

# Installation
install(TARGETS ${RASTER_MODULE_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/quantum_canvas/raster
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN "internal" EXCLUDE
    PATTERN "tests" EXCLUDE
    PATTERN "benchmarks" EXCLUDE
)

# Export configuration
export(TARGETS ${RASTER_MODULE_NAME}
    FILE "${CMAKE_BINARY_DIR}/QuantumCanvasRasterConfig.cmake"
)

# Module information
message(STATUS "QuantumCanvas Raster Module Configuration:")
message(STATUS "  Version: ${RASTER_MODULE_VERSION}")
message(STATUS "  GPU Brush: ${QUANTUM_CANVAS_ENABLE_GPU_BRUSH}")
message(STATUS "  Fluid Simulation: ${QUANTUM_CANVAS_ENABLE_FLUID_SIMULATION}")
message(STATUS "  Layer Effects: ${QUANTUM_CANVAS_ENABLE_LAYER_EFFECTS}")
message(STATUS "  Color Management: ${QUANTUM_CANVAS_ENABLE_COLOR_MANAGEMENT}")
message(STATUS "  SIMD Optimizations: ${QUANTUM_CANVAS_ENABLE_SIMD}")
message(STATUS "  Memory Pools: ${QUANTUM_CANVAS_ENABLE_MEMORY_POOLS}")

# Performance targets (for reference)
# - Brush rendering: >1000 brush dabs/second
# - Layer compositing: >30 FPS for 4K images
# - Filter processing: >15 FPS for real-time preview
# - Color conversion: <1ms for typical operations
# - Memory allocation: <1μs using memory pools