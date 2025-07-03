@echo off
REM =============================================
REM 3Drawing 项目 Windows 构建脚本（支持 Qt + vcpkg）
REM 作者：你自己
REM =============================================

setlocal enabledelayedexpansion

REM ========== 构建参数初始化 ==========
set "BUILD_TYPE=Release"
set "CLEAN_BUILD=false"
set "USE_LOCAL_QT=false"
set "LOCAL_QT_PATH=D:\QT5.15.16\5.15.2\msvc2019_64"

REM ========== 解析命令行参数 ==========
:parse_args
if "%~1"=="" goto start_build
if /i "%~1"=="/h" goto show_help
if /i "%~1"=="/help" goto show_help
if /i "%~1"=="/d" set "BUILD_TYPE=Debug"
if /i "%~1"=="/debug" set "BUILD_TYPE=Debug"
if /i "%~1"=="/c" set "CLEAN_BUILD=true"
if /i "%~1"=="/clean" set "CLEAN_BUILD=true"
if /i "%~1"=="/qt" set "USE_LOCAL_QT=true"
if /i "%~1"=="/local-qt" set "USE_LOCAL_QT=true"
shift
goto parse_args

REM ========== 帮助信息 ==========
:show_help
echo 3Drawing 构建脚本
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   /h, /help        显示帮助信息
echo   /d, /debug       构建 Debug 版本
echo   /c, /clean       清理构建目录
echo   /qt, /local-qt   使用本地 Qt（默认通过 vcpkg）
echo.
echo 示例:
echo   %~nx0                  # 构建 Release 版本
echo   %~nx0 /d               # 构建 Debug 版本
echo   %~nx0 /qt /d           # 使用本地 Qt 构建 Debug
echo   %~nx0 /c /qt /debug    # 清理并用本地 Qt 构建 Debug
exit /b 0

REM ========== 主构建流程 ==========
:start_build
echo ==========================================
echo 3Drawing 构建开始
echo BUILD_TYPE: %BUILD_TYPE%
echo USE_LOCAL_QT: %USE_LOCAL_QT%
echo ==========================================

REM 设置构建输出目录
if /i "%BUILD_TYPE%"=="Debug" (
    set "BUILD_DIR=build\debug"
) else (
    set "BUILD_DIR=build\release"
)

echo 构建目录: !BUILD_DIR!

REM 清理构建目录（如果指定）
if "%CLEAN_BUILD%"=="true" (
    echo 清理构建目录...
    if exist "!BUILD_DIR!" rmdir /s /q "!BUILD_DIR!"
)

REM 创建构建目录
if not exist build mkdir build
if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!"

REM ========== 拼接 CMake 参数 ==========
set CMAKE_ARGS=-S . -B "!BUILD_DIR!" -DCMAKE_BUILD_TYPE=!BUILD_TYPE!

REM 添加本地 Qt 路径（优先）
if "%USE_LOCAL_QT%"=="true" (
    echo 使用本地 Qt: %LOCAL_QT_PATH%
    set CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_PREFIX_PATH="%LOCAL_QT_PATH%"
)

REM 添加 vcpkg 工具链文件（如果存在）
if not "%VCPKG_ROOT%"=="" (
    echo 使用 vcpkg 工具链文件: %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
    set CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
)

REM 指定 Visual Studio 生成器
set CMAKE_ARGS=!CMAKE_ARGS! -G "Visual Studio 17 2022" -A x64

REM ========== 执行 CMake 配置 ==========
echo ==========================================
echo 配置 CMake...
echo ==========================================
cmake !CMAKE_ARGS!
if errorlevel 1 (
    echo ❌ 错误：CMake 配置失败
    exit /b 1
)

REM ========== 构建项目 ==========
echo ==========================================
echo 构建项目...
echo ==========================================
cmake --build "!BUILD_DIR!" --config !BUILD_TYPE! --parallel
if errorlevel 1 (
    echo ❌ 错误：构建失败
    exit /b 1
)

REM ========== 构建完成 ==========
echo ==========================================
echo ✅ 构建成功完成!
echo ==========================================

REM 查找生成的可执行文件
if exist "!BUILD_DIR!\!BUILD_TYPE!\3Drawing.exe" (
    echo 可执行文件: !BUILD_DIR!\!BUILD_TYPE!\3Drawing.exe
) else if exist "!BUILD_DIR!\3Drawing.exe" (
    echo 可执行文件: !BUILD_DIR!\3Drawing.exe
) else (
    echo ⚠️ 请在 !BUILD_DIR! 或其子目录中查找可执行文件
)

exit /b 0
