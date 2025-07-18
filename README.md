# 3Drawing - 3D绘图项目

这是一个基于Qt6和OpenSceneGraph的3D绘图应用程序。

## 项目结构

```
src/
├── main.cpp                 # 主程序入口
├── ui/                      # 用户界面模块
│   ├── MainWindow.h/cpp     # 主窗口
│   └── OSGWidget.h/cpp      # OSG渲染窗口
├── core/                    # 核心功能模块
│   ├── Common3D.h/cpp       # 通用3D功能
│   ├── GeometryBase.h/cpp   # 几何基类
│   ├── Enums3D.h           # 枚举定义
│   ├── geometry/           # 几何对象
│   │   ├── Point3D.h/cpp
│   │   ├── Line3D.h/cpp
│   │   ├── Triangle3D.h/cpp
│   │   ├── Quad3D.h/cpp
│   │   ├── Polygon3D.h/cpp
│   │   ├── Box3D.h/cpp
│   │   ├── Cube3D.h/cpp
│   │   ├── Sphere3D.h/cpp
│   │   ├── Cylinder3D.h/cpp
│   │   ├── Cone3D.h/cpp
│   │   ├── Torus3D.h/cpp
│   │   ├── Arc3D.h/cpp
│   │   └── BezierCurve3D.h/cpp
│   └── picking/            # 拾取系统
│       ├── PickingSystem.h/cpp
│       ├── PickingIntegration.h/cpp
│       └── PickingIndicator.h/cpp
└── util/                   # 工具模块
    ├── OSGUtils.h/cpp      # OSG工具函数
    ├── GeometryFactory.h/cpp # 几何工厂
    ├── IndicatorFactory.h/cpp # 指示器工厂
    └── MathUtils.h/cpp     # 数学工具
```

## 依赖项

- **Qt6**: Core, Widgets, OpenGL, OpenGLWidgets
- **OpenSceneGraph**: osg, osgDB, osgViewer, osgGA, osgUtil, osgText, osgManipulator
- **osgQt**: Qt与OSG的集成
- **glm**: 数学库

## 构建说明

### 使用CMake构建

1. **确保已安装依赖项**:
   - Qt6
   - OpenSceneGraph
   - osgQt
   - glm
   - Visual Studio 2022

2. **构建项目**:
   ```bash
   # 方法1: 使用构建脚本
   build.bat
   
   # 方法2: 手动构建
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Release
   ```

3. **在Visual Studio中打开**:
   - 打开 `build/3Drawing.sln`
   - 项目将按文件夹结构组织，便于导航

### VS文件夹结构

项目在Visual Studio中按以下结构组织：

- **3Drawing** (根项目)
  - **Main** - 主程序文件
  - **UI** - 用户界面模块
  - **Core** - 核心功能
    - **Core\Geometry** - 几何对象
    - **Core\Picking** - 拾取系统
  - **Util** - 工具模块

## 功能特性

- 3D几何对象绘制
- 实时拾取和选择
- 多种视图模式（线框、实体、点）
- 相机控制
- 高级拾取系统

## 开发说明

- 使用C++17标准
- 支持Windows平台
- 自动Qt库部署
- 调试和发布配置支持

## 功能特性

### 🎯 几何对象支持
- **点对象**: 圆形、方形、三角形、菱形、十字、星形等多种点形状
- **线对象**: 直线、圆弧、三点弧、贝塞尔曲线、流线等
- **面对象**: 三角形、四边形、多边形、圆面等
- **体对象**: 长方体、正方体、圆锥、圆柱、球体、圆环等

### 🎨 视觉效果
- **多种显示模式**: 线框、着色、着色+线框
- **材质系统**: 基础、Phong、Blinn、Lambert、PBR材质
- **可调节属性**: 颜色、透明度、光泽度、线宽等
- **实时预览**: 绘制过程中的实时预览效果

### 🔧 交互功能
- **多视角**: 俯视图、前视图、右视图、等轴测图
- **相机控制**: 旋转、缩放、平移、重置、适应窗口
- **选择编辑**: 对象选择、属性编辑、参数调节
- **工具面板**: 直观的绘制工具选择界面

## 技术架构

### 核心组件
```
src/
├── main.cpp           # 应用程序入口
├── MainWindow.h/cpp   # 主窗口界面
├── Common3D.h/cpp     # 通用定义和全局设置
├── Enums3D.h          # 枚举定义
├── Geo3D.h/cpp        # 几何对象基类和子类
```

### 依赖库
- **Qt 5.15.16**: UI界面、事件处理、国际化支持
- **OpenSceneGraph 3.6.5**: 3D渲染引擎、场景图管理
- **GLM**: 数学库，向量和矩阵运算
- **vcpkg**: 包管理工具

### 设计模式
- **工厂模式**: 几何对象创建 (`createGeo3D`)
- **观察者模式**: 事件信号机制
- **策略模式**: 不同几何对象的绘制策略
- **MVC模式**: 模型-视图-控制器分离

## 编译环境

### 环境要求
- **操作系统**: Windows 10+ / Ubuntu 18.04+ / 麒麟系统
- **编译器**: MSVC 2019+ / GCC 8+ / Clang 10+
- **Qt**: 5.15.16+
- **OpenSceneGraph**: 3.6.5+

### 使用vcpkg安装依赖
```bash
# 安装依赖包
vcpkg install osg-qt glm

# vcpkg.json配置
{
    "name": "osg-drawing",
    "version-string": "0.1.0",
    "description": "An OpenSceneGraph-based drawing app supporting customizable points, lines, surfaces and solids.",
    "dependencies": [
      "osg-qt",
      "glm"
    ]
}
```

### 编译说明
```bash
# 使用CMake编译
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
make

# 或使用Qt Creator直接打开项目文件
```

## 使用说明

### 基本操作
1. **启动应用**: 运行exe文件，等待OSG引擎初始化
2. **选择工具**: 在左侧工具面板选择绘制工具
3. **绘制对象**: 在3D视图中点击或拖拽创建几何对象
4. **编辑属性**: 在右侧属性面板调整对象属性
5. **视角控制**: 使用鼠标或快捷键切换视角

### 快捷键
- `Ctrl+N`: 新建文档
- `Ctrl+O`: 打开文档
- `Ctrl+S`: 保存文档
- `F`: 适应窗口
- `T`: 俯视图
- `1`: 前视图
- `3`: 右视图
- `7`: 等轴测图

### 文件格式
- 项目文件: `.3dd` (3D Drawing Document)
- 支持导出: STL、OBJ、PLY等常见3D格式

## 国际化支持

应用程序支持多语言界面，所有用户可见文本都使用Qt的tr()函数进行国际化处理。

### 字符编码
- 源码使用UTF-8编码
- 支持中文界面显示
- 兼容Qt 5.15+的字符处理机制

## 开发计划

### v1.1 计划功能
- [ ] 文件保存/加载功能
- [ ] 撤销/重做系统
- [ ] 复制/粘贴功能
- [ ] 更多几何对象类型
- [ ] 纹理映射支持

### v1.2 计划功能
- [ ] 动画系统
- [ ] 插件架构
- [ ] 脚本支持
- [ ] 网格优化
- [ ] 物理仿真

## 问题排查

### 常见问题
1. **编译错误**: 检查vcpkg依赖是否正确安装
2. **中文乱码**: 确保系统支持UTF-8或使用系统区域设置
3. **OSG初始化失败**: 检查显卡驱动和OpenGL支持
4. **界面响应慢**: 降低几何对象细分级别

### 日志系统
使用内置的Log3D宏进行调试输出：
```cpp
Log3D << "Debug message: " << value;
```

## 贡献指南

1. Fork 项目仓库
2. 创建功能分支
3. 提交更改并添加测试
4. 创建Pull Request

## 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件获取详细信息。

## 联系方式

- 项目维护者: [Your Name]
- 电子邮件: [your.email@example.com]
- 项目主页: [https://github.com/yourname/osg-drawing]

## 更新日志

### v1.0.0 (2024-07-02)
- ✨ 初始版本发布
- ✨ 基础几何对象绘制功能
- ✨ 属性编辑和视角控制
- ✨ 国际化支持
- ✨ OSG 3.6.5 和 Qt 5.15.16 兼容性
