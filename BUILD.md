# 3Drawing 跨平台构建指南

本项目已配置为使用 CMake + vcpkg 的跨平台构建系统，支持 Windows、Linux 和 macOS。

## 🚀 快速开始

### 1. 环境准备

#### 通用要求
- **CMake 3.20+**
- **vcpkg 包管理器**
- **C++17 兼容编译器**

#### Windows
- Visual Studio 2019+ 或 Build Tools for Visual Studio
- 可选：Ninja 构建系统

#### Linux
- GCC 8+ 或 Clang 10+
- 推荐：Ninja 构建系统

#### macOS
- Xcode Command Line Tools
- 推荐：Ninja 构建系统

### 2. 安装 vcpkg

如果还没有安装 vcpkg：

```bash
# 克隆 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# 构建 vcpkg
./bootstrap-vcpkg.sh  # Linux/macOS
# 或
.\bootstrap-vcpkg.bat  # Windows

# 设置环境变量
export VCPKG_ROOT=/path/to/vcpkg  # Linux/macOS
# 或
set VCPKG_ROOT=C:\path\to\vcpkg   # Windows
```

### 3. 一键构建

我们提供了便捷的构建脚本：

#### Windows
```cmd
# 构建 Release 版本
build.cmd

# 构建 Debug 版本
build.cmd /d

# 清理并构建
build.cmd /c

# 查看所有选项
build.cmd /h
```

#### Linux/macOS
```bash
# 构建 Release 版本
./build.sh

# 构建 Debug 版本  
./build.sh -d

# 清理并构建
./build.sh -c

# 查看所有选项
./build.sh -h
```

## 📦 依赖管理

项目使用 vcpkg 管理以下依赖：

- **OpenSceneGraph**: 3D 渲染引擎
- **osg-qt**: OSG 的 Qt 集成
- **Qt5**: 用户界面框架（core, gui, widgets）
- **GLM**: 数学库

依赖配置文件：
- `vcpkg.json`: 包清单文件
- `vcpkg-configuration.json`: vcpkg 配置

## ⚙️ 手动构建

如果需要更精细的控制，可以手动使用 CMake：

### 使用 CMake Presets（推荐）

```bash
# 查看可用预设
cmake --list-presets

# 配置（自动选择平台预设）
cmake --preset windows-msvc-release  # Windows
cmake --preset linux-gcc-release     # Linux  
cmake --preset macos-clang-release   # macOS

# 构建
cmake --build --preset windows-msvc-release
```

### 传统 CMake 方式

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release

# 构建项目
cmake --build . -j$(nproc)

# 安装（可选）
cmake --install . --prefix /usr/local
```

## 🎯 构建选项

### CMake 变量

| 变量 | 描述 | 默认值 |
|------|------|--------|
| `CMAKE_BUILD_TYPE` | 构建类型 | `Release` |
| `CMAKE_INSTALL_PREFIX` | 安装前缀 | 系统默认 |
| `VCPKG_TARGET_TRIPLET` | vcpkg 目标三元组 | 自动检测 |

### 构建类型

- **Debug**: 调试版本，包含调试信息
- **Release**: 发布版本，优化性能
- **RelWithDebInfo**: 带调试信息的发布版本
- **MinSizeRel**: 最小化大小的发布版本

## 🔧 IDE 集成

### Visual Studio Code
1. 安装 C++ 和 CMake 扩展
2. 设置 `VCPKG_ROOT` 环境变量
3. 使用 `Ctrl+Shift+P` → `CMake: Configure`

### Visual Studio
1. 打开文件夹或使用 "Open CMake"
2. 确保 vcpkg 已集成：`vcpkg integrate install`
3. 配置工具链文件路径

### Qt Creator
1. 打开 CMakeLists.txt
2. 配置 CMake 参数：`-DCMAKE_TOOLCHAIN_FILE=...`
3. 构建项目

### CLion
1. 打开项目根目录
2. 在 CMake 设置中添加工具链文件
3. 配置构建类型

## 📱 平台特定说明

### Windows

#### Visual Studio 集成
项目自动配置为使用 MSVC 编译器和 Windows SDK。

#### Qt 部署
构建脚本会自动运行 `windeployqt` 复制必要的 Qt DLL。

#### 创建安装包
```cmd
build.cmd /p  # 创建 NSIS 或 ZIP 安装包
```

### Linux

#### 系统依赖
某些发行版可能需要额外的系统包：

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake ninja-build

# CentOS/RHEL
sudo yum install gcc-c++ cmake ninja-build

# Arch Linux
sudo pacman -S base-devel cmake ninja
```

#### 创建软件包
```bash
./build.sh -p  # 创建 DEB 或 TGZ 包
```

### macOS

#### Xcode 集成
确保已安装 Xcode Command Line Tools：
```bash
xcode-select --install
```

#### 应用程序包
构建脚本会自动运行 `macdeployqt` 创建 .app 包。

#### 创建 DMG
```bash
./build.sh -p  # 创建 DMG 安装镜像
```

## 🐛 故障排除

### 常见问题

#### 1. "vcpkg not found"
```bash
# 确保设置了 VCPKG_ROOT 环境变量
echo $VCPKG_ROOT  # Linux/macOS
echo %VCPKG_ROOT% # Windows

# 如果未设置，添加到你的 shell 配置文件
export VCPKG_ROOT=/path/to/vcpkg  # ~/.bashrc 或 ~/.zshrc
```

#### 2. "Qt5 not found"
```bash
# 手动安装 Qt5
vcpkg install qtbase[gui,widgets]:x64-windows  # Windows
vcpkg install qtbase[gui,widgets]:x64-linux    # Linux
vcpkg install qtbase[gui,widgets]:x64-osx      # macOS
```

#### 3. "OpenSceneGraph not found"
```bash
# 安装 OSG 相关包
vcpkg install osg osg-qt
```

#### 4. 编码问题（Windows）
确保系统支持 UTF-8 或在 Visual Studio 中设置正确的代码页。

#### 5. 构建缓慢
```bash
# 使用 Ninja 代替默认生成器
cmake .. -G Ninja

# 增加并行编译任务数
cmake --build . -j8
```

### 调试构建

#### 启用详细输出
```bash
cmake --build . --verbose
# 或
make VERBOSE=1
```

#### 检查 CMake 配置
```bash
cmake .. -LA  # 列出所有变量
cmake .. -LH  # 列出带帮助的变量
```

#### vcpkg 调试
```bash
vcpkg list                    # 查看已安装包
vcpkg install --debug <pkg>   # 调试安装过程
```

## 🚀 持续集成

项目配置支持以下 CI/CD 平台：

### GitHub Actions
```yaml
- name: Setup vcpkg  
  uses: lukka/run-vcpkg@v10

- name: Build
  uses: lukka/run-cmake@v10
  with:
    configurePreset: 'linux-gcc-release'
    buildPreset: 'linux-gcc-release'
```

### 其他平台
参考 `CMakePresets.json` 中的预设配置。

## 📞 获取帮助

如果遇到构建问题：

1. 检查 [故障排除](#-故障排除) 部分
2. 查看项目 Issues
3. 检查 vcpkg 和 CMake 版本兼容性
4. 提交详细的错误日志

## 🔄 更新依赖

```bash
# 更新 vcpkg
cd $VCPKG_ROOT
git pull
./bootstrap-vcpkg.sh  # 重新构建

# 更新项目依赖
vcpkg upgrade
```

---

**祝你构建愉快！** 🎉 