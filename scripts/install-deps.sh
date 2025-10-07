#!/bin/bash
# Install external dependencies for asyncle format library
# Usage: ./scripts/install-deps.sh [simdjson|glaze|all]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}======================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}======================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            OS=$ID
        else
            OS="linux"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
    else
        OS="unknown"
    fi
    echo "$OS"
}

install_simdjson() {
    print_header "Installing simdjson"

    local os=$(detect_os)

    case "$os" in
        ubuntu|debian)
            print_warning "Attempting to install simdjson via apt..."
            if sudo apt update && sudo apt install -y libsimdjson-dev; then
                print_success "simdjson installed via apt"
                return 0
            else
                print_warning "apt installation failed, falling back to manual build"
            fi
            ;;
        arch|manjaro)
            print_warning "Attempting to install simdjson via pacman..."
            if sudo pacman -S --noconfirm simdjson; then
                print_success "simdjson installed via pacman"
                return 0
            else
                print_warning "pacman installation failed, falling back to manual build"
            fi
            ;;
        macos)
            print_warning "Attempting to install simdjson via Homebrew..."
            if command -v brew &> /dev/null; then
                if brew install simdjson; then
                    print_success "simdjson installed via Homebrew"
                    return 0
                fi
            else
                print_error "Homebrew not found. Install from https://brew.sh"
                print_warning "Falling back to manual build"
            fi
            ;;
        *)
            print_warning "Unsupported OS for package manager, using manual build"
            ;;
    esac

    # Manual build fallback
    print_warning "Building simdjson from source..."

    local temp_dir=$(mktemp -d)
    cd "$temp_dir"

    git clone https://github.com/simdjson/simdjson.git
    cd simdjson
    git checkout v3.10.1

    mkdir build && cd build
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DSIMDJSON_JUST_LIBRARY=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local

    cmake --build . -j$(nproc 2>/dev/null || echo 4)

    if [ "$EUID" -ne 0 ]; then
        print_warning "Need sudo for installation..."
        sudo cmake --install .
    else
        cmake --install .
    fi

    cd "$PROJECT_ROOT"
    rm -rf "$temp_dir"

    print_success "simdjson built and installed"
}

install_glaze() {
    print_header "Installing Glaze"

    local os=$(detect_os)

    # Check if already installed via pkg-config
    if pkg-config --exists glaze 2>/dev/null; then
        print_success "Glaze already installed (found via pkg-config)"
        return 0
    fi

    case "$os" in
        arch|manjaro)
            print_warning "Attempting to install Glaze via pacman..."
            if sudo pacman -S --noconfirm glaze; then
                print_success "Glaze installed via pacman"
                return 0
            else
                print_warning "pacman installation failed, falling back to manual install"
            fi
            ;;
    esac

    # Header-only manual installation
    print_warning "Installing Glaze headers manually..."

    local temp_dir=$(mktemp -d)
    cd "$temp_dir"

    git clone https://github.com/stephenberry/glaze.git
    cd glaze
    git checkout v3.6.3

    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

    if [ "$EUID" -ne 0 ]; then
        print_warning "Need sudo for installation..."
        sudo cmake --install .
    else
        cmake --install .
    fi

    cd "$PROJECT_ROOT"
    rm -rf "$temp_dir"

    print_success "Glaze headers installed"
}

check_simdjson() {
    if pkg-config --exists simdjson 2>/dev/null; then
        local version=$(pkg-config --modversion simdjson)
        print_success "simdjson is installed (version $version)"
        return 0
    elif ldconfig -p 2>/dev/null | grep -q libsimdjson; then
        print_success "simdjson library found"
        return 0
    else
        print_warning "simdjson not found"
        return 1
    fi
}

check_glaze() {
    if [ -d "/usr/local/include/glaze" ] || [ -d "/usr/include/glaze" ]; then
        print_success "Glaze headers found"
        return 0
    elif pkg-config --exists glaze 2>/dev/null; then
        print_success "Glaze is installed"
        return 0
    else
        print_warning "Glaze not found"
        return 1
    fi
}

show_status() {
    print_header "Dependency Status"
    check_simdjson || true
    check_glaze || true
    echo ""
}

show_usage() {
    cat <<EOF
Usage: $0 [COMMAND]

Commands:
  simdjson    Install only simdjson
  glaze       Install only Glaze
  all         Install all dependencies (default)
  status      Show installation status
  help        Show this help message

Examples:
  $0 all              # Install both simdjson and Glaze
  $0 simdjson         # Install only simdjson
  $0 status           # Check what's installed

Environment:
  SUDO_ASKPASS        Path to sudo password helper
  CMAKE_INSTALL_PREFIX Override installation prefix (default: /usr/local)

Notes:
  - This script requires sudo for system-wide installation
  - Alternatively, use CMake FetchContent: cmake -DFORMAT_USE_FETCHCONTENT=ON
  - See docs/EXTERNAL_DEPENDENCIES.md for more options
EOF
}

main() {
    local command="${1:-all}"

    case "$command" in
        simdjson)
            install_simdjson
            ;;
        glaze)
            install_glaze
            ;;
        all)
            install_simdjson
            echo ""
            install_glaze
            echo ""
            show_status
            ;;
        status)
            show_status
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            print_error "Unknown command: $command"
            echo ""
            show_usage
            exit 1
            ;;
    esac
}

main "$@"
