@echo off
REM 3Drawing Windows 构建脚本

setlocal enabledelayedexpansion

REM 设置颜色
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "BLUE=[94m"
set "NC=[0m"

REM 打印带颜色的消息
:print_message
echo !BLUE![3Drawing]!NC! %~1
goto :eof

:print_success
echo !GREEN![3Drawing]!NC! %~1
goto :eof

:print_warning
echo !YELLOW![3Drawing]!NC! %~1
goto :eof

:print_error
echo !RED![3Drawing]!NC! %~1
goto :eof

REM 显示帮助信息
:show_help
echo 3Drawing 构建脚本
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
echo.
echo 示例:
echo   %~nx0               # 构建Release版本
echo   %~nx0 /d            # 构建Debug版本
echo   %~nx0 /c /d         # 清理并构建Debug版本
echo   %~nx0 /i            # 构建并安装
echo   %~nx0 /p            # 构建并创建发布包
goto :eof

REM 检查依赖
:check_dependencies
call :print_message "检查构建依赖..."

REM 检查cmake
cmake --version >nul 2>&1
if errorlevel 1 (
    call :print_error "CMake 未安装，请先安装 CMake"
    exit /b 1
)

REM 检查ninja
ninja --version >nul 2>&1
if errorlevel 1 (
    call :print_warning "Ninja 未安装，将使用 Visual Studio 生成器"
    set "USE_NINJA=false"
) else (
    set "USE_NINJA=true"
)

REM 检查vcpkg
if "%VCPKG_ROOT%"=="" (
    call :print_error "VCPKG_ROOT 环境变量未设置"
    call :print_error "请设置 VCPKG_ROOT 指向您的 vcpkg 安装目录"
    exit /b 1
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    call :print_error "vcpkg 工具链文件未找到: %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
    exit /b 1
)

call :print_success "依赖检查完成"
goto :eof

REM 初始化变量
set "BUILD_TYPE=Release"
set "CLEAN_BUILD=false"
set "INSTALL_AFTER_BUILD=false"
set "CREATE_PACKAGE=false"
set "JOBS=%NUMBER_OF_PROCESSORS%"

REM 入口点 - 开始解析命令行参数
goto :parse_args

REM 解析命令行参数
:parse_args
if "%~1"=="" goto :main
if /i "%~1"=="/h" goto :help_exit
if /i "%~1"=="/help" goto :help_exit
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
call :print_error "未知选项: %~1"
goto :help_exit

:help_exit
call :show_help
exit /b 0

REM 主构建函数
:main
call :print_message "开始构建 3Drawing (%BUILD_TYPE%)"

REM 检查依赖
call :check_dependencies
if errorlevel 1 exit /b 1

set "PLATFORM=windows"
set "PRESET_NAME=windows-msvc"
set "BUILD_DIR=build\%PRESET_NAME%-%BUILD_TYPE%"

REM 清理构建目录
if "%CLEAN_BUILD%"=="true" (
    call :print_message "清理构建目录: %BUILD_DIR%"
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM 配置CMake
call :print_message "配置 CMake..."
set CMAKE_ARGS=-S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"

if "%USE_NINJA%"=="true" (
    set CMAKE_ARGS=%CMAKE_ARGS% -G Ninja
) else (
    set CMAKE_ARGS=%CMAKE_ARGS% -G "Visual Studio 16 2019" -A x64
)

cmake %CMAKE_ARGS%
if errorlevel 1 (
    call :print_error "CMake 配置失败"
    exit /b 1
)

REM 构建项目
call :print_message "构建项目..."
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -j %JOBS%
if errorlevel 1 (
    call :print_error "构建失败"
    exit /b 1
)

call :print_success "构建完成"

REM 安装
if "%INSTALL_AFTER_BUILD%"=="true" (
    call :print_message "安装项目..."
    cmake --install "%BUILD_DIR%" --config %BUILD_TYPE%
    if errorlevel 1 (
        call :print_error "安装失败"
        exit /b 1
    )
    call :print_success "安装完成"
)

REM 创建包
if "%CREATE_PACKAGE%"=="true" (
    call :print_message "创建发布包..."
    cd "%BUILD_DIR%"
    cpack
    if errorlevel 1 (
        call :print_error "创建发布包失败"
        cd ..
        exit /b 1
    )
    cd ..
    call :print_success "发布包创建完成"
)

call :print_success "所有操作完成!"
call :print_message "可执行文件位置: %BUILD_DIR%\3Drawing.exe"

exit /b 0 
