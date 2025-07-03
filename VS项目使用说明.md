# 3Drawing VS项目使用说明

## 项目结构

我已经将src目录下的所有代码手动添加到VS项目中，并按照以下文件夹结构组织：

### 文件夹结构
- **Main** - 主程序文件
  - `main.cpp`
- **UI** - 用户界面模块
  - `MainWindow.h/cpp` - 主窗口
  - `OSGWidget.h/cpp` - OSG渲染窗口
- **Core** - 核心功能模块
  - `Common3D.h/cpp` - 通用3D功能
  - `GeometryBase.h/cpp` - 几何基类
  - `Enums3D.h` - 枚举定义
- **Core\Geometry** - 几何对象
  - `Point3D.h/cpp` - 点
  - `Line3D.h/cpp` - 线
  - `Triangle3D.h/cpp` - 三角形
  - `Quad3D.h/cpp` - 四边形
  - `Polygon3D.h/cpp` - 多边形
  - `Box3D.h/cpp` - 盒子
  - `Cube3D.h/cpp` - 立方体
  - `Sphere3D.h/cpp` - 球体
  - `Cylinder3D.h/cpp` - 圆柱体
  - `Cone3D.h/cpp` - 圆锥体
  - `Torus3D.h/cpp` - 圆环
  - `Arc3D.h/cpp` - 圆弧
  - `BezierCurve3D.h/cpp` - 贝塞尔曲线
- **Core\Picking** - 拾取系统
  - `PickingSystem.h/cpp` - 拾取系统
  - `PickingIntegration.h/cpp` - 拾取集成
  - `PickingIndicator.h/cpp` - 拾取指示器
- **Util** - 工具模块
  - `OSGUtils.h/cpp` - OSG工具函数
  - `GeometryFactory.h/cpp` - 几何工厂
  - `IndicatorFactory.h/cpp` - 指示器工厂
  - `MathUtils.h/cpp` - 数学工具

## 配置说明

### 已配置的依赖项
1. **Qt5.15.2** - 包含core、gui、widgets、opengl模块
2. **OpenSceneGraph** - 通过vcpkg管理
3. **osgQt** - Qt与OSG的集成
4. **glm** - 数学库

### 项目设置
- 使用C++17标准
- 支持x64平台
- 包含目录已配置为vcpkg安装路径
- 启用了多处理器编译

## 使用方法

1. **打开项目**：
   - 双击 `3Drawing.vcxproj` 文件
   - 或通过VS打开解决方案

2. **编译项目**：
   - 选择Debug或Release配置
   - 按F7或点击"生成解决方案"

3. **运行项目**：
   - 按F5或点击"开始调试"
   - 或按Ctrl+F5运行不调试

## 注意事项

1. **Qt配置**：
   - 确保Qt5.15.2已正确安装
   - QtMsBuild路径可能需要根据实际安装位置调整

2. **依赖库**：
   - 确保vcpkg已安装相关依赖
   - 如果缺少依赖，请运行：`vcpkg install openscenegraph osgqt glm`

3. **路径问题**：
   - 如果编译时出现路径错误，请检查vcpkg安装路径
   - 可能需要调整项目属性中的包含目录

## 故障排除

### 常见问题
1. **找不到Qt模块**：检查Qt安装路径和版本
2. **链接错误**：确保所有依赖库都已正确安装
3. **路径错误**：检查vcpkg安装路径是否正确

### 解决方案
1. 重新安装Qt5.15.2
2. 重新安装vcpkg依赖
3. 清理并重新生成项目 