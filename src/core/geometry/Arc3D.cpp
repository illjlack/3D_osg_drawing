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
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        
        if (controlPoints.size() == 3)
        {
            mm_state()->setStateComplete();
        }
        
        
        mm_state()->setVertexGeometryInvalid();
        mm_state()->setEdgeGeometryInvalid();
        emit stateChanged(this);
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
        
        // 通过状态管理器通知临时点更新
        mm_state()->setTemporaryPointsUpdated();
        emit stateChanged(this);
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
    
    // 通过状态管理器清除顶点几何体无效状态
    mm_state()->clearVertexGeometryInvalid();
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
    
    // 通过状态管理器清除边几何体无效状态
    mm_state()->clearEdgeGeometryInvalid();
}

void Arc3D_Geo::buildFaceGeometries()
{
    // 圆弧不需要画面
    return;
}

