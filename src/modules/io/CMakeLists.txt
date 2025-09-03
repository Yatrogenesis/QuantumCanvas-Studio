# QuantumCanvas Studio - I/O Module
# Export/Import Engine for comprehensive file format support

cmake_minimum_required(VERSION 3.20)

# Module definition
set(IO_MODULE_NAME "quantum_canvas_io")

# Source files
set(IO_SOURCES
    # Core I/O system
    file_format_manager.cpp
    
    # Image codecs
    image_codecs.cpp
    png_codec.cpp
    jpeg_codec.cpp
    tiff_codec.cpp
    webp_codec.cpp
    raw_codec.cpp
    bmp_codec.cpp
    gif_codec.cpp
    
    # Vector formats
    vector_formats.cpp
    svg_handler.cpp
    ai_handler.cpp
    pdf_handler.cpp
    eps_handler.cpp
    
    # CAD formats
    dwg_handler.cpp
    dxf_handler.cpp
    step_handler.cpp
    iges_handler.cpp
    
    # QuantumCanvas native formats
    qcsx_handler.cpp
    qcs_handler.cpp
    
    # Metadata handling
    metadata_extractor.cpp
    exif_reader.cpp
    xmp_processor.cpp
    
    # Streaming I/O
    stream_reader.cpp
    stream_writer.cpp
    progressive_loader.cpp
    
    # Utilities
    format_detection.cpp
    thumbnail_generator.cpp
    batch_processor.cpp
    
    # Third-party integration
    oda_integration.cpp      # Open Design Alliance for DWG/DXF
    poppler_integration.cpp  # PDF handling
    libraw_integration.cpp   # RAW camera formats
)

# Header files
set(IO_HEADERS
    file_format_manager.hpp
    image_codecs.hpp
    vector_formats.hpp
    dwg_handler.hpp
    
    # Private headers
    private/xml_parser.hpp
    private/postscript_parser.hpp
    private/pdf_parser.hpp
    private/magic_bytes.hpp
    private/stream_buffer.hpp
    private/format_registry.hpp
    private/color_profile_manager.hpp
)

# Create the I/O module library
add_library(${IO_MODULE_NAME} STATIC ${IO_SOURCES} ${IO_HEADERS})

# Include directories
target_include_directories(${IO_MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# Link with core modules
target_link_libraries(${IO_MODULE_NAME}
    PRIVATE
        quantum_canvas_core
        quantum_canvas_rendering
        quantum_canvas_vector
        quantum_canvas_raster
)

# External dependencies
find_package(PkgConfig REQUIRED)

# Image format libraries
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(TIFF REQUIRED)
find_package(ZLIB REQUIRED)

# WebP support
pkg_check_modules(WEBP REQUIRED libwebp libwebpmux libwebpdemux)

# LibRaw for camera RAW formats
pkg_check_modules(LIBRAW REQUIRED libraw)

# FreeImage as fallback
find_package(FreeImage)

# XML parsing for SVG
find_package(LibXml2 REQUIRED)

# PDF support via Poppler
pkg_check_modules(POPPLER REQUIRED poppler-cpp)

# LCMS2 for color management
pkg_check_modules(LCMS2 REQUIRED lcms2)

# OpenEXR for HDR formats (optional)
find_package(OpenEXR)

# Open Design Alliance SDK for DWG/DXF (optional)
find_path(ODA_INCLUDE_DIR NAMES "OdaCommon.h" PATHS 
    "C:/Program Files/Open Design Alliance"
    "C:/ODA"
    "/opt/oda"
    "/usr/local/include/oda"
    PATH_SUFFIXES "Include")

if(ODA_INCLUDE_DIR)
    set(ODA_FOUND TRUE)
    message(STATUS "Open Design Alliance SDK found: ${ODA_INCLUDE_DIR}")
    
    # Find ODA libraries
    find_library(ODA_ROOT_LIB NAMES "TD_Root" PATHS ${ODA_INCLUDE_DIR}/../Lib)
    find_library(ODA_DB_LIB NAMES "TD_Db" PATHS ${ODA_INCLUDE_DIR}/../Lib)
    find_library(ODA_GI_LIB NAMES "TD_Gi" PATHS ${ODA_INCLUDE_DIR}/../Lib)
    
    target_include_directories(${IO_MODULE_NAME} PRIVATE ${ODA_INCLUDE_DIR})
    target_link_libraries(${IO_MODULE_NAME} PRIVATE ${ODA_ROOT_LIB} ${ODA_DB_LIB} ${ODA_GI_LIB})
    target_compile_definitions(${IO_MODULE_NAME} PRIVATE QUANTUM_CANVAS_HAS_ODA=1)
else()
    message(STATUS "Open Design Alliance SDK not found - DWG/DXF support will be limited")
    target_compile_definitions(${IO_MODULE_NAME} PRIVATE QUANTUM_CANVAS_HAS_ODA=0)
endif()

# ASSIMP for 3D model formats (optional)
find_package(assimp)
if(assimp_FOUND)
    target_link_libraries(${IO_MODULE_NAME} PRIVATE assimp::assimp)
    target_compile_definitions(${IO_MODULE_NAME} PRIVATE QUANTUM_CANVAS_HAS_ASSIMP=1)
endif()

# Link external libraries
target_link_libraries(${IO_MODULE_NAME}
    PRIVATE
        PNG::PNG
        ${JPEG_LIBRARIES}
        TIFF::TIFF
        ZLIB::ZLIB
        ${WEBP_LIBRARIES}
        ${LIBRAW_LIBRARIES}
        LibXml2::LibXml2
        ${POPPLER_LIBRARIES}
        ${LCMS2_LIBRARIES}
)

# Add include directories for external libraries
target_include_directories(${IO_MODULE_NAME}
    PRIVATE
        ${JPEG_INCLUDE_DIRS}
        ${WEBP_INCLUDE_DIRS}
        ${LIBRAW_INCLUDE_DIRS}
        ${POPPLER_INCLUDE_DIRS}
        ${LCMS2_INCLUDE_DIRS}
)

# FreeImage fallback
if(FreeImage_FOUND)
    target_link_libraries(${IO_MODULE_NAME} PRIVATE ${FreeImage_LIBRARIES})
    target_include_directories(${IO_MODULE_NAME} PRIVATE ${FreeImage_INCLUDE_DIRS})
    target_compile_definitions(${IO_MODULE_NAME} PRIVATE QUANTUM_CANVAS_HAS_FREEIMAGE=1)
endif()

# OpenEXR support
if(OpenEXR_FOUND)
    target_link_libraries(${IO_MODULE_NAME} PRIVATE OpenEXR::OpenEXR)
    target_compile_definitions(${IO_MODULE_NAME} PRIVATE QUANTUM_CANVAS_HAS_OPENEXR=1)
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(${IO_MODULE_NAME} PRIVATE
        /W4                 # High warning level
        /WX                 # Treat warnings as errors
        /permissive-        # Strict conformance
        /Zc:__cplusplus     # Correct __cplusplus macro
        /fp:fast            # Fast floating point
        /arch:AVX2          # AVX2 instructions
    )
    
    # Disable specific warnings for third-party headers
    target_compile_options(${IO_MODULE_NAME} PRIVATE
        /wd4996             # Deprecated functions
        /wd4458             # Declaration hides class member
        /wd4251             # DLL interface warnings
    )
else()
    target_compile_options(${IO_MODULE_NAME} PRIVATE
        -Wall
        -Wextra
        -Werror
        -pedantic
        -march=native       # Optimize for current CPU
        -ffast-math         # Fast math optimizations
        -fno-exceptions     # Disable exceptions for performance
    )
endif()

# Platform-specific definitions
if(WIN32)
    target_compile_definitions(${IO_MODULE_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        QUANTUM_CANVAS_PLATFORM_WINDOWS=1
    )
elseif(APPLE)
    target_compile_definitions(${IO_MODULE_NAME} PRIVATE
        QUANTUM_CANVAS_PLATFORM_MACOS=1
    )
else()
    target_compile_definitions(${IO_MODULE_NAME} PRIVATE
        QUANTUM_CANVAS_PLATFORM_LINUX=1
    )
endif()

# C++ standard and features
target_compile_features(${IO_MODULE_NAME}
    PUBLIC
        cxx_std_20
    PRIVATE
        cxx_concepts
        cxx_modules
        cxx_coroutines
)

# Module-specific definitions
target_compile_definitions(${IO_MODULE_NAME}
    PRIVATE
        QUANTUM_CANVAS_IO_MODULE=1
        QUANTUM_CANVAS_VERSION_MAJOR=1
        QUANTUM_CANVAS_VERSION_MINOR=0
        QUANTUM_CANVAS_VERSION_PATCH=0
    PUBLIC
        QUANTUM_CANVAS_HAS_IO_MODULE=1
)

# Install configuration
install(TARGETS ${IO_MODULE_NAME}
    EXPORT QuantumCanvasIOTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES ${IO_HEADERS}
    DESTINATION include/quantum_canvas/io
)

# Export targets
install(EXPORT QuantumCanvasIOTargets
    FILE QuantumCanvasIOTargets.cmake
    NAMESPACE QuantumCanvas::
    DESTINATION lib/cmake/QuantumCanvas
)

# Package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/QuantumCanvasIOConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QuantumCanvasIOConfig.cmake"
    INSTALL_DESTINATION lib/cmake/QuantumCanvas
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QuantumCanvasIOConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QuantumCanvasIOConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QuantumCanvasIOConfigVersion.cmake"
    DESTINATION lib/cmake/QuantumCanvas
)

# Testing support
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Performance benchmarks
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_PROJECT_NAME "QuantumCanvas I/O Module")
        set(DOXYGEN_PROJECT_BRIEF "High-performance file format support")
        set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs/io")
        
        doxygen_add_docs(${IO_MODULE_NAME}_docs
            ${IO_SOURCES} ${IO_HEADERS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generate I/O module documentation"
        )
    endif()
endif()

# Static analysis
if(ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set_target_properties(${IO_MODULE_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*;-warnings-as-errors=*"
        )
    endif()
    
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if(CPPCHECK_EXE)
        add_custom_target(${IO_MODULE_NAME}_cppcheck
            COMMAND ${CPPCHECK_EXE}
            --enable=all
            --std=c++20
            --template=gcc
            --error-exitcode=1
            ${IO_SOURCES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Run cppcheck on I/O module"
        )
    endif()
endif()

# Memory debugging
if(ENABLE_MEMORY_DEBUGGING AND NOT WIN32)
    find_program(VALGRIND_EXE NAMES "valgrind")
    if(VALGRIND_EXE)
        add_custom_target(${IO_MODULE_NAME}_memcheck
            COMMAND ${VALGRIND_EXE}
            --tool=memcheck
            --leak-check=full
            --show-leak-kinds=all
            --track-origins=yes
            --error-exitcode=1
            $<TARGET_FILE:${IO_MODULE_NAME}_tests>
            DEPENDS ${IO_MODULE_NAME}_tests
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Run memory check on I/O module tests"
        )
    endif()
endif()

# Performance profiling
if(ENABLE_PROFILING)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${IO_MODULE_NAME} PRIVATE -pg -g3)
        target_link_options(${IO_MODULE_NAME} PRIVATE -pg)
    endif()
endif()

# Code coverage
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${IO_MODULE_NAME} PRIVATE --coverage -g -O0)
        target_link_libraries(${IO_MODULE_NAME} PRIVATE --coverage)
    endif()
endif()

# Summary
message(STATUS "QuantumCanvas I/O Module Configuration:")
message(STATUS "  - PNG support: ${PNG_FOUND}")
message(STATUS "  - JPEG support: ${JPEG_FOUND}")
message(STATUS "  - TIFF support: ${TIFF_FOUND}")
message(STATUS "  - WebP support: ${WEBP_FOUND}")
message(STATUS "  - LibRaw support: ${LIBRAW_FOUND}")
message(STATUS "  - OpenEXR support: ${OpenEXR_FOUND}")
message(STATUS "  - FreeImage fallback: ${FreeImage_FOUND}")
message(STATUS "  - ODA SDK (DWG/DXF): ${ODA_FOUND}")
message(STATUS "  - ASSIMP (3D models): ${assimp_FOUND}")
message(STATUS "  - Poppler (PDF): ${POPPLER_FOUND}")
message(STATUS "  - LCMS2 (Color): ${LCMS2_FOUND}")