# QuantumCanvas Studio - CAD Module Tests
# Comprehensive test suite for CAD Graphics Engine

cmake_minimum_required(VERSION 3.25)

# =============================================================================
# Test Configuration
# =============================================================================

# Find required testing framework
find_package(GTest REQUIRED)

# Test configuration
set(CAD_TEST_SOURCES
    test_cad_types.cpp
    test_constraint_solver.cpp
    test_precision_renderer.cpp
    test_annotation_renderer.cpp
    test_3d_kernel.cpp
    test_integration.cpp
)

# =============================================================================
# CAD Test Executable
# =============================================================================

add_executable(cad_tests ${CAD_TEST_SOURCES})

# Include directories
target_include_directories(cad_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../core
        ${EIGEN3_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(cad_tests
    PRIVATE
        qcs_cad
        qcs_core
        qcs_rendering
        Eigen3::Eigen
        GTest::GTest
        GTest::Main
)

# Compiler settings for tests
target_compile_definitions(cad_tests
    PRIVATE
        QCS_TESTING_BUILD
        QCS_CAD_ENABLE_DEBUG
)

# =============================================================================
# Test Registration
# =============================================================================

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(cad_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# =============================================================================
# Performance Tests
# =============================================================================

if(QCS_BUILD_BENCHMARKS)
    add_executable(cad_benchmarks
        benchmark_constraint_solver.cpp
        benchmark_precision_renderer.cpp
        benchmark_3d_operations.cpp
    )
    
    target_link_libraries(cad_benchmarks
        PRIVATE
            qcs_cad
            benchmark::benchmark
            benchmark::benchmark_main
    )
    
    target_include_directories(cad_benchmarks
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
endif()

# =============================================================================
# Coverage Analysis
# =============================================================================

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cad_tests PRIVATE --coverage)
    target_link_options(cad_tests PRIVATE --coverage)
    
    # Coverage target
    add_custom_target(cad_coverage
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating CAD module coverage report"
        DEPENDS cad_tests
    )
endif()

# =============================================================================
# Test Data and Resources
# =============================================================================

# Copy test data files
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data/
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_data/)

# =============================================================================
# Memory and Valgrind Tests
# =============================================================================

if(UNIX AND NOT APPLE)
    find_program(VALGRIND_PROGRAM valgrind)
    if(VALGRIND_PROGRAM)
        add_custom_target(cad_memcheck
            COMMAND ${VALGRIND_PROGRAM} --leak-check=full --show-leak-kinds=all 
                    --track-origins=yes --verbose $<TARGET_FILE:cad_tests>
            DEPENDS cad_tests
            COMMENT "Running CAD tests with Valgrind memory check"
        )
    endif()
endif()

# =============================================================================
# Custom Test Targets
# =============================================================================

# Quick tests (fast subset)
add_custom_target(cad_tests_quick
    COMMAND cad_tests --gtest_filter="*Quick*:CADTypesTest.*:ConstraintSolverTest.*"
    DEPENDS cad_tests
    COMMENT "Running quick CAD tests"
)

# Integration tests only
add_custom_target(cad_tests_integration
    COMMAND cad_tests --gtest_filter="*Integration*"
    DEPENDS cad_tests
    COMMENT "Running CAD integration tests"
)

# Stress tests
add_custom_target(cad_tests_stress
    COMMAND cad_tests --gtest_filter="*Stress*:*Performance*"
    DEPENDS cad_tests
    COMMENT "Running CAD stress tests"
)

# =============================================================================
# Test Installation
# =============================================================================

if(QCS_INSTALL_TESTS)
    install(TARGETS cad_tests
        RUNTIME DESTINATION test/cad
    )
    
    install(DIRECTORY test_data/
        DESTINATION test/cad/test_data/
    )
endif()

# =============================================================================
# Test Summary
# =============================================================================

message(STATUS "CAD Tests Configuration:")
message(STATUS "  GTest Found: ${GTEST_FOUND}")
message(STATUS "  Coverage: ${CMAKE_BUILD_TYPE STREQUAL Debug}")
message(STATUS "  Benchmarks: ${QCS_BUILD_BENCHMARKS}")
message(STATUS "  Valgrind: ${VALGRIND_PROGRAM}")