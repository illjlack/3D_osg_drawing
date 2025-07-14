#include "Common3D.h"
#include <fstream>
#include <string>

// 全局变量定义
DrawMode3D GlobalDrawMode3D = DrawSelect3D;
PointShape3D GlobalPointShape3D = Point_Circle3D;
float GlobalPointSize3D = 5.0f;
QColor GlobalPointColor3D = QColor(255, 0, 0);

LineStyle3D GlobalLineStyle3D = Line_Solid3D;
float GlobalLineWidth3D = 2.0f;
QColor GlobalLineColor3D = QColor(0, 0, 255);
float GlobalLineDashPattern3D = 1.0f;
NodeLineStyle3D GlobalNodeLineStyle3D = NodeLine_Polyline3D;

FillType3D GlobalFillType3D = Fill_Solid3D;
QColor GlobalFillColor3D = QColor(128, 128, 128);
QColor GlobalBorderColor3D = QColor(0, 0, 0);
bool GlobalShowBorder3D = true;

MaterialType3D GlobalMaterialType3D = Material_Basic3D;
float GlobalShininess3D = 32.0f;
float GlobalTransparency3D = 1.0f;
SubdivisionLevel3D GlobalSubdivisionLevel3D = Subdivision_Medium3D;

// 显示控制全局变量
bool GlobalShowPoints3D = true;
bool GlobalShowEdges3D = true;
bool GlobalShowFaces3D = true;

QStatusBar* GlobalStatusBar3D = nullptr;

// GeoParameters3D 实现
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
    showPoints = true;  // 默认显示点
    
    // 线属性
    lineStyle = GlobalLineStyle3D;
    lineWidth = GlobalLineWidth3D;
    lineColor = Color3D(GlobalLineColor3D);
    lineDashPattern = GlobalLineDashPattern3D;
    nodeLineStyle = GlobalNodeLineStyle3D;
    showEdges = true;  // 默认显示边
    
    // 面属性
    fillType = GlobalFillType3D;
    fillColor = Color3D(GlobalFillColor3D);
    borderColor = Color3D(GlobalBorderColor3D);
    showBorder = GlobalShowBorder3D;
    showFaces = true;  // 默认显示面
    
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

// 初始化全局设置
void initializeGlobal3DSettings()
{
    // 设置默认值（已在全局变量定义中设置）
}

// 枚举转换函数实现
QString drawMode3DToString(DrawMode3D mode)
{
    switch (mode)
    {
        case DrawSelect3D: return "选择";
        case DrawPoint3D: return "点";
        case DrawLine3D: return "线";
        case DrawArc3D: return "弧";
        case DrawThreePointArc3D: return "三点弧";
        case DrawBezierCurve3D: return "贝塞尔曲线";
        case DrawStreamline3D: return "流线";
        case DrawTriangle3D: return "三角形";
        case DrawQuad3D: return "四边形";
        case DrawPolygon3D: return "多边形";
        case DrawCircleSurface3D: return "圆面";
        case DrawBox3D: return "长方体";
        case DrawCube3D: return "正方体";
        case DrawCone3D: return "圆锥";
        case DrawCylinder3D: return "圆柱";
        case DrawPrism3D: return "多棱柱";
        case DrawTorus3D: return "圆环";
        case DrawSphere3D: return "球";
        case DrawHemisphere3D: return "半球";
        case DrawEllipsoid3D: return "椭球";
        default: return "未知";
    }
}

QString pointShape3DToString(PointShape3D shape)
{
    switch (shape)
    {
        case Point_Circle3D: return "圆形";
        case Point_Square3D: return "方形";
        case Point_Triangle3D: return "三角形";
        case Point_Diamond3D: return "菱形";
        case Point_Cross3D: return "十字";
        case Point_Star3D: return "星形";
        default: return "未知";
    }
}

QString lineStyle3DToString(LineStyle3D style)
{
    switch (style)
    {
        case Line_Solid3D: return "实线";
        case Line_Dashed3D: return "虚线";
        case Line_Dotted3D: return "点线";
        case Line_DashDot3D: return "点划线";
        case Line_DashDotDot3D: return "双点划线";
        case Line_Custom3D: return "自定义";
        default: return "未知";
    }
}

QString nodeLineStyle3DToString(NodeLineStyle3D style)
{
    switch (style)
    {
        case NodeLine_Polyline3D: return "折线";
        case NodeLine_Spline3D: return "样条曲线";
        case NodeLine_Bezier3D: return "贝塞尔曲线";
        case NodeLine_Arc3D: return "圆弧";
        case NodeLine_ThreePointArc3D: return "三点弧";
        case NodeLine_Streamline3D: return "流线";
        default: return "未知";
    }
}

QString fillType3DToString(FillType3D type)
{
    switch (type)
    {
        case Fill_None3D: return "无填充";
        case Fill_Solid3D: return "实心";
        case Fill_Wireframe3D: return "线框";
        case Fill_Points3D: return "点填充";
        case Fill_Texture3D: return "纹理";
        default: return "未知";
    }
}

QString materialType3DToString(MaterialType3D type)
{
    switch (type)
    {
        case Material_Basic3D: return "基础";
        case Material_Phong3D: return "Phong";
        case Material_Blinn3D: return "Blinn";
        case Material_Lambert3D: return "Lambert";
        case Material_PBR3D: return "PBR";
        default: return "未知";
    }
}

// 字符串转枚举函数实现
DrawMode3D stringToDrawMode3D(const QString& str)
{
    if (str == "选择") return DrawSelect3D;
    if (str == "点") return DrawPoint3D;
    if (str == "线") return DrawLine3D;
    if (str == "弧") return DrawArc3D;
    if (str == "三点弧") return DrawThreePointArc3D;
    if (str == "贝塞尔曲线") return DrawBezierCurve3D;
    if (str == "流线") return DrawStreamline3D;
    if (str == "三角形") return DrawTriangle3D;
    if (str == "四边形") return DrawQuad3D;
    if (str == "多边形") return DrawPolygon3D;
    if (str == "圆面") return DrawCircleSurface3D;
    if (str == "长方体") return DrawBox3D;
    if (str == "正方体") return DrawCube3D;
    if (str == "圆锥") return DrawCone3D;
    if (str == "圆柱") return DrawCylinder3D;
    if (str == "多棱柱") return DrawPrism3D;
    if (str == "圆环") return DrawTorus3D;
    if (str == "球") return DrawSphere3D;
    if (str == "半球") return DrawHemisphere3D;
    if (str == "椭球") return DrawEllipsoid3D;
    return DrawSelect3D;
}

PointShape3D stringToPointShape3D(const QString& str)
{
    if (str == "圆形") return Point_Circle3D;
    if (str == "方形") return Point_Square3D;
    if (str == "三角形") return Point_Triangle3D;
    if (str == "菱形") return Point_Diamond3D;
    if (str == "十字") return Point_Cross3D;
    if (str == "星形") return Point_Star3D;
    return Point_Circle3D;
}

LineStyle3D stringToLineStyle3D(const QString& str)
{
    if (str == "实线") return Line_Solid3D;
    if (str == "虚线") return Line_Dashed3D;
    if (str == "点线") return Line_Dotted3D;
    if (str == "点划线") return Line_DashDot3D;
    if (str == "双点划线") return Line_DashDotDot3D;
    if (str == "自定义") return Line_Custom3D;
    return Line_Solid3D;
}

NodeLineStyle3D stringToNodeLineStyle3D(const QString& str)
{
    if (str == "折线") return NodeLine_Polyline3D;
    if (str == "样条曲线") return NodeLine_Spline3D;
    if (str == "贝塞尔曲线") return NodeLine_Bezier3D;
    if (str == "圆弧") return NodeLine_Arc3D;
    if (str == "三点弧") return NodeLine_ThreePointArc3D;
    if (str == "流线") return NodeLine_Streamline3D;
    return NodeLine_Polyline3D;
}

FillType3D stringToFillType3D(const QString& str)
{
    if (str == "无填充") return Fill_None3D;
    if (str == "实心") return Fill_Solid3D;
    if (str == "线框") return Fill_Wireframe3D;
    if (str == "点填充") return Fill_Points3D;
    if (str == "纹理") return Fill_Texture3D;
    return Fill_Solid3D;
}

MaterialType3D stringToMaterialType3D(const QString& str)
{
    if (str == "基础") return Material_Basic3D;
    if (str == "Phong") return Material_Phong3D;
    if (str == "Blinn") return Material_Blinn3D;
    if (str == "Lambert") return Material_Lambert3D;
    if (str == "PBR") return Material_PBR3D;
    return Material_Basic3D;
} 

// GeoParameters3D 新方法实现
void GeoParameters3D::setAllProperties(
    const PointShape3D& pShape,
    float pSize,
    const Color3D& pColor,
    bool showPts,
    
    const LineStyle3D& lStyle,
    float lWidth,
    const Color3D& lColor,
    float lDashPattern,
    const NodeLineStyle3D& nlStyle,
    bool showEdgs,
    
    const FillType3D& fType,
    const Color3D& fColor,
    const Color3D& bColor,
    bool showBrd,
    bool showFcs,
    
    const MaterialType3D& mType,
    float shininess,
    float transparency,
    const SubdivisionLevel3D& subLevel)
{
    // 设置点属性
    pointShape = pShape;
    pointSize = pSize;
    pointColor = pColor;
    showPoints = showPts;
    
    // 设置线属性
    lineStyle = lStyle;
    lineWidth = lWidth;
    lineColor = lColor;
    lineDashPattern = lDashPattern;
    nodeLineStyle = nlStyle;
    showEdges = showEdgs;
    
    // 设置面属性
    fillType = fType;
    fillColor = fColor;
    borderColor = bColor;
    showBorder = showBrd;
    showFaces = showFcs;
    
    // 设置材质属性
    material.type = mType;
    material.shininess = shininess;
    material.transparency = transparency;
    
    // 设置体属性
    subdivisionLevel = subLevel;
}

void GeoParameters3D::setPresetStyle(const std::string& styleName)
{
    auto& manager = GlobalParametersManager::getInstance();
    *this = manager.getPreset(styleName);
}

GeoParameters3D GeoParameters3D::getDefaultStyle()
{
    GeoParameters3D params;
    params.resetToGlobal();
    return params;
}

GeoParameters3D GeoParameters3D::getWireframeStyle()
{
    GeoParameters3D params;
    params.resetToGlobal();
    params.showPoints = false;
    params.showEdges = true;
    params.showFaces = false;
    params.lineWidth = 1.0f;
    params.lineColor = Color3D(0.0f, 1.0f, 0.0f, 1.0f); // 绿色
    return params;
}

GeoParameters3D GeoParameters3D::getPointStyle()
{
    GeoParameters3D params;
    params.resetToGlobal();
    params.showPoints = true;
    params.showEdges = false;
    params.showFaces = false;
    params.pointSize = 8.0f;
    params.pointColor = Color3D(1.0f, 0.0f, 0.0f, 1.0f); // 红色
    return params;
}

GeoParameters3D GeoParameters3D::getHighlightStyle()
{
    GeoParameters3D params;
    params.resetToGlobal();
    params.showPoints = true;
    params.showEdges = true;
    params.showFaces = true;
    params.pointSize = 10.0f;
    params.lineWidth = 3.0f;
    params.pointColor = Color3D(1.0f, 1.0f, 0.0f, 1.0f); // 黄色
    params.lineColor = Color3D(1.0f, 1.0f, 0.0f, 1.0f); // 黄色
    params.fillColor = Color3D(1.0f, 1.0f, 0.0f, 0.3f); // 半透明黄色
    return params;
}

GeoParameters3D GeoParameters3D::getTransparentStyle()
{
    GeoParameters3D params;
    params.resetToGlobal();
    params.material.transparency = 0.5f;
    params.fillColor.a = 0.5f;
    return params;
}

GeoParameters3D GeoParameters3D::getHighQualityStyle()
{
    GeoParameters3D params;
    params.resetToGlobal();
    params.subdivisionLevel = Subdivision_High3D;
    params.material.type = Material_Phong3D;
    params.material.shininess = 64.0f;
    return params;
}

GeoParameters3D GeoParameters3D::getLowQualityStyle()
{
    GeoParameters3D params;
    params.resetToGlobal();
    params.subdivisionLevel = Subdivision_Low3D;
    params.material.type = Material_Basic3D;
    params.material.shininess = 16.0f;
    return params;
}

bool GeoParameters3D::validateParameters() const
{
    // 验证参数有效性
    if (pointSize <= 0.0f || lineWidth <= 0.0f) return false;
    if (pointColor.a < 0.0f || pointColor.a > 1.0f) return false;
    if (lineColor.a < 0.0f || lineColor.a > 1.0f) return false;
    if (fillColor.a < 0.0f || fillColor.a > 1.0f) return false;
    if (material.transparency < 0.0f || material.transparency > 1.0f) return false;
    if (material.shininess < 0.0f || material.shininess > 128.0f) return false;
    
    return true;
}

bool GeoParameters3D::operator==(const GeoParameters3D& other) const
{
    return pointShape == other.pointShape &&
           pointSize == other.pointSize &&
           pointColor.r == other.pointColor.r &&
           pointColor.g == other.pointColor.g &&
           pointColor.b == other.pointColor.b &&
           pointColor.a == other.pointColor.a &&
           showPoints == other.showPoints &&
           lineStyle == other.lineStyle &&
           lineWidth == other.lineWidth &&
           lineColor.r == other.lineColor.r &&
           lineColor.g == other.lineColor.g &&
           lineColor.b == other.lineColor.b &&
           lineColor.a == other.lineColor.a &&
           showEdges == other.showEdges &&
           fillType == other.fillType &&
           fillColor.r == other.fillColor.r &&
           fillColor.g == other.fillColor.g &&
           fillColor.b == other.fillColor.b &&
           fillColor.a == other.fillColor.a &&
           showFaces == other.showFaces &&
           material.type == other.material.type &&
           material.transparency == other.material.transparency &&
           subdivisionLevel == other.subdivisionLevel;
}

bool GeoParameters3D::operator!=(const GeoParameters3D& other) const
{
    return !(*this == other);
}

GeoParameters3D GeoParameters3D::lerp(const GeoParameters3D& other, float t) const
{
    GeoParameters3D result;
    
    // 插值数值属性
    result.pointSize = pointSize + (other.pointSize - pointSize) * t;
    result.lineWidth = lineWidth + (other.lineWidth - lineWidth) * t;
    
    // 插值颜色
    result.pointColor.r = pointColor.r + (other.pointColor.r - pointColor.r) * t;
    result.pointColor.g = pointColor.g + (other.pointColor.g - pointColor.g) * t;
    result.pointColor.b = pointColor.b + (other.pointColor.b - pointColor.b) * t;
    result.pointColor.a = pointColor.a + (other.pointColor.a - pointColor.a) * t;
    
    result.lineColor.r = lineColor.r + (other.lineColor.r - lineColor.r) * t;
    result.lineColor.g = lineColor.g + (other.lineColor.g - lineColor.g) * t;
    result.lineColor.b = lineColor.b + (other.lineColor.b - lineColor.b) * t;
    result.lineColor.a = lineColor.a + (other.lineColor.a - lineColor.a) * t;
    
    result.fillColor.r = fillColor.r + (other.fillColor.r - fillColor.r) * t;
    result.fillColor.g = fillColor.g + (other.fillColor.g - fillColor.g) * t;
    result.fillColor.b = fillColor.b + (other.fillColor.b - fillColor.b) * t;
    result.fillColor.a = fillColor.a + (other.fillColor.a - fillColor.a) * t;
    
    // 插值材质
    result.material.transparency = material.transparency + (other.material.transparency - material.transparency) * t;
    result.material.shininess = material.shininess + (other.material.shininess - material.shininess) * t;
    
    // 枚举类型使用阈值切换
    result.pointShape = (t < 0.5f) ? pointShape : other.pointShape;
    result.lineStyle = (t < 0.5f) ? lineStyle : other.lineStyle;
    result.fillType = (t < 0.5f) ? fillType : other.fillType;
    result.material.type = (t < 0.5f) ? material.type : other.material.type;
    result.subdivisionLevel = (t < 0.5f) ? subdivisionLevel : other.subdivisionLevel;
    
    // 布尔类型使用阈值切换
    result.showPoints = (t < 0.5f) ? showPoints : other.showPoints;
    result.showEdges = (t < 0.5f) ? showEdges : other.showEdges;
    result.showFaces = (t < 0.5f) ? showFaces : other.showFaces;
    result.showBorder = (t < 0.5f) ? showBorder : other.showBorder;
    
    return result;
}

std::string GeoParameters3D::toString() const
{
    std::stringstream ss;
    ss << "GeoParameters3D{";
    ss << "pointShape:" << static_cast<int>(pointShape) << ",";
    ss << "pointSize:" << pointSize << ",";
    ss << "pointColor:" << pointColor.r << "," << pointColor.g << "," << pointColor.b << "," << pointColor.a << ",";
    ss << "showPoints:" << showPoints << ",";
    ss << "lineStyle:" << static_cast<int>(lineStyle) << ",";
    ss << "lineWidth:" << lineWidth << ",";
    ss << "lineColor:" << lineColor.r << "," << lineColor.g << "," << lineColor.b << "," << lineColor.a << ",";
    ss << "showEdges:" << showEdges << ",";
    ss << "fillType:" << static_cast<int>(fillType) << ",";
    ss << "fillColor:" << fillColor.r << "," << fillColor.g << "," << fillColor.b << "," << fillColor.a << ",";
    ss << "showFaces:" << showFaces << ",";
    ss << "materialType:" << static_cast<int>(material.type) << ",";
    ss << "transparency:" << material.transparency << ",";
    ss << "shininess:" << material.shininess << ",";
    ss << "subdivisionLevel:" << static_cast<int>(subdivisionLevel);
    ss << "}";
    return ss.str();
}

bool GeoParameters3D::fromString(const std::string& str)
{
    // 简化的字符串解析实现
    // 在实际应用中，这里应该有完整的解析逻辑
    return false; // 暂时返回false，表示解析失败
}

// GlobalParametersManager 实现
GlobalParametersManager* GlobalParametersManager::s_instance = nullptr;

GlobalParametersManager& GlobalParametersManager::getInstance()
{
    if (!s_instance) {
        s_instance = new GlobalParametersManager();
        
        // 注册预设样式
        s_instance->registerPreset("default", GeoParameters3D::getDefaultStyle());
        s_instance->registerPreset("wireframe", GeoParameters3D::getWireframeStyle());
        s_instance->registerPreset("points", GeoParameters3D::getPointStyle());
        s_instance->registerPreset("highlight", GeoParameters3D::getHighlightStyle());
        s_instance->registerPreset("transparent", GeoParameters3D::getTransparentStyle());
        s_instance->registerPreset("high_quality", GeoParameters3D::getHighQualityStyle());
        s_instance->registerPreset("low_quality", GeoParameters3D::getLowQualityStyle());
    }
    return *s_instance;
}

void GlobalParametersManager::setAllGlobalDefaults(const GeoParameters3D& params)
{
    // 更新全局变量
    GlobalPointShape3D = params.pointShape;
    GlobalPointSize3D = params.pointSize;
    GlobalPointColor3D = QColor(params.pointColor.r * 255, params.pointColor.g * 255, params.pointColor.b * 255, params.pointColor.a * 255);
    
    GlobalLineStyle3D = params.lineStyle;
    GlobalLineWidth3D = params.lineWidth;
    GlobalLineColor3D = QColor(params.lineColor.r * 255, params.lineColor.g * 255, params.lineColor.b * 255, params.lineColor.a * 255);
    GlobalLineDashPattern3D = params.lineDashPattern;
    GlobalNodeLineStyle3D = params.nodeLineStyle;
    
    GlobalFillType3D = params.fillType;
    GlobalFillColor3D = QColor(params.fillColor.r * 255, params.fillColor.g * 255, params.fillColor.b * 255, params.fillColor.a * 255);
    GlobalBorderColor3D = QColor(params.borderColor.r * 255, params.borderColor.g * 255, params.borderColor.b * 255, params.borderColor.a * 255);
    GlobalShowBorder3D = params.showBorder;
    
    GlobalMaterialType3D = params.material.type;
    GlobalShininess3D = params.material.shininess;
    GlobalTransparency3D = params.material.transparency;
    GlobalSubdivisionLevel3D = params.subdivisionLevel;
    
    GlobalShowPoints3D = params.showPoints;
    GlobalShowEdges3D = params.showEdges;
    GlobalShowFaces3D = params.showFaces;
    
    notifyParametersChanged();
}

GeoParameters3D GlobalParametersManager::getAllGlobalDefaults() const
{
    GeoParameters3D params;
    params.resetToGlobal();
    return params;
}

void GlobalParametersManager::saveGlobalSettings(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    file << "# 3Drawing Global Parameters Configuration File\n";
    file << "# Format: key=value\n\n";
    
    // 绘制模式
    file << "draw_mode=" << static_cast<int>(GlobalDrawMode3D) << "\n";
    
    // 点属性
    file << "point_shape=" << static_cast<int>(GlobalPointShape3D) << "\n";
    file << "point_size=" << GlobalPointSize3D << "\n";
    file << "point_color_r=" << GlobalPointColor3D.red() << "\n";
    file << "point_color_g=" << GlobalPointColor3D.green() << "\n";
    file << "point_color_b=" << GlobalPointColor3D.blue() << "\n";
    file << "point_color_a=" << GlobalPointColor3D.alpha() << "\n";
    
    // 线属性
    file << "line_style=" << static_cast<int>(GlobalLineStyle3D) << "\n";
    file << "line_width=" << GlobalLineWidth3D << "\n";
    file << "line_color_r=" << GlobalLineColor3D.red() << "\n";
    file << "line_color_g=" << GlobalLineColor3D.green() << "\n";
    file << "line_color_b=" << GlobalLineColor3D.blue() << "\n";
    file << "line_color_a=" << GlobalLineColor3D.alpha() << "\n";
    file << "line_dash_pattern=" << GlobalLineDashPattern3D << "\n";
    file << "node_line_style=" << static_cast<int>(GlobalNodeLineStyle3D) << "\n";
    
    // 面属性
    file << "fill_type=" << static_cast<int>(GlobalFillType3D) << "\n";
    file << "fill_color_r=" << GlobalFillColor3D.red() << "\n";
    file << "fill_color_g=" << GlobalFillColor3D.green() << "\n";
    file << "fill_color_b=" << GlobalFillColor3D.blue() << "\n";
    file << "fill_color_a=" << GlobalFillColor3D.alpha() << "\n";
    file << "border_color_r=" << GlobalBorderColor3D.red() << "\n";
    file << "border_color_g=" << GlobalBorderColor3D.green() << "\n";
    file << "border_color_b=" << GlobalBorderColor3D.blue() << "\n";
    file << "border_color_a=" << GlobalBorderColor3D.alpha() << "\n";
    file << "show_border=" << (GlobalShowBorder3D ? 1 : 0) << "\n";
    
    // 材质属性
    file << "material_type=" << static_cast<int>(GlobalMaterialType3D) << "\n";
    file << "shininess=" << GlobalShininess3D << "\n";
    file << "transparency=" << GlobalTransparency3D << "\n";
    file << "subdivision_level=" << static_cast<int>(GlobalSubdivisionLevel3D) << "\n";
    
    // 显示控制
    file << "show_points=" << (GlobalShowPoints3D ? 1 : 0) << "\n";
    file << "show_edges=" << (GlobalShowEdges3D ? 1 : 0) << "\n";
    file << "show_faces=" << (GlobalShowFaces3D ? 1 : 0) << "\n";
    
    file.close();
}

bool GlobalParametersManager::loadGlobalSettings(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 查找等号
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // 去除首尾空格
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // 解析配置项
        if (key == "draw_mode") {
            GlobalDrawMode3D = static_cast<DrawMode3D>(std::stoi(value));
        }
        else if (key == "point_shape") {
            GlobalPointShape3D = static_cast<PointShape3D>(std::stoi(value));
        }
        else if (key == "point_size") {
            GlobalPointSize3D = std::stof(value);
        }
        else if (key == "point_color_r") {
            int r = std::stoi(value);
            GlobalPointColor3D.setRed(r);
        }
        else if (key == "point_color_g") {
            int g = std::stoi(value);
            GlobalPointColor3D.setGreen(g);
        }
        else if (key == "point_color_b") {
            int b = std::stoi(value);
            GlobalPointColor3D.setBlue(b);
        }
        else if (key == "point_color_a") {
            int a = std::stoi(value);
            GlobalPointColor3D.setAlpha(a);
        }
        else if (key == "line_style") {
            GlobalLineStyle3D = static_cast<LineStyle3D>(std::stoi(value));
        }
        else if (key == "line_width") {
            GlobalLineWidth3D = std::stof(value);
        }
        else if (key == "line_color_r") {
            int r = std::stoi(value);
            GlobalLineColor3D.setRed(r);
        }
        else if (key == "line_color_g") {
            int g = std::stoi(value);
            GlobalLineColor3D.setGreen(g);
        }
        else if (key == "line_color_b") {
            int b = std::stoi(value);
            GlobalLineColor3D.setBlue(b);
        }
        else if (key == "line_color_a") {
            int a = std::stoi(value);
            GlobalLineColor3D.setAlpha(a);
        }
        else if (key == "line_dash_pattern") {
            GlobalLineDashPattern3D = std::stof(value);
        }
        else if (key == "node_line_style") {
            GlobalNodeLineStyle3D = static_cast<NodeLineStyle3D>(std::stoi(value));
        }
        else if (key == "fill_type") {
            GlobalFillType3D = static_cast<FillType3D>(std::stoi(value));
        }
        else if (key == "fill_color_r") {
            int r = std::stoi(value);
            GlobalFillColor3D.setRed(r);
        }
        else if (key == "fill_color_g") {
            int g = std::stoi(value);
            GlobalFillColor3D.setGreen(g);
        }
        else if (key == "fill_color_b") {
            int b = std::stoi(value);
            GlobalFillColor3D.setBlue(b);
        }
        else if (key == "fill_color_a") {
            int a = std::stoi(value);
            GlobalFillColor3D.setAlpha(a);
        }
        else if (key == "border_color_r") {
            int r = std::stoi(value);
            GlobalBorderColor3D.setRed(r);
        }
        else if (key == "border_color_g") {
            int g = std::stoi(value);
            GlobalBorderColor3D.setGreen(g);
        }
        else if (key == "border_color_b") {
            int b = std::stoi(value);
            GlobalBorderColor3D.setBlue(b);
        }
        else if (key == "border_color_a") {
            int a = std::stoi(value);
            GlobalBorderColor3D.setAlpha(a);
        }
        else if (key == "show_border") {
            GlobalShowBorder3D = (std::stoi(value) != 0);
        }
        else if (key == "material_type") {
            GlobalMaterialType3D = static_cast<MaterialType3D>(std::stoi(value));
        }
        else if (key == "shininess") {
            GlobalShininess3D = std::stof(value);
        }
        else if (key == "transparency") {
            GlobalTransparency3D = std::stof(value);
        }
        else if (key == "subdivision_level") {
            GlobalSubdivisionLevel3D = static_cast<SubdivisionLevel3D>(std::stoi(value));
        }
        else if (key == "show_points") {
            GlobalShowPoints3D = (std::stoi(value) != 0);
        }
        else if (key == "show_edges") {
            GlobalShowEdges3D = (std::stoi(value) != 0);
        }
        else if (key == "show_faces") {
            GlobalShowFaces3D = (std::stoi(value) != 0);
        }
    }
    
    file.close();
    notifyParametersChanged();
    return true;
}

void GlobalParametersManager::resetToFactoryDefaults()
{
    // 恢复到出厂设置
    GlobalDrawMode3D = DrawSelect3D;
    GlobalPointShape3D = Point_Circle3D;
    GlobalPointSize3D = 5.0f;
    GlobalPointColor3D = QColor(255, 0, 0);
    
    GlobalLineStyle3D = Line_Solid3D;
    GlobalLineWidth3D = 2.0f;
    GlobalLineColor3D = QColor(0, 0, 255);
    GlobalLineDashPattern3D = 1.0f;
    GlobalNodeLineStyle3D = NodeLine_Polyline3D;
    
    GlobalFillType3D = Fill_Solid3D;
    GlobalFillColor3D = QColor(128, 128, 128);
    GlobalBorderColor3D = QColor(0, 0, 0);
    GlobalShowBorder3D = true;
    
    GlobalMaterialType3D = Material_Basic3D;
    GlobalShininess3D = 32.0f;
    GlobalTransparency3D = 1.0f;
    GlobalSubdivisionLevel3D = Subdivision_Medium3D;
    
    GlobalShowPoints3D = true;
    GlobalShowEdges3D = true;
    GlobalShowFaces3D = true;
    
    notifyParametersChanged();
}

void GlobalParametersManager::notifyParametersChanged()
{
    // 这里应该通知所有几何对象更新参数
    // 暂时留空，实际应用中需要维护对象列表
}

void GlobalParametersManager::registerPreset(const std::string& name, const GeoParameters3D& params)
{
    m_presets[name] = params;
}

GeoParameters3D GlobalParametersManager::getPreset(const std::string& name) const
{
    auto it = m_presets.find(name);
    if (it != m_presets.end()) {
        return it->second;
    }
    return GeoParameters3D::getDefaultStyle();
}

std::vector<std::string> GlobalParametersManager::getPresetNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : m_presets) {
        names.push_back(pair.first);
    }
    return names;
} 