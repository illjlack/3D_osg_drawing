#include "Common3D.h"
#include <sstream>
#include <filesystem>

// 全局变量定义(用于初始化新的对象，可以写个配置文件让重新启动时保存上次设置)
DrawMode3D GlobalDrawMode3D = DrawSelect3D;
PointShape3D GlobalPointShape3D = Point_Circle3D;
float GlobalPointSize3D = 5.0f;
QColor GlobalPointColor3D = QColor(255, 0, 0);

LineStyle3D GlobalLineStyle3D = Line_Solid3D;
float GlobalLineWidth3D = 2.0f;
QColor GlobalLineColor3D = QColor(0, 0, 255);
float GlobalLineDashPattern3D = 5.0f;
NodeLineStyle3D GlobalNodeLineStyle3D = NodeLine_Polyline3D;

FillType3D GlobalFillType3D = Fill_Solid3D;
QColor GlobalFillColor3D = QColor(0, 255, 0, 128);
QColor GlobalBorderColor3D = QColor(0, 0, 0);
bool GlobalShowBorder3D = true;

MaterialType3D GlobalMaterialType3D = Material_Basic3D;
float GlobalShininess3D = 32.0f;
float GlobalTransparency3D = 1.0f;
SubdivisionLevel3D GlobalSubdivisionLevel3D = Subdivision_Medium3D;

QStatusBar* GlobalStatusBar3D = nullptr;

// GeoParameters3D 构造函数实现
GeoParameters3D::GeoParameters3D()
{
    resetToGlobal();
}

void GeoParameters3D::resetToGlobal()
{
    // 点属性
    pointShape = GlobalPointShape3D;
    pointSize = GlobalPointSize3D;
    pointColor = Color3D(GlobalPointColor3D);
    
    // 线属性
    lineStyle = GlobalLineStyle3D;
    lineWidth = GlobalLineWidth3D;
    lineColor = Color3D(GlobalLineColor3D);
    lineDashPattern = GlobalLineDashPattern3D;
    nodeLineStyle = GlobalNodeLineStyle3D;
    
    // 面属性
    fillType = GlobalFillType3D;
    fillColor = Color3D(GlobalFillColor3D);
    borderColor = Color3D(GlobalBorderColor3D);
    showBorder = GlobalShowBorder3D;
    
    // 材质属性
    material.type = GlobalMaterialType3D;
    material.shininess = GlobalShininess3D;
    material.transparency = GlobalTransparency3D;
    
    // 体属性
    subdivisionLevel = GlobalSubdivisionLevel3D;
    
    // 样条曲线属性
    splineOrder = 3;
    splineNodeCount = 10;
    steps = 50;
}

void initializeGlobal3DSettings()
{
    GlobalDrawMode3D = DrawSelect3D;
    GlobalPointShape3D = Point_Circle3D;
    GlobalPointSize3D = 5.0f;
    GlobalPointColor3D = QColor(255, 0, 0);

    GlobalLineStyle3D = Line_Solid3D;
    GlobalLineWidth3D = 2.0f;
    GlobalLineColor3D = QColor(0, 0, 255);
    GlobalLineDashPattern3D = 5.0f;
    GlobalNodeLineStyle3D = NodeLine_Polyline3D;

    GlobalFillType3D = Fill_Solid3D;
    GlobalFillColor3D = QColor(0, 255, 0, 128);
    GlobalBorderColor3D = QColor(0, 0, 0);
    GlobalShowBorder3D = true;

    GlobalMaterialType3D = Material_Basic3D;
    GlobalShininess3D = 32.0f;
    GlobalTransparency3D = 1.0f;
    GlobalSubdivisionLevel3D = Subdivision_Medium3D;
}

// 转换函数实现
QString drawMode3DToString(DrawMode3D mode)
{
    switch (mode) {
        case DrawSelect3D: return QObject::tr("选择");
        case DrawPoint3D: return QObject::tr("点");
        case DrawLine3D: return QObject::tr("线");
        case DrawArc3D: return QObject::tr("圆弧");
        case DrawThreePointArc3D: return QObject::tr("三点弧");
        case DrawBezierCurve3D: return QObject::tr("贝塞尔曲线");
        case DrawStreamline3D: return QObject::tr("流线");
        case DrawTriangle3D: return QObject::tr("三角形");
        case DrawQuad3D: return QObject::tr("四边形");
        case DrawPolygon3D: return QObject::tr("多边形");
        case DrawCircleSurface3D: return QObject::tr("圆面");
        case DrawBox3D: return QObject::tr("长方体");
        case DrawCube3D: return QObject::tr("正方体");
        case DrawCone3D: return QObject::tr("圆锥");
        case DrawCylinder3D: return QObject::tr("圆柱");
        case DrawPrism3D: return QObject::tr("多棱柱");
        case DrawTorus3D: return QObject::tr("圆环");
        case DrawSphere3D: return QObject::tr("球");
        case DrawHemisphere3D: return QObject::tr("半球");
        case DrawEllipsoid3D: return QObject::tr("椭球");
        default: return QObject::tr("未知");
    }
}

QString pointShape3DToString(PointShape3D shape)
{
    switch (shape) {
        case Point_Circle3D: return QObject::tr("圆形");
        case Point_Square3D: return QObject::tr("方形");
        case Point_Triangle3D: return QObject::tr("三角形");
        case Point_Diamond3D: return QObject::tr("菱形");
        case Point_Cross3D: return QObject::tr("十字");
        case Point_Star3D: return QObject::tr("星形");
        default: return QObject::tr("未知");
    }
}

QString lineStyle3DToString(LineStyle3D style)
{
    switch (style) {
        case Line_Solid3D: return QObject::tr("实线");
        case Line_Dashed3D: return QObject::tr("虚线");
        case Line_Dotted3D: return QObject::tr("点线");
        case Line_DashDot3D: return QObject::tr("点划线");
        case Line_DashDotDot3D: return QObject::tr("双点划线");
        case Line_Custom3D: return QObject::tr("自定义");
        default: return QObject::tr("未知");
    }
}

QString nodeLineStyle3DToString(NodeLineStyle3D style)
{
    switch (style) {
        case NodeLine_Polyline3D: return QObject::tr("折线");
        case NodeLine_Spline3D: return QObject::tr("样条曲线");
        case NodeLine_Bezier3D: return QObject::tr("贝塞尔曲线");
        case NodeLine_Arc3D: return QObject::tr("圆弧");
        case NodeLine_ThreePointArc3D: return QObject::tr("三点弧");
        case NodeLine_Streamline3D: return QObject::tr("流线");
        default: return QObject::tr("未知");
    }
}

QString fillType3DToString(FillType3D type)
{
    switch (type) {
        case Fill_None3D: return QObject::tr("无填充");
        case Fill_Solid3D: return QObject::tr("实心");
        case Fill_Wireframe3D: return QObject::tr("线框");
        case Fill_Points3D: return QObject::tr("点填充");
        case Fill_Texture3D: return QObject::tr("纹理");
        default: return QObject::tr("未知");
    }
}

QString materialType3DToString(MaterialType3D type)
{
    switch (type) {
        case Material_Basic3D: return QObject::tr("基础材质");
        case Material_Phong3D: return QObject::tr("Phong材质");
        case Material_Blinn3D: return QObject::tr("Blinn材质");
        case Material_Lambert3D: return QObject::tr("Lambert材质");
        case Material_PBR3D: return QObject::tr("PBR材质");
        default: return QObject::tr("未知");
    }
}

DrawMode3D stringToDrawMode3D(const QString& str)
{
    if (str == QObject::tr("选择")) return DrawSelect3D;
    if (str == QObject::tr("点")) return DrawPoint3D;
    if (str == QObject::tr("线")) return DrawLine3D;
    if (str == QObject::tr("圆弧")) return DrawArc3D;
    if (str == QObject::tr("三点弧")) return DrawThreePointArc3D;
    if (str == QObject::tr("贝塞尔曲线")) return DrawBezierCurve3D;
    if (str == QObject::tr("流线")) return DrawStreamline3D;
    if (str == QObject::tr("三角形")) return DrawTriangle3D;
    if (str == QObject::tr("四边形")) return DrawQuad3D;
    if (str == QObject::tr("多边形")) return DrawPolygon3D;
    if (str == QObject::tr("圆面")) return DrawCircleSurface3D;
    if (str == QObject::tr("长方体")) return DrawBox3D;
    if (str == QObject::tr("正方体")) return DrawCube3D;
    if (str == QObject::tr("圆锥")) return DrawCone3D;
    if (str == QObject::tr("圆柱")) return DrawCylinder3D;
    if (str == QObject::tr("多棱柱")) return DrawPrism3D;
    if (str == QObject::tr("圆环")) return DrawTorus3D;
    if (str == QObject::tr("球")) return DrawSphere3D;
    if (str == QObject::tr("半球")) return DrawHemisphere3D;
    if (str == QObject::tr("椭球")) return DrawEllipsoid3D;
    return DrawSelect3D;
}

PointShape3D stringToPointShape3D(const QString& str)
{
    if (str == QObject::tr("圆形")) return Point_Circle3D;
    if (str == QObject::tr("方形")) return Point_Square3D;
    if (str == QObject::tr("三角形")) return Point_Triangle3D;
    if (str == QObject::tr("菱形")) return Point_Diamond3D;
    if (str == QObject::tr("十字")) return Point_Cross3D;
    if (str == QObject::tr("星形")) return Point_Star3D;
    return Point_Circle3D;
}

LineStyle3D stringToLineStyle3D(const QString& str)
{
    if (str == QObject::tr("实线")) return Line_Solid3D;
    if (str == QObject::tr("虚线")) return Line_Dashed3D;
    if (str == QObject::tr("点线")) return Line_Dotted3D;
    if (str == QObject::tr("点划线")) return Line_DashDot3D;
    if (str == QObject::tr("双点划线")) return Line_DashDotDot3D;
    if (str == QObject::tr("自定义")) return Line_Custom3D;
    return Line_Solid3D;
}

NodeLineStyle3D stringToNodeLineStyle3D(const QString& str)
{
    if (str == QObject::tr("折线")) return NodeLine_Polyline3D;
    if (str == QObject::tr("样条曲线")) return NodeLine_Spline3D;
    if (str == QObject::tr("贝塞尔曲线")) return NodeLine_Bezier3D;
    if (str == QObject::tr("圆弧")) return NodeLine_Arc3D;
    if (str == QObject::tr("三点弧")) return NodeLine_ThreePointArc3D;
    if (str == QObject::tr("流线")) return NodeLine_Streamline3D;
    return NodeLine_Polyline3D;
}

FillType3D stringToFillType3D(const QString& str)
{
    if (str == QObject::tr("无填充")) return Fill_None3D;
    if (str == QObject::tr("实心")) return Fill_Solid3D;
    if (str == QObject::tr("线框")) return Fill_Wireframe3D;
    if (str == QObject::tr("点填充")) return Fill_Points3D;
    if (str == QObject::tr("纹理")) return Fill_Texture3D;
    return Fill_Solid3D;
}

MaterialType3D stringToMaterialType3D(const QString& str)
{
    if (str == QObject::tr("基础材质")) return Material_Basic3D;
    if (str == QObject::tr("Phong材质")) return Material_Phong3D;
    if (str == QObject::tr("Blinn材质")) return Material_Blinn3D;
    if (str == QObject::tr("Lambert材质")) return Material_Lambert3D;
    if (str == QObject::tr("PBR材质")) return Material_PBR3D;
    return Material_Basic3D;
} 