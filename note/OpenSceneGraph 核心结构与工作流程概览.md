# OpenSceneGraph 场景图与渲染流程综述

## 场景图结构组织

> **图 1：OpenSceneGraph 场景图示例**——一个 Box 几何体节点同时作为两个变换组节点的子节点，因此经过两种不同的变换后在场景中绘制出两个 Box，但 Box 在内存中只保留了一份数据[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=挂在 组 节点下的子节点会执行相同的操作（如：平移，旋转，放缩等），)。这体现了 OSG 场景图的 DAG 结构可重用子节点的特性。

### 有向无环图 (DAG)

OSG 的场景图本质上是一个有向无环图结构，允许多个父节点共享同一个子节点，没有循环引用[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=2,，可以多个父节点共享一个子节点。 Image 节点主要分三类：根节点，枝节节点，叶子节点。OSG使用Node类表达一个基本节点，也是所有类型场景节点的基类。)。这意味着一个子场景可被插入到场景树的多个位置，从而在场景中重复利用而无需复制数据[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=挂在 组 节点下的子节点会执行相同的操作（如：平移，旋转，放缩等），)。例如上图所示，一个几何体节点可被两个不同的变换节点引用，经过各自变换后在场景中出现多次，但底层几何数据仅存储一份。

### 节点类型层次

所有场景中的元素皆为 `osg::Node` 派生类[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=2,，可以多个父节点共享一个子节点。 Image 节点主要分三类：根节点，枝节节点，叶子节点。OSG使用Node类表达一个基本节点，也是所有类型场景节点的基类。)。一般将节点分为三类：

- **根节点**：场景图的起点
- **组节点 (Group)**：内部枝干节点
- **叶节点 (Geode)**[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=2,，可以多个父节点共享一个子节点。 Image 节点主要分三类：根节点，枝节节点，叶子节点。OSG使用Node类表达一个基本节点，也是所有类型场景节点的基类。)

组节点可以拥有多个子节点，用于分组组织场景；叶节点 `osg::Geode` 是场景图的末端，不再有子节点，其作用是持有一个或多个可绘制对象 `osg::Drawable`（如几何体、模型等）[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=3)。换言之，Geode 是 **"几何体节点"**，通过包含的 Drawable 来承载实际绘制的数据。Geode 没有子节点，但允许有一个或多个父节点引用它[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=3)（符合 DAG 的共享特性）。

### 变换与继承

组节点通常用来应用空间变换或其他逻辑操作，其对子节点的影响会递归传播给所有后代节点。例如，将平移/旋转/缩放变换设置在一个组节点上，则该组节点下的所有几何体都会执行该变换[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=其中，叶子节点（osg%3A%3AGeode）是模型数据节点（存放数据的节点），中间节点（或称枝节节点）为组节点（osg%3A%3AGroup）。)。当组节点嵌套时，子节点受到的变换是从根节点到该子节点路径上所有变换的综合结果[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=其中，叶子节点（osg%3A%3AGeode）是模型数据节点（存放数据的节点），中间节点（或称枝节节点）为组节点（osg%3A%3AGroup）。)。由于场景图是DAG结构，共享的子节点可以被多个上级变换节点作用：每个上级会对该子节点施加各自的变换，使该子节点的内容在不同位置/姿态下重复出现，但并不产生数据的冗余拷贝[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=挂在 组 节点下的子节点会执行相同的操作（如：平移，旋转，放缩等），)。

### 包围体层次 (BVH)

为加速场景管理和裁剪，OSG 在场景图中使用包围体层次结构[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=1,，可以多个父节点共享一个子节点。 Image 节点主要分三类：根节点，枝节节点，叶子节点。OSG使用Node类表达一个基本节点，也是所有类型场景节点的基类。)。每个 `osg::Node` 维护一个简单的包围体（通常为**包围球**，某些情况下为**包围盒**），能够包裹住该节点及其所有子孙节点的空间范围[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=1,，可以多个父节点共享一个子节点。 Image 节点主要分三类：根节点，枝节节点，叶子节点。OSG使用Node类表达一个基本节点，也是所有类型场景节点的基类。)。这些包围体会在场景更新时自动计算或更新。利用包围体，OSG 可以在视锥裁剪等过程中快速判断整个子树是否可能可见——若某节点的包围球完全落在视锥体外，则整个子树都可以被高效剔除而无需逐个检查其子节点。

### 扩展的场景节点

除了基本的 Group/Geode，OSG 提供了许多从 `osg::Group` 派生的特殊节点类型，以实现各种场景图功能[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=根据不同的用途，有各种不同的组结点，它们都继承自osg%3A%3AGroup。)。例如：

- **距离LOD控制节点** `osg::LOD`：根据视距自动选择不同细节级别的子节点渲染[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=如 osg%3A%3ALOD，可以根据距离远近等因素选择不同的子结点渲染。)
- **开关节点** `osg::Switch`：在多个子节点中按需激活一个显示[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=如 osg%3A%3ALOD，可以根据距离远近等因素选择不同的子结点渲染。)
- **序列节点** `osg::Sequence`：顺序播放子节点以实现动画效果[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=如 osg%3A%3ASwitch，可以在两个子结点中任选其一。)
- **变换节点** `osg::Transform`：对其子节点坐标系进行平移、旋转、缩放变换[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=如 osg%3A%3ASequence，可以构建序列动画。)

常用的变换节点如 `osg::MatrixTransform` 和 `osg::PositionAttitudeTransform` 就是 `osg::Transform` 的具体实例[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=如 osg%3A%3ATransform，改变其所有子结点相对于场景中其它结点的坐标，可以是旋转、平移或缩放等。)。此外，`osg::Camera` 也是一种特殊的变换节点，用于定义渲染视角[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=我们常用 osg%3A%3AMatrixTransform 和 osg%3A%3APositionAttitudeTransform。)。这些丰富的节点类型使场景图能够以模块化方式表达各种场景逻辑。

## OpenGL 状态系统

> **图 2：场景图中状态集（StateSet）的应用示意**——左侧"卡车"模型的 Drawable 附加了启用线框模式的 StateSet（将多边形模式设置为线框），因此渲染为线框效果；右侧"道路"节点的 Geode 附加了关闭光照的 StateSet，使道路在渲染时不受光照影响呈现恒定明暗[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=那么在 OSG 中是如何改变状态的呢？首先 OSG 将状态分成,StateSet，要注意它只是 WebGL 全部状态的子集，需要依附场景图中的节点而存在。例如下图中开启 wireframe 的叶节点：)。这些状态集随着场景图层次向下继承，其设置会影响各自子节点的渲染状态。

### StateSet 概念

OSG 使用 **状态集**（`osg::StateSet`）来管理绘制状态。StateSet 本质上是对 OpenGL **状态集合** 的抽象，包括两类状态：

1. **模式 (Mode)**：可以通过 `glEnable/glDisable` 开关的状态（例如光照开启/关闭、深度测试开启/关闭等）
2. **属性 (Attribute)**：以对象封装的渲染属性（如材质、光源、纹理、着色器等）[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=那么在 OSG 中是如何改变状态的呢？首先 OSG 将状态分成,StateSet，要注意它只是 WebGL 全部状态的子集，需要依附场景图中的节点而存在。例如下图中开启 wireframe 的叶节点：)

每个 StateSet 都可以附加在场景图的 **Node 或 Drawable** 上，表示对该节点（及其所有子节点）应用的渲染状态配置[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=那么在 OSG 中是如何改变状态的呢？首先 OSG 将状态分成,StateSet，要注意它只是 WebGL 全部状态的子集，需要依附场景图中的节点而存在。例如下图中开启 wireframe 的叶节点：)。需要注意，StateSet 仅涵盖了渲染过程中常用的一部分 OpenGL/WebGL 状态，并通过更高级的抽象（如 Fog、Light、Texture、Shader 等）提供接口[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=Image)。通过在节点上附加不同的 StateSet，我们可以控制该子树的绘制效果（如开启/关闭某些渲染效果、设置特定材质或着色器等）。

### 状态继承机制

由于场景图的层次结构，StateSet 的影响是**向下继承**的。父节点附加的状态集会作用到其所有子节点，除非子节点显式改变了某项状态[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=状态的继承)。这种机制允许我们在场景的高层次节点设置**全局或默认状态**，而在局部的子节点上**覆写**部分状态以获得特殊效果[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=状态的继承)。例如，可以在场景根节点的 StateSet 中统一关闭光照，这将使默认情况下整个场景都不受光照影响；然后在某些需要光照的子节点上附加自己的 StateSet 来重新开启光照，从而局部覆盖父节点的设置。

### OVERRIDE 与 PROTECTED

为了精细控制状态继承的优先级，OSG 提供了 **覆盖 (OVERRIDE)** 和 **保护 (PROTECTED)** 两个标志位机制。当在 StateSet 中设置状态时，可以按位或方式启用这些标志[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=rootSS)。

- **OVERRIDE**：表示此状态设置应覆盖下层所有节点的同类状态，即使子节点试图修改也将被忽略[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=rootSS)
- **PROTECTED**：表示该状态设置不受上层 Override 的影响，保护子节点自己的设置[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=rootSS)

通过组合这两个标志，可以实现类似 CSS "!important" 样式那样的父子状态优先级控制。例如，父节点可以将光照模式设置为关闭并加上 OVERRIDE，强制所有子节点都维持光照关闭[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=rootSS)；某子节点若想无视此限制，可在自身 StateSet 中将光照开启且加上 PROTECTED，以保护其开启光照的状态不被父节点覆盖[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=一个完整的重载机制一定需要包含对于父节点和子节点优先级的限制，类似 CSS 的重载机制也需要 ,在 OSG 中是通过 Mask 位运算实现的：)[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=rootSS)。OSG 内部通过位掩码计算来决定当父节点的 OVERRIDE 与子节点的 PROTECTED 冲突时的处理逻辑，确保状态继承的规则明确且一致。

### 状态组合与 StateGraph

在实际渲染时，场景图中分散于各节点的状态集会被收集、合并成一棵 **状态图 (StateGraph)** 结构，以优化批处理绘制[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=OSG中所有的Drawable几何体对象都会自动关联一个StateSet对象，无论用户是否在自己的程序中作了设置。进入渲染后台之后，OSG将为场景生成"状态树 "，它是由"状态节点"StateGraph和"渲染叶"RenderLeaf所组成的。)。状态图是根据状态集合来组织的树：每个节点代表一种特定的状态组合（对应场景图某处的累计状态），叶节点则挂载实际需要绘制的图元（Drawable），称为 **渲染叶 (RenderLeaf)**[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=OSG中所有的Drawable几何体对象都会自动关联一个StateSet对象，无论用户是否在自己的程序中作了设置。进入渲染后台之后，OSG将为场景生成"状态树 "，它是由"状态节点"StateGraph和"渲染叶"RenderLeaf所组成的。)。

构建状态图时，会首先在根部自动插入一个 **全局状态节点**（`_globalStateSet`），其内容是场景主摄像机的全局 StateSet[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=图中的"状态根节点"和"局部状态节点"都是由状态树自动生成的，其中后者的主要工作是保存和维护一些渲染后台自动创建的渲染属性；而"全局状态节点"则保存一个名为_g lobalStateSet的渲染状态集对象，它的取值是场景主相机的StateSet，换句话说，任何对状态树的遍历都将首先至场景主相机的渲染状态，然后才是各个节点 的渲染状态，这就是_globalStateSet的功能所在。)。也就是说，每次渲染遍历都会以相机的初始状态开始，然后在此基础上叠加场景各节点的状态集设置[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=图中的"状态根节点"和"局部状态节点"都是由状态树自动生成的，其中后者的主要工作是保存和维护一些渲染后台自动创建的渲染属性；而"全局状态节点"则保存一个名为_g lobalStateSet的渲染状态集对象，它的取值是场景主相机的StateSet，换句话说，任何对状态树的遍历都将首先至场景主相机的渲染状态，然后才是各个节点 的渲染状态，这就是_globalStateSet的功能所在。)。

随着 Cull遍历深入场景图，遇到每个有 StateSet 的节点就向状态图插入相应的新节点（或进入已存在的相同状态分支），再遇到 Drawable 时在状态图当前分支下创建一个 RenderLeaf 叶子节点[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/20/OSG-学习笔记-二.html#:~:text=%2F%2F Camera and lights must,return%3B)[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=1、状态树是根据渲染状态（stateset）来生成的，那些没有设置StateSet的场景节点将不会影响状态树的构架。)。如果场景中多个 Drawable **共享完全相同**的一组渲染状态组合，它们将在状态图中对应到同一个状态节点下，从而共享一个 RenderLeaf 列表进行绘制[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=1、状态树是根据渲染状态（stateset）来生成的，那些没有设置StateSet的场景节点将不会影响状态树的构架。)。通过这种状态合并，OSG 可以在绘制时批量处理具有相同状态的一组图元，**减少重复的OpenGL状态切换**[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=,LOD)。这种机制实现了按照渲染状态对物体进行排序，最大化地降低了 OpenGL 状态改动频率，从而提升渲染性能[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=,LOD)。

## 剔除与绘制流程

> **图 3：OSG 渲染树结构示意**——Cull 剔除阶段收集的可见图元按照渲染状态和渲染顺序被组织进不同的 **渲染分支 (RenderBin)** 中进行管理。示例中，RenderStage（渲染台）作为根容器，下面划分出"不透明渲染元"（包含普通不透明物体）和"透明体渲染元"（用于深度排序的透明物体）等分支[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=根据渲染顺序的不同，渲染树生出了三个分支。相应的状态节点置入各个渲染元（RenderBin）分支中，其中渲染细节设置为"RenderBin"的状态节点（Stat eGraph）所处的渲染元也可称为"不透明渲染元"；而设置为"DepthSortedBin"的状态节点则将其附带的渲染叶（RenderLeaf）送入"透明体渲染 元"，预其中采用了按深度值降序的方式排序绘制，以获得正确的透明体渲染结果；未设置渲染细节的状态节点则直接由根节点（渲染台）负责维护。)。在随后的绘制阶段，OSG 将首先绘制不透明物体，再按深度从远到近绘制透明物体，以保证正确的混合叠加效果。

### 渲染帧的阶段划分

在每帧渲染过程中，OSG 基本遵循 **更新 (Update)** → **裁剪 (Cull)** → **绘制 (Draw)** 三个阶段的流程：

- **更新阶段**：处理场景状态更新和动画计算
- **剔除阶段**：遍历场景图进行视景体裁剪和渲染准备
- **绘制阶段**：执行最终的图元绘制命令[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/20/OSG-学习笔记-二.html#:~:text=renderingTraversal%3A function() ,draw()%3B %2F%2F 渲染)

通常开发者使用 `osgViewer::Viewer::frame()` 来驱动这一循环，每次调用都会完成上述三个子阶段，生成一帧图像[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/20/OSG-学习笔记-二.html#:~:text=renderingTraversal%3A function() ,draw()%3B %2F%2F 渲染)。其中 Cull 和 Draw 两个阶段直接与渲染管线相关：Cull 阶段可以理解为**收集并组织待绘制物体**，而 Draw 阶段则根据整理好的列表**排序并实际绘制**这些物体。

### 视锥剔除

Cull 阶段的首要任务是视锥体裁剪（视锥剔除）。CullVisitor在遍历场景时，会使用每个节点预计算的包围体与当前摄像机的视锥体进行相交测试，快速判断整个节点子树是否在视野范围内[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=,LOD)。如果某节点的包围体完全位于视锥体之外，则该节点和其所有子孙节点都会被跳过（裁剪），不进入渲染队列。这种**视锥裁剪**在 CPU 侧提前淘汰了大量不可见的场景元素，降低了后续绘制阶段发送至 GPU 的图元数量[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=,LOD)。

除了视锥体之外，OSG 还支持**遮挡剔除**（隐藏面剔除）：即如果一个物体被其他几何完全挡住不可见，也可以在 Cull 阶段将其裁剪掉。OSG 提供了专门的 `osg::OccluderNode` 等遮挡节点来实现此功能，它通过定义遮挡多边形并在遍历时测试遮挡体积，自动去除那些被遮挡物体[blog.csdn.net](https://blog.csdn.net/hanshuobest/article/details/53179428#:~:text=遮挡裁剪节点osg%3A%3AOccuderNode继承自osg%3AGroup节点。主要作用是裁剪掉被遮挡的物体，也就是场景中被其他物体所遮挡的物体。 目前遮挡裁剪算法主要有两种，分别是基于点的遮挡裁剪和基于单元的遮挡裁剪。)。使用遮挡剔除能进一步减少需要绘制的物体数，提高渲染效率。

### 状态图构建与渲染列表填充

CullVisitor 在遍历场景图的过程中，不仅进行空间裁剪，还会同时按照上一节描述的机制**构建状态树(StateGraph)** 并准备**渲染树(RenderBins)**[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/20/OSG-学习笔记-二.html#:~:text=视锥裁剪 )[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=OSG中所有的Drawable几何体对象都会自动关联一个StateSet对象，无论用户是否在自己的程序中作了设置。进入渲染后台之后，OSG将为场景生成"状态树 "，它是由"状态节点"StateGraph和"渲染叶"RenderLeaf所组成的。)。

具体而言，当遍历进入一个节点时，若该节点附有 StateSet，CullVisitor 便将此 StateSet **压入状态堆栈**，更新当前有效的渲染状态组合，并在状态图中插入/定位到对应的 StateGraph 子节点[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/20/OSG-学习笔记-二.html#:~:text=%2F%2F Camera and lights must,return%3B)；当离开该节点时，则将该状态从堆栈弹出恢复先前状态。这样，CullTraversal沿着场景树一路下行，维护着一个累积的当前状态集合。

当遇到叶节点（如 Geode）的 Drawable 时，CullVisitor 会将该 Drawable 按当前状态组合加入到渲染树中：即在状态图的当前分支下创建一个 **RenderLeaf** 节点，并将 Drawable 放入其中等待绘制[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=1、状态树是根据渲染状态（stateset）来生成的，那些没有设置StateSet的场景节点将不会影响状态树的构架。)。这个 RenderLeaf 记录了具体要绘制的几何体以及对应的状态。在整个 Cull 遍历完成后，OSG 就收集到了**所有通过裁剪测试的可见物体**，并根据它们的状态分布构建好了渲染所需的**绘制列表**。

### 绘制排序与提交 GPU

进入绘制阶段后，之前Cull阶段生成的渲染树（RenderStage/RenderBin结构）将被遍历，用来决定物体的**绘制顺序**[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=生成状态树的同时，OSG渲染后台还将生成对应的"渲染树"，其组成为一个RenderStage对象和多个RenderBind对象，如果我们不使用setRender BinDetails设置StateSet的渲染细节的话，那么所有状态树中的末端节点（其中必然包含一个或多个"渲染叶"）都会按遍历顺序保存到渲染树根节点（渲染台） 中。)。

在OSG默认设置下，渲染树的根是一个 **渲染舞台 (RenderStage)**，代表当前摄像机视景下的一组待绘制场景。所有不要求特殊排序的物体都会被直接置于根RenderStage或其默认的不透明RenderBin中；如果某些StateSet被标记了特定的渲染顺序（RenderBin编号/类型），Cull阶段已将它们分别归类到对应的RenderBin队列中[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=生成状态树的同时，OSG渲染后台还将生成对应的"渲染树"，其组成为一个RenderStage对象和多个RenderBind对象，如果我们不使用setRender BinDetails设置StateSet的渲染细节的话，那么所有状态树中的末端节点（其中必然包含一个或多个"渲染叶"）都会按遍历顺序保存到渲染树根节点（渲染台） 中。)。

绘制时，OSG 会按RenderBin的编号从小到大依次执行绘制[blog.csdn.net](https://blog.csdn.net/a819721810/article/details/77823447#:~:text=有点懒。。不想画图，随便说一说吧，其实状态树你可以看成是一个二叉树，非叶子节点StateGraph和叶子节点ReaderLeaf组成，然后一个Read erLeaf你可以看成有一个Drawable对象，但是场景渲染的是渲染树，状态树只是为了好转换成渲染树而存在的。 渲染树你也可以看成一棵二叉树，RenderStage（渲染台）是根节点，RenderBin)：一般而言，**不透明物体**先绘制（通常放在缺省的不透明Bin，深度测试确保遮挡关系），而**透明物体**则通常被指定到一个开启深度排序的 **透明Bin** 中，Cull阶段将它们独立收集。Draw阶段会在绘制不透明物体之后，再根据摄像机视点到物体的距离对透明Bin内的物体进行从远到近的排序，然后逐一绘制，以正确混合半透明效果[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=根据渲染顺序的不同，渲染树生出了三个分支。相应的状态节点置入各个渲染元（RenderBin）分支中，其中渲染细节设置为"RenderBin"的状态节点（Stat eGraph）所处的渲染元也可称为"不透明渲染元"；而设置为"DepthSortedBin"的状态节点则将其附带的渲染叶（RenderLeaf）送入"透明体渲染 元"，预其中采用了按深度值降序的方式排序绘制，以获得正确的透明体渲染结果；未设置渲染细节的状态节点则直接由根节点（渲染台）负责维护。)。

在遍历RenderBin进行绘制时，OSG 内部的渲染器会利用已经构建好的状态图 (StateGraph)，**尽量减少OpenGL状态切换**：它会比较连续绘制的两个 RenderLeaf 所对应状态组合的异同，只对差异部分执行状态更改操作，例如启用/禁用相应的功能或绑定新的材质/纹理，其余保持不变[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/20/OSG-学习笔记-二.html#:~:text=match at L517 %2F%2F common,RenderLeaf so we dont need)[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/20/OSG-学习笔记-二.html#:~:text=%2F%2F to do anything except,if we used an insertStateSet)。这种做法避免了在绘制每个对象时重复设置大量相同的状态，大幅优化了批量绘制的效率。当需要绘制的批次状态与上一批完全相同时，OSG 几乎不会进行额外的OpenGL调用，只是重复提交几何体绘制命令即可[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=,LOD)。

最终，按照渲染树顺序遍历完所有 RenderBin 中的 RenderLeaf，对应的图元也就全部提交到了GPU，完成该帧的场景绘制[blog.csdn.net](https://blog.csdn.net/a819721810/article/details/77823447#:~:text=有点懒。。不想画图，随便说一说吧，其实状态树你可以看成是一个二叉树，非叶子节点StateGraph和叶子节点ReaderLeaf组成，然后一个Read erLeaf你可以看成有一个Drawable对象，但是场景渲染的是渲染树，状态树只是为了好转换成渲染树而存在的。 渲染树你也可以看成一棵二叉树，RenderStage（渲染台）是根节点，RenderBin)。

---

## 参考文献

1. Wang Rui 等, *"OSG 最长的一帧"* 内部解析[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=OSG中所有的Drawable几何体对象都会自动关联一个StateSet对象，无论用户是否在自己的程序中作了设置。进入渲染后台之后，OSG将为场景生成"状态树 "，它是由"状态节点"StateGraph和"渲染叶"RenderLeaf所组成的。)[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=1、状态树是根据渲染状态（stateset）来生成的，那些没有设置StateSet的场景节点将不会影响状态树的构架。)[cnblogs.com](https://www.cnblogs.com/lyggqm/p/6401462.html#:~:text=根据渲染顺序的不同，渲染树生出了三个分支。相应的状态节点置入各个渲染元（RenderBin）分支中，其中渲染细节设置为"RenderBin"的状态节点（Stat eGraph）所处的渲染元也可称为"不透明渲染元"；而设置为"DepthSortedBin"的状态节点则将其附带的渲染叶（RenderLeaf）送入"透明体渲染 元"，预其中采用了按深度值降序的方式排序绘制，以获得正确的透明体渲染结果；未设置渲染细节的状态节点则直接由根节点（渲染台）负责维护。)

2. OpenSceneGraph 官方文档, *场景图与状态管理*[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=2,，可以多个父节点共享一个子节点。 Image 节点主要分三类：根节点，枝节节点，叶子节点。OSG使用Node类表达一个基本节点，也是所有类型场景节点的基类。)[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=那么在 OSG 中是如何改变状态的呢？首先 OSG 将状态分成,StateSet，要注意它只是 WebGL 全部状态的子集，需要依附场景图中的节点而存在。例如下图中开启 wireframe 的叶节点：)

3. Xiaoiver 博客, *OSG 学习笔记*（2019）[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=状态的继承)[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=rootSS)[xiaoiver.github.io](https://xiaoiver.github.io/coding/2019/01/15/OSG-学习笔记.html#:~:text=,LOD)

4. 可可西, *"osg场景图（DAG-有向无环图）"* 博客园文章[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=其中，叶子节点（osg%3A%3AGeode）是模型数据节点（存放数据的节点），中间节点（或称枝节节点）为组节点（osg%3A%3AGroup）。)[cnblogs.com](https://www.cnblogs.com/kekec/archive/2011/09/25/2190322.html#:~:text=根据不同的用途，有各种不同的组结点，它们都继承自osg%3A%3AGroup。)

5. CSDN社区, *OSG 场景管理与渲染总结*[blog.csdn.net](https://blog.csdn.net/aoqiangliang243023/article/details/101357275#:~:text=1,，可以多个父节点共享一个子节点。 Image 节点主要分三类：根节点，枝节节点，叶子节点。OSG使用Node类表达一个基本节点，也是所有类型场景节点的基类。)[blog.csdn.net](https://blog.csdn.net/hanshuobest/article/details/53179428#:~:text=遮挡裁剪节点osg%3A%3AOccuderNode继承自osg%3AGroup节点。主要作用是裁剪掉被遮挡的物体，也就是场景中被其他物体所遮挡的物体。 目前遮挡裁剪算法主要有两种，分别是基于点的遮挡裁剪和基于单元的遮挡裁剪。)