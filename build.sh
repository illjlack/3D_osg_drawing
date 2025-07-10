#!/bin/bash

# 3Drawing Linux 构建脚本 - 支持Ninja构建系统

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_message() {
    echo -e "${BLUE}[3Drawing]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[3Drawing]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[3Drawing]${NC} $1"
}

print_error() {
    echo -e "${RED}[3Drawing]${NC} $1"
}

# 显示帮助信息
show_help() {
    echo "3Drawing Linux 构建脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示此帮助信息"
    echo "  -d, --debug         构建Debug版本（默认为Release）"
    echo "  -c, --clean         清理构建目录"
    echo "  -j, --jobs N        并行编译任务数（默认为CPU核心数）"
    echo ""
    echo "示例:"
    echo "  $0                  # 构建Release版本"
    echo "  $0 -d               # 构建Debug版本"
    echo "  $0 -c -d            # 清理并构建Debug版本"
    echo "  $0 -j 8             # 使用8个并行任务构建"
}

# 检查依赖
check_dependencies() {
    print_message "检查构建依赖..."
    
    # 检查cmake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake 未安装，请先安装 CMake"
        exit 1
    fi
    
    # 检查ninja
    if ! command -v ninja &> /dev/null; then
        print_warning "Ninja 未安装，将使用系统默认生成器"
        USE_NINJA=false
    else
        USE_NINJA=true
        print_message "找到 Ninja 构建系统"
    fi
    
    # 检查vcpkg
    if [ -z "$VCPKG_ROOT" ]; then
        print_error "VCPKG_ROOT 环境变量未设置"
        print_error "请设置 VCPKG_ROOT 指向您的 vcpkg 安装目录"
        exit 1
    fi
    
    if [ ! -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
        print_error "vcpkg 工具链文件未找到: $VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
        exit 1
    fi
    
    print_success "依赖检查完成"
}

# 初始化变量
BUILD_TYPE="Release"
CLEAN_BUILD=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
done

# 主构建函数
main() {
    print_message "开始构建 3Drawing ($BUILD_TYPE)"
    
    # 检查依赖
    check_dependencies
    
    # 获取操作系统信息
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="linux"
        PRESET_NAME="linux-gcc"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macos"
        PRESET_NAME="macos-clang"
    else
        print_error "不支持的操作系统: $OSTYPE"
        exit 1
    fi
    
    BUILD_DIR="build/${PLATFORM}-$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    
    # 清理构建目录
    if [ "$CLEAN_BUILD" = true ]; then
        print_message "清理构建目录: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi
    
    # 创建构建目录
    mkdir -p "$BUILD_DIR"
    
    # 配置CMake
    print_message "配置 CMake..."
    CMAKE_ARGS=(
        -S .
        -B "$BUILD_DIR"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    )
    
    # 优先使用Ninja生成器
    if [ "$USE_NINJA" = true ]; then
        CMAKE_ARGS+=(-G Ninja)
        print_message "使用 Ninja 生成器"
    else
        print_message "使用系统默认生成器"
    fi
    
    cmake "${CMAKE_ARGS[@]}"
    
    # 构建项目
    print_message "构建项目..."
    if [ "$USE_NINJA" = true ]; then
        # Ninja构建
        ninja -C "$BUILD_DIR" -j "$JOBS"
    else
        # 默认构建
        cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$JOBS"
    fi
    
    print_success "构建完成！"
    
    # 查找生成的可执行文件
    if [ -f "$BUILD_DIR/3Drawing" ]; then
        print_message "可执行文件: $BUILD_DIR/3Drawing"
    elif [ -f "$BUILD_DIR/3Drawing.exe" ]; then
        print_message "可执行文件: $BUILD_DIR/3Drawing.exe"
    else
        print_warning "请在 $BUILD_DIR 目录中查找可执行文件"
    fi
    
    print_success "所有操作完成！"
}

# 运行主函数
main "$@" 
