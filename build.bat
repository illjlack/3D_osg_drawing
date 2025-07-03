@echo off
echo 正在构建3Drawing项目...

REM 创建构建目录
if not exist "build" mkdir build
cd build

REM 配置CMake项目
cmake .. -G "Visual Studio 17 2022" -A x64

REM 构建项目
cmake --build . --config Release

echo 构建完成！
pause 