#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "../Enums3D.h"
#include <glm/glm.hpp>
#include <osg/Vec3>
#include <osg/BoundingBox>
#include <QObject>
#include <QString>

// 坐标系统范围管理类
class CoordinateSystem3D : public QObject
{
    Q_OBJECT

public:
    // 坐标范围结构
    struct CoordinateRange
    {
        double minX, maxX;
        double minY, maxY;
        double minZ, maxZ;
        
        CoordinateRange()
            : minX(-1e7), maxX(1e7)
            , minY(-1e7), maxY(1e7)
            , minZ(-1e7), maxZ(1e7)
        {}
        
        CoordinateRange(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
            : minX(xMin), maxX(xMax)
            , minY(yMin), maxY(yMax)
            , minZ(zMin), maxZ(zMax)
        {}
        
        // 检查点是否在范围内
        bool contains(const glm::vec3& point) const
        {
            return point.x >= minX && point.x <= maxX &&
                   point.y >= minY && point.y <= maxY &&
                   point.z >= minZ && point.z <= maxZ;
        }
        
        // 获取范围大小
        glm::vec3 size() const
        {
            return glm::vec3(maxX - minX, maxY - minY, maxZ - minZ);
        }
        
        // 获取中心点
        glm::vec3 center() const
        {
            return glm::vec3((minX + maxX) * 0.5, (minY + maxY) * 0.5, (minZ + maxZ) * 0.5);
        }
        
        // 获取最大范围
        double maxRange() const
        {
            return std::max({maxX - minX, maxY - minY, maxZ - minZ});
        }
    };
    
    // 预设范围类型
    enum PresetRange
    {
        Range_Small,        // 小范围 (1km)
        Range_Medium,       // 中等范围 (100km)
        Range_Large,        // 大范围 (1000km)
        Range_City,         // 城市范围 (50km)
        Range_Country,      // 国家范围 (5000km)
        Range_Continent,    // 大陆范围 (10000km)
        Range_Earth,        // 地球范围 (12742km)
        Range_Custom        // 自定义范围
    };

public:
    static CoordinateSystem3D* getInstance();
    
    // 设置坐标范围
    void setCoordinateRange(const CoordinateRange& range);
    void setCoordinateRange(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
    
    // 获取当前坐标范围
    const CoordinateRange& getCoordinateRange() const { return m_currentRange; }
    
    // 设置预设范围
    void setPresetRange(PresetRange preset);
    
    // 设置天空盒范围（与坐标范围绑定）
    void setSkyboxRange(const CoordinateRange& range);
    void setSkyboxRange(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
    
    // 获取天空盒范围
    const CoordinateRange& getSkyboxRange() const { return m_skyboxRange; }
    
    // 检查点是否在天空盒范围内
    bool isPointInSkyboxRange(const glm::vec3& point) const;
    bool isPointInSkyboxRange(const osg::Vec3& point) const;
    
    // 限制点到天空盒范围
    glm::vec3 clampPointToSkybox(const glm::vec3& point) const;
    osg::Vec3 clampPointToSkybox(const osg::Vec3& point) const;
    
    // 检查点是否在有效范围内
    bool isValidPoint(const glm::vec3& point) const;
    bool isValidPoint(const osg::Vec3& point) const;
    
    // 限制点到有效范围
    glm::vec3 clampPoint(const glm::vec3& point) const;
    osg::Vec3 clampPoint(const osg::Vec3& point) const;
    
    // 获取范围信息
    QString getRangeInfo() const;
    QString getSkyboxRangeInfo() const;
    
    // 设置是否启用范围限制
    void setRangeLimitEnabled(bool enabled) { m_rangeLimitEnabled = enabled; }
    bool isRangeLimitEnabled() const { return m_rangeLimitEnabled; }
    
    // 设置是否绑定天空盒范围
    void setSkyboxRangeBinding(bool enabled) { m_skyboxRangeBinding = enabled; }
    bool isSkyboxRangeBinding() const { return m_skyboxRangeBinding; }
    
    // 坐标系设置
    void setCoordinateSystemType(CoordinateSystemType3D type) { m_coordSystemType = type; emit coordinateSystemTypeChanged(type); }
    CoordinateSystemType3D getCoordinateSystemType() const { return m_coordSystemType; }
    
    void setAxisVisible(CoordinateAxis3D axis, bool visible);
    bool isAxisVisible(CoordinateAxis3D axis) const;
    
    void setGridVisible(bool visible) { m_gridVisible = visible; emit gridVisibleChanged(visible); }
    bool isGridVisible() const { return m_gridVisible; }
    
    // 网格平面设置
    void setGridPlaneVisible(GridPlane3D plane, bool visible);
    bool isGridPlaneVisible(GridPlane3D plane) const;
    
    void setScaleUnit(ScaleUnit3D unit) { m_scaleUnit = unit; emit scaleUnitChanged(unit); }
    ScaleUnit3D getScaleUnit() const { return m_scaleUnit; }
    
    void setCustomUnitName(const QString& name) { m_customUnitName = name; emit customUnitNameChanged(name); }
    QString getCustomUnitName() const { return m_customUnitName; }
    
    void setScaleInterval(double interval) { m_scaleInterval = interval; emit scaleIntervalChanged(interval); }
    double getScaleInterval() const { return m_scaleInterval; }
    
    void setAxisLength(double length) { m_axisLength = length; emit axisLengthChanged(length); }
    double getAxisLength() const { return m_axisLength; }
    
    void setAxisThickness(double thickness) { m_axisThickness = thickness; emit axisThicknessChanged(thickness); }
    double getAxisThickness() const { return m_axisThickness; }
    
    void setGridSpacing(double spacing) { m_gridSpacing = spacing; emit gridSpacingChanged(spacing); }
    double getGridSpacing() const { return m_gridSpacing; }
    
    void setGridThickness(double thickness) { m_gridThickness = thickness; emit gridThicknessChanged(thickness); }
    double getGridThickness() const { return m_gridThickness; }
    
    // 字体设置
    void setFontSize(FontSize3D size) { m_fontSize = size; emit fontSizeChanged(size); }
    FontSize3D getFontSize() const { return m_fontSize; }
    
    void setCustomFontSize(double size) { m_customFontSize = size; emit customFontSizeChanged(size); }
    double getCustomFontSize() const { return m_customFontSize; }
    
    // 获取字体大小
    double getActualFontSize() const;
    
    // 获取单位名称
    QString getUnitName() const;
    
    // 获取预设范围信息
    static QString getPresetRangeName(PresetRange preset);
    static CoordinateRange getPresetRange(PresetRange preset);

signals:
    void coordinateRangeChanged(const CoordinateRange& range);
    void skyboxRangeChanged(const CoordinateRange& range);
    void rangeLimitEnabledChanged(bool enabled);
    void skyboxRangeBindingChanged(bool enabled);
    
    // 坐标系相关信号
    void coordinateSystemTypeChanged(CoordinateSystemType3D type);
    void axisVisibleChanged(CoordinateAxis3D axis, bool visible);
    void gridVisibleChanged(bool visible);
    void gridPlaneVisibleChanged(GridPlane3D plane, bool visible);
    void scaleUnitChanged(ScaleUnit3D unit);
    void customUnitNameChanged(const QString& name);
    void scaleIntervalChanged(double interval);
    void axisLengthChanged(double length);
    void axisThicknessChanged(double thickness);
    void gridSpacingChanged(double spacing);
    void gridThicknessChanged(double thickness);
    void fontSizeChanged(FontSize3D size);
    void customFontSizeChanged(double size);

private:
    CoordinateSystem3D(QObject* parent = nullptr);
    ~CoordinateSystem3D() = default;
    
    // 单例实例
    static CoordinateSystem3D* s_instance;
    
    // 当前坐标范围
    CoordinateRange m_currentRange;
    
    // 天空盒范围
    CoordinateRange m_skyboxRange;
    
    // 是否启用范围限制
    bool m_rangeLimitEnabled;
    
    // 是否绑定天空盒范围
    bool m_skyboxRangeBinding;
    
    // 坐标系设置
    CoordinateSystemType3D m_coordSystemType;
    bool m_axisVisible[3];  // X, Y, Z轴可见性
    bool m_gridVisible;
    bool m_gridPlaneVisible[3];  // XY, YZ, XZ平面可见性
    ScaleUnit3D m_scaleUnit;
    QString m_customUnitName;
    double m_scaleInterval;
    double m_axisLength;
    double m_axisThickness;
    double m_gridSpacing;
    double m_gridThickness;
    FontSize3D m_fontSize;
    double m_customFontSize;
    
    // 更新天空盒范围（当绑定启用时）
    void updateSkyboxRange();
}; 