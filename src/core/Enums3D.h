#pragma once

#include <cstdint>

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

// 节点线型（用于复杂线条）
enum NodeLineStyle3D
{
    BeginNodeLineStyle3D = EndLineStyle3D,
    
    NodeLine_Polyline3D,       // 折线
    NodeLine_Spline3D,         // 样条曲线
    NodeLine_Bezier3D,         // 贝塞尔曲线
    NodeLine_Arc3D,            // 圆弧
    NodeLine_ThreePointArc3D,  // 三点弧
    NodeLine_Streamline3D,     // 流线
    
    EndNodeLineStyle3D
};

// 几何对象类型
enum GeoType3D
{
    BeginGeoType3D = EndNodeLineStyle3D,
    
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
    Fill_Points3D,         // 点填充
    Fill_Texture3D,        // 纹理填充
    
    EndFillType3D
};

// 材质类型
enum MaterialType3D
{
    BeginMaterialType3D = EndFillType3D,
    
    Material_Basic3D,      // 基础材质
    Material_Phong3D,      // Phong材质
    Material_Blinn3D,      // Blinn材质
    Material_Lambert3D,    // Lambert材质
    Material_PBR3D,        // PBR材质
    
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