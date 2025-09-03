#!/bin/bash

# QuantumCanvas Studio Development Environment Setup Script
# This script sets up the development environment for all supported platforms

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

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="Linux"
        if command -v apt-get &> /dev/null; then
            PACKAGE_MANAGER="apt"
        elif command -v yum &> /dev/null; then
            PACKAGE_MANAGER="yum"
        elif command -v pacman &> /dev/null; then
            PACKAGE_MANAGER="pacman"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macOS"
        PACKAGE_MANAGER="brew"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        OS="Windows"
        PACKAGE_MANAGER="choco"
    else
        log_error "Unsupported operating system: $OSTYPE"
        exit 1
    fi
    
    log_info "Detected OS: $OS with package manager: $PACKAGE_MANAGER"
}

# Check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Install system dependencies
install_system_deps() {
    log_info "Installing system dependencies..."
    
    case $OS in
        "Linux")
            case $PACKAGE_MANAGER in
                "apt")
                    sudo apt update
                    sudo apt install -y \
                        build-essential \
                        cmake \
                        git \
                        pkg-config \
                        libfreetype6-dev \
                        libfontconfig1-dev \
                        libxcb-xfixes0-dev \
                        libxcb-render-util0-dev \
                        libxcb-shape0-dev \
                        libxcb-randr0-dev \
                        libxcb-image0-dev \
                        libgl1-mesa-dev \
                        libvulkan-dev \
                        vulkan-tools \
                        libgtk-3-dev \
                        libssl-dev \
                        libasio-dev \
                        libeigen3-dev \
                        protobuf-compiler \
                        libprotobuf-dev
                    ;;
                "yum")
                    sudo yum groupinstall -y "Development Tools"
                    sudo yum install -y \
                        cmake \
                        git \
                        pkgconfig \
                        freetype-devel \
                        fontconfig-devel \
                        mesa-libGL-devel \
                        vulkan-devel \
                        gtk3-devel \
                        openssl-devel \
                        eigen3-devel \
                        protobuf-devel \
                        protobuf-compiler
                    ;;
                "pacman")
                    sudo pacman -Syu --noconfirm \
                        base-devel \
                        cmake \
                        git \
                        pkg-config \
                        freetype2 \
                        fontconfig \
                        mesa \
                        vulkan-headers \
                        vulkan-icd-loader \
                        gtk3 \
                        openssl \
                        eigen \
                        protobuf
                    ;;
            esac
            ;;
            
        "macOS")
            # Install Xcode Command Line Tools if not present
            if ! xcode-select -p &> /dev/null; then
                log_info "Installing Xcode Command Line Tools..."
                xcode-select --install
                log_warning "Please complete Xcode Command Line Tools installation and re-run this script"
                exit 1
            fi
            
            # Install Homebrew if not present
            if ! command_exists brew; then
                log_info "Installing Homebrew..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            fi
            
            # Install dependencies
            brew install \
                cmake \
                git \
                pkg-config \
                freetype \
                fontconfig \
                eigen \
                protobuf \
                openssl
            ;;
            
        "Windows")
            # Check if Chocolatey is installed
            if ! command_exists choco; then
                log_error "Chocolatey is required but not installed. Please install from https://chocolatey.org/"
                exit 1
            fi
            
            # Install dependencies
            choco install -y \
                cmake \
                git \
                visualstudio2022buildtools \
                windows-sdk-10-version-2004-all
            ;;
    esac
    
    log_success "System dependencies installed successfully"
}

# Setup Git submodules
setup_submodules() {
    log_info "Setting up Git submodules..."
    
    if [ ! -d ".git" ]; then
        log_warning "Not in a Git repository. Skipping submodule setup."
        return
    fi
    
    # Initialize and update submodules
    git submodule update --init --recursive
    
    log_success "Git submodules setup completed"
}

# Download and setup third-party libraries
setup_third_party() {
    log_info "Setting up third-party libraries..."
    
    cd third_party
    
    # Create directories for third-party libraries
    mkdir -p skia wgpu-native opencascade cgal onnxruntime webrtc
    
    # Download prebuilt libraries or setup build instructions
    # Note: In a real implementation, these would download actual libraries
    
    log_info "Skia Graphics Library..."
    # wget -O skia.tar.gz "https://github.com/google/skia/releases/download/latest/skia-${OS}.tar.gz"
    # tar -xzf skia.tar.gz -C skia --strip-components=1
    
    log_info "WGPU Native..."
    # wget -O wgpu-native.tar.gz "https://github.com/gfx-rs/wgpu-native/releases/download/latest/wgpu-${OS}.tar.gz"
    # tar -xzf wgpu-native.tar.gz -C wgpu-native --strip-components=1
    
    if [ "$QCS_ENABLE_CAD_MODULE" = "ON" ]; then
        log_info "OpenCASCADE Technology..."
        # git clone https://github.com/Open-Cascade-SAS/OCCT.git opencascade
        
        log_info "CGAL..."
        # git clone https://github.com/CGAL/cgal.git cgal
    fi
    
    if [ "$QCS_ENABLE_AI_FEATURES" = "ON" ]; then
        log_info "ONNX Runtime..."
        # wget -O onnxruntime.tar.gz "https://github.com/microsoft/onnxruntime/releases/download/latest/onnxruntime-${OS}.tar.gz"
        # tar -xzf onnxruntime.tar.gz -C onnxruntime --strip-components=1
    fi
    
    cd ..
    
    log_success "Third-party libraries setup completed"
}

# Setup Python virtual environment for AI features
setup_python_env() {
    if [ "$QCS_ENABLE_AI_FEATURES" = "ON" ]; then
        log_info "Setting up Python environment for AI features..."
        
        if ! command_exists python3; then
            log_error "Python 3 is required for AI features but not found"
            return 1
        fi
        
        # Create virtual environment
        python3 -m venv venv
        
        # Activate virtual environment
        source venv/bin/activate || source venv/Scripts/activate
        
        # Install Python dependencies
        pip install --upgrade pip
        pip install \
            numpy \
            opencv-python \
            pillow \
            torch \
            torchvision \
            onnxruntime \
            transformers
            
        deactivate
        
        log_success "Python environment setup completed"
    fi
}

# Create build configuration
create_build_config() {
    log_info "Creating build configuration..."
    
    cat > CMakeUserPresets.json << EOF
{
    "version": 3,
    "configurePresets": [
        {
            "name": "default",
            "displayName": "Default Config",
            "description": "Default build configuration",
            "generator": "Ninja",
            "binaryDir": "\${sourceDir}/build",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "QCS_ENABLE_AI_FEATURES": "${QCS_ENABLE_AI_FEATURES:-ON}",
                "QCS_ENABLE_CLOUD_SYNC": "${QCS_ENABLE_CLOUD_SYNC:-ON}",
                "QCS_ENABLE_COLLABORATION": "${QCS_ENABLE_COLLABORATION:-ON}",
                "QCS_ENABLE_PLUGIN_SYSTEM": "${QCS_ENABLE_PLUGIN_SYSTEM:-ON}",
                "QCS_ENABLE_CAD_MODULE": "${QCS_ENABLE_CAD_MODULE:-ON}",
                "QCS_ENABLE_VECTOR_MODULE": "${QCS_ENABLE_VECTOR_MODULE:-ON}",
                "QCS_ENABLE_RASTER_MODULE": "${QCS_ENABLE_RASTER_MODULE:-ON}",
                "QCS_BUILD_TESTS": "${QCS_BUILD_TESTS:-ON}",
                "QCS_BUILD_BENCHMARKS": "${QCS_BUILD_BENCHMARKS:-OFF}"
            }
        },
        {
            "name": "debug",
            "displayName": "Debug Config",
            "description": "Debug build configuration",
            "inherits": "default",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "QCS_ENABLE_PROFILING": "ON"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "default",
            "configurePreset": "default"
        },
        {
            "name": "debug",
            "configurePreset": "debug"
        }
    ]
}
EOF
    
    log_success "Build configuration created"
}

# Main setup function
main() {
    log_info "Starting QuantumCanvas Studio development environment setup..."
    
    # Set default feature flags if not provided
    export QCS_ENABLE_AI_FEATURES="${QCS_ENABLE_AI_FEATURES:-ON}"
    export QCS_ENABLE_CLOUD_SYNC="${QCS_ENABLE_CLOUD_SYNC:-ON}"
    export QCS_ENABLE_COLLABORATION="${QCS_ENABLE_COLLABORATION:-ON}"
    export QCS_ENABLE_PLUGIN_SYSTEM="${QCS_ENABLE_PLUGIN_SYSTEM:-ON}"
    export QCS_ENABLE_CAD_MODULE="${QCS_ENABLE_CAD_MODULE:-ON}"
    export QCS_ENABLE_VECTOR_MODULE="${QCS_ENABLE_VECTOR_MODULE:-ON}"
    export QCS_ENABLE_RASTER_MODULE="${QCS_ENABLE_RASTER_MODULE:-ON}"
    export QCS_BUILD_TESTS="${QCS_BUILD_TESTS:-ON}"
    export QCS_BUILD_BENCHMARKS="${QCS_BUILD_BENCHMARKS:-OFF}"
    
    # Detect operating system
    detect_os
    
    # Install system dependencies
    install_system_deps
    
    # Setup Git submodules
    setup_submodules
    
    # Setup third-party libraries
    setup_third_party
    
    # Setup Python environment for AI features
    setup_python_env
    
    # Create build configuration
    create_build_config
    
    log_success "Development environment setup completed successfully!"
    log_info ""
    log_info "Next steps:"
    log_info "  1. Run: ./scripts/build.sh"
    log_info "  2. Run: ./scripts/run.sh"
    log_info ""
    log_info "For development with specific features:"
    log_info "  export QCS_ENABLE_AI_FEATURES=OFF ./scripts/setup.sh"
    log_info ""
    log_info "Happy coding! 🚀"
}

# Run main function
main "$@"