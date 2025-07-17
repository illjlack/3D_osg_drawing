#include "Arc3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Arc3D_Geo::Arc3D_Geo()
    : m_radius(0.0f)
    , m_startAngle(0.0f)
    , m_endAngle(0.0f)
    , m_normal(0, 0, 1)
{
    m_geoType = Geo_Arc3D;
    // 确保基类正确初始化
    initialize();
}

void Arc3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        // 使用新的检查方法
        if (isDrawingComplete() && areControlPointsValid())
        {
            mm_state()->setStateComplete();
        }
    }
}

void Arc3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        
        // 设置临时点用于预览
        if (!controlPoints.empty())
        {
            mm_controlPoint()->setTempPoint(Point3D(worldPos));
        }
    }
}

void Arc3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制 - 控制点使用较大的点大小以便拾取
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置点的大小和颜色
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> point = new osg::Point;
    point->setSize(8.0f);  // 控制点大小
    stateSet->setAttribute(point);
    
    // 几何体构建完成
}

void Arc3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建圆弧边界线几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 使用lambda表达式计算圆弧参数
    auto calculateArcParams = [&]() -> MathUtils::ArcParameters {
        const auto& p1 = controlPoints[0].position;
        const auto& p2 = controlPoints[1].position;
        const auto& p3 = controlPoints[2].position;
        return MathUtils::calculateArcFromThreePoints(p1, p2, p3);
    };
    
    auto arcParams = calculateArcParams();
    
    // 更新成员变量
    m_center = arcParams.center;
    m_radius = arcParams.radius;
    m_startAngle = arcParams.startAngle;
    m_endAngle = arcParams.endAngle;
    m_sweepAngle = arcParams.sweepAngle;
    m_normal = arcParams.normal;
    m_uAxis = arcParams.uAxis;
    m_vAxis = arcParams.vAxis;
    
    // 使用lambda表达式生成圆弧点
    auto generateArcVertices = [&](int segments) {
        std::vector<glm::vec3> arcPoints = MathUtils::generateArcPoints(arcParams, segments);
        for (const auto& point : arcPoints)
        {
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
        }
    };
    
    // 生成圆弧边界线点
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    if (segments < 8) segments = 16;
    generateArcVertices(segments);
    
    geometry->setVertexArray(vertices);
    
    // 线绘制 - 边界线
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f);  // 边界线宽度
    stateSet->setAttribute(lineWidth);
    
    // 几何体构建完成
}

void Arc3D_Geo::buildFaceGeometries()
{
    // 圆弧不需要画面
    return;
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Arc3D_Geo::isDrawingComplete() const
{
    // 圆弧需要3个控制点（起点、中点、终点）才能完成绘制
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 3;
}

bool Arc3D_Geo::areControlPointsValid() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
    // 检查控制点数量
    if (controlPoints.size() < 3) {
        return false;
    }
    
    // 检查控制点是否重合（允许一定的误差）
    const float epsilon = 0.001f;
    
    // 检查前两个点是否重合
    glm::vec3 diff1 = controlPoints[1].position - controlPoints[0].position;
    float distance1 = glm::length(diff1);
    if (distance1 < epsilon) {
        return false;
    }
    
    // 检查后两个点是否重合
    glm::vec3 diff2 = controlPoints[2].position - controlPoints[1].position;
    float distance2 = glm::length(diff2);
    if (distance2 < epsilon) {
        return false;
    }
    
    // 检查第一个和第三个点是否重合
    glm::vec3 diff3 = controlPoints[2].position - controlPoints[0].position;
    float distance3 = glm::length(diff3);
    if (distance3 < epsilon) {
        return false;
    }
    
    // 检查控制点坐标是否有效（不是NaN或无穷大）
    for (const auto& point : controlPoints) {
        if (std::isnan(point.x()) || std::isnan(point.y()) || std::isnan(point.z()) ||
            std::isinf(point.x()) || std::isinf(point.y()) || std::isinf(point.z())) {
            return false;
        }
    }
    
    // 检查三点是否共线（如果共线则无法形成圆弧）
    glm::vec3 v1 = controlPoints[1].position - controlPoints[0].position;
    glm::vec3 v2 = controlPoints[2].position - controlPoints[0].position;
    glm::vec3 cross = glm::cross(v1, v2);
    float crossLength = glm::length(cross);
    
    if (crossLength < epsilon) {
        return false; // 三点共线，无法形成圆弧
    }
    
    return true;
}

