#include "Point3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
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
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        markGeometryDirty();
        completeDrawing();
    }
}

void Point3D_Geo::completeDrawing()
{
    if (hasControlPoints())
    {
        Geo3D::completeDrawing();
    }
}

void Point3D_Geo::updateGeometry()
{
    // 清除点线面节点
    clearVertexGeometries();
    clearEdgeGeometries();
    clearFaceGeometries();
    
    updateOSGNode();
    
    // 构建点线面几何体
    buildVertexGeometries();
    buildEdgeGeometries();
    buildFaceGeometries();
    
    // 确保节点名称正确设置（用于拾取识别）
    setupNodeNames();
    
    // 更新捕捉点
    updateSnapPoints();
    
    // 更新可见性
    updateFeatureVisibility();
    
    // 更新KDTree
    if (getNodeManager()) {
        getNodeManager()->updateKdTree();
    }
}

osg::ref_ptr<osg::Geometry> Point3D_Geo::createGeometry()
{
    if (!hasControlPoints())
        return nullptr;
    
    return createPointGeometry(m_parameters.pointShape, m_parameters.pointSize);
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Point3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    if (!hasControlPoints())
        return;
    
    // 只为控制点创建几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    const auto& controlPoints = getControlPoints();
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
    
    addVertexGeometry(geometry);
}

void Point3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    // 点对象没有边
}

void Point3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    // 点对象没有面
}

osg::ref_ptr<osg::Geometry> Point3D_Geo::createPointGeometry(PointShape3D shape, float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty()) {
        return nullptr;
    }
    
    const Point3D& point = controlPoints[0];
    
    switch (shape)
    {
    case Point_Circle3D:
        {
            // 创建圆形点
            int segments = 16;
            float radius = size * 0.01f;
            for (int i = 0; i <= segments; ++i)
            {
                float angle = 2.0f * M_PI * i / segments;
                float x = point.x() + radius * cos(angle);
                float y = point.y() + radius * sin(angle);
                vertices->push_back(osg::Vec3(x, y, point.z()));
            }
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_FAN, 0, vertices->size()));
        }
        break;
        
    case Point_Square3D:
        {
            // 创建方形点
            float half = size * 0.01f;
            vertices->push_back(osg::Vec3(point.x() - half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() + half, point.z()));
            vertices->push_back(osg::Vec3(point.x() - half, point.y() + half, point.z()));
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));
        }
        break;
        
    case Point_Triangle3D:
        {
            // 创建三角形点
            float half = size * 0.01f;
            vertices->push_back(osg::Vec3(point.x(), point.y() + half, point.z()));
            vertices->push_back(osg::Vec3(point.x() - half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() - half, point.z()));
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
        }
        break;
        
    case Point_Cross3D:
        {
            // 创建十字形点
            float half = size * 0.01f;
            // 垂直线
            vertices->push_back(osg::Vec3(point.x(), point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x(), point.y() + half, point.z()));
            // 水平线
            vertices->push_back(osg::Vec3(point.x() - half, point.y(), point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y(), point.z()));
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 4));
        }
        break;
        
    case Point_Diamond3D:
        {
            // 创建菱形点
            float half = size * 0.01f;
            vertices->push_back(osg::Vec3(point.x(), point.y() + half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y(), point.z()));
            vertices->push_back(osg::Vec3(point.x(), point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() - half, point.y(), point.z()));
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));
        }
        break;
        
    default:
        // 默认为简单点
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, 1));
        break;
    }
    
    // 设置颜色
    for (size_t i = 0; i < vertices->size(); ++i)
    {
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    return geometry;
} 

bool Point3D_Geo::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 获取点的位置
    glm::vec3 pointPos = getPosition();
    
    // 计算射线到点的距离
    glm::vec3 rayDir = glm::normalize(ray.direction);
    glm::vec3 rayToPoint = pointPos - ray.origin;
    
    // 计算射线到点的垂直距离
    float projection = glm::dot(rayToPoint, rayDir);
    glm::vec3 closestPoint = ray.origin + projection * rayDir;
    float distance = glm::length(pointPos - closestPoint);
    
    // 如果距离小于点的拾取阈值，则认为命中
    float pickThreshold = 0.1f; // 拾取阈值
    if (distance <= pickThreshold && projection >= 0)
    {
        result.hit = true;
        result.distance = projection;
        result.userData = const_cast<Point3D_Geo*>(this);
        result.point = pointPos;
        return true;
    }
    
    return false;
} 