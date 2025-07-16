#include "Point3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include "../../util/MathUtils.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Point3D_Geo::Point3D_Geo()
{
    m_geoType = Geo_Point3D;
    // 确保基类正确初始化
    initialize();
}

void Point3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        mm_state()->setStateComplete();
        mm_state()->setControlPointsUpdated();
    }
}

void Point3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 点对象的鼠标移动事件 - 默认实现
    // 点对象不需要处理鼠标移动事件
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Point3D_Geo::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    if (!mm_controlPoint()->hasControlPoints())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getVertexGeometry();
    if (!geometry.valid())
        return;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    const Point3D& point = controlPoints[0];
    
    // 添加控制点
    vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                               m_parameters.pointColor.b, m_parameters.pointColor.a));
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 点绘制 - 控制点使用较大的点大小以便拾取
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置点的大小
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> pointAttr = new osg::Point;
    pointAttr->setSize(8.0f);  // 控制点大小
    stateSet->setAttribute(pointAttr);
}

void Point3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    // 点对象没有边
}

void Point3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    // 点对象没有面
}
