# QuantumCanvas Studio - CAD Module CMakeLists.txt
# Module B3: CAD Graphics Engine
# High-precision rendering and geometric constraint solving

cmake_minimum_required(VERSION 3.25)

# =============================================================================
# CAD Module Configuration
# =============================================================================

set(QCS_CAD_MODULE_NAME "qcs_cad")

# CAD-specific compile definitions
add_compile_definitions(
    QCS_CAD_MODULE_ENABLED
    QCS_PRECISION_DOUBLE
    QCS_CAD_ENTERPRISE_FEATURES
)

# =============================================================================
# Dependencies
# =============================================================================

# Core dependencies
find_package(Eigen3 REQUIRED)

# OpenCASCADE Technology (3D kernel)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/opencascade")
    set(OpenCASCADE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/opencascade")
    find_package(OpenCASCADE REQUIRED)
    add_compile_definitions(QCS_OPENCASCADE_ENABLED)
endif()

# CGAL (Computational Geometry)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/cgal")
    set(CGAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/cgal")
    find_package(CGAL REQUIRED)
    add_compile_definitions(QCS_CGAL_ENABLED)
endif()

# =============================================================================
# Source Files
# =============================================================================

set(CAD_HEADERS
    # Core CAD interfaces
    cad_types.hpp
    cad_common.hpp
    
    # Precision Renderer
    precision_renderer.hpp
    precision_renderer_impl.hpp
    
    # Constraint Solver
    constraint_solver.hpp
    constraint_types.hpp
    geometric_constraints.hpp
    solver_engine.hpp
    
    # Annotation Renderer  
    annotation_renderer.hpp
    annotation_types.hpp
    dimension_renderer.hpp
    symbol_renderer.hpp
    
    # 3D Kernel
    3d_kernel.hpp
    solid_modeling.hpp
    surface_modeling.hpp
    geometry_engine.hpp
    
    # Utility headers
    math_utils.hpp
    precision_math.hpp
    geometric_algorithms.hpp
)

set(CAD_SOURCES
    # Precision Renderer implementation
    precision_renderer.cpp
    precision_renderer_impl.cpp
    
    # Constraint Solver implementation
    constraint_solver.cpp
    geometric_constraints.cpp
    solver_engine.cpp
    
    # Annotation Renderer implementation
    annotation_renderer.cpp
    dimension_renderer.cpp
    symbol_renderer.cpp
    
    # 3D Kernel implementation
    3d_kernel.cpp
    solid_modeling.cpp
    surface_modeling.cpp
    geometry_engine.cpp
    
    # Utility implementations
    math_utils.cpp
    precision_math.cpp
    geometric_algorithms.cpp
)

# =============================================================================
# CAD Module Library
# =============================================================================

add_library(${QCS_CAD_MODULE_NAME} STATIC ${CAD_HEADERS} ${CAD_SOURCES})

# Include directories
target_include_directories(${QCS_CAD_MODULE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/core
        ${EIGEN3_INCLUDE_DIR}
)

# Link dependencies
target_link_libraries(${QCS_CAD_MODULE_NAME}
    PUBLIC
        qcs_core
        qcs_rendering
        Eigen3::Eigen
    PRIVATE
        qcs_math
)

# OpenCASCADE linking
if(TARGET OpenCASCADE::Foundation)
    target_link_libraries(${QCS_CAD_MODULE_NAME}
        PRIVATE
            OpenCASCADE::Foundation
            OpenCASCADE::Modeling
            OpenCASCADE::Visualization
            OpenCASCADE::DataExchange
    )
endif()

# CGAL linking
if(TARGET CGAL::CGAL)
    target_link_libraries(${QCS_CAD_MODULE_NAME}
        PRIVATE
            CGAL::CGAL
            CGAL::CGAL_Core
    )
endif()

# =============================================================================
# Compiler Configuration
# =============================================================================

# CAD requires high precision arithmetic
target_compile_definitions(${QCS_CAD_MODULE_NAME}
    PRIVATE
        QCS_CAD_HIGH_PRECISION
        QCS_GEOMETRIC_TOLERANCE=1e-12
        QCS_ANGULAR_TOLERANCE=1e-10
)

# Enhanced floating-point precision
if(MSVC)
    target_compile_options(${QCS_CAD_MODULE_NAME} PRIVATE /fp:precise)
else()
    target_compile_options(${QCS_CAD_MODULE_NAME} PRIVATE -ffast-math)
endif()

# =============================================================================
# Installation
# =============================================================================

install(TARGETS ${QCS_CAD_MODULE_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${CAD_HEADERS}
    DESTINATION include/quantumcanvas/cad
)

# =============================================================================
# Testing
# =============================================================================

if(QCS_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# =============================================================================
# Benchmarks
# =============================================================================

if(QCS_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# =============================================================================
# Documentation
# =============================================================================

if(DOXYGEN_FOUND)
    set(DOXYGEN_CAD_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.cad)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in 
        ${DOXYGEN_CAD_CONFIG} 
        @ONLY
    )
    
    add_custom_target(docs_cad
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CAD_CONFIG}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating CAD module documentation"
    )
endif()

# =============================================================================
# Status Information
# =============================================================================

message(STATUS "CAD Module Configuration:")
message(STATUS "  OpenCASCADE: ${OpenCASCADE_FOUND}")
message(STATUS "  CGAL: ${CGAL_FOUND}")
message(STATUS "  Precision: Double (1e-12 tolerance)")
message(STATUS "  Enterprise Features: Enabled")