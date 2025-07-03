@echo off
REM 3Drawing Windows 构建脚本 - 修复版本

setlocal enabledelayedexpansion

REM 初始化变量
set "BUILD_TYPE=Release"
set "CLEAN_BUILD=false"
set "INSTALL_AFTER_BUILD=false"
set "CREATE_PACKAGE=false"
set "USE_LOCAL_QT=false"
set "LOCAL_QT_PATH=D:\QT5.15.16\5.15.2\msvc2019_64"
set "JOBS=%NUMBER_OF_PROCESSORS%"

REM 解析命令行参数
:parse_args
if "%~1"=="" goto :main

if /i "%~1"=="/h" goto :show_help
if /i "%~1"=="/help" goto :show_help

if /i "%~1"=="/d" (
    set "BUILD_TYPE=Debug"
    shift
    goto :parse_args
)
if /i "%~1"=="/debug" (
    set "BUILD_TYPE=Debug"
    shift
    goto :parse_args
)

if /i "%~1"=="/c" (
    set "CLEAN_BUILD=true"
    shift
    goto :parse_args
)
if /i "%~1"=="/clean" (
    set "CLEAN_BUILD=true"
    shift
    goto :parse_args
)

if /i "%~1"=="/i" (
    set "INSTALL_AFTER_BUILD=true"
    shift
    goto :parse_args
)
if /i "%~1"=="/install" (
    set "INSTALL_AFTER_BUILD=true"
    shift
    goto :parse_args
)

if /i "%~1"=="/p" (
    set "CREATE_PACKAGE=true"
    shift
    goto :parse_args
)
if /i "%~1"=="/package" (
    set "CREATE_PACKAGE=true"
    shift
    goto :parse_args
)

if /i "%~1"=="/j" (
    set "JOBS=%~2"
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="/jobs" (
    set "JOBS=%~2"
    shift
    shift
    goto :parse_args
)

if /i "%~1"=="/qt" (
    set "USE_LOCAL_QT=true"
    shift
    goto :parse_args
)
if /i "%~1"=="/local-qt" (
    set "USE_LOCAL_QT=true"
    shift
    goto :parse_args
)

echo [错误] 未知选项: %~1
goto :show_help

REM 显示帮助信息
:show_help
echo 3Drawing 构建脚本 - 修复版本
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   /h, /help           显示此帮助信息
echo   /d, /debug          构建Debug版本（默认为Release）
echo   /c, /clean          清理构建目录
echo   /i, /install        安装到系统
echo   /p, /package        创建发布包
echo   /j, /jobs N         并行编译任务数（默认为CPU核心数）
echo   /qt, /local-qt      使用本地Qt安装而非vcpkg
echo.
echo 示例:
echo   %~nx0               # 构建Release版本
echo   %~nx0 /d            # 构建Debug版本  
echo   %~nx0 /c /d         # 清理并构建Debug版本
echo   %~nx0 /qt /d        # 使用本地Qt构建Debug版本
echo   %~nx0 /i            # 构建并安装
echo   %~nx0 /p            # 构建并创建发布包
exit /b 0

REM 主构建函数
:main
echo [3Drawing] 开始构建 3Drawing (%BUILD_TYPE%)

REM 检查依赖
if "%USE_LOCAL_QT%"=="true" (
    call :check_dependencies_local_qt
) else (
    call :check_dependencies
)
if errorlevel 1 exit /b 1

echo [调试] 依赖检查后继续执行...

REM 设置预设名称，与CMakePresets.json匹配
if /i "%BUILD_TYPE%"=="Debug" (
    set "PRESET_NAME=windows-msvc-debug"
) else (
    set "PRESET_NAME=windows-msvc-release"
)

set "BUILD_DIR=build\%PRESET_NAME%"

echo [调试] 预设名称和构建目录设置完成...

REM 清理构建目录
if "%CLEAN_BUILD%"=="true" (
    echo [3Drawing] 清理构建目录: "%BUILD_DIR%"
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

echo [调试] 清理构建目录完成

REM 清理可能存在的CMake缓存文件
if exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [信息] 清理CMake缓存文件...
    del "%BUILD_DIR%\CMakeCache.txt"
)
if exist "%BUILD_DIR%\CMakeFiles" (
    echo [信息] 清理CMakeFiles目录...
    rmdir /s /q "%BUILD_DIR%\CMakeFiles"
)

echo [调试] 清理缓存文件完成

REM 创建构建目录
if not exist build mkdir build
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [调试] 创建构建目录完成

REM 使用CMake预设进行配置
if "%USE_LOCAL_QT%"=="true" (
    echo [3Drawing] 配置 CMake (使用本地Qt)...
    echo [警告] 使用本地Qt时跳过预设配置，直接手动配置...
    goto :manual_config
) else (
    echo [3Drawing] 配置 CMake (使用预设: %PRESET_NAME%)...
    cmake --preset %PRESET_NAME%
    if errorlevel 1 (
        echo [警告] CMake预设配置失败，尝试手动配置...
    ) else (
        goto :build_project
    )
)

:manual_config
    
    REM 备用配置方法
    set CMAKE_ARGS=-S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    
    REM 检查是否使用本地Qt
    if "%USE_LOCAL_QT%"=="true" (
        echo [信息] 使用本地Qt安装
        set CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_PREFIX_PATH="%LOCAL_QT_PATH%"
    ) else (
        REM 检查vcpkg
        if not "%VCPKG_ROOT%"=="" (
            set CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
        )
    )
    
    REM 检查ninja
    ninja --version >nul 2>&1
    if errorlevel 1 (
        echo [信息] 使用 Visual Studio 生成器
        set CMAKE_ARGS=!CMAKE_ARGS! -G "Visual Studio 17 2022" -A x64
    ) else (
        echo [信息] 使用 Ninja 生成器
        set CMAKE_ARGS=!CMAKE_ARGS! -G Ninja
    )
    
    cmake !CMAKE_ARGS!
    if errorlevel 1 (
        echo [错误] 手动配置也失败
        exit /b 1
    )

:build_project
REM 构建项目
echo [3Drawing] 构建项目...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel %JOBS%
if errorlevel 1 (
    echo [错误] 构建失败
    exit /b 1
)

echo [成功] 构建完成

REM 安装
if "%INSTALL_AFTER_BUILD%"=="true" (
    echo [3Drawing] 安装项目...
    cmake --install "%BUILD_DIR%" --config %BUILD_TYPE%
    if errorlevel 1 (
        echo [错误] 安装失败
        exit /b 1
    )
    echo [成功] 安装完成
)

REM 创建包
if "%CREATE_PACKAGE%"=="true" (
    echo [3Drawing] 创建发布包...
    pushd "%BUILD_DIR%"
    cpack
    if errorlevel 1 (
        echo [错误] 创建发布包失败
        popd
        exit /b 1
    )
    popd
    echo [成功] 发布包创建完成
)

echo [成功] 所有操作完成!
echo [信息] 构建目录: %BUILD_DIR%

REM 尝试找到生成的可执行文件
if exist "%BUILD_DIR%\%BUILD_TYPE%\3Drawing.exe" (
    echo [信息] 可执行文件位置: %BUILD_DIR%\%BUILD_TYPE%\3Drawing.exe
) else if exist "%BUILD_DIR%\3Drawing.exe" (
    echo [信息] 可执行文件位置: %BUILD_DIR%\3Drawing.exe
) else (
    echo [信息] 请在 %BUILD_DIR% 目录中查找生成的可执行文件
)

exit /b 0

REM 检查依赖
:check_dependencies
echo [3Drawing] 检查构建依赖...

REM 检查cmake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [错误] CMake 未安装，请先安装 CMake
    echo [信息] 请访问 https://cmake.org/download/ 下载安装
    exit /b 1
)

REM 检查vcpkg (可选)
if "%VCPKG_ROOT%"=="" (
    echo [警告] VCPKG_ROOT 环境变量未设置
    echo [信息] 如果项目依赖vcpkg，请设置 VCPKG_ROOT 环境变量
) else (
    if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        echo [警告] vcpkg 工具链文件未找到: %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
    ) else (
        echo [信息] 找到 vcpkg: %VCPKG_ROOT%
    )
)

echo [成功] 依赖检查完成
goto :eof

REM 检查依赖（本地Qt模式）
:check_dependencies_local_qt
echo [3Drawing] 检查构建依赖（本地Qt模式）...

echo [调试] 跳过CMake检查（测试）...

echo [调试] 开始检查Qt路径...
REM 检查本地Qt安装
if not exist "%LOCAL_QT_PATH%" (
    echo [错误] 本地Qt路径不存在
    echo [信息] 请检查Qt安装路径或使用vcpkg模式
    exit /b 1
)
echo [调试] Qt路径存在

echo [调试] 开始检查qmake...
if not exist "%LOCAL_QT_PATH%\bin\qmake.exe" (
    echo [错误] Qt qmake 未找到
    echo [信息] 请检查Qt安装是否完整
    exit /b 1
)
echo [调试] qmake检查通过

echo [信息] 找到 Qt5 安装
echo [信息] 跳过 vcpkg 检查（使用本地Qt模式）
echo [成功] 依赖检查完成（本地Qt模式）
goto :eof 