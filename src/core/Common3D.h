#pragma once
#pragma execution_character_set("utf-8")

#include "Enums3D.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <QColor>
#include <QDebug>
#include <QString>
#include <QStatusBar>
#include <QObject>
#include <vector>
#include <filesystem>
#include <sstream>
#include <cfloat>
#include <climits>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/ref_ptr>

// 前向声明
class Geo3D;

// 全局变量声明
extern DrawMode3D GlobalDrawMode3D;
extern PointShape3D GlobalPointShape3D;
extern double GlobalPointSize3D;
extern QColor GlobalPointColor3D;

extern LineStyle3D GlobalLineStyle3D;
extern double GlobalLineWidth3D;
extern QColor GlobalLineColor3D;
extern double GlobalLineDashPattern3D;
extern NodeLineStyle3D GlobalNodeLineStyle3D;

extern FillType3D GlobalFillType3D;
extern QColor GlobalFillColor3D;
extern QColor GlobalBorderColor3D;
extern bool GlobalShowBorder3D;

extern MaterialType3D GlobalMaterialType3D;
extern double GlobalShininess3D;
extern double GlobalTransparency3D;
extern SubdivisionLevel3D GlobalSubdivisionLevel3D;

// 显示控制全局变量
extern bool GlobalShowPoints3D;
extern bool GlobalShowEdges3D;
extern bool GlobalShowFaces3D;

extern QStatusBar* GlobalStatusBar3D;

// 三维点结构
struct Point3D
{
    glm::dvec3 position;
    
    Point3D() : position(0.0, 0.0, 0.0) {}
    Point3D(double x, double y, double z) : position(x, y, z) {}
    Point3D(const glm::dvec3& pos) : position(pos) {}
    
    double x() const { return position.x; }
    double y() const { return position.y; }
    double z() const { return position.z; }
    
    void setX(double x) { position.x = x; }
    void setY(double y) { position.y = y; }
    void setZ(double z) { position.z = z; }
};

// 颜色结构（扩展QColor以支持Alpha）
struct Color3D
{
    double r, g, b, a;
    
    Color3D() : r(1.0), g(1.0), b(1.0), a(1.0) {}
    Color3D(double red, double green, double blue, double alpha = 1.0) 
        : r(red), g(green), b(blue), a(alpha) {}
    Color3D(const QColor& color) 
        : r(color.redF()), g(color.greenF()), b(color.blueF()), a(color.alphaF()) {}
    
    QColor toQColor() const { return QColor::fromRgbF(r, g, b, a); }
    glm::dvec4 toGLM() const { return glm::dvec4(r, g, b, a); }
    glm::dvec3 toGLM3() const { return glm::dvec3(r, g, b); }
    
    // 比较操作符
    bool operator==(const Color3D& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const Color3D& other) const {
        return !(*this == other);
    }
};

// 材质属性
struct Material3D
{
    Color3D ambient;      // 环境光
    Color3D diffuse;      // 漫反射
    Color3D specular;     // 镜面反射
    Color3D emission;     // 自发光
    double shininess;      // 光泽度
    double transparency;   // 透明度
    MaterialType3D type;  // 材质类型
    
    Material3D()
        : ambient(0.2, 0.2, 0.2, 1.0)
        , diffuse(0.8, 0.8, 0.8, 1.0)
        , specular(1.0, 1.0, 1.0, 1.0)
        , emission(0.0, 0.0, 0.0, 1.0)
        , shininess(32.0)
        , transparency(1.0)
        , type(Material_Basic3D)
    {}
    
    // 比较操作符
    bool operator==(const Material3D& other) const {
        return ambient == other.ambient && 
               diffuse == other.diffuse && 
               specular == other.specular && 
               emission == other.emission && 
               shininess == other.shininess && 
               transparency == other.transparency && 
               type == other.type;
    }
    bool operator!=(const Material3D& other) const {
        return !(*this == other);
    }
};

// 几何对象参数
struct GeoParameters3D
{
    // 点属性
    PointShape3D pointShape;
    double pointSize;
    Color3D pointColor;
    bool showPoints;  // 是否显示点
    
    // 线属性
    LineStyle3D lineStyle;
    double lineWidth;
    Color3D lineColor;
    double lineDashPattern;
    NodeLineStyle3D nodeLineStyle;

    bool showEdges;  // 是否显示边
    
    // 面属性
    FillType3D fillType;
    Color3D fillColor;
    Color3D borderColor;
    bool showBorder;
    bool showFaces;  // 是否显示面
    
    // 材质属性
    Material3D material;
    
    // 体属性
    SubdivisionLevel3D subdivisionLevel;
    
    // 构造函数，使用全局默认值初始化
    GeoParameters3D();
    
    // 重置为全局默认值
    void resetToGlobal();
    
    // 一次性设置所有属性
    void setAllProperties(
        const PointShape3D& pShape = Point_Circle3D,
        double pSize = 5.0,
        const Color3D& pColor = Color3D(1.0, 0.0, 0.0, 1.0),
        bool showPts = true,
        
        const LineStyle3D& lStyle = Line_Solid3D,
        double lWidth = 2.0,
        const Color3D& lColor = Color3D(0.0, 0.0, 1.0, 1.0),
        double lDashPattern = 1.0,
        const NodeLineStyle3D& nlStyle = NodeLine_Polyline3D,
        bool showEdgs = true,
        
        const FillType3D& fType = Fill_Solid3D,
        const Color3D& fColor = Color3D(0.5, 0.5, 0.5, 1.0),
        const Color3D& bColor = Color3D(0.0, 0.0, 0.0, 1.0),
        bool showBrd = true,
        bool showFcs = true,
        
        const MaterialType3D& mType = Material_Basic3D,
        double shininess = 32.0,
        double transparency = 1.0,
        const SubdivisionLevel3D& subLevel = Subdivision_Medium3D
    );
    
    // 快速设置预设样式
    void setPresetStyle(const std::string& styleName);
    
    // 预设样式
    static GeoParameters3D getDefaultStyle();
    static GeoParameters3D getWireframeStyle();
    static GeoParameters3D getPointStyle();
    static GeoParameters3D getHighlightStyle();
    static GeoParameters3D getTransparentStyle();
    static GeoParameters3D getHighQualityStyle();
    static GeoParameters3D getLowQualityStyle();
    
    // 属性验证
    bool validateParameters() const;
    
    // 参数比较
    bool operator==(const GeoParameters3D& other) const;
    bool operator!=(const GeoParameters3D& other) const;
    
    // 参数混合（用于动画或渐变）
    GeoParameters3D lerp(const GeoParameters3D& other, double t) const;
    
    // 保存/加载到字符串（用于配置文件）
    std::string toString() const;
    bool fromString(const std::string& str);
};

// 全局参数管理类
class GlobalParametersManager
{
public:
    // 获取单例实例
    static GlobalParametersManager& getInstance();
    
    // 全局设置
    void setAllGlobalDefaults(const GeoParameters3D& params);
    GeoParameters3D getAllGlobalDefaults() const;
    
    // 保存/加载全局设置
    void saveGlobalSettings(const std::string& filename);
    bool loadGlobalSettings(const std::string& filename);
    
    // 重置为默认值
    void resetToFactoryDefaults();
    
    // 通知所有对象更新参数
    void notifyParametersChanged();
    
    // 预设管理
    void registerPreset(const std::string& name, const GeoParameters3D& params);
    GeoParameters3D getPreset(const std::string& name) const;
    std::vector<std::string> getPresetNames() const;
    
private:
    GlobalParametersManager() = default;
    std::map<std::string, GeoParameters3D> m_presets;
    static GlobalParametersManager* s_instance;
};

// 变换矩阵
struct Transform3D
{
    glm::dvec3 translation;
    glm::dvec3 rotation;    // 欧拉角（度）
    glm::dvec3 scale;
    
    Transform3D()
        : translation(0.0), rotation(0.0), scale(1.0)
    {}
    
    glm::dmat4 getMatrix() const
    {
        glm::dmat4 t = glm::translate(glm::dmat4(1.0), translation);
        glm::dmat4 rx = glm::rotate(glm::dmat4(1.0), glm::radians(rotation.x), glm::dvec3(1, 0, 0));
        glm::dmat4 ry = glm::rotate(glm::dmat4(1.0), glm::radians(rotation.y), glm::dvec3(0, 1, 0));
        glm::dmat4 rz = glm::rotate(glm::dmat4(1.0), glm::radians(rotation.z), glm::dvec3(0, 0, 1));
        glm::dmat4 s = glm::scale(glm::dmat4(1.0), scale);
        
        return t * rz * ry * rx * s;
    }
};

// 包围盒
struct BoundingBox3D
{
    glm::dvec3 min;
    glm::dvec3 max;
    
    BoundingBox3D() : min(FLT_MAX), max(-FLT_MAX) {}
    BoundingBox3D(const glm::dvec3& minPos, const glm::dvec3& maxPos) : min(minPos), max(maxPos) {}
    
    void expand(const glm::dvec3& point)
    {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    
    void expand(const BoundingBox3D& box)
    {
        min = glm::min(min, box.min);
        max = glm::max(max, box.max);
    }
    
    glm::dvec3 center() const { return (min + max) * 0.5; }
    glm::dvec3 size() const { return max - min; }
    bool isValid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
};

// 射线结构（用于拾取）
struct Ray3D
{
    glm::dvec3 origin;
    glm::dvec3 direction;
    
    Ray3D() : origin(0.0), direction(0.0, 0.0, -1.0) {}
    Ray3D(const glm::dvec3& orig, const glm::dvec3& dir) : origin(orig), direction(glm::normalize(dir)) {}
    
    glm::dvec3 pointAt(double t) const { return origin + t * direction; }
};

/*
// 日志辅助类
class LogHelper3D
{
public:
    inline LogHelper3D(const char* file, int line)
        : file_(file), line_(line) {}

    template <typename T>
    LogHelper3D& operator<<(const T& value)
    {
        stream_ << value;
        return *this;
    }

    inline LogHelper3D& operator<<(const QString& value) {
        stream_ << value.toStdString();
        return *this;
    }

    inline ~LogHelper3D()
    {
        qDebug() << "3DLog:" << QString::fromStdString(stream_.str())
                << ", file:" << QString::fromStdString(std::filesystem::path(file_).filename().string()) 
                << ", line:" << line_;
    }

private:
    std::ostringstream stream_;
    const char* file_;
    int line_;
};

#define Log3D LogHelper3D(__FILE__, __LINE__)
*/

// 初始化全局设置
void initializeGlobal3DSettings();

// 实用函数
QString drawMode3DToString(DrawMode3D mode);
QString pointShape3DToString(PointShape3D shape);
QString lineStyle3DToString(LineStyle3D style);
QString nodeLineStyle3DToString(NodeLineStyle3D style);
QString fillType3DToString(FillType3D type);
QString materialType3DToString(MaterialType3D type);

DrawMode3D stringToDrawMode3D(const QString& str);
PointShape3D stringToPointShape3D(const QString& str);
LineStyle3D stringToLineStyle3D(const QString& str);
NodeLineStyle3D stringToNodeLineStyle3D(const QString& str);
FillType3D stringToFillType3D(const QString& str);
MaterialType3D stringToMaterialType3D(const QString& str); 




