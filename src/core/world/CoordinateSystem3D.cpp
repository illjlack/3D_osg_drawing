#include "CoordinateSystem3D.h"
#include <QDebug>
#include <algorithm>
#include <cmath>

// 单例实例
CoordinateSystem3D* CoordinateSystem3D::s_instance = nullptr;

CoordinateSystem3D::CoordinateSystem3D(QObject* parent)
    : QObject(parent)
    , m_currentRange()
    , m_skyboxRange()
    , m_rangeLimitEnabled(true)
    , m_skyboxRangeBinding(true)
    , m_coordSystemType(CoordSystem_Axis3D)
    , m_gridVisible(true)
    , m_scaleUnit(Unit_Meter3D)
    , m_customUnitName("单位")
    , m_scaleInterval(1000.0)
    , m_axisLength(10000.0) // 增加默认轴长度
    , m_axisThickness(3.0)  // 增加轴线粗细
    , m_gridSpacing(500.0)  // 减小网格间距，更密集的网格
    , m_gridThickness(1.5)  // 增加网格线粗细
    , m_fontSize(FontSize_Medium3D)
    , m_customFontSize(120.0) // 增加默认字体大小
{
    // 默认设置为城市范围，更合理的初始范围
    setPresetRange(Range_City);
    
    // 默认显示所有轴
    m_axisVisible[0] = true;  // X轴
    m_axisVisible[1] = true;  // Y轴
    m_axisVisible[2] = true;  // Z轴
    
    // 默认显示所有网格平面
    m_gridPlaneVisible[0] = true;  // XY平面
    m_gridPlaneVisible[1] = true;  // YZ平面
    m_gridPlaneVisible[2] = true;  // XZ平面
}

CoordinateSystem3D* CoordinateSystem3D::getInstance()
{
    if (!s_instance)
    {
        s_instance = new CoordinateSystem3D();
    }
    return s_instance;
}

void CoordinateSystem3D::setCoordinateRange(const CoordinateRange& range)
{
    if (m_currentRange.minX != range.minX || m_currentRange.maxX != range.maxX ||
        m_currentRange.minY != range.minY || m_currentRange.maxY != range.maxY ||
        m_currentRange.minZ != range.minZ || m_currentRange.maxZ != range.maxZ)
    {
        m_currentRange = range;
        
        // 如果启用了天空盒范围绑定，则更新天空盒范围
        if (m_skyboxRangeBinding)
        {
            updateSkyboxRange();
        }
        
        emit coordinateRangeChanged(m_currentRange);
        
        qDebug() << "坐标范围已更新:" << getRangeInfo();
    }
}

void CoordinateSystem3D::setCoordinateRange(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
    setCoordinateRange(CoordinateRange(xMin, xMax, yMin, yMax, zMin, zMax));
}

void CoordinateSystem3D::setPresetRange(PresetRange preset)
{
    CoordinateRange range = getPresetRange(preset);
    setCoordinateRange(range);
}

void CoordinateSystem3D::setSkyboxRange(const CoordinateRange& range)
{
    if (m_skyboxRange.minX != range.minX || m_skyboxRange.maxX != range.maxX ||
        m_skyboxRange.minY != range.minY || m_skyboxRange.maxY != range.maxY ||
        m_skyboxRange.minZ != range.minZ || m_skyboxRange.maxZ != range.maxZ)
    {
        m_skyboxRange = range;
        emit skyboxRangeChanged(m_skyboxRange);
        
        qDebug() << "天空盒范围已更新:" << getSkyboxRangeInfo();
    }
}

void CoordinateSystem3D::setSkyboxRange(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax)
{
    setSkyboxRange(CoordinateRange(xMin, xMax, yMin, yMax, zMin, zMax));
}

bool CoordinateSystem3D::isValidPoint(const glm::dvec3& point) const
{
    if (!m_rangeLimitEnabled)
        return true;
    
    // 修改：检查点是否在天空盒范围内，而不是坐标范围内
    return m_skyboxRange.contains(point);
}

bool CoordinateSystem3D::isValidPoint(const osg::Vec3& point) const
{
    return isValidPoint(glm::dvec3(point.x(), point.y(), point.z()));
}

bool CoordinateSystem3D::isPointInSkyboxRange(const glm::dvec3& point) const
{
    return m_skyboxRange.contains(point);
}

bool CoordinateSystem3D::isPointInSkyboxRange(const osg::Vec3& point) const
{
    return isPointInSkyboxRange(glm::dvec3(point.x(), point.y(), point.z()));
}

glm::dvec3 CoordinateSystem3D::clampPoint(const glm::dvec3& point) const
{
    if (!m_rangeLimitEnabled)
        return point;
    
    // 修改：限制点到天空盒范围，而不是坐标范围
    return clampPointToSkybox(point);
}

osg::Vec3 CoordinateSystem3D::clampPoint(const osg::Vec3& point) const
{
    glm::dvec3 clamped = clampPoint(glm::dvec3(point.x(), point.y(), point.z()));
    return osg::Vec3(clamped.x, clamped.y, clamped.z);
}

glm::dvec3 CoordinateSystem3D::clampPointToSkybox(const glm::dvec3& point) const
{
    return glm::dvec3(
        std::max(static_cast<double>(m_skyboxRange.minX), std::min(static_cast<double>(m_skyboxRange.maxX), point.x)),
        std::max(static_cast<double>(m_skyboxRange.minY), std::min(static_cast<double>(m_skyboxRange.maxY), point.y)),
        std::max(static_cast<double>(m_skyboxRange.minZ), std::min(static_cast<double>(m_skyboxRange.maxZ), point.z))
    );
}

osg::Vec3 CoordinateSystem3D::clampPointToSkybox(const osg::Vec3& point) const
{
    glm::dvec3 clamped = clampPointToSkybox(glm::dvec3(point.x(), point.y(), point.z()));
    return osg::Vec3(clamped.x, clamped.y, clamped.z);
}

QString CoordinateSystem3D::getRangeInfo() const
{
    return QString("X: [%1, %2], Y: [%3, %4], Z: [%5, %6]")
        .arg(m_currentRange.minX, 0, 'e', 2)
        .arg(m_currentRange.maxX, 0, 'e', 2)
        .arg(m_currentRange.minY, 0, 'e', 2)
        .arg(m_currentRange.maxY, 0, 'e', 2)
        .arg(m_currentRange.minZ, 0, 'e', 2)
        .arg(m_currentRange.maxZ, 0, 'e', 2);
}

QString CoordinateSystem3D::getSkyboxRangeInfo() const
{
    return QString("X: [%1, %2], Y: [%3, %4], Z: [%5, %6]")
        .arg(m_skyboxRange.minX, 0, 'e', 2)
        .arg(m_skyboxRange.maxX, 0, 'e', 2)
        .arg(m_skyboxRange.minY, 0, 'e', 2)
        .arg(m_skyboxRange.maxY, 0, 'e', 2)
        .arg(m_skyboxRange.minZ, 0, 'e', 2)
        .arg(m_skyboxRange.maxZ, 0, 'e', 2);
}

void CoordinateSystem3D::updateSkyboxRange()
{
    // 对于坐标系统边界，使用更合理的边距计算
    double maxRange = m_currentRange.maxRange();
    
    // 改进的边距计算：使用更大的边距确保天空盒足够大
    double marginRatio;
    double minMargin = 1000.0; // 最小边距1000单位
    
    if (maxRange < 1000.0)
    {
        marginRatio = 2.0; // 200%边距，对小范围给予更大空间
    }
    else if (maxRange < 5000.0)
    {
        marginRatio = 1.0; // 100%边距，适合小到中等范围
    }
    else if (maxRange < 10000.0)
    {
        marginRatio = 0.5; // 50%边距，适合中等范围
    }
    else if (maxRange < 50000.0)
    {
        marginRatio = 0.3; // 30%边距，适合大范围
    }
    else
    {
        marginRatio = 0.2; // 20%边距，适合超大范围
    }
    
    double margin = std::max(maxRange * marginRatio, minMargin);
    
    CoordinateRange skyboxRange(
        m_currentRange.minX - margin,
        m_currentRange.maxX + margin,
        m_currentRange.minY - margin,
        m_currentRange.maxY + margin,
        m_currentRange.minZ - margin,
        m_currentRange.maxZ + margin
    );
    
    setSkyboxRange(skyboxRange);
    
    // 自动调整坐标轴长度以覆盖整个坐标范围
    double coordMaxRange = m_currentRange.maxRange();
    double newAxisLength = coordMaxRange * 1.5; // 使用坐标范围的150%作为轴长度，确保轴长度足够
    
    // 确保轴长度至少为5000单位
    newAxisLength = std::max(newAxisLength, 5000.0);
    
    // 只有当轴长度需要显著调整时才更新
    if (std::abs(m_axisLength - newAxisLength) > 100.0)
    {
        m_axisLength = newAxisLength;
        emit axisLengthChanged(m_axisLength);
        
        qDebug() << "自动调整轴长度为:" << m_axisLength << "坐标范围:" << coordMaxRange;
    }
    
    // 自动调整网格间距
    double newGridSpacing = coordMaxRange / 20.0; // 将坐标范围分为20个网格
    newGridSpacing = std::max(newGridSpacing, 10.0); // 最小网格间距10单位
    
    // 网格间距使用合理的步长
    if (newGridSpacing < 100.0)
        newGridSpacing = std::round(newGridSpacing / 10.0) * 10.0;
    else if (newGridSpacing < 1000.0)
        newGridSpacing = std::round(newGridSpacing / 50.0) * 50.0;
    else
        newGridSpacing = std::round(newGridSpacing / 100.0) * 100.0;
    
    if (std::abs(m_gridSpacing - newGridSpacing) > 1.0)
    {
        m_gridSpacing = newGridSpacing;
        emit gridSpacingChanged(m_gridSpacing);
        
        qDebug() << "自动调整网格间距为:" << m_gridSpacing;
    }
}

QString CoordinateSystem3D::getPresetRangeName(PresetRange preset)
{
    switch (preset)
    {
        case Range_Small: return "小范围 (1km)";
        case Range_Medium: return "中等范围 (100km)";
        case Range_Large: return "大范围 (1000km)";
        case Range_City: return "城市范围 (50km)";
        case Range_Country: return "国家范围 (5000km)";
        case Range_Continent: return "大陆范围 (10000km)";
        case Range_Earth: return "地球范围 (12742km)";
        case Range_Custom: return "自定义范围";
        default: return "未知范围";
    }
}

CoordinateSystem3D::CoordinateRange CoordinateSystem3D::getPresetRange(PresetRange preset)
{
    switch (preset)
    {
        case Range_Small:
            return CoordinateRange(-500, 500, -500, 500, -500, 500); // 1km范围
            
        case Range_Medium:
            return CoordinateRange(-50000, 50000, -50000, 50000, -50000, 50000); // 100km范围
            
        case Range_Large:
            return CoordinateRange(-500000, 500000, -500000, 500000, -500000, 500000); // 1000km范围
            
        case Range_City:
            return CoordinateRange(-25000, 25000, -25000, 25000, -25000, 25000); // 50km范围
            
        case Range_Country:
            return CoordinateRange(-2500000, 2500000, -2500000, 2500000, -2500000, 2500000); // 5000km范围
            
        case Range_Continent:
            return CoordinateRange(-5000000, 5000000, -5000000, 5000000, -5000000, 5000000); // 10000km范围
            
        case Range_Earth:
            return CoordinateRange(-6371000, 6371000, -6371000, 6371000, -6371000, 6371000); // 地球直径范围
            
        case Range_Custom:
        default:
            return CoordinateRange(-1e7, 1e7, -1e7, 1e7, -1e7, 1e7); // 默认大范围
    }
}

void CoordinateSystem3D::setAxisVisible(CoordinateAxis3D axis, bool visible)
{
    switch (axis)
    {
        case Axis_X3D:
            if (m_axisVisible[0] != visible)
            {
                m_axisVisible[0] = visible;
                emit axisVisibleChanged(axis, visible);
            }
            break;
        case Axis_Y3D:
            if (m_axisVisible[1] != visible)
            {
                m_axisVisible[1] = visible;
                emit axisVisibleChanged(axis, visible);
            }
            break;
        case Axis_Z3D:
            if (m_axisVisible[2] != visible)
            {
                m_axisVisible[2] = visible;
                emit axisVisibleChanged(axis, visible);
            }
            break;
        case Axis_All3D:
            if (m_axisVisible[0] != visible || m_axisVisible[1] != visible || m_axisVisible[2] != visible)
            {
                m_axisVisible[0] = visible;
                m_axisVisible[1] = visible;
                m_axisVisible[2] = visible;
                emit axisVisibleChanged(Axis_X3D, visible);
                emit axisVisibleChanged(Axis_Y3D, visible);
                emit axisVisibleChanged(Axis_Z3D, visible);
            }
            break;
    }
}

bool CoordinateSystem3D::isAxisVisible(CoordinateAxis3D axis) const
{
    switch (axis)
    {
        case Axis_X3D: return m_axisVisible[0];
        case Axis_Y3D: return m_axisVisible[1];
        case Axis_Z3D: return m_axisVisible[2];
        case Axis_All3D: return m_axisVisible[0] && m_axisVisible[1] && m_axisVisible[2];
        default: return false;
    }
}

QString CoordinateSystem3D::getUnitName() const
{
    switch (m_scaleUnit)
    {
        case Unit_Meter3D: return "m";
        case Unit_Kilometer3D: return "km";
        case Unit_Centimeter3D: return "cm";
        case Unit_Millimeter3D: return "mm";
        case Unit_Custom3D: return m_customUnitName.isEmpty() ? "单位" : m_customUnitName;
        default: return "m";
    }
}

double CoordinateSystem3D::getActualFontSize() const
{
    switch (m_fontSize)
    {
        case FontSize_Small3D: return 50.0;
        case FontSize_Medium3D: return 100.0;
        case FontSize_Large3D: return 150.0;
        case FontSize_Custom3D: return m_customFontSize;
        default: return 100.0;
    }
}

void CoordinateSystem3D::setGridPlaneVisible(GridPlane3D plane, bool visible)
{
    int index = -1;
    switch (plane)
    {
        case GridPlane_XY3D: index = 0; break;
        case GridPlane_YZ3D: index = 1; break;
        case GridPlane_XZ3D: index = 2; break;
        case GridPlane_All3D:
            m_gridPlaneVisible[0] = visible;
            m_gridPlaneVisible[1] = visible;
            m_gridPlaneVisible[2] = visible;
            emit gridPlaneVisibleChanged(GridPlane_XY3D, visible);
            emit gridPlaneVisibleChanged(GridPlane_YZ3D, visible);
            emit gridPlaneVisibleChanged(GridPlane_XZ3D, visible);
            return;
        default: return;
    }
    
    if (index >= 0 && m_gridPlaneVisible[index] != visible)
    {
        m_gridPlaneVisible[index] = visible;
        emit gridPlaneVisibleChanged(plane, visible);
    }
}

bool CoordinateSystem3D::isGridPlaneVisible(GridPlane3D plane) const
{
    switch (plane)
    {
        case GridPlane_XY3D: return m_gridPlaneVisible[0];
        case GridPlane_YZ3D: return m_gridPlaneVisible[1];
        case GridPlane_XZ3D: return m_gridPlaneVisible[2];
        case GridPlane_All3D: return m_gridPlaneVisible[0] && m_gridPlaneVisible[1] && m_gridPlaneVisible[2];
        default: return false;
    }
}





