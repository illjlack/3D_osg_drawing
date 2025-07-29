#pragma once
#pragma execution_character_set("utf-8")

#include "Types3D.h"
#include "Enums3D.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <QStandardPaths>
#include <vector>
#include <string>

namespace Geo3D {

// 几何对象参数配置
struct GeoParameters3D
{
    // 点属性
    PointShape3D pointShape;
    double pointSize;
    Color3D pointColor;
    bool showPoints;
    
    // 线属性
    LineStyle3D lineStyle;
    double lineWidth;
    Color3D lineColor;
    double lineDashPattern;
    bool showEdges;
    
    // 面属性
    FillType3D fillType;
    Color3D fillColor;
    bool showFaces;
    
    // 材质属性
    Material3D material;
    
    // 体属性
    SubdivisionLevel3D subdivisionLevel;
    
    // 构造函数，使用默认值初始化
    GeoParameters3D()
        : pointShape(Point_Dot3D)
        , pointSize(5.0)
        , pointColor(1.0, 0.0, 0.0, 1.0)
        , showPoints(true)
        , lineStyle(Line_Solid3D)
        , lineWidth(2.0)
        , lineColor(0.0, 0.0, 1.0, 1.0)
        , lineDashPattern(1.0)
        , showEdges(true)
        , fillType(Fill_Solid3D)
        , fillColor(0.5, 0.5, 0.5, 1.0)
        , showFaces(true)
        , subdivisionLevel(Subdivision_Medium3D)
    {}
    
    // 重置为默认值
    void resetToDefault();
    
    // 一次性设置所有属性
    void setAllProperties(
        const PointShape3D& pShape = Point_Dot3D,
        double pSize = 5.0,
        const Color3D& pColor = Color3D(1.0, 0.0, 0.0, 1.0),
        bool showPts = true,
        
        const LineStyle3D& lStyle = Line_Solid3D,
        double lWidth = 2.0,
        const Color3D& lColor = Color3D(0.0, 0.0, 1.0, 1.0),
        double lDashPattern = 1.0,
        bool showEdgs = true,
        
        const FillType3D& fType = Fill_Solid3D,
        const Color3D& fColor = Color3D(0.5, 0.5, 0.5, 1.0),
        bool showFcs = true,
        
        const MaterialType3D& mType = Material_Basic3D,
        const SubdivisionLevel3D& subLevel = Subdivision_Medium3D
    );
    
    // 显示约束：确保至少有一个组件可见
    void enforceVisibilityConstraint();
    
    // 快速设置预设样式
    void setPresetStyle(const std::string& styleName);
    
    // 预设样式
    static GeoParameters3D getDefaultStyle();
    static GeoParameters3D getWireframeStyle();
    static GeoParameters3D getPointStyle();
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
    
    // JSON序列化支持
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);
};

// 全局参数管理类（只管理geo3D模块内的参数）
class ParametersManager
{
public:
    // 获取单例实例
    static ParametersManager& getInstance();
    
    // 默认参数设置
    void setDefaultParameters(const GeoParameters3D& params);
    GeoParameters3D getDefaultParameters() const;
    
    // 预设管理
    void registerPreset(const std::string& name, const GeoParameters3D& params);
    GeoParameters3D getPreset(const std::string& name) const;
    std::vector<std::string> getPresetNames() const;
    bool hasPreset(const std::string& name) const;
    void removePreset(const std::string& name);
    
    // 配置文件管理
    void saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    
    // 重置为出厂默认值
    void resetToFactoryDefaults();
    
    // 初始化内置预设
    void initializeBuiltinPresets();
    
private:
    ParametersManager();
    ~ParametersManager() = default;
    ParametersManager(const ParametersManager&) = delete;
    ParametersManager& operator=(const ParametersManager&) = delete;
    
    GeoParameters3D m_defaultParams;
    std::map<std::string, GeoParameters3D> m_presets;
};

// 渲染配置
struct RenderConfig3D
{
    // 质量设置
    bool enableAntiAliasing;
    bool enableShadows;
    bool enableReflections;
    int multiSamplingLevel;
    
    // 性能设置
    bool enableLevelOfDetail;
    bool enableFrustumCulling;
    bool enableBackfaceCulling;
    int maxRenderDistance;
    
    // 调试设置
    bool showBoundingBoxes;
    bool showNormals;
    bool showWireframe;
    bool enableProfiling;
    
    RenderConfig3D()
        : enableAntiAliasing(true)
        , enableShadows(false)
        , enableReflections(false)
        , multiSamplingLevel(4)
        , enableLevelOfDetail(true)
        , enableFrustumCulling(true)
        , enableBackfaceCulling(true)
        , maxRenderDistance(1000)
        , showBoundingBoxes(false)
        , showNormals(false)
        , showWireframe(false)
        , enableProfiling(false)
    {}
};

// 导出设置
struct ExportConfig3D
{
    // 文件格式设置
    bool exportTextures;
    bool exportMaterials;
    bool exportAnimations;
    bool optimizeMesh;
    
    // 质量设置
    double tessellationTolerance;
    int maxVerticesPerObject;
    bool mergeVertices;
    double vertexMergeTolerance;
    
    ExportConfig3D()
        : exportTextures(true)
        , exportMaterials(true)
        , exportAnimations(false)
        , optimizeMesh(true)
        , tessellationTolerance(0.01)
        , maxVerticesPerObject(100000)
        , mergeVertices(true)
        , vertexMergeTolerance(1e-6)
    {}
};

} // namespace Geo3D 