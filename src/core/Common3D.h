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
extern float GlobalPointSize3D;
extern QColor GlobalPointColor3D;

extern LineStyle3D GlobalLineStyle3D;
extern float GlobalLineWidth3D;
extern QColor GlobalLineColor3D;
extern float GlobalLineDashPattern3D;
extern NodeLineStyle3D GlobalNodeLineStyle3D;

extern FillType3D GlobalFillType3D;
extern QColor GlobalFillColor3D;
extern QColor GlobalBorderColor3D;
extern bool GlobalShowBorder3D;

extern MaterialType3D GlobalMaterialType3D;
extern float GlobalShininess3D;
extern float GlobalTransparency3D;
extern SubdivisionLevel3D GlobalSubdivisionLevel3D;

// 显示控制全局变量
extern bool GlobalShowPoints3D;
extern bool GlobalShowEdges3D;
extern bool GlobalShowFaces3D;

extern QStatusBar* GlobalStatusBar3D;

// 三维点结构
struct Point3D
{
    glm::vec3 position;
    
    Point3D() : position(0.0f, 0.0f, 0.0f) {}
    Point3D(float x, float y, float z) : position(x, y, z) {}
    Point3D(const glm::vec3& pos) : position(pos) {}
    
    float x() const { return position.x; }
    float y() const { return position.y; }
    float z() const { return position.z; }
    
    void setX(float x) { position.x = x; }
    void setY(float y) { position.y = y; }
    void setZ(float z) { position.z = z; }
};

// 颜色结构（扩展QColor以支持Alpha）
struct Color3D
{
    float r, g, b, a;
    
    Color3D() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color3D(float red, float green, float blue, float alpha = 1.0f) 
        : r(red), g(green), b(blue), a(alpha) {}
    Color3D(const QColor& color) 
        : r(color.redF()), g(color.greenF()), b(color.blueF()), a(color.alphaF()) {}
    
    QColor toQColor() const { return QColor::fromRgbF(r, g, b, a); }
    glm::vec4 toGLM() const { return glm::vec4(r, g, b, a); }
    glm::vec3 toGLM3() const { return glm::vec3(r, g, b); }
};

// 材质属性
struct Material3D
{
    Color3D ambient;      // 环境光
    Color3D diffuse;      // 漫反射
    Color3D specular;     // 镜面反射
    Color3D emission;     // 自发光
    float shininess;      // 光泽度
    float transparency;   // 透明度
    MaterialType3D type;  // 材质类型
    
    Material3D()
        : ambient(0.2f, 0.2f, 0.2f, 1.0f)
        , diffuse(0.8f, 0.8f, 0.8f, 1.0f)
        , specular(1.0f, 1.0f, 1.0f, 1.0f)
        , emission(0.0f, 0.0f, 0.0f, 1.0f)
        , shininess(32.0f)
        , transparency(1.0f)
        , type(Material_Basic3D)
    {}
};

// 几何对象参数
struct GeoParameters3D
{
    // 点属性
    PointShape3D pointShape;
    float pointSize;
    Color3D pointColor;
    bool showPoints;  // 是否显示点
    
    // 线属性
    LineStyle3D lineStyle;
    float lineWidth;
    Color3D lineColor;
    float lineDashPattern;
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
    
    // 样条曲线属性
    int splineOrder;
    int splineNodeCount;
    int steps;
    
    // 构造函数，使用全局默认值初始化
    GeoParameters3D();
    
    // 重置为全局默认值
    void resetToGlobal();
    
    // 一次性设置所有属性
    void setAllProperties(
        const PointShape3D& pShape = Point_Circle3D,
        float pSize = 5.0f,
        const Color3D& pColor = Color3D(1.0f, 0.0f, 0.0f, 1.0f),
        bool showPts = true,
        
        const LineStyle3D& lStyle = Line_Solid3D,
        float lWidth = 2.0f,
        const Color3D& lColor = Color3D(0.0f, 0.0f, 1.0f, 1.0f),
        float lDashPattern = 1.0f,
        const NodeLineStyle3D& nlStyle = NodeLine_Polyline3D,
        bool showEdgs = true,
        
        const FillType3D& fType = Fill_Solid3D,
        const Color3D& fColor = Color3D(0.5f, 0.5f, 0.5f, 1.0f),
        const Color3D& bColor = Color3D(0.0f, 0.0f, 0.0f, 1.0f),
        bool showBrd = true,
        bool showFcs = true,
        
        const MaterialType3D& mType = Material_Basic3D,
        float shininess = 32.0f,
        float transparency = 1.0f,
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
    GeoParameters3D lerp(const GeoParameters3D& other, float t) const;
    
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
    glm::vec3 translation;
    glm::vec3 rotation;    // 欧拉角（度）
    glm::vec3 scale;
    
    Transform3D()
        : translation(0.0f), rotation(0.0f), scale(1.0f)
    {}
    
    glm::mat4 getMatrix() const
    {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0));
        glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0));
        glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        
        return t * rz * ry * rx * s;
    }
};

// 包围盒
struct BoundingBox3D
{
    glm::vec3 min;
    glm::vec3 max;
    
    BoundingBox3D() : min(FLT_MAX), max(-FLT_MAX) {}
    BoundingBox3D(const glm::vec3& minPos, const glm::vec3& maxPos) : min(minPos), max(maxPos) {}
    
    void expand(const glm::vec3& point)
    {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    
    void expand(const BoundingBox3D& box)
    {
        min = glm::min(min, box.min);
        max = glm::max(max, box.max);
    }
    
    glm::vec3 center() const { return (min + max) * 0.5f; }
    glm::vec3 size() const { return max - min; }
    bool isValid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
};

// 射线结构（用于拾取）
struct Ray3D
{
    glm::vec3 origin;
    glm::vec3 direction;
    
    Ray3D() : origin(0.0f), direction(0.0f, 0.0f, -1.0f) {}
    Ray3D(const glm::vec3& orig, const glm::vec3& dir) : origin(orig), direction(glm::normalize(dir)) {}
    
    glm::vec3 pointAt(float t) const { return origin + t * direction; }
};

// 拾取结果
struct PickResult3D
{
    bool hit;
    float distance;
    glm::vec3 point;
    glm::vec3 normal;
    void* userData;  // 可以存储几何对象指针
    
    // KDTree相关成员
    Geo3D* geoObject;  // 几何对象指针
    int geometryType;   // 几何体类型 (0:点, 1:线, 2:面)
    int geometryIndex;  // 几何体索引
    
    PickResult3D() : hit(false), distance(FLT_MAX), point(0.0f), normal(0.0f), 
                     userData(nullptr), geoObject(nullptr), geometryType(0), geometryIndex(-1) {}
};

// 拾取Feature结构
struct PickingFeature
{
    FeatureType type;
    uint32_t index;                    // Feature在该类型中的索引
    osg::ref_ptr<osg::Geometry> geometry;  // 该Feature的几何体
    osg::Vec3 center;                  // Feature中心点(用于指示器定位)
    float size;                        // Feature大小(用于指示器缩放)
    
    PickingFeature(FeatureType t, uint32_t idx) 
        : type(t), index(idx), center(0,0,0), size(1.0f) {}
};

// 指示器配置
struct IndicatorConfig
{
    float size;                 // 指示器大小
    osg::Vec4 color;           // 颜色
    float lineWidth;           // 线宽
    float animationSpeed;      // 动画速度
    bool enableAnimation;      // 是否启用动画
    float fadeTime;            // 淡入淡出时间
    
    IndicatorConfig()
        : size(0.1f)
        , color(1.0f, 1.0f, 0.0f, 1.0f)  // 黄色
        , lineWidth(3.0f)
        , animationSpeed(2.0f)
        , enableAnimation(true)
        , fadeTime(0.3f)
    {}
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