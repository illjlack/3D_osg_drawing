#include "Polygon3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>

Polygon3D_Geo::Polygon3D_Geo()
    : m_normal(0, 0, 1)
{
    m_geoType = Geo_Polygon3D;
    // 确保基类正确初始化
    initialize();
}

void Polygon3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        const auto& controlPoints = getControlPoints();
        
        if (controlPoints.size() >= 3)
        {
            // 检查是否是双击完成多边形
            if (event && event->type() == QEvent::MouseButtonDblClick)
            {
                completeDrawing();
            }
        }
        
        updateGeometry();
        emit stateChanged(this);
    }
}

void Polygon3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Polygon3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (!isStateComplete())
    {
        const auto& controlPoints = getControlPoints();
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        {
            if (controlPoints.size() >= 3)
            {
                completeDrawing();
            }
        }
        else if (event->key() == Qt::Key_Backspace)
        {
            if (!controlPoints.empty())
            {
                removeControlPoint(controlPoints.size() - 1);
                updateGeometry();
            }
        }
        else if (event->key() == Qt::Key_Escape)
        {
            clearControlPoints();
            updateGeometry();
        }
    }
}

void Polygon3D_Geo::updateGeometry()
{
    if (!isGeometryDirty()) return;
    
    // 清除点线面节点
    clearVertexGeometries();
    clearEdgeGeometries();
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
    {
        updateOSGNode();
        clearGeometryDirty();
        return;
    }
    
    // 构建几何体
    buildVertexGeometries();
    buildEdgeGeometries();
    
    if (isStateComplete() && controlPoints.size() >= 3)
    {
        buildFaceGeometries();
    }
    
    updateOSGNode();
    clearGeometryDirty();
    
    emit geometryUpdated(this);
}



osg::ref_ptr<osg::Geometry> Polygon3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return nullptr;
    
    // 计算多边形法向量
    glm::vec3 normal = calculateNormal();
    
    // 创建多边形几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加顶点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 创建三角形索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
    
    // 简单的扇形三角化
    for (size_t i = 1; i < controlPoints.size() - 1; ++i)
    {
        indices->push_back(0);
        indices->push_back(i);
        indices->push_back(i + 1);
    }
    
    geometry->addPrimitiveSet(indices);
    
    // 设置法向量
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
    
    return geometry;
}



glm::vec3 Polygon3D_Geo::calculateNormal() const
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() >= 3)
    {
        glm::vec3 v1 = controlPoints[1].position - controlPoints[0].position;
        glm::vec3 v2 = controlPoints[2].position - controlPoints[0].position;
        return glm::normalize(glm::cross(v1, v2));
    }
    return glm::vec3(0, 0, 1); // 默认法向量
}

void Polygon3D_Geo::triangulatePolygon()
{
    m_triangleIndices.clear();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 简单的耳切三角化算法
    // 这里使用简单的扇形三角化，更复杂的多边形可能需要更高级的算法
    for (size_t i = 1; i < controlPoints.size() - 1; ++i)
    {
        m_triangleIndices.push_back(0);
        m_triangleIndices.push_back(i);
        m_triangleIndices.push_back(i + 1);
    }
} 

void Polygon3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加所有控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    addVertexGeometry(geometry);
}

void Polygon3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 添加边顶点
    for (size_t i = 0; i < controlPoints.size(); ++i)
    {
        size_t next = (i + 1) % controlPoints.size();
        vertices->push_back(osg::Vec3(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z()));
        vertices->push_back(osg::Vec3(controlPoints[next].x(), controlPoints[next].y(), controlPoints[next].z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 线绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    addEdgeGeometry(geometry);
}

void Polygon3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 创建面几何体
    osg::ref_ptr<osg::Geometry> geometry = createGeometry();
    if (geometry.valid())
    {
        addFaceGeometry(geometry);
    }
} 