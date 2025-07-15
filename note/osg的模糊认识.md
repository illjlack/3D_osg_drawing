尝试基于`osg`，`qt`开发了十几天。

之前使用组合，自定义对象保存几个节点（好几个`Group`, `Transform`）。

现在想来破坏了原本的场景树的组织。

而且文件保存的序列化反序列化也不好做了。



应该是可以用继承`Geometry`直接把每个对象作为一个节点。

自然的能序列化反序列化。

参数可以提供保存到渲染状态的接口。

控制点可以作为属性保存。



### 1. 节点的继承树

```cpp
osg::Referenced
└── osg::Object
    ├── osg::Node                        # 场景图节点基类
    │   ├── osg::Group                   # 普通组节点
    │   │   ├── osg::Transform           # 抽象变换基类
    │   │   │   ├── osg::MatrixTransform            # 直接使用 4×4 矩阵
    │   │   │   └── osg::PositionAttitudeTransform  # 使用位置+姿态
    │   │   ├── osg::Camera              # 相机节点（视图/投影）
    │   │   ├── osg::Switch              # 子节点显隐开关
    │   │   └── osg::OccluderNode        # 凸遮挡裁剪节点
    │   └── osg::Geode                   # 几何容器（叶节点）
    │       └── — 存放 `osg::Drawable` 列表
    ├── osg::Drawable                    # 可绘制对象基类（不继承 Node）
    │   └── osg::Geometry                # 顶点/索引几何图元
    │       ├── （其他 Geometry 子类，如 GeometryDrawable、ShapeDrawable 等）
    │       └── …  
    └── osg::NodeVisitor                 # 节点访问器（Visitor 模式）
        ├── osgUtil::UpdateVisitor       # 更新遍历
        ├── osgUtil::CullVisitor         # 裁剪 & 渲染收集
        └── osgGA::EventVisitor          # 事件分发

```

### 2.  场景树he

在OSG中存在场景树和渲染树两颗树。场景树由Node组成，这些Node可能是矩阵变换、状态切换或真正的可绘制对象，反映了场景的空间结构和对象的状态。渲染树是一颗以`StateSet`和`RenderLeaf`为节点的树。

(1)

场景树事实上是一棵 **有向无环图**（DAG）。

由 `osg::Node` 派生类组成，包括 `Group`、`Transform`、`Geode`、`Switch`、`Camera`、`OccluderNode` 等。

它反映了场景的**层次关系**（父子结构）、**空间变换**（Transform）、**渲染状态继承/覆盖**（StateSet 附着在节点上）和**可见性逻辑**（Switch、NodeMask、Occluder）。

**持久存在**于内存中，由应用程序通过 `addChild()`、`setStateSet()`、`setMatrix()` 等 API 构建和修改。

比如在几个`Transform`下挂相同的几个模型，来绘制重复场景。

(2)

**渲染树**：`CullVisitor` 根据场景树和当前摄像机/视点、裁剪规则，**每帧临时**生成的“绘制计划”，是对场景树的“精简＋重组”——把图元按 `StateSet` 和渲染顺序组织好，最终提交给 GPU。

不是持久保存的，而是在**每次渲染帧**的**Cull（裁剪）阶段**由 `osgUtil::CullVisitor` 动态生成。





