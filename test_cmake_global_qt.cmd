@echo off
REM 3Drawing CMake 测试脚本 - 使用全局Qt5

echo [3Drawing] 开始测试 CMake 配置（使用全局Qt5）...

REM 检查环境变量
if "%VCPKG_ROOT%"=="" (
    echo [错误] VCPKG_ROOT 环境变量未设置
    echo 请设置 VCPKG_ROOT 指向您的 vcpkg 安装目录
    echo 例如: set VCPKG_ROOT=C:\vcpkg
    exit /b 1
)

echo [3Drawing] VCPKG_ROOT: %VCPKG_ROOT%

REM 检查vcpkg工具链文件
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [错误] vcpkg 工具链文件未找到
    echo 请检查 VCPKG_ROOT 路径是否正确
    exit /b 1
)

echo [3Drawing] vcpkg 工具链文件存在

REM 检查全局Qt5
set QT5_PREFIX=D:\Qt5.15\5.15.2\msvc2019_64
if not exist "%QT5_PREFIX%" (
    echo [错误] 全局Qt5未找到: %QT5_PREFIX%
    exit /b 1
)

echo [3Drawing] 全局Qt5路径: %QT5_PREFIX%

REM 创建测试构建目录
if not exist "test_build_global_qt" mkdir test_build_global_qt
cd test_build_global_qt

echo [3Drawing] 测试 CMake 配置（使用全局Qt5）...

REM 尝试配置项目
cmake .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release -DQT5_PREFIX="%QT5_PREFIX%" -DDISABLE_WINDEPLOYQT=TRUE
if errorlevel 1 (
    echo [错误] CMake 配置失败
    echo 请检查依赖是否正确安装
    cd ..
    exit /b 1
)

echo [3Drawing] CMake 配置成功！

REM 尝试生成构建文件（不实际编译）
echo [3Drawing] 测试生成构建文件...
cmake --build . --target ALL_BUILD --config Release
if errorlevel 1 (
    echo [错误] 生成构建文件失败
    cd ..
    exit /b 1
)

echo [3Drawing] 生成构建文件成功！

cd ..
echo [3Drawing] CMake 测试完成，配置正确！
echo [3Drawing] 现在可以使用 build_simple.cmd 进行完整构建 