#include "Common3D.h"
#include "../util/LogManager.h"
#include <fstream>
#include <string>
#include <sstream> // Required for std::ostringstream and std::istringstream
#include <vector> // Required for std::vector
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>

// 全局变量定义
DrawMode3D GlobalDrawMode3D = DrawSelect3D;
PointShape3D GlobalPointShape3D = Point_Dot3D;
double GlobalPointSize3D = 2.0;
QColor GlobalPointColor3D = QColor(255, 0, 0);

LineStyle3D GlobalLineStyle3D = Line_Solid3D;
double GlobalLineWidth3D = 1.0;  // 与根节点保持一致，更好地展示抗锯齿效果
QColor GlobalLineColor3D = QColor(0, 0, 255);
double GlobalLineDashPattern3D = 1.0;

FillType3D GlobalFillType3D = Fill_Solid3D;
QColor GlobalFillColor3D = QColor(128, 128, 128);

MaterialType3D GlobalMaterialType3D = Material_Basic3D;
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
    pointColor = GlobalPointColor3D;
    showPoints = GlobalShowPoints3D;
    
    // 线属性
    lineStyle = GlobalLineStyle3D;
    lineWidth = GlobalLineWidth3D;
    lineColor = GlobalLineColor3D;
    lineDashPattern = GlobalLineDashPattern3D;
    showEdges = GlobalShowEdges3D;
    
    // 面属性
    fillType = GlobalFillType3D;
    fillColor = GlobalFillColor3D;
    showFaces = GlobalShowFaces3D;
    
    // 材质属性
    material.type = GlobalMaterialType3D;
    
    // 体属性
    subdivisionLevel = GlobalSubdivisionLevel3D;
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
        case DrawBezierCurve3D: return "贝塞尔曲线";
        case DrawTriangle3D: return "三角形";
        case DrawQuad3D: return "四边形";
        case DrawPolygon3D: return "多边形";
        case DrawBox3D: return "长方体";
        case DrawCube3D: return "正方体";
        case DrawCone3D: return "圆锥";
        case DrawCylinder3D: return "圆柱";
        case DrawPrism3D: return "多棱柱";
        case DrawTorus3D: return "圆环";
        case DrawSphere3D: return "球";
        case DrawHemisphere3D: return "半球";
        case DrawEllipsoid3D: return "椭球";
        case DrawGableHouse3D: return "人字房";
        case DrawSpireHouse3D: return "尖顶房";
        case DrawDomeHouse3D: return "穹顶房";
        case DrawFlatHouse3D: return "平顶房";
        case DrawLHouse3D: return "L型房";
        default: return "未知";
    }
}

QString pointShape3DToString(PointShape3D shape)
{
    switch (shape)
    {
        case Point_Dot3D: return "圆点";
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

QString fillType3DToString(FillType3D type)
{
    switch (type)
    {
        case Fill_None3D: return "无填充";
        case Fill_Solid3D: return "实心填充";
        case Fill_Wireframe3D: return "线框";
        default: return "未知";
    }
}

QString materialType3DToString(MaterialType3D type)
{
    switch (type)
    {
        case Material_Basic3D: return "基础材质";
        case Material_Phong3D: return "Phong材质";
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
    if (str == "贝塞尔曲线") return DrawBezierCurve3D;
    if (str == "三角形") return DrawTriangle3D;
    if (str == "四边形") return DrawQuad3D;
    if (str == "多边形") return DrawPolygon3D;
    if (str == "长方体") return DrawBox3D;
    if (str == "正方体") return DrawCube3D;
    if (str == "圆锥") return DrawCone3D;
    if (str == "圆柱") return DrawCylinder3D;
    if (str == "多棱柱") return DrawPrism3D;
    if (str == "圆环") return DrawTorus3D;
    if (str == "球") return DrawSphere3D;
    if (str == "半球") return DrawHemisphere3D;
    if (str == "椭球") return DrawEllipsoid3D;
    if (str == "人字房") return DrawGableHouse3D;
    if (str == "尖顶房") return DrawSpireHouse3D;
    if (str == "穹顶房") return DrawDomeHouse3D;
    if (str == "平顶房") return DrawFlatHouse3D;
    if (str == "L型房") return DrawLHouse3D;
    return DrawSelect3D;
}

PointShape3D stringToPointShape3D(const QString& str)
{
    if (str == "圆点") return Point_Dot3D;
    if (str == "圆形") return Point_Circle3D;
    if (str == "方形") return Point_Square3D;
    if (str == "三角形") return Point_Triangle3D;
    if (str == "菱形") return Point_Diamond3D;
    if (str == "十字") return Point_Cross3D;
    if (str == "星形") return Point_Star3D;
    return Point_Dot3D;
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

FillType3D stringToFillType3D(const QString& str)
{
    if (str == "无填充") return Fill_None3D;
    if (str == "实心填充") return Fill_Solid3D;
    if (str == "线框") return Fill_Wireframe3D;
    return Fill_Solid3D;
}

MaterialType3D stringToMaterialType3D(const QString& str)
{
    if (str == "基础材质") return Material_Basic3D;
    if (str == "Phong材质") return Material_Phong3D;
    return Material_Basic3D;
}

// 显示约束：确保至少有一个组件可见
void GeoParameters3D::enforceVisibilityConstraint()
{
    if (!showPoints && !showEdges && !showFaces) {
        // 如果全部隐藏，强制显示线框
        showEdges = true;
    }
}

// 修复setAllProperties函数的参数
void GeoParameters3D::setAllProperties(
    const PointShape3D& pShape,
    double pSize,
    const Color3D& pColor,
    bool showPts,
    
    const LineStyle3D& lStyle,
    double lWidth,
    const Color3D& lColor,
    double lDashPattern,
    bool showEdgs,
    
    const FillType3D& fType,
    const Color3D& fColor,
    bool showFcs,
    
    const MaterialType3D& mType,
    const SubdivisionLevel3D& subLevel
)
{
    pointShape = pShape;
    pointSize = pSize;
    pointColor = pColor;
    showPoints = showPts;
    
    // 线属性
    lineStyle = lStyle;
    lineWidth = lWidth;
    lineColor = lColor;
    lineDashPattern = lDashPattern;
    showEdges = showEdgs;
    
    // 面属性
    fillType = fType;
    fillColor = fColor;
    showFaces = showFcs;
    
    // 材质属性
    material.type = mType;
    
    // 体属性
    subdivisionLevel = subLevel;
    
    // 应用显示约束
    enforceVisibilityConstraint();
}

// 预设样式
GeoParameters3D GeoParameters3D::getDefaultStyle()
{
    GeoParameters3D params;
    return params;
}

GeoParameters3D GeoParameters3D::getWireframeStyle()
{
    GeoParameters3D params;
    params.fillType = Fill_Wireframe3D;
    params.showPoints = true;
    params.showEdges = true;
    params.showFaces = false;
    return params;
}

GeoParameters3D GeoParameters3D::getPointStyle()
{
    GeoParameters3D params;
    params.showPoints = true;
    params.showEdges = false;
    params.showFaces = false;
    return params;
}

GeoParameters3D GeoParameters3D::getTransparentStyle()
{
    GeoParameters3D params;
    // 透明度现在直接在颜色的alpha通道中设置
    params.pointColor.a = 0.5;
    params.lineColor.a = 0.5;
    params.fillColor.a = 0.5;
    return params;
}

GeoParameters3D GeoParameters3D::getHighQualityStyle()
{
    GeoParameters3D params;
    params.subdivisionLevel = Subdivision_Ultra3D;
    params.material.type = Material_Phong3D;
    return params;
}

GeoParameters3D GeoParameters3D::getLowQualityStyle()
{
    GeoParameters3D params;
    params.subdivisionLevel = Subdivision_Low3D;
    params.material.type = Material_Basic3D;
    return params;
}

// 参数验证
bool GeoParameters3D::validateParameters() const
{
    if (pointSize <= 0.0) return false;
    if (lineWidth <= 0.0) return false;
    if (material.transparency < 0.0 || material.transparency > 1.0) return false;
    return true;
}

// 参数比较
bool GeoParameters3D::operator==(const GeoParameters3D& other) const
{
    return pointShape == other.pointShape &&
           pointSize == other.pointSize &&
           pointColor == other.pointColor &&
           showPoints == other.showPoints &&
           
           lineStyle == other.lineStyle &&
           lineWidth == other.lineWidth &&
           lineColor == other.lineColor &&
           lineDashPattern == other.lineDashPattern &&
           showEdges == other.showEdges &&
           
           fillType == other.fillType &&
           fillColor == other.fillColor &&
           showFaces == other.showFaces &&
           
           material == other.material &&
           subdivisionLevel == other.subdivisionLevel;
}

bool GeoParameters3D::operator!=(const GeoParameters3D& other) const
{
    return !(*this == other);
}

// 参数混合（用于动画或渐变）
GeoParameters3D GeoParameters3D::lerp(const GeoParameters3D& other, double t) const
{
    GeoParameters3D result = *this;
    
    t = std::max(0.0, std::min(1.0, t)); // 确保t在[0,1]范围内
    
    // 线性插值数值属性
    result.pointSize = pointSize + (other.pointSize - pointSize) * t;
    result.lineWidth = lineWidth + (other.lineWidth - lineWidth) * t;
    result.lineDashPattern = lineDashPattern + (other.lineDashPattern - lineDashPattern) * t;
    result.material.transparency = material.transparency + (other.material.transparency - material.transparency) * t;
    
    // 颜色插值
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
    
    // 对于枚举类型，使用阈值切换
    if (t >= 0.5) {
        result.pointShape = other.pointShape;
        result.lineStyle = other.lineStyle;
        result.fillType = other.fillType;
        result.material.type = other.material.type;
        result.subdivisionLevel = other.subdivisionLevel;
        result.showPoints = other.showPoints;
        result.showEdges = other.showEdges;
        result.showFaces = other.showFaces;
    }
    
    return result;
}

// 保存/加载到字符串（用于配置文件）
std::string GeoParameters3D::toString() const
{
    // 使用分号分隔各个参数
    std::ostringstream oss;
    
    // 点属性
    oss << static_cast<int>(pointShape) << ";" << pointSize << ";" 
        << pointColor.r << ";" << pointColor.g << ";" << pointColor.b << ";" << pointColor.a << ";"
        << (showPoints ? 1 : 0) << ";";
    
    // 线属性  
    oss << static_cast<int>(lineStyle) << ";" << lineWidth << ";"
        << lineColor.r << ";" << lineColor.g << ";" << lineColor.b << ";" << lineColor.a << ";"
        << lineDashPattern << ";" << (showEdges ? 1 : 0) << ";";
    
    // 面属性
    oss << static_cast<int>(fillType) << ";"
        << fillColor.r << ";" << fillColor.g << ";" << fillColor.b << ";" << fillColor.a << ";"
        << (showFaces ? 1 : 0) << ";";
    
    // 材质属性
    oss << static_cast<int>(material.type) << ";" << material.transparency << ";";
    
    // 体属性
    oss << static_cast<int>(subdivisionLevel);
    
    return oss.str();
}

bool GeoParameters3D::fromString(const std::string& str)
{
    if (str.empty()) {
        // 如果字符串为空，使用默认参数
        *this = getDefaultStyle();
        return true;
    }
    
    std::istringstream iss(str);
    std::string token;
    std::vector<std::string> tokens;
    
    // 分割字符串
    while (std::getline(iss, token, ';')) {
        tokens.push_back(token);
    }
    
    // 检查token数量（应该有24个参数）
    if (tokens.size() < 24) {
        LOG_WARNING("参数数据不完整，使用默认值补充", "参数反序列化");
        *this = getDefaultStyle();
        
        // 尝试解析可用的参数
        size_t idx = 0;
        try {
            if (idx < tokens.size()) pointShape = static_cast<PointShape3D>(std::stoi(tokens[idx++]));
            if (idx < tokens.size()) pointSize = std::stod(tokens[idx++]);
            if (idx < tokens.size()) pointColor.r = std::stod(tokens[idx++]);
            if (idx < tokens.size()) pointColor.g = std::stod(tokens[idx++]);
            if (idx < tokens.size()) pointColor.b = std::stod(tokens[idx++]);
            if (idx < tokens.size()) pointColor.a = std::stod(tokens[idx++]);
            if (idx < tokens.size()) showPoints = (std::stoi(tokens[idx++]) != 0);
        } catch (const std::exception& e) {
            LOG_WARNING("部分参数解析失败，使用默认值", "参数反序列化");
        }
        return true;
    }
    
    try {
        size_t idx = 0;
        
        // 点属性
        pointShape = static_cast<PointShape3D>(std::stoi(tokens[idx++]));
        pointSize = std::stod(tokens[idx++]);
        pointColor.r = std::stod(tokens[idx++]);
        pointColor.g = std::stod(tokens[idx++]);
        pointColor.b = std::stod(tokens[idx++]);
        pointColor.a = std::stod(tokens[idx++]);
        showPoints = (std::stoi(tokens[idx++]) != 0);
        
        // 线属性
        lineStyle = static_cast<LineStyle3D>(std::stoi(tokens[idx++]));
        lineWidth = std::stod(tokens[idx++]);
        lineColor.r = std::stod(tokens[idx++]);
        lineColor.g = std::stod(tokens[idx++]);
        lineColor.b = std::stod(tokens[idx++]);
        lineColor.a = std::stod(tokens[idx++]);
        lineDashPattern = std::stod(tokens[idx++]);
        showEdges = (std::stoi(tokens[idx++]) != 0);
        
        // 面属性
        fillType = static_cast<FillType3D>(std::stoi(tokens[idx++]));
        fillColor.r = std::stod(tokens[idx++]);
        fillColor.g = std::stod(tokens[idx++]);
        fillColor.b = std::stod(tokens[idx++]);
        fillColor.a = std::stod(tokens[idx++]);
        showFaces = (std::stoi(tokens[idx++]) != 0);
        
        // 材质属性
        material.type = static_cast<MaterialType3D>(std::stoi(tokens[idx++]));
        material.transparency = std::stod(tokens[idx++]);
        
        // 体属性
        subdivisionLevel = static_cast<SubdivisionLevel3D>(std::stoi(tokens[idx++]));
        
        // 验证参数有效性
        if (!validateParameters()) {
            LOG_WARNING("反序列化的参数无效，重置为默认值", "参数反序列化");
            *this = getDefaultStyle();
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("参数反序列化失败，使用默认值", "参数反序列化");
        *this = getDefaultStyle();
        return false;
    }
}

void GeoParameters3D::setPresetStyle(const std::string& styleName)
{
    if (styleName == "default") {
        *this = getDefaultStyle();
    } else if (styleName == "wireframe") {
        *this = getWireframeStyle();
    } else if (styleName == "point") {
        *this = getPointStyle();
    } else if (styleName == "transparent") {
        *this = getTransparentStyle();
    } else if (styleName == "high_quality") {
        *this = getHighQualityStyle();
    } else if (styleName == "low_quality") {
        *this = getLowQualityStyle();
    }
}

// 全局参数管理器实现
GlobalParametersManager* GlobalParametersManager::s_instance = nullptr;

// 静态常量定义
const QString GlobalParametersManager::DEFAULT_CONFIG_FILENAME = "3d_drawing_config.json";
const QString GlobalParametersManager::CONFIG_SUBDIRECTORY = "3DDrawing";

GlobalParametersManager& GlobalParametersManager::getInstance()
{
    if (!s_instance) {
        s_instance = new GlobalParametersManager();
    }
    return *s_instance;
}

void GlobalParametersManager::setAllGlobalDefaults(const GeoParameters3D& params)
{
    GlobalPointShape3D = params.pointShape;
    GlobalPointSize3D = params.pointSize;
    GlobalPointColor3D = params.pointColor.toQColor();
    GlobalShowPoints3D = params.showPoints;
    
    GlobalLineStyle3D = params.lineStyle;
    GlobalLineWidth3D = params.lineWidth;
    GlobalLineColor3D = params.lineColor.toQColor();
    GlobalLineDashPattern3D = params.lineDashPattern;
    GlobalShowEdges3D = params.showEdges;
    
    GlobalFillType3D = params.fillType;
    GlobalFillColor3D = params.fillColor.toQColor();
    GlobalShowFaces3D = params.showFaces;
    
    GlobalMaterialType3D = params.material.type;
    GlobalSubdivisionLevel3D = params.subdivisionLevel;
    
    LOG_INFO("全局默认参数已更新", "配置管理");
}

GeoParameters3D GlobalParametersManager::getAllGlobalDefaults() const
{
    GeoParameters3D params;
    params.resetToGlobal();
    return params;
}

QString GlobalParametersManager::getConfigDirectory() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(configDir).absoluteFilePath(CONFIG_SUBDIRECTORY);
}

QString GlobalParametersManager::getDefaultConfigPath() const
{
    return QDir(getConfigDirectory()).absoluteFilePath(DEFAULT_CONFIG_FILENAME);
}

bool GlobalParametersManager::ensureConfigDirectoryExists()
{
    QDir dir(getConfigDirectory());
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            LOG_ERROR("无法创建配置目录: " + dir.absolutePath(), "配置管理");
            return false;
        }
        LOG_INFO("已创建配置目录: " + dir.absolutePath(), "配置管理");
    }
    return true;
}

QJsonObject GlobalParametersManager::colorToJson(const Color3D& color) const
{
    QJsonObject colorObj;
    colorObj["r"] = color.r;
    colorObj["g"] = color.g;
    colorObj["b"] = color.b;
    colorObj["a"] = color.a;
    return colorObj;
}

Color3D GlobalParametersManager::colorFromJson(const QJsonObject& colorObj) const
{
    Color3D color;
    color.r = colorObj["r"].toDouble(1.0);
    color.g = colorObj["g"].toDouble(1.0);
    color.b = colorObj["b"].toDouble(1.0);
    color.a = colorObj["a"].toDouble(1.0);
    return color;
}

QJsonObject GlobalParametersManager::materialToJson(const Material3D& material) const
{
    QJsonObject materialObj;
    materialObj["ambient"] = colorToJson(material.ambient);
    materialObj["diffuse"] = colorToJson(material.diffuse);
    materialObj["specular"] = colorToJson(material.specular);
    materialObj["emission"] = colorToJson(material.emission);
    materialObj["shininess"] = material.shininess;
    materialObj["transparency"] = material.transparency;
    materialObj["type"] = static_cast<int>(material.type);
    return materialObj;
}

Material3D GlobalParametersManager::materialFromJson(const QJsonObject& materialObj) const
{
    Material3D material;
    if (materialObj.contains("ambient")) {
        material.ambient = colorFromJson(materialObj["ambient"].toObject());
    }
    if (materialObj.contains("diffuse")) {
        material.diffuse = colorFromJson(materialObj["diffuse"].toObject());
    }
    if (materialObj.contains("specular")) {
        material.specular = colorFromJson(materialObj["specular"].toObject());
    }
    if (materialObj.contains("emission")) {
        material.emission = colorFromJson(materialObj["emission"].toObject());
    }
    material.shininess = materialObj["shininess"].toDouble(32.0);
    material.transparency = materialObj["transparency"].toDouble(1.0);
    material.type = static_cast<MaterialType3D>(materialObj["type"].toInt(Material_Basic3D));
    return material;
}

QJsonObject GlobalParametersManager::parametersToJson(const GeoParameters3D& params) const
{
    QJsonObject jsonObj;
    
    // 点属性
    QJsonObject pointObj;
    pointObj["shape"] = static_cast<int>(params.pointShape);
    pointObj["size"] = params.pointSize;
    pointObj["color"] = colorToJson(params.pointColor);
    pointObj["show"] = params.showPoints;
    jsonObj["point"] = pointObj;
    
    // 线属性
    QJsonObject lineObj;
    lineObj["style"] = static_cast<int>(params.lineStyle);
    lineObj["width"] = params.lineWidth;
    lineObj["color"] = colorToJson(params.lineColor);
    lineObj["dashPattern"] = params.lineDashPattern;
    lineObj["show"] = params.showEdges;
    jsonObj["line"] = lineObj;
    
    // 面属性
    QJsonObject fillObj;
    fillObj["type"] = static_cast<int>(params.fillType);
    fillObj["color"] = colorToJson(params.fillColor);
    fillObj["show"] = params.showFaces;
    jsonObj["fill"] = fillObj;
    
    // 材质属性
    jsonObj["material"] = materialToJson(params.material);
    
    // 体属性
    jsonObj["subdivisionLevel"] = static_cast<int>(params.subdivisionLevel);
    
    return jsonObj;
}

GeoParameters3D GlobalParametersManager::parametersFromJson(const QJsonObject& jsonObj) const
{
    GeoParameters3D params;
    
    // 点属性
    if (jsonObj.contains("point")) {
        QJsonObject pointObj = jsonObj["point"].toObject();
        params.pointShape = static_cast<PointShape3D>(pointObj["shape"].toInt(Point_Dot3D));
        params.pointSize = pointObj["size"].toDouble(2.0);
        if (pointObj.contains("color")) {
            params.pointColor = colorFromJson(pointObj["color"].toObject());
        }
        params.showPoints = pointObj["show"].toBool(true);
    }
    
    // 线属性
    if (jsonObj.contains("line")) {
        QJsonObject lineObj = jsonObj["line"].toObject();
        params.lineStyle = static_cast<LineStyle3D>(lineObj["style"].toInt(Line_Solid3D));
        params.lineWidth = lineObj["width"].toDouble(1.0);
        if (lineObj.contains("color")) {
            params.lineColor = colorFromJson(lineObj["color"].toObject());
        }
        params.lineDashPattern = lineObj["dashPattern"].toDouble(1.0);
        params.showEdges = lineObj["show"].toBool(true);
    }
    
    // 面属性
    if (jsonObj.contains("fill")) {
        QJsonObject fillObj = jsonObj["fill"].toObject();
        params.fillType = static_cast<FillType3D>(fillObj["type"].toInt(Fill_Solid3D));
        if (fillObj.contains("color")) {
            params.fillColor = colorFromJson(fillObj["color"].toObject());
        }
        params.showFaces = fillObj["show"].toBool(true);
    }
    
    // 材质属性
    if (jsonObj.contains("material")) {
        params.material = materialFromJson(jsonObj["material"].toObject());
    }
    
    // 体属性
    params.subdivisionLevel = static_cast<SubdivisionLevel3D>(
        jsonObj["subdivisionLevel"].toInt(Subdivision_Medium3D));
    
    return params;
}

bool GlobalParametersManager::validateConfig(const QJsonObject& config) const
{
    // 检查基本结构
    if (!config.contains("version") || !config.contains("globalParameters")) {
        return false;
    }
    
    // 检查版本兼容性
    QString version = config["version"].toString();
    if (version.isEmpty()) {
        return false;
    }
    
    // 检查全局参数结构
    QJsonObject globalParams = config["globalParameters"].toObject();
    if (!globalParams.contains("point") || !globalParams.contains("line") || 
        !globalParams.contains("fill")) {
        return false;
    }
    
    return true;
}

void GlobalParametersManager::repairInvalidConfig(QJsonObject& config) const
{
    // 确保版本信息存在
    if (!config.contains("version")) {
        config["version"] = "1.0.0";
    }
    
    // 确保全局参数存在
    if (!config.contains("globalParameters")) {
        GeoParameters3D defaultParams;
        config["globalParameters"] = parametersToJson(defaultParams);
    }
    
    // 确保预设参数存在
    if (!config.contains("presets")) {
        config["presets"] = QJsonObject();
    }
    
    LOG_INFO("配置文件已修复", "配置管理");
}

void GlobalParametersManager::saveGlobalSettings(const std::string& filename)
{
    QString configPath;
    if (filename.empty()) {
        if (!ensureConfigDirectoryExists()) {
            return;
        }
        configPath = getDefaultConfigPath();
    } else {
        configPath = QString::fromStdString(filename);
    }
    
    try {
        // 创建配置JSON对象
        QJsonObject config;
        config["version"] = "1.0.0";
        config["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        config["application"] = QCoreApplication::applicationName();
        
        // 保存全局参数
        GeoParameters3D globalParams = getAllGlobalDefaults();
        config["globalParameters"] = parametersToJson(globalParams);
        
        // 保存预设
        QJsonObject presetsObj;
        for (const auto& pair : m_presets) {
            presetsObj[QString::fromStdString(pair.first)] = parametersToJson(pair.second);
        }
        config["presets"] = presetsObj;
        
        // 写入文件
        QJsonDocument doc(config);
        QFile file(configPath);
        if (!file.open(QIODevice::WriteOnly)) {
            LOG_ERROR("无法打开配置文件进行写入: " + configPath, "配置管理");
            return;
        }
        
        file.write(doc.toJson());
        file.close();
        
        LOG_INFO("配置已保存到: " + configPath, "配置管理");
        
    } catch (const std::exception& e) {
        LOG_ERROR("保存配置时发生异常: " + QString::fromStdString(e.what()), "配置管理");
    }
}

bool GlobalParametersManager::loadGlobalSettings(const std::string& filename)
{
    QString configPath;
    if (filename.empty()) {
        configPath = getDefaultConfigPath();
    } else {
        configPath = QString::fromStdString(filename);
    }
    
    QFile file(configPath);
    if (!file.exists()) {
        LOG_INFO("配置文件不存在，使用默认设置: " + configPath, "配置管理");
        resetToFactoryDefaults();
        return true;  // 不算错误，使用默认值
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR("无法打开配置文件: " + configPath, "配置管理");
        resetToFactoryDefaults();
        return false;
    }
    
    try {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            LOG_ERROR("配置文件JSON解析错误: " + parseError.errorString(), "配置管理");
            resetToFactoryDefaults();
            return false;
        }
        
        QJsonObject config = doc.object();
        
        // 验证和修复配置
        if (!validateConfig(config)) {
            LOG_WARNING("配置文件格式无效，正在修复", "配置管理");
            repairInvalidConfig(config);
        }
        
        // 加载全局参数
        if (config.contains("globalParameters")) {
            GeoParameters3D params = parametersFromJson(config["globalParameters"].toObject());
            setAllGlobalDefaults(params);
        }
        
        // 加载预设
        if (config.contains("presets")) {
            QJsonObject presetsObj = config["presets"].toObject();
            m_presets.clear();
            
            for (auto it = presetsObj.begin(); it != presetsObj.end(); ++it) {
                GeoParameters3D preset = parametersFromJson(it.value().toObject());
                m_presets[it.key().toStdString()] = preset;
            }
        }
        
        LOG_INFO("配置已从文件加载: " + configPath, "配置管理");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("加载配置时发生异常: " + QString::fromStdString(e.what()), "配置管理");
        resetToFactoryDefaults();
        return false;
    }
}

bool GlobalParametersManager::autoLoadSettings()
{
    bool success = loadGlobalSettings();
    
    // 注册默认预设（如果没有的话）
    if (m_presets.empty()) {
        registerPreset("default", GeoParameters3D::getDefaultStyle());
        registerPreset("wireframe", GeoParameters3D::getWireframeStyle());
        registerPreset("point", GeoParameters3D::getPointStyle());
        registerPreset("transparent", GeoParameters3D::getTransparentStyle());
        registerPreset("high_quality", GeoParameters3D::getHighQualityStyle());
        registerPreset("low_quality", GeoParameters3D::getLowQualityStyle());
        
        LOG_INFO("已注册默认预设样式", "配置管理");
    }
    
    return success;
}

void GlobalParametersManager::autoSaveSettings()
{
    saveGlobalSettings();
}

QString GlobalParametersManager::getConfigInfo() const
{
    QString info;
    info += QString("配置目录: %1\n").arg(getConfigDirectory());
    info += QString("配置文件: %1\n").arg(getDefaultConfigPath());
    info += QString("预设数量: %1\n").arg(static_cast<int>(m_presets.size()));
    
    QFile configFile(getDefaultConfigPath());
    if (configFile.exists()) {
        info += QString("配置文件大小: %1 字节\n").arg(configFile.size());
        info += QString("最后修改: %1\n").arg(
            QFileInfo(configFile).lastModified().toString(Qt::ISODate));
    } else {
        info += "配置文件不存在\n";
    }
    
    return info;
}

void GlobalParametersManager::resetToFactoryDefaults()
{
    GlobalDrawMode3D = DrawSelect3D;
    GlobalPointShape3D = Point_Dot3D;
    GlobalPointSize3D = 2.0;
    GlobalPointColor3D = QColor(255, 0, 0);
    
    GlobalLineStyle3D = Line_Solid3D;
    GlobalLineWidth3D = 1.0;
    GlobalLineColor3D = QColor(0, 0, 255);
    GlobalLineDashPattern3D = 1.0;
    
    GlobalFillType3D = Fill_Solid3D;
    GlobalFillColor3D = QColor(128, 128, 128);
    
    GlobalMaterialType3D = Material_Basic3D;
    GlobalSubdivisionLevel3D = Subdivision_Medium3D;
    
    GlobalShowPoints3D = true;
    GlobalShowEdges3D = true;
    GlobalShowFaces3D = true;
    
    LOG_INFO("已重置为出厂默认设置", "配置管理");
}

void GlobalParametersManager::notifyParametersChanged()
{
    LOG_INFO("全局参数已更改，通知相关组件", "配置管理");
    // 这里可以添加信号发射或回调通知机制
}

void GlobalParametersManager::registerPreset(const std::string& name, const GeoParameters3D& params)
{
    m_presets[name] = params;
    LOG_INFO("已注册预设样式: " + QString::fromStdString(name), "配置管理");
}

GeoParameters3D GlobalParametersManager::getPreset(const std::string& name) const
{
    auto it = m_presets.find(name);
    if (it != m_presets.end()) {
        return it->second;
    }
    LOG_WARNING("未找到预设样式: " + QString::fromStdString(name), "配置管理");
    return GeoParameters3D();
}

std::vector<std::string> GlobalParametersManager::getPresetNames() const
{
    std::vector<std::string> names;
    for (const auto& pair : m_presets) {
        names.push_back(pair.first);
    }
    return names;
}

// Config3D命名空间实现
namespace Config3D
{
    bool initializeConfigSystem()
    {
        try {
            LOG_INFO("初始化配置系统", "配置管理");
            
            // 获取配置管理器实例并加载设置
            auto& manager = GlobalParametersManager::getInstance();
            bool success = manager.autoLoadSettings();
            
            if (success) {
                LOG_INFO("配置系统初始化成功", "配置管理");
            } else {
                LOG_WARNING("配置系统初始化时使用了默认设置", "配置管理");
            }
            
            return success;
            
        } catch (const std::exception& e) {
            LOG_ERROR("配置系统初始化失败: " + QString::fromStdString(e.what()), "配置管理");
            return false;
        }
    }
    
    void finalizeConfigSystem()
    {
        try {
            LOG_INFO("保存配置并关闭配置系统", "配置管理");
            
            auto& manager = GlobalParametersManager::getInstance();
            manager.autoSaveSettings();
            
            LOG_INFO("配置系统已关闭", "配置管理");
            
        } catch (const std::exception& e) {
            LOG_ERROR("配置系统关闭时发生错误: " + QString::fromStdString(e.what()), "配置管理");
        }
    }
    
    GeoParameters3D getCurrentGlobalConfig()
    {
        auto& manager = GlobalParametersManager::getInstance();
        return manager.getAllGlobalDefaults();
    }
    
    void setCurrentGlobalConfig(const GeoParameters3D& config)
    {
        auto& manager = GlobalParametersManager::getInstance();
        manager.setAllGlobalDefaults(config);
        manager.notifyParametersChanged();
    }
    
    bool loadPreset(const std::string& presetName)
    {
        try {
            auto& manager = GlobalParametersManager::getInstance();
            GeoParameters3D preset = manager.getPreset(presetName);
            
            // 检查预设是否有效（通过验证参数）
            if (preset.validateParameters()) {
                manager.setAllGlobalDefaults(preset);
                manager.notifyParametersChanged();
                LOG_INFO("已加载预设: " + QString::fromStdString(presetName), "配置管理");
                return true;
            } else {
                LOG_WARNING("预设无效: " + QString::fromStdString(presetName), "配置管理");
                return false;
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("加载预设时发生错误: " + QString::fromStdString(e.what()), "配置管理");
            return false;
        }
    }
    
    bool saveCurrentAsPreset(const std::string& presetName)
    {
        try {
            auto& manager = GlobalParametersManager::getInstance();
            GeoParameters3D currentConfig = manager.getAllGlobalDefaults();
            
            manager.registerPreset(presetName, currentConfig);
            manager.autoSaveSettings();  // 立即保存到文件
            
            LOG_INFO("当前配置已保存为预设: " + QString::fromStdString(presetName), "配置管理");
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR("保存预设时发生错误: " + QString::fromStdString(e.what()), "配置管理");
            return false;
        }
    }
    
    std::vector<std::string> getAvailablePresets()
    {
        auto& manager = GlobalParametersManager::getInstance();
        return manager.getPresetNames();
    }
    
    bool exportConfig(const QString& filePath)
    {
        try {
            auto& manager = GlobalParametersManager::getInstance();
            manager.saveGlobalSettings(filePath.toStdString());
            LOG_INFO("配置已导出到: " + filePath, "配置管理");
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR("导出配置时发生错误: " + QString::fromStdString(e.what()), "配置管理");
            return false;
        }
    }
    
    bool importConfig(const QString& filePath)
    {
        try {
            auto& manager = GlobalParametersManager::getInstance();
            bool success = manager.loadGlobalSettings(filePath.toStdString());
            
            if (success) {
                manager.notifyParametersChanged();
                LOG_INFO("配置已从文件导入: " + filePath, "配置管理");
            } else {
                LOG_ERROR("导入配置失败: " + filePath, "配置管理");
            }
            
            return success;
            
        } catch (const std::exception& e) {
            LOG_ERROR("导入配置时发生错误: " + QString::fromStdString(e.what()), "配置管理");
            return false;
        }
    }
    
    QString getConfigurationInfo()
    {
        auto& manager = GlobalParametersManager::getInstance();
        return manager.getConfigInfo();
    }
    
    void resetToDefaults()
    {
        try {
            auto& manager = GlobalParametersManager::getInstance();
            manager.resetToFactoryDefaults();
            manager.notifyParametersChanged();
            manager.autoSaveSettings();  // 立即保存默认设置
            
            LOG_INFO("配置已重置为默认值", "配置管理");
            
        } catch (const std::exception& e) {
            LOG_ERROR("重置配置时发生错误: " + QString::fromStdString(e.what()), "配置管理");
        }
    }
} 



