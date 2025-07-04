#!/bin/bash

echo "========================================"
echo "3Drawing Build Script for Linux/macOS"
echo "========================================"

# 检查是否在正确的目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# 设置构建目录
BUILD_DIR="build"
VCPKG_ROOT="${VCPKG_ROOT}"

# 检查vcpkg环境变量
if [ -z "$VCPKG_ROOT" ]; then
    echo "Warning: VCPKG_ROOT environment variable not set."
    echo "Please set VCPKG_ROOT to your vcpkg installation directory."
    echo "Example: export VCPKG_ROOT=/path/to/vcpkg"
    echo ""
    echo "Attempting to find vcpkg in common locations..."
    
    # 尝试常见位置
    if [ -f "/usr/local/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
        VCPKG_ROOT="/usr/local/vcpkg"
    elif [ -f "$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
        VCPKG_ROOT="$HOME/vcpkg"
    else
        echo "Error: vcpkg not found. Please install vcpkg and set VCPKG_ROOT."
        exit 1
    fi
fi

echo "Using vcpkg root: $VCPKG_ROOT"

# 创建构建目录
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi
cd "$BUILD_DIR"

# 配置项目
echo ""
echo "Configuring project..."
cmake .. -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed."
    exit 1
fi

# 构建项目
echo ""
echo "Building project..."
cmake --build . --config Release

if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

echo ""
echo "========================================"
echo "Build completed successfully!"
echo "Executable location: $BUILD_DIR/bin/3Drawing"
echo "========================================"

cd .. 