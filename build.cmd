@echo off
REM 3Drawing Windows 构建脚本 - 基于成功的test_cmake_global_qt.cmd配置

setlocal enabledelayedexpansion

REM 打印消息
:print_message
echo [3Drawing] %~1
goto :eof

:print_success
echo [3Drawing] %~1
goto :eof

:print_warning
echo [3Drawing] %~1
goto :eof

:print_error
echo [3Drawing] %~1
goto :eof

REM 显示帮助信息
:show_help
echo 3Drawing Windows 构建脚本
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   /h, /help           显示此帮助信息
echo   /d, /debug          构建Debug版本（默认为Release）
echo   /c, /clean          清理构建目录
echo   /qt, /local-qt      使用本地Qt5（默认通过vcpkg）
echo   /j, /jobs N         并行编译任务数（默认为CPU核心数）
echo.
echo 示例:
echo   %~nx0               # 构建Release版本
echo   %~nx0 /d            # 构建Debug版本
echo   %~nx0 /c /d         # 清理并构建Debug版本
echo   %~nx0 /qt /d        # 使用本地Qt5构建Debug版本
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
set "USE_LOCAL_QT=false"
set "LOCAL_QT_PATH=D:\Qt5.15\5.15.2\msvc2019_64"
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

REM 检查本地Qt5（如果指定使用）
if "%USE_LOCAL_QT%"=="true" (
    if not exist "%LOCAL_QT_PATH%" (
        call :print_error "本地Qt5未找到: %LOCAL_QT_PATH%"
        exit /b 1
    )
    call :print_message "使用本地Qt5: %LOCAL_QT_PATH%"
)

set "BUILD_DIR=build\windows-%BUILD_TYPE%"

REM 清理构建目录
if "%CLEAN_BUILD%"=="true" (
    call :print_message "清理构建目录: %BUILD_DIR%"
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM 创建构建目录
if not exist build mkdir build
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM 配置CMake
call :print_message "配置 CMake..."
set CMAKE_ARGS=-S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"

REM 添加本地Qt5路径（如果指定）
if "%USE_LOCAL_QT%"=="true" (
    set CMAKE_ARGS=!CMAKE_ARGS! -DQT5_PREFIX="%LOCAL_QT_PATH%" -DDISABLE_WINDEPLOYQT=TRUE
)

REM 指定Visual Studio生成器
set CMAKE_ARGS=!CMAKE_ARGS! -G "Visual Studio 17 2022" -A x64

cmake !CMAKE_ARGS!
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

call :print_success "构建完成！"

REM 查找生成的可执行文件
if exist "%BUILD_DIR%\%BUILD_TYPE%\3Drawing.exe" (
    call :print_message "可执行文件: %BUILD_DIR%\%BUILD_TYPE%\3Drawing.exe"
) else if exist "%BUILD_DIR%\3Drawing.exe" (
    call :print_message "可执行文件: %BUILD_DIR%\3Drawing.exe"
) else (
    call :print_warning "请在 %BUILD_DIR% 或其子目录中查找可执行文件"
)

call :print_success "所有操作完成！"
exit /b 0 
