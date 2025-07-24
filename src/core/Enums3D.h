#pragma once
#pragma execution_character_set("utf-8")

#include <cstdint>
#include <string>

// 三维绘图系统枚举定义

enum DrawMode3D 
{
    BeginDrawMode3D = 0,

    DrawSelect3D,
    
    // 点绘制
    DrawPoint3D,
    
    // 线绘制
    DrawLine3D,
    DrawArc3D,
    DrawThreePointArc3D,
    DrawBezierCurve3D,
    DrawStreamline3D,
    
    // 面绘制
    DrawTriangle3D,
    DrawQuad3D,
    DrawPolygon3D,
    DrawCircleSurface3D,
    
    // 体绘制
    DrawBox3D,           // 长方体
    DrawCube3D,          // 正方体
    DrawCone3D,          // 圆锥
    DrawCylinder3D,      // 圆柱
    DrawPrism3D,         // 多棱柱
    DrawTorus3D,         // 圆环
    DrawSphere3D,        // 球
    DrawHemisphere3D,    // 半球
    DrawEllipsoid3D,     // 椭球

    // 建筑类型
    DrawGableHouse3D,    // 人字形房屋
    DrawSpireHouse3D,    // 尖顶房屋
    DrawDomeHouse3D,     // 穹顶房屋
    DrawFlatHouse3D,     // 平顶房屋
    DrawLHouse3D,        // L型房屋

    EndDrawMode3D
};

// 点的形状
enum PointShape3D
{
    BeginPointShape3D = EndDrawMode3D,
    
    Point_Circle3D,        // 圆形
    Point_Square3D,        // 方形
    Point_Triangle3D,      // 三角形
    Point_Diamond3D,       // 菱形
    Point_Cross3D,         // 十字
    Point_Star3D,          // 星形
    
    EndPointShape3D
};

// 线的类型
enum LineStyle3D
{
    BeginLineStyle3D = EndPointShape3D,
    
    Line_Solid3D,          // 实线
    Line_Dashed3D,         // 虚线
    Line_Dotted3D,         // 点线
    Line_DashDot3D,        // 点划线
    Line_DashDotDot3D,     // 双点划线
    Line_Custom3D,         // 自定义虚线
    
    EndLineStyle3D
};

// 几何对象类型
enum GeoType3D
{
    BeginGeoType3D = EndLineStyle3D,
    
    Geo_Undefined3D,
    
    // 点类型
    Geo_Point3D,
    
    // 线类型
    Geo_Line3D,
    Geo_Arc3D,
    Geo_BezierCurve3D,
    Geo_Streamline3D,
    
    // 面类型
    Geo_Triangle3D,
    Geo_Quad3D,
    Geo_Polygon3D,
    Geo_CircleSurface3D,
    
    // 体类型
    Geo_Box3D,
    Geo_Cube3D,
    Geo_Cone3D,
    Geo_Cylinder3D,
    Geo_Prism3D,
    Geo_Torus3D,
    Geo_Sphere3D,
    Geo_Hemisphere3D,
    Geo_Ellipsoid3D,
    
    // 建筑类型
    Geo_FlatHouse3D,
    Geo_DomeHouse3D,
    Geo_SpireHouse3D,
    Geo_GableHouse3D,
    Geo_LHouse3D,
    
    // 未定义几何体类型
    Geo_UndefinedGeo3D,
    
    EndGeoType3D
};

// 几何对象状态
enum GeoState3D
{
    BeginGeoState3D = EndGeoType3D,
    
    GeoState_Initialized3D = 1 << 0,
    GeoState_Complete3D = 1 << 1,
    GeoState_Invalid3D = 1 << 2,
    GeoState_Selected3D = 1 << 3,
    GeoState_Editing3D = 1 << 4,
    
    EndGeoState3D
};

// 填充类型
enum FillType3D
{
    BeginFillType3D = EndGeoState3D,
    
    Fill_None3D,           // 无填充
    Fill_Solid3D,          // 实心填充
    Fill_Wireframe3D,      // 线框
    
    EndFillType3D
};

// 材质类型
enum MaterialType3D
{
    BeginMaterialType3D = EndFillType3D,
    
    Material_Basic3D,      // 基础材质
    Material_Phong3D,      // Phong材质
    
    EndMaterialType3D
};

// 体的细分级别
enum SubdivisionLevel3D
{
    BeginSubdivisionLevel3D = EndMaterialType3D,
    
    Subdivision_Low3D = 8,      // 低细分度
    Subdivision_Medium3D = 16,   // 中等细分度
    Subdivision_High3D = 32,     // 高细分度
    Subdivision_Ultra3D = 64,    // 超高细分度
    
    EndSubdivisionLevel3D
};

// 拾取Feature接口
enum class FeatureType : uint8_t
{
    FACE = 0,
    EDGE = 1, 
    VERTEX = 2
};

// 指示器类型
enum class IndicatorType
{
    VERTEX_INDICATOR,    // 顶点指示器(小方框)
    EDGE_INDICATOR,      // 边指示器(小三角箭头)
    FACE_INDICATOR,       // 面指示器(圆环)
    VOLUME_INDICATOR
};

// 坐标系类型
enum CoordinateSystemType3D
{
    BeginCoordinateSystemType3D = EndSubdivisionLevel3D,
    
    CoordSystem_None3D,      // 无坐标系
    CoordSystem_Axis3D,      // 光轴线坐标系
    CoordSystem_Grid3D,      // 网格线坐标系
    CoordSystem_Both3D,      // 光轴线+网格线
    
    EndCoordinateSystemType3D
};

// 坐标轴
enum CoordinateAxis3D
{
    BeginCoordinateAxis3D = EndCoordinateSystemType3D,
    
    Axis_X3D,           // X轴
    Axis_Y3D,           // Y轴
    Axis_Z3D,           // Z轴
    Axis_All3D,         // 所有轴
    
    EndCoordinateAxis3D
};

// 网格平面
enum GridPlane3D
{
    BeginGridPlane3D = EndCoordinateAxis3D,
    
    GridPlane_XY3D,     // XY平面 (Z=0)
    GridPlane_YZ3D,     // YZ平面 (X=0)
    GridPlane_XZ3D,     // XZ平面 (Y=0)
    GridPlane_All3D,    // 所有平面
    
    EndGridPlane3D
};

// 刻度单位类型
enum ScaleUnit3D
{
    BeginScaleUnit3D = EndCoordinateAxis3D,
    
    Unit_Meter3D,       // 米
    Unit_Kilometer3D,   // 千米
    Unit_Centimeter3D,  // 厘米
    Unit_Millimeter3D,  // 毫米
    Unit_Custom3D,      // 自定义单位
    
    EndScaleUnit3D
};

// 字体大小类型
enum FontSize3D
{
    BeginFontSize3D = EndScaleUnit3D,
    
    FontSize_Small3D,     // 小字体
    FontSize_Medium3D,    // 中等字体
    FontSize_Large3D,     // 大字体
    FontSize_Custom3D,    // 自定义字体大小
    
    EndFontSize3D
};

// 建筑类型枚举
enum BuildingType3D
{
    BeginBuildingType3D = EndFontSize3D,
    
    Building_GableHouse3D,      // 人字形房屋
    Building_SpireHouse3D,      // 尖顶房屋
    Building_DomeHouse3D,       // 穹顶房屋
    Building_FlatHouse3D,       // 平顶房屋
    Building_LHouse3D,          // L型房屋
    Building_ParapetHouse3D,    // 带女儿墙的平顶房屋
    Building_OverlapHouse3D,    // 搭边房屋
    Building_CourtyardHouse3D,  // 回型房屋
    Building_GableSpireHouse3D, // 人字尖点房屋
    Building_ArcHouse3D,        // 弧顶房屋
    
    EndBuildingType3D
};

// 节点标记常量 - 用于从文件加载时的节点识别
namespace NodeTags3D
{
    const std::string VERTEX_GEOMETRY = "3D_VERTEX_GEOM";
    const std::string EDGE_GEOMETRY = "3D_EDGE_GEOM";
    const std::string FACE_GEOMETRY = "3D_FACE_GEOM";
    const std::string CONTROL_POINTS_GEOMETRY = "3D_CONTROL_POINTS_GEOM";
    const std::string BOUNDING_BOX_GEOMETRY = "3D_BOUNDING_BOX_GEOM";
    const std::string TRANSFORM_NODE = "3D_TRANSFORM_NODE";
    const std::string ROOT_GROUP = "3D_ROOT_GROUP";
    const std::string SCENE_ROOT = "3D_SCENE_ROOT";  // 场景根节点标识
}

// 节点掩码定义 - 用于OSG节点的显示/隐藏和拾取控制
// 每个组件使用独立的位标识，显示时通过位组合控制，不可见时置0
enum NodeMask3D : uint32_t
{
    // 基础掩码
    NODE_MASK_NONE        = 0x00000000,  // 隐藏节点（完全不可见，不可拾取）
    NODE_MASK_ALL         = 0xFFFFFFFF,  // 所有节点（可见，可拾取）
    NODE_MASK_NOSELECT    = 0x80000000,  // 只可见

    // 几何体类型掩码（用于拾取系统分类）
    NODE_MASK_VERTEX      = 0x00000001,  // 顶点几何体（第1位）
    NODE_MASK_EDGE        = 0x00000002,  // 边几何体（第2位）
    NODE_MASK_FACE        = 0x00000004,  // 面几何体（第3位）
    NODE_MASK_CONTROL_POINTS = 0x00000008,  // 控制点（第4位）
    NODE_MASK_BOUNDING_BOX   = 0x00000010,  // 包围盒（第5位）
    
    // 特殊功能组件掩码（高位区域，避免与几何体冲突）
    NODE_MASK_PICKING_INDICATOR = 0x00000020,  // 拾取指示器
    NODE_MASK_UI_OVERLAY        = 0x00000040,  // UI覆盖层
    NODE_MASK_DEBUG_INFO        = 0x00000080,  // 调试信息
    NODE_MASK_SKYBOX            = 0x00000100,  // 天空盒
    NODE_MASK_COORDINATE_SYSTEM = 0x08000200,  // 坐标系统
    
    // 常用组合掩码（通过位组合实现）
    NODE_MASK_ALL_GEOMETRY = NODE_MASK_VERTEX | NODE_MASK_EDGE | NODE_MASK_FACE,  // 所有主要几何体
    NODE_MASK_ALL_VISIBLE  = NODE_MASK_ALL_GEOMETRY | NODE_MASK_CONTROL_POINTS | NODE_MASK_BOUNDING_BOX,  // 所有可见几何元素
    NODE_MASK_WIREFRAME_ONLY = NODE_MASK_VERTEX | NODE_MASK_EDGE,              // 仅线框模式
    NODE_MASK_SOLID_ONLY     = NODE_MASK_FACE,                                 // 仅实体模式
    NODE_MASK_POINTS_ONLY    = NODE_MASK_VERTEX,                              // 仅点模式
    
    // 场景组合掩码
    NODE_MASK_GEOMETRY_WITH_UI = NODE_MASK_ALL_GEOMETRY | NODE_MASK_UI_OVERLAY | NODE_MASK_PICKING_INDICATOR,  // 几何体+UI
    NODE_MASK_FULL_SCENE = NODE_MASK_ALL_VISIBLE | NODE_MASK_SKYBOX | NODE_MASK_COORDINATE_SYSTEM,            // 完整场景
};

// NodeMask3D 操作函数（内联函数，提高性能）
inline bool hasNodeMask(uint32_t currentMask, NodeMask3D checkMask) 
{
    return (currentMask & checkMask) != 0;
}

inline uint32_t addNodeMask(uint32_t currentMask, NodeMask3D addMask) 
{
    return currentMask | addMask;
}

inline uint32_t removeNodeMask(uint32_t currentMask, NodeMask3D removeMask) 
{
    return currentMask & (~removeMask);
}

inline uint32_t toggleNodeMask(uint32_t currentMask, NodeMask3D toggleMask) 
{
    return currentMask ^ toggleMask;
}

