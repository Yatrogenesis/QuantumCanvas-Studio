# =============================================================================
# QuantumCanvas Studio - Testing Suite
# =============================================================================

# Enable testing
enable_testing()

# Find required testing dependencies
find_package(GTest REQUIRED)

# =============================================================================
# Test Configuration
# =============================================================================

# Test sources
set(COMPREHENSIVE_TEST_SOURCES
    comprehensive/comprehensive_test_suite.hpp
    comprehensive/comprehensive_test_suite.cpp
)

set(UNIT_TEST_SOURCES
    unit/test_memory_manager.cpp
    unit/test_shader_compiler.cpp
    unit/test_kernel_manager.cpp
    unit/test_rendering_engine.cpp
    unit/test_vector_renderer.cpp
    unit/test_privacy_compliance.cpp
    unit/test_mobile_platforms.cpp
)

set(INTEGRATION_TEST_SOURCES
    integration/test_cross_platform.cpp
    integration/test_mobile_desktop_parity.cpp
    integration/test_privacy_integration.cpp
    integration/test_store_compliance.cpp
)

# System validation executable
set(VALIDATION_SOURCES
    validate_system.cpp
)

# =============================================================================
# Test Library
# =============================================================================

# Create test utility library
add_library(qcs_test_utils STATIC
    ${COMPREHENSIVE_TEST_SOURCES}
)

target_include_directories(qcs_test_utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(qcs_test_utils
    PUBLIC
        qcs_core_kernel
        qcs_core_memory
        qcs_core_rendering
        qcs_ui
        GTest::gtest
        GTest::gmock
    PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
)

# Platform-specific test libraries
if(QCS_PLATFORM_IOS)
    target_link_libraries(qcs_test_utils PRIVATE
        ${UIKIT_LIBRARY}
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
    )
endif()

if(QCS_PLATFORM_ANDROID)
    target_link_libraries(qcs_test_utils PRIVATE
        ${ANDROID_LIBRARY}
        ${LOG_LIBRARY}
        ${EGL_LIBRARY}
        ${GLES3_LIBRARY}
    )
endif()

# Compiler-specific options for tests
target_compile_options(qcs_test_utils PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# Test-specific compile definitions
target_compile_definitions(qcs_test_utils PRIVATE
    QCS_TESTING_ENABLED
    $<$<CONFIG:Debug>:QCS_DEBUG_TESTING>
    $<$<BOOL:${QCS_PLATFORM_IOS}>:QCS_TEST_IOS>
    $<$<BOOL:${QCS_PLATFORM_ANDROID}>:QCS_TEST_ANDROID>
)

# =============================================================================
# Unit Tests
# =============================================================================

# Individual unit test executables
foreach(test_source ${UNIT_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    
    add_executable(${test_name} ${test_source})
    
    target_link_libraries(${test_name}
        PRIVATE
            qcs_test_utils
            GTest::gtest_main
    )
    
    # Register with CTest
    add_test(
        NAME ${test_name}
        COMMAND ${test_name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    
    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 300  # 5 minutes timeout
        LABELS "unit"
    )
endforeach()

# =============================================================================
# Integration Tests
# =============================================================================

# Integration test executables
foreach(test_source ${INTEGRATION_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    
    add_executable(${test_name} ${test_source})
    
    target_link_libraries(${test_name}
        PRIVATE
            qcs_test_utils
            GTest::gtest_main
    )
    
    # Register with CTest
    add_test(
        NAME ${test_name}
        COMMAND ${test_name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    
    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 600  # 10 minutes timeout
        LABELS "integration"
    )
endforeach()

# =============================================================================
# System Validation Executable
# =============================================================================

add_executable(validate_system ${VALIDATION_SOURCES})

target_link_libraries(validate_system
    PRIVATE
        qcs_test_utils
)

# Set validation executable properties
set_target_properties(validate_system PROPERTIES
    OUTPUT_NAME "qcs_validate"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# =============================================================================
# Performance Tests
# =============================================================================

add_executable(performance_benchmark
    performance/benchmark_memory.cpp
    performance/benchmark_rendering.cpp
    performance/benchmark_mobile.cpp
    performance/benchmark_privacy.cpp
)

target_link_libraries(performance_benchmark
    PRIVATE
        qcs_test_utils
)

add_test(
    NAME performance_benchmark
    COMMAND performance_benchmark
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(performance_benchmark PROPERTIES
    TIMEOUT 1800  # 30 minutes for performance tests
    LABELS "performance"
)

# =============================================================================
# Security Tests
# =============================================================================

add_executable(security_validation
    security/test_memory_safety.cpp
    security/test_input_validation.cpp
    security/test_encryption.cpp
    security/test_permissions.cpp
)

target_link_libraries(security_validation
    PRIVATE
        qcs_test_utils
)

add_test(
    NAME security_validation
    COMMAND security_validation
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(security_validation PROPERTIES
    TIMEOUT 900  # 15 minutes for security tests
    LABELS "security"
)

# =============================================================================
# Mobile Platform Tests
# =============================================================================

if(QCS_PLATFORM_IOS OR QCS_PLATFORM_ANDROID)
    add_executable(mobile_platform_tests
        mobile/test_ios_compliance.cpp
        mobile/test_android_compliance.cpp
        mobile/test_touch_input.cpp
        mobile/test_stylus_integration.cpp
        mobile/test_mobile_performance.cpp
    )
    
    target_link_libraries(mobile_platform_tests
        PRIVATE
            qcs_test_utils
    )
    
    add_test(
        NAME mobile_platform_tests
        COMMAND mobile_platform_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    
    set_tests_properties(mobile_platform_tests PROPERTIES
        TIMEOUT 1200  # 20 minutes for mobile tests
        LABELS "mobile"
    )
endif()

# =============================================================================
# Privacy Compliance Tests
# =============================================================================

add_executable(privacy_compliance_tests
    compliance/test_gdpr_articles.cpp
    compliance/test_data_subject_rights.cpp
    compliance/test_consent_management.cpp
    compliance/test_cross_border_transfers.cpp
    compliance/test_store_compliance.cpp
)

target_link_libraries(privacy_compliance_tests
    PRIVATE
        qcs_test_utils
)

add_test(
    NAME privacy_compliance_tests
    COMMAND privacy_compliance_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(privacy_compliance_tests PROPERTIES
    TIMEOUT 600  # 10 minutes for compliance tests
    LABELS "compliance"
)

# =============================================================================
# Test Data Setup
# =============================================================================

# Create test data directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/data)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/data/images)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/data/vectors)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/data/cad)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/data/projects)

# Copy test data files
file(GLOB TEST_DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
if(TEST_DATA_FILES)
    file(COPY ${TEST_DATA_FILES} 
         DESTINATION ${CMAKE_BINARY_DIR}/tests/data/)
endif()

# =============================================================================
# Test Targets
# =============================================================================

# Custom test targets for different test categories
add_custom_target(test_core
    COMMAND ${CMAKE_CTEST_COMMAND} -L "unit" --output-on-failure
    DEPENDS validate_system
    COMMENT "Running core unit tests"
)

add_custom_target(test_integration
    COMMAND ${CMAKE_CTEST_COMMAND} -L "integration" --output-on-failure
    DEPENDS validate_system
    COMMENT "Running integration tests"
)

add_custom_target(test_mobile
    COMMAND ${CMAKE_CTEST_COMMAND} -L "mobile" --output-on-failure
    DEPENDS validate_system
    COMMENT "Running mobile platform tests"
)

add_custom_target(test_performance
    COMMAND ${CMAKE_CTEST_COMMAND} -L "performance" --output-on-failure
    DEPENDS performance_benchmark
    COMMENT "Running performance benchmarks"
)

add_custom_target(test_security
    COMMAND ${CMAKE_CTEST_COMMAND} -L "security" --output-on-failure
    DEPENDS security_validation
    COMMENT "Running security validation"
)

add_custom_target(test_compliance
    COMMAND ${CMAKE_CTEST_COMMAND} -L "compliance" --output-on-failure
    DEPENDS privacy_compliance_tests
    COMMENT "Running privacy compliance tests"
)

# Master test target that runs everything
add_custom_target(test_all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS validate_system performance_benchmark security_validation privacy_compliance_tests
    COMMENT "Running complete test suite"
)

# Production readiness validation
add_custom_target(validate_production
    COMMAND $<TARGET_FILE:validate_system> --production --report ${CMAKE_BINARY_DIR}/production_report.md
    DEPENDS validate_system
    COMMENT "Validating production readiness"
)

# Store submission readiness validation
add_custom_target(validate_stores
    COMMAND $<TARGET_FILE:validate_system> --store-readiness --ios --android --report ${CMAKE_BINARY_DIR}/store_readiness_report.md
    DEPENDS validate_system
    COMMENT "Validating app store submission readiness"
)

# =============================================================================
# Test Reports
# =============================================================================

# Custom target to generate comprehensive test report
add_custom_target(generate_test_report
    COMMAND $<TARGET_FILE:validate_system> --all --report ${CMAKE_BINARY_DIR}/comprehensive_test_report.md
    DEPENDS validate_system
    COMMENT "Generating comprehensive test report"
)

# =============================================================================
# Continuous Integration Support
# =============================================================================

# CI-specific test configuration
if(DEFINED ENV{CI} OR DEFINED ENV{GITHUB_ACTIONS})
    # Shorter timeouts for CI
    set_tests_properties(performance_benchmark PROPERTIES TIMEOUT 600)
    set_tests_properties(security_validation PROPERTIES TIMEOUT 300)
    
    # Disable interactive tests in CI
    set_tests_properties(mobile_platform_tests PROPERTIES 
        DISABLED TRUE
        SKIP_RETURN_CODE 77
    )
endif()

# =============================================================================
# Test Installation
# =============================================================================

# Install test executables (optional)
if(QCS_INSTALL_TESTS)
    install(TARGETS validate_system
        RUNTIME DESTINATION bin/tests
    )
    
    install(TARGETS performance_benchmark security_validation privacy_compliance_tests
        RUNTIME DESTINATION bin/tests
    )
    
    # Install test data
    install(DIRECTORY ${CMAKE_BINARY_DIR}/tests/data/
        DESTINATION share/qcs/tests/data/
    )
endif()

# =============================================================================
# Test Documentation
# =============================================================================

# Generate test documentation (if Doxygen is available)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY
    )
    
    add_custom_target(test_docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating test documentation with Doxygen"
        VERBATIM
    )
endif()

message(STATUS "QuantumCanvas Studio Test Suite configured")
message(STATUS "  Unit tests: ${UNIT_TEST_SOURCES}")
message(STATUS "  Integration tests: ${INTEGRATION_TEST_SOURCES}")
message(STATUS "  Mobile testing: ${QCS_PLATFORM_IOS} ${QCS_PLATFORM_ANDROID}")
message(STATUS "  Performance testing: enabled")
message(STATUS "  Security testing: enabled")
message(STATUS "  Compliance testing: enabled")