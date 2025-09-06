# =============================================================================
# QuantumCanvas Studio - UI Module
# =============================================================================

if(NOT QCS_ENABLE_UI_MODULE)
    return()
endif()

# =============================================================================
# UI Module Configuration
# =============================================================================

# Find ImGui (assuming it's installed via vcpkg or similar)
find_package(imgui CONFIG REQUIRED)

# Platform-specific dependencies
if(QCS_PLATFORM_WINDOWS)
    # Windows-specific UI dependencies
    set(UI_PLATFORM_LIBS user32 gdi32 shell32 ole32)
elseif(QCS_PLATFORM_MACOS)
    # macOS-specific UI dependencies  
    find_library(COCOA_LIBRARY Cocoa)
    set(UI_PLATFORM_LIBS ${COCOA_LIBRARY})
elseif(QCS_PLATFORM_LINUX)
    # Linux-specific UI dependencies
    find_package(X11 REQUIRED)
    set(UI_PLATFORM_LIBS ${X11_LIBRARIES})
elseif(QCS_PLATFORM_IOS)
    # iOS-specific UI dependencies
    find_library(UIKIT_LIBRARY UIKit)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(PENCILKIT_LIBRARY PencilKit)
    find_library(APPTRACKINGTRANSPARENCY_LIBRARY AppTrackingTransparency)
    set(UI_PLATFORM_LIBS 
        ${UIKIT_LIBRARY} 
        ${METAL_LIBRARY} 
        ${METALKIT_LIBRARY} 
        ${PENCILKIT_LIBRARY} 
        ${APPTRACKINGTRANSPARENCY_LIBRARY}
    )
elseif(QCS_PLATFORM_ANDROID)
    # Android-specific UI dependencies
    find_library(ANDROID_LIBRARY android)
    find_library(LOG_LIBRARY log)
    find_library(EGL_LIBRARY EGL)
    find_library(GLES3_LIBRARY GLESv3)
    set(UI_PLATFORM_LIBS 
        ${ANDROID_LIBRARY} 
        ${LOG_LIBRARY} 
        ${EGL_LIBRARY} 
        ${GLES3_LIBRARY}
    )
endif()

# Privacy compliance dependencies
find_package(OpenSSL REQUIRED)
set(COMPLIANCE_LIBS OpenSSL::SSL OpenSSL::Crypto)

# =============================================================================
# UI Module Sources
# =============================================================================

# Window Management
set(UI_WINDOW_SOURCES
    window/window_manager.hpp
    window/window_manager.cpp
)

# Mobile Platform Window Management
if(QCS_PLATFORM_IOS)
    list(APPEND UI_WINDOW_SOURCES
        window/ios_window_manager.hpp
        window/ios_window_manager.cpp
    )
elseif(QCS_PLATFORM_ANDROID)
    list(APPEND UI_WINDOW_SOURCES
        window/android_window_manager.hpp
        window/android_window_manager.cpp
    )
endif()

# Event System
set(UI_EVENT_SOURCES  
    events/input_events.hpp
    events/input_events.cpp
)

# UI Controls
set(UI_CONTROL_SOURCES
    controls/ui_manager.hpp
    controls/ui_manager.cpp
    controls/property_panel.cpp
    controls/toolbar.cpp
    controls/canvas_panel.cpp
)

# Mobile UI Controls
if(QCS_PLATFORM_IOS OR QCS_PLATFORM_ANDROID)
    list(APPEND UI_CONTROL_SOURCES
        controls/mobile_ui_manager.hpp
        controls/mobile_ui_manager.cpp
    )
endif()

# Privacy Compliance System
set(UI_COMPLIANCE_SOURCES
    compliance/privacy_compliance_manager.hpp
    compliance/privacy_compliance_manager.cpp
    compliance/store_compliance_validator.hpp
    compliance/store_compliance_validator.cpp
)

# Layout System
set(UI_LAYOUT_SOURCES
    layout/dock_manager.hpp
    layout/dock_manager.cpp
)

# All UI sources
set(UI_MODULE_SOURCES
    ${UI_WINDOW_SOURCES}
    ${UI_EVENT_SOURCES}
    ${UI_CONTROL_SOURCES}
    ${UI_LAYOUT_SOURCES}
    ${UI_COMPLIANCE_SOURCES}
)

# =============================================================================
# UI Module Target
# =============================================================================

add_library(qcs_ui STATIC ${UI_MODULE_SOURCES})

# Target properties
set_target_properties(qcs_ui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Include directories
target_include_directories(qcs_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(qcs_ui
    PUBLIC
        imgui::imgui
        qcs_core_rendering  # Rendering engine integration
        qcs_core_kernel     # Core system integration
    PRIVATE
        ${UI_PLATFORM_LIBS}
        ${COMPLIANCE_LIBS}  # OpenSSL for privacy compliance
)

# Platform-specific compile definitions
if(QCS_PLATFORM_WINDOWS)
    target_compile_definitions(qcs_ui PRIVATE PLATFORM_WINDOWS)
    target_compile_definitions(qcs_ui PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(qcs_ui PRIVATE NOMINMAX)
elseif(QCS_PLATFORM_MACOS)
    target_compile_definitions(qcs_ui PRIVATE PLATFORM_MACOS)
elseif(QCS_PLATFORM_LINUX)
    target_compile_definitions(qcs_ui PRIVATE PLATFORM_LINUX)
elseif(QCS_PLATFORM_IOS)
    target_compile_definitions(qcs_ui PRIVATE PLATFORM_IOS)
    target_compile_definitions(qcs_ui PRIVATE QCS_IOS_APPSTORE_COMPLIANCE)
elseif(QCS_PLATFORM_ANDROID)
    target_compile_definitions(qcs_ui PRIVATE PLATFORM_ANDROID)
    target_compile_definitions(qcs_ui PRIVATE QCS_ANDROID_PLAYSTORE_COMPLIANCE)
    target_compile_definitions(qcs_ui PRIVATE QCS_ANDROID_GALAXYSTORE_COMPLIANCE)
    target_compile_definitions(qcs_ui PRIVATE QCS_ANDROID_APPGALLERY_COMPLIANCE)
endif()

# Privacy compliance definitions
target_compile_definitions(qcs_ui PRIVATE QCS_GDPR_COMPLIANCE_ENABLED)
target_compile_definitions(qcs_ui PRIVATE QCS_PRIVACY_COMPLIANCE_ENABLED)

# Compiler-specific options
if(MSVC)
    target_compile_options(qcs_ui PRIVATE /W4 /WX)
else()
    target_compile_options(qcs_ui PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# =============================================================================
# UI Module Tests
# =============================================================================

if(QCS_BUILD_TESTS)
    # UI Test sources
    set(UI_TEST_SOURCES
        tests/test_window_manager.cpp
        tests/test_ui_manager.cpp
        tests/test_input_events.cpp
    )
    
    add_executable(qcs_ui_tests ${UI_TEST_SOURCES})
    
    target_link_libraries(qcs_ui_tests
        PRIVATE
            qcs_ui
            gtest_main
    )
    
    # Register tests
    include(GoogleTest)
    gtest_discover_tests(qcs_ui_tests)
endif()

# =============================================================================
# Installation
# =============================================================================

# Install headers
install(
    DIRECTORY
        window/
        events/
        controls/
        layout/
        compliance/
    DESTINATION include/qcs/ui
    FILES_MATCHING PATTERN "*.hpp"
)

# Install library
install(TARGETS qcs_ui
    EXPORT qcs_ui_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install export targets
install(EXPORT qcs_ui_targets
    FILE qcs_ui_targets.cmake
    NAMESPACE qcs::
    DESTINATION lib/cmake/qcs
)