#!/bin/bash

# QuantumCanvas Studio Run Script
# Launch the application with various runtime options

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Default values
DEBUG_MODE=false
PROFILING=false
VERBOSE=false
CONFIG_FILE=""
LOG_LEVEL="INFO"
WORKSPACE_PATH=""

# Parse arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --debug)
                DEBUG_MODE=true
                shift
                ;;
            --profile)
                PROFILING=true
                shift
                ;;
            --verbose)
                VERBOSE=true
                LOG_LEVEL="DEBUG"
                shift
                ;;
            --config)
                CONFIG_FILE="$2"
                shift 2
                ;;
            --workspace)
                WORKSPACE_PATH="$2"
                shift 2
                ;;
            --log-level)
                LOG_LEVEL="$2"
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

show_help() {
    cat << EOF
QuantumCanvas Studio Run Script

Usage: $0 [OPTIONS]

Options:
    --debug         Run in debug mode with additional logging
    --profile       Enable performance profiling
    --verbose       Enable verbose output
    --config FILE   Use specific configuration file
    --workspace DIR Set workspace directory
    --log-level LVL Set log level (DEBUG, INFO, WARNING, ERROR)
    --help, -h      Show this help message

Examples:
    $0                              # Run normally
    $0 --debug --verbose            # Run with debug output
    $0 --workspace ~/MyProjects     # Set custom workspace
    $0 --config ~/.qcs/config.json  # Use custom config

EOF
}

# Check if application exists
check_application() {
    local binary_path="build/bin/QuantumCanvas-Studio"
    
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        binary_path="build/bin/QuantumCanvas-Studio.exe"
    fi
    
    if [ ! -f "$binary_path" ]; then
        log_error "Application binary not found: $binary_path"
        log_info "Please run: ./scripts/build.sh first"
        exit 1
    fi
    
    BINARY_PATH="$binary_path"
    log_info "Found application: $BINARY_PATH"
}

# Setup runtime environment
setup_environment() {
    log_info "Setting up runtime environment..."
    
    # Create runtime directories
    mkdir -p ~/.quantumcanvas/{config,cache,logs,plugins,templates}
    
    # Set environment variables
    export QCS_LOG_LEVEL="$LOG_LEVEL"
    export QCS_CACHE_DIR="$HOME/.quantumcanvas/cache"
    export QCS_CONFIG_DIR="$HOME/.quantumcanvas/config"
    export QCS_PLUGINS_DIR="$HOME/.quantumcanvas/plugins"
    
    if [ -n "$WORKSPACE_PATH" ]; then
        export QCS_WORKSPACE_PATH="$WORKSPACE_PATH"
        log_info "Using workspace: $WORKSPACE_PATH"
    fi
    
    if [ -n "$CONFIG_FILE" ]; then
        export QCS_CONFIG_FILE="$CONFIG_FILE"
        log_info "Using config file: $CONFIG_FILE"
    fi
    
    # Platform-specific setup
    case "$OSTYPE" in
        linux*)
            export QCS_PLATFORM="Linux"
            # Ensure proper graphics drivers
            if command -v nvidia-smi &> /dev/null; then
                export QCS_GPU_VENDOR="NVIDIA"
            elif command -v rocm-smi &> /dev/null; then
                export QCS_GPU_VENDOR="AMD"
            else
                export QCS_GPU_VENDOR="Intel"
            fi
            ;;
        darwin*)
            export QCS_PLATFORM="macOS"
            export QCS_GPU_VENDOR="Apple"
            ;;
        msys*|cygwin*)
            export QCS_PLATFORM="Windows"
            export QCS_GPU_VENDOR="DirectX"
            ;;
    esac
    
    log_success "Runtime environment configured"
}

# Launch with debugging tools
launch_debug() {
    log_info "Launching in debug mode..."
    
    if command -v gdb &> /dev/null; then
        cat > /tmp/gdb_commands << EOF
set confirm off
set print pretty on
set pagination off
run
bt
quit
EOF
        gdb -batch -x /tmp/gdb_commands "$BINARY_PATH"
    elif command -v lldb &> /dev/null; then
        lldb -o "run" -o "bt" -o "quit" -- "$BINARY_PATH"
    else
        log_warning "No debugger found. Running normally..."
        "$BINARY_PATH"
    fi
}

# Launch with profiling
launch_profile() {
    log_info "Launching with profiling enabled..."
    
    local profile_output="profile_$(date +%Y%m%d_%H%M%S)"
    
    if command -v valgrind &> /dev/null; then
        log_info "Using Valgrind for profiling..."
        valgrind --tool=callgrind --callgrind-out-file="$profile_output.callgrind" "$BINARY_PATH"
        log_info "Profile saved to: $profile_output.callgrind"
    elif command -v perf &> /dev/null; then
        log_info "Using perf for profiling..."
        perf record -o "$profile_output.data" "$BINARY_PATH"
        log_info "Profile saved to: $profile_output.data"
        log_info "View with: perf report -i $profile_output.data"
    else
        log_warning "No profiler found. Setting internal profiling..."
        export QCS_ENABLE_PROFILING=1
        "$BINARY_PATH"
    fi
}

# Normal launch
launch_normal() {
    log_info "Launching QuantumCanvas Studio..."
    
    # Create log file with timestamp
    local log_file="$HOME/.quantumcanvas/logs/app_$(date +%Y%m%d_%H%M%S).log"
    
    if [ "$VERBOSE" = true ]; then
        # Launch with output to both terminal and log file
        "$BINARY_PATH" 2>&1 | tee "$log_file"
    else
        # Launch normally with background logging
        "$BINARY_PATH" > "$log_file" 2>&1 &
        local app_pid=$!
        log_success "Application launched (PID: $app_pid)"
        log_info "Log file: $log_file"
        log_info "To view logs: tail -f $log_file"
        
        # Wait a moment to check if app started successfully
        sleep 2
        if kill -0 $app_pid 2>/dev/null; then
            log_success "Application is running successfully"
        else
            log_error "Application failed to start. Check log file: $log_file"
            exit 1
        fi
    fi
}

# Cleanup function
cleanup() {
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        log_error "Application exited with code $exit_code"
        
        # Show recent log entries if available
        local recent_log=$(find "$HOME/.quantumcanvas/logs" -name "app_*.log" -newer /tmp -print0 2>/dev/null | xargs -0 ls -t | head -1)
        if [ -n "$recent_log" ]; then
            log_info "Recent log entries:"
            tail -20 "$recent_log"
        fi
    fi
}

# Check for updates
check_updates() {
    local version_file="build/VERSION"
    if [ -f "$version_file" ]; then
        local current_version=$(cat "$version_file")
        log_info "Current version: $current_version"
        
        # In a real implementation, this would check for updates
        # log_info "Checking for updates..."
    fi
}

# System diagnostics
run_diagnostics() {
    log_info "Running system diagnostics..."
    
    # Check system resources
    log_info "System Information:"
    echo "  OS: $(uname -s) $(uname -r)"
    echo "  Architecture: $(uname -m)"
    
    # Check memory
    if command -v free &> /dev/null; then
        echo "  Memory: $(free -h | grep Mem: | awk '{print $2 " total, " $7 " available"}')"
    elif command -v vm_stat &> /dev/null; then
        local pages_free=$(vm_stat | grep "Pages free" | awk '{print $3}' | sed 's/\.//')
        local page_size=$(vm_stat | grep "page size" | awk '{print $8}')
        local free_mb=$((pages_free * page_size / 1024 / 1024))
        echo "  Memory: ~${free_mb}MB free"
    fi
    
    # Check disk space
    echo "  Disk Space: $(df -h . | tail -1 | awk '{print $4 " free of " $2}')"
    
    # Check graphics
    if command -v glxinfo &> /dev/null; then
        local gpu_info=$(glxinfo | grep "OpenGL renderer string" | cut -d':' -f2 | xargs)
        echo "  GPU: $gpu_info"
    elif command -v system_profiler &> /dev/null; then
        local gpu_info=$(system_profiler SPDisplaysDataType | grep "Chipset Model" | cut -d':' -f2 | xargs | head -1)
        echo "  GPU: $gpu_info"
    fi
    
    log_success "Diagnostics completed"
}

# Main function
main() {
    log_info "QuantumCanvas Studio Launcher v1.0"
    log_info "=================================="
    
    parse_args "$@"
    
    # Run diagnostics in verbose mode
    if [ "$VERBOSE" = true ]; then
        run_diagnostics
        check_updates
    fi
    
    check_application
    setup_environment
    
    # Launch based on mode
    if [ "$DEBUG_MODE" = true ]; then
        launch_debug
    elif [ "$PROFILING" = true ]; then
        launch_profile
    else
        launch_normal
    fi
}

# Set cleanup trap
trap cleanup EXIT

# Run main with all arguments
main "$@"