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
    
    // 更新KDTree
    if (getNodeManager()) {
        getNodeManager()->updateKdTree();
    }
    
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
    
    // 只为控制点创建几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    // 如果有临时点且几何体未完成，添加临时点
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a * 0.5f));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 点绘制 - 控制点使用较大的点大小以便拾取
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置点的大小
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> point = new osg::Point;
    point->setSize(8.0f);  // 控制点大小
    stateSet->setAttribute(point);
    
    addVertexGeometry(geometry);
}

void Polygon3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 创建多边形边界线几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 构建所有点（包括临时点）
    std::vector<Point3D> allPoints = controlPoints;
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        allPoints.push_back(getTempPoint());
    }
    
    // 添加边顶点
    for (size_t i = 0; i < allPoints.size(); ++i)
    {
        size_t next = (i + 1) % allPoints.size();
        vertices->push_back(osg::Vec3(allPoints[i].x(), allPoints[i].y(), allPoints[i].z()));
        vertices->push_back(osg::Vec3(allPoints[next].x(), allPoints[next].y(), allPoints[next].z()));
        
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 线绘制 - 边界线
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f);  // 边界线宽度
    stateSet->setAttribute(lineWidth);
    
    addEdgeGeometry(geometry);
}

void Polygon3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 创建多边形面几何体
    osg::ref_ptr<osg::Geometry> geometry = createGeometry();
    if (geometry.valid())
    {
        addFaceGeometry(geometry);
    }
} 

bool Polygon3D_Geo::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 获取多边形的包围盒
    const BoundingBox3D& bbox = getBoundingBox();
    if (!bbox.isValid())
    {
        return false;
    }
    
    // 使用包围盒进行相交测试
    glm::vec3 rayDir = glm::normalize(ray.direction);
    glm::vec3 invDir = 1.0f / rayDir;
    
    // 计算与包围盒各面的交点参数
    float t1 = (bbox.min.x - ray.origin.x) * invDir.x;
    float t2 = (bbox.max.x - ray.origin.x) * invDir.x;
    float t3 = (bbox.min.y - ray.origin.y) * invDir.y;
    float t4 = (bbox.max.y - ray.origin.y) * invDir.y;
    float t5 = (bbox.min.z - ray.origin.z) * invDir.z;
    float t6 = (bbox.max.z - ray.origin.z) * invDir.z;
    
    // 计算进入和退出参数
    float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
    float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));
    
    // 检查是否相交
    if (tmax >= 0 && tmin <= tmax)
    {
        float t = (tmin >= 0) ? tmin : tmax;
        if (t >= 0)
        {
            result.hit = true;
            result.distance = t;
            result.userData = const_cast<Polygon3D_Geo*>(this);
            result.point = ray.origin + t * rayDir;
            return true;
        }
    }
    
    return false;
} 