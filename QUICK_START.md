# 🚀 3Drawing 快速开始

## 一分钟构建指南

### 1️⃣ 准备环境
```bash
# 设置vcpkg环境变量（必须）
export VCPKG_ROOT=/path/to/vcpkg  # Linux/macOS
set VCPKG_ROOT=C:\path\to\vcpkg   # Windows
```

### 2️⃣ 安装依赖
```bash
# 项目依赖会自动通过vcpkg安装
# 无需手动操作
```

### 3️⃣ 一键构建

#### Windows
```cmd
build.cmd        # Release版本
build.cmd /d     # Debug版本
build.cmd /h     # 查看帮助
```

#### Linux/macOS
```bash
./build.sh       # Release版本
./build.sh -d    # Debug版本  
./build.sh -h    # 查看帮助
```

### 4️⃣ 运行程序
```bash
# Windows
build\windows-msvc-release\3Drawing.exe

# Linux/macOS
build/linux-gcc-release/3Drawing
build/macos-clang-release/3Drawing
```

## 📁 项目结构

```
3Drawing/
├── src/                    # 源代码
│   ├── main.cpp           # 程序入口
│   ├── MainWindow.h/cpp   # 主窗口
│   ├── Geo3D.h/cpp        # 几何对象
│   ├── Common3D.h/cpp     # 通用功能
│   └── Enums3D.h          # 枚举定义
├── build/                 # 构建输出
├── CMakeLists.txt         # CMake配置
├── vcpkg.json            # 依赖清单
├── CMakePresets.json     # CMake预设
├── build.sh              # Linux/macOS构建脚本
├── build.cmd             # Windows构建脚本
├── BUILD.md              # 详细构建说明
└── Drawing.qrc           # Qt资源文件
```

## 🔧 IDE 配置

### Visual Studio Code
1. 安装 C++ 和 CMake 扩展
2. 打开项目文件夹
3. 按 `Ctrl+Shift+P` → `CMake: Configure`

### Visual Studio 2019/2022
1. 文件 → 打开 → 文件夹，选择项目根目录
2. CMake会自动配置（确保已设置VCPKG_ROOT）

### Qt Creator
1. 文件 → 打开文件或项目 → 选择 CMakeLists.txt
2. 配置CMake参数中添加工具链文件路径

## ❓ 常见问题

**Q: 提示找不到vcpkg？**  
A: 确保设置了 `VCPKG_ROOT` 环境变量指向vcpkg安装目录

**Q: Qt或OSG找不到？**  
A: vcpkg会自动安装依赖，首次构建可能需要较长时间

**Q: 构建失败？**  
A: 查看 [BUILD.md](BUILD.md) 的故障排除部分

## 💡 提示

- 首次构建会自动下载和编译所有依赖，请耐心等待
- 使用 `-j` 参数可以加速编译：`./build.sh -j8`
- 构建脚本支持清理、安装、打包等功能，用 `-h` 查看完整选项

📖 **详细文档**: [BUILD.md](BUILD.md) 