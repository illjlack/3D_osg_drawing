#include "Box3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include <algorithm>

Box3D_Geo::Box3D_Geo()
    : m_size(1.0f, 1.0f, 1.0f)
{
    m_geoType = Geo_Box3D;
    // 确保基类正确初始化
    initialize();
}

void Box3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        const auto& controlPoints = getControlPoints();
        
        if (controlPoints.size() == 2)
        {
            // 计算长方体尺寸
            glm::vec3 diff = controlPoints[1].position - controlPoints[0].position;
            m_size = glm::abs(diff);
            completeDrawing();
        }
        
        updateGeometry();
        emit stateChanged(this);
    }
}

void Box3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = getControlPoints();
    if (!isStateComplete() && controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Box3D_Geo::updateGeometry()
{
    if (!isGeometryDirty()) return;
    
    // 清除点线面节点
    clearVertexGeometries();
    clearEdgeGeometries();
    clearFaceGeometries();
    
    // 构建几何体
    buildVertexGeometries();
    buildEdgeGeometries();
    buildFaceGeometries();
    
    updateOSGNode();
    clearGeometryDirty();
    
    emit geometryUpdated(this);
}

osg::ref_ptr<osg::Geometry> Box3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
    {
        return nullptr;
    }

    glm::vec3 center = controlPoints[0].position;
    glm::vec3 size = m_size;
    
    if (controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        size = glm::abs(diff);
    }
    else if (controlPoints.size() == 2)
    {
        center = (controlPoints[0].position + controlPoints[1].position) * 0.5f;
        glm::vec3 diff = controlPoints[1].position - controlPoints[0].position;
        size = glm::abs(diff);
    }

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;

    // 生成长方体的8个顶点
    glm::vec3 halfSize = size * 0.5f;
    
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z)); // 0
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z)); // 1
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z)); // 2
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z)); // 3
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z)); // 4
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z)); // 5
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z)); // 6
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z)); // 7

    geometry->setVertexArray(vertices);

    // 生成长方体的面
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
    
    // 前面
    indices->push_back(0); indices->push_back(1); indices->push_back(2);
    indices->push_back(0); indices->push_back(2); indices->push_back(3);
    
    // 右面
    indices->push_back(1); indices->push_back(5); indices->push_back(6);
    indices->push_back(1); indices->push_back(6); indices->push_back(2);
    
    // 后面
    indices->push_back(5); indices->push_back(4); indices->push_back(7);
    indices->push_back(5); indices->push_back(7); indices->push_back(6);
    
    // 左面
    indices->push_back(4); indices->push_back(0); indices->push_back(3);
    indices->push_back(4); indices->push_back(3); indices->push_back(7);
    
    // 顶面
    indices->push_back(3); indices->push_back(2); indices->push_back(6);
    indices->push_back(3); indices->push_back(6); indices->push_back(7);
    
    // 底面
    indices->push_back(4); indices->push_back(5); indices->push_back(1);
    indices->push_back(4); indices->push_back(1); indices->push_back(0);

    geometry->addPrimitiveSet(indices);

    // 计算法线
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(0, 0, -1)); // 前面
    normals->push_back(osg::Vec3(0, 0, -1));
    normals->push_back(osg::Vec3(0, 0, -1));
    normals->push_back(osg::Vec3(0, 0, -1));
    normals->push_back(osg::Vec3(0, 0, -1));
    normals->push_back(osg::Vec3(0, 0, -1));
    normals->push_back(osg::Vec3(0, 0, 1)); // 后面
    normals->push_back(osg::Vec3(0, 0, 1));
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

    return geometry;
}

void Box3D_Geo::buildVertexGeometries()
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

void Box3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建长方体边界线几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    glm::vec3 center = (controlPoints.size() == 1) ? controlPoints[0].position :
                       (controlPoints[0].position + controlPoints[1].position) * 0.5f;
    glm::vec3 size = m_size;
    
    if (controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        size = glm::abs(diff);
    }
    else if (controlPoints.size() == 2)
    {
        glm::vec3 diff = controlPoints[1].position - controlPoints[0].position;
        size = glm::abs(diff);
    }
    
    // 生成长方体的8个顶点
    glm::vec3 halfSize = size * 0.5f;
    
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z)); // 0
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z)); // 1
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z)); // 2
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z)); // 3
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z)); // 4
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z)); // 5
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z)); // 6
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z)); // 7
    
    // 为所有顶点设置颜色
    for (int i = 0; i < 8; ++i)
    {
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 生成长方体的12条边
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    // 底面的4条边
    indices->push_back(0); indices->push_back(1);
    indices->push_back(1); indices->push_back(2);
    indices->push_back(2); indices->push_back(3);
    indices->push_back(3); indices->push_back(0);
    
    // 顶面的4条边
    indices->push_back(4); indices->push_back(5);
    indices->push_back(5); indices->push_back(6);
    indices->push_back(6); indices->push_back(7);
    indices->push_back(7); indices->push_back(4);
    
    // 垂直的4条边
    indices->push_back(0); indices->push_back(4);
    indices->push_back(1); indices->push_back(5);
    indices->push_back(2); indices->push_back(6);
    indices->push_back(3); indices->push_back(7);
    
    geometry->addPrimitiveSet(indices);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f);  // 边界线宽度
    stateSet->setAttribute(lineWidth);
    
    addEdgeGeometry(geometry);
}

void Box3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建长方体面几何体
    osg::ref_ptr<osg::Geometry> geometry = createGeometry();
    if (geometry.valid())
    {
        addFaceGeometry(geometry);
    }
}

// 计算长方体的体积
float Box3D_Geo::calculateVolume() const
{
    return m_size.x * m_size.y * m_size.z;
}

// 计算长方体的表面积
float Box3D_Geo::calculateSurfaceArea() const
{
    return 2.0f * (m_size.x * m_size.y + m_size.y * m_size.z + m_size.z * m_size.x);
}

// 获取长方体的中心点
glm::vec3 Box3D_Geo::getCenter() const
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() == 2)
    {
        return (controlPoints[0].position + controlPoints[1].position) * 0.5f;
    }
    else if (controlPoints.size() == 1)
    {
        return controlPoints[0].position;
    }
    return glm::vec3(0);
}

// 获取长方体的尺寸
glm::vec3 Box3D_Geo::getSize() const
{
    return m_size;
}

// 设置长方体的尺寸
void Box3D_Geo::setSize(const glm::vec3& size)
{
    m_size = size;
    markGeometryDirty();
    updateGeometry();
} 