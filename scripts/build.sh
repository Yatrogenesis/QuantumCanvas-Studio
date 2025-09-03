#!/bin/bash

# QuantumCanvas Studio Build Script
# Cross-platform build script with optimization options

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Default values
BUILD_TYPE="Release"
CLEAN_BUILD=false
RUN_TESTS=true
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
VERBOSE=false
PRESET="default"

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --debug)
                BUILD_TYPE="Debug"
                PRESET="debug"
                shift
                ;;
            --release)
                BUILD_TYPE="Release"
                PRESET="default"
                shift
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --no-tests)
                RUN_TESTS=false
                shift
                ;;
            --jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --preset)
                PRESET="$2"
                shift 2
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Show help message
show_help() {
    cat << EOF
QuantumCanvas Studio Build Script

Usage: $0 [OPTIONS]

Options:
    --debug         Build in Debug mode
    --release       Build in Release mode (default)
    --clean         Clean build directory before building
    --no-tests      Skip running tests after build
    --jobs N        Use N parallel jobs (default: auto-detected)
    --verbose       Enable verbose build output
    --preset NAME   Use specific CMake preset
    --help, -h      Show this help message

Examples:
    $0                          # Build in Release mode
    $0 --debug --verbose        # Build in Debug mode with verbose output
    $0 --clean --jobs 8         # Clean build with 8 parallel jobs
    $0 --preset debug           # Build using debug preset

Environment Variables:
    QCS_ENABLE_AI_FEATURES      Enable/disable AI features (ON/OFF)
    QCS_ENABLE_CAD_MODULE       Enable/disable CAD module (ON/OFF)
    QCS_BUILD_TESTS             Build tests (ON/OFF)
    QCS_BUILD_BENCHMARKS        Build benchmarks (ON/OFF)

EOF
}

# Check if CMake is available
check_cmake() {
    if ! command -v cmake &> /dev/null; then
        log_error "CMake is not installed or not in PATH"
        log_info "Please install CMake 3.25 or later"
        exit 1
    fi
    
    local cmake_version
    cmake_version=$(cmake --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
    local required_version="3.25.0"
    
    if ! printf '%s\n%s\n' "$required_version" "$cmake_version" | sort -V -C; then
        log_error "CMake version $cmake_version is too old. Required: $required_version or later"
        exit 1
    fi
    
    log_info "Using CMake version: $cmake_version"
}

# Check build dependencies
check_dependencies() {
    log_info "Checking build dependencies..."
    
    check_cmake
    
    # Check for build tools
    if command -v ninja &> /dev/null; then
        BUILD_GENERATOR="Ninja"
        log_info "Using Ninja build system"
    elif command -v make &> /dev/null; then
        BUILD_GENERATOR="Unix Makefiles"
        log_info "Using Make build system"
    else
        log_error "No suitable build system found (Ninja or Make required)"
        exit 1
    fi
    
    # Platform-specific checks
    case "$OSTYPE" in
        linux*)
            if ! command -v pkg-config &> /dev/null; then
                log_error "pkg-config is required but not found"
                exit 1
            fi
            ;;
        darwin*)
            if ! command -v xcodebuild &> /dev/null; then
                log_error "Xcode Command Line Tools are required"
                exit 1
            fi
            ;;
        msys*|cygwin*)
            if ! command -v cl.exe &> /dev/null && ! command -v gcc &> /dev/null; then
                log_error "No suitable compiler found (MSVC or GCC required)"
                exit 1
            fi
            ;;
    esac
    
    log_success "All dependencies satisfied"
}

# Clean build directory
clean_build() {
    if [ "$CLEAN_BUILD" = true ]; then
        log_info "Cleaning build directory..."
        rm -rf build
        rm -rf CMakeCache.txt
        rm -rf CMakeFiles
    fi
}

# Configure the build
configure_build() {
    log_info "Configuring build (Type: $BUILD_TYPE, Preset: $PRESET)..."
    
    local cmake_args=(
        "--preset" "$PRESET"
    )
    
    if [ "$VERBOSE" = true ]; then
        cmake_args+=("--" "-v")
    fi
    
    # Override build type if specified
    if [ "$BUILD_TYPE" != "Release" ]; then
        cmake_args+=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")
    fi
    
    # Apply environment variable overrides
    if [ -n "$QCS_ENABLE_AI_FEATURES" ]; then
        cmake_args+=("-DQCS_ENABLE_AI_FEATURES=$QCS_ENABLE_AI_FEATURES")
    fi
    
    if [ -n "$QCS_ENABLE_CAD_MODULE" ]; then
        cmake_args+=("-DQCS_ENABLE_CAD_MODULE=$QCS_ENABLE_CAD_MODULE")
    fi
    
    if [ -n "$QCS_BUILD_TESTS" ]; then
        cmake_args+=("-DQCS_BUILD_TESTS=$QCS_BUILD_TESTS")
    fi
    
    if [ -n "$QCS_BUILD_BENCHMARKS" ]; then
        cmake_args+=("-DQCS_BUILD_BENCHMARKS=$QCS_BUILD_BENCHMARKS")
    fi
    
    cmake "${cmake_args[@]}"
    
    log_success "Configuration completed"
}

# Build the project
build_project() {
    log_info "Building project with $PARALLEL_JOBS parallel jobs..."
    
    local build_args=(
        "--build" "build"
        "--parallel" "$PARALLEL_JOBS"
    )
    
    if [ "$VERBOSE" = true ]; then
        build_args+=("--verbose")
    fi
    
    # Start time measurement
    local start_time
    start_time=$(date +%s)
    
    cmake "${build_args[@]}"
    
    # Calculate build time
    local end_time
    end_time=$(date +%s)
    local build_time=$((end_time - start_time))
    
    log_success "Build completed in ${build_time} seconds"
}

# Run tests if enabled
run_tests() {
    if [ "$RUN_TESTS" = true ] && [ -f "build/CTestTestfile.cmake" ]; then
        log_info "Running tests..."
        
        cd build
        
        local test_args=(
            "--parallel" "$PARALLEL_JOBS"
            "--output-on-failure"
        )
        
        if [ "$VERBOSE" = true ]; then
            test_args+=("--verbose")
        fi
        
        if ctest "${test_args[@]}"; then
            log_success "All tests passed"
        else
            log_error "Some tests failed"
            return 1
        fi
        
        cd ..
    else
        log_info "Skipping tests (disabled or not available)"
    fi
}

# Generate build report
generate_report() {
    log_info "Generating build report..."
    
    local report_file="build/build_report.txt"
    
    cat > "$report_file" << EOF
QuantumCanvas Studio Build Report
================================
Generated: $(date)
Build Type: $BUILD_TYPE
Preset: $PRESET
Parallel Jobs: $PARALLEL_JOBS
Tests Run: $RUN_TESTS

Configuration:
$(cat build/CMakeCache.txt | grep -E '^QCS_|^CMAKE_BUILD_TYPE' | sort)

Build Summary:
$(ls -la build/bin/ 2>/dev/null || echo "No binaries found")

EOF
    
    if [ -f "build/Testing/Temporary/LastTest.log" ]; then
        echo -e "\nTest Results:" >> "$report_file"
        tail -20 "build/Testing/Temporary/LastTest.log" >> "$report_file"
    fi
    
    log_success "Build report saved to: $report_file"
}

# Check build artifacts
check_artifacts() {
    log_info "Checking build artifacts..."
    
    local expected_artifacts=(
        "build/bin/QuantumCanvas-Studio"
        "build/lib/"
    )
    
    for artifact in "${expected_artifacts[@]}"; do
        if [ -e "$artifact" ]; then
            log_success "Found: $artifact"
        else
            log_warning "Missing: $artifact"
        fi
    done
}

# Main build function
main() {
    log_info "Starting QuantumCanvas Studio build process..."
    
    # Parse command line arguments
    parse_args "$@"
    
    # Check dependencies
    check_dependencies
    
    # Clean build if requested
    clean_build
    
    # Configure build
    configure_build
    
    # Build project
    build_project
    
    # Run tests
    run_tests
    
    # Generate build report
    generate_report
    
    # Check artifacts
    check_artifacts
    
    log_success "Build process completed successfully!"
    log_info ""
    log_info "Build artifacts location: $(pwd)/build"
    log_info "To run the application: ./scripts/run.sh"
    log_info "To install: cmake --install build --prefix /your/install/path"
    log_info ""
    
    if [ "$BUILD_TYPE" = "Debug" ]; then
        log_info "Debug build completed. You can now:"
        log_info "  - Use debugger: gdb build/bin/QuantumCanvas-Studio"
        log_info "  - Profile with valgrind: valgrind build/bin/QuantumCanvas-Studio"
        log_info "  - Analyze with sanitizers (if enabled)"
    fi
}

# Handle script termination
cleanup() {
    local exit_code=$?
    if [ $exit_code -ne 0 ]; then
        log_error "Build failed with exit code $exit_code"
        log_info "Check the build log for details"
        log_info "Common solutions:"
        log_info "  - Run with --clean to start fresh"
        log_info "  - Check that all dependencies are installed"
        log_info "  - Try --debug for more detailed output"
    fi
}

trap cleanup EXIT

# Run main function with all arguments
main "$@"