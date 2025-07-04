@echo off
setlocal enabledelayedexpansion

echo ========================================
echo 3Drawing Build Script for Windows
echo ========================================

:: 检查是否在正确的目录
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found. Please run this script from the project root directory.
    exit /b 1
)

:: 设置构建目录
set BUILD_DIR=build
set VCPKG_ROOT=%VCPKG_ROOT%

:: 检查vcpkg环境变量
if "%VCPKG_ROOT%"=="" (
    echo Warning: VCPKG_ROOT environment variable not set.
    echo Please set VCPKG_ROOT to your vcpkg installation directory.
    echo Example: set VCPKG_ROOT=C:\vcpkg
    echo.
    echo Attempting to find vcpkg in common locations...
    
    :: 尝试常见位置
    if exist "C:\vcpkg\scripts\buildsystems\vcpkg.cmake" (
        set VCPKG_ROOT=C:\vcpkg
    ) else if exist "%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake" (
        set VCPKG_ROOT=%USERPROFILE%\vcpkg
    ) else (
        echo Error: vcpkg not found. Please install vcpkg and set VCPKG_ROOT.
        exit /b 1
    )
)

echo Using vcpkg root: %VCPKG_ROOT%

:: 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

:: 配置项目
echo.
echo Configuring project...
cmake .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed.
    exit /b 1
)

:: 构建项目
echo.
echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Error: Build failed.
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Executable location: %BUILD_DIR%\bin\Release\3Drawing.exe
echo ========================================

cd .. 
