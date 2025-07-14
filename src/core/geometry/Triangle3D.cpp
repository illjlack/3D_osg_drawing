#include "Triangle3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Triangle3D_Geo::Triangle3D_Geo()
    : m_area(0.0f)
    , m_normal(0, 0, 1)
{
    m_geoType = Geo_Triangle3D;
    // 确保基类正确初始化
    initialize();
}

void Triangle3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        const auto& controlPoints = getControlPoints();
        if (controlPoints.size() == 3)
        {
            calculateNormal();
            calculateArea();
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Triangle3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

void Triangle3D_Geo::updateGeometry()
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
    
    // 更新可见性
    updateFeatureVisibility();
}

osg::ref_ptr<osg::Geometry> Triangle3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    // 根据控制点数量决定绘制内容
    if (controlPoints.size() == 3 || (controlPoints.size() == 2 && getTempPoint().position != glm::vec3(0)))
    {
        // 三个点：绘制完整三角形
        Point3D p1 = controlPoints[0];
        Point3D p2 = controlPoints[1];
        Point3D p3 = (controlPoints.size() == 3) ? controlPoints[2] : getTempPoint();
        
        vertices->push_back(osg::Vec3(p1.x(), p1.y(), p1.z()));
        vertices->push_back(osg::Vec3(p2.x(), p2.y(), p2.z()));
        vertices->push_back(osg::Vec3(p3.x(), p3.y(), p3.z()));
        
        // 计算法向量
        glm::vec3 v1 = p2.position - p1.position;
        glm::vec3 v2 = p3.position - p1.position;
        glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
        
        for (int i = 0; i < 3; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
        }
        
        // 设置颜色
        Color3D color = isStateComplete() ? m_parameters.fillColor : 
                       Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                              m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
        
        for (int i = 0; i < 3; ++i)
        {
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
        
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
        
        // 如果需要显示边界
        if (m_parameters.showBorder || !isStateComplete())
        {
            osg::ref_ptr<osg::DrawElementsUInt> wireframe = new osg::DrawElementsUInt(GL_LINE_LOOP, 3);
            (*wireframe)[0] = 0;
            (*wireframe)[1] = 1;
            (*wireframe)[2] = 2;
            geometry->addPrimitiveSet(wireframe.get());
        }
    }
    else if (controlPoints.size() == 2)
    {
        // 两个点：绘制线段
        Point3D p1 = controlPoints[0];
        Point3D p2 = controlPoints[1];
        
        vertices->push_back(osg::Vec3(p1.x(), p1.y(), p1.z()));
        vertices->push_back(osg::Vec3(p2.x(), p2.y(), p2.z()));
        
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
        
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 2));
    }
    else if (controlPoints.size() == 1)
    {
        // 一个点：绘制点
        Point3D p1 = controlPoints[0];
        
        vertices->push_back(osg::Vec3(p1.x(), p1.y(), p1.z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
        
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, 1));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    if (!normals->empty())
    {
        geometry->setNormalArray(normals.get());
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    
    return geometry;
}

float Triangle3D_Geo::calculateArea() const
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
    {
        return 0.0f;
    }
    
    glm::vec3 v1 = controlPoints[1].position - controlPoints[0].position;
    glm::vec3 v2 = controlPoints[2].position - controlPoints[0].position;
    glm::vec3 cross = glm::cross(v1, v2);
    return glm::length(cross) * 0.5f;
}

void Triangle3D_Geo::calculateNormal()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() >= 3)
    {
        glm::vec3 v1 = controlPoints[1].position - controlPoints[0].position;
        glm::vec3 v2 = controlPoints[2].position - controlPoints[0].position;
        m_normal = glm::normalize(glm::cross(v1, v2));
    }
} 

void Triangle3D_Geo::buildVertexGeometries()
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

void Triangle3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 创建三角形边界线几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 构建所有点（包括临时点）
    std::vector<Point3D> allPoints = controlPoints;
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        allPoints.push_back(getTempPoint());
    }
    
    // 添加边线
    if (allPoints.size() >= 2)
    {
        for (size_t i = 0; i < allPoints.size(); ++i)
        {
            vertices->push_back(osg::Vec3(allPoints[i].x(), allPoints[i].y(), allPoints[i].z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
        }
        
        // 如果有3个或更多点，形成闭合三角形
        if (allPoints.size() >= 3)
        {
            vertices->push_back(osg::Vec3(allPoints[0].x(), allPoints[0].y(), allPoints[0].z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 线绘制 - 边界线
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置线的宽度
    osg::ref_ptr<osg::StateSet> stateSet = geometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth;
    lineWidth->setWidth(2.0f);  // 边界线宽度
    stateSet->setAttribute(lineWidth);
    
    addEdgeGeometry(geometry);
}

void Triangle3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 创建三角形面几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 添加三角形的三个顶点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                   m_parameters.fillColor.b, m_parameters.fillColor.a));
    }
    
    // 计算法向量
    calculateNormal();
    for (int i = 0; i < 3; ++i)
    {
        normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角面绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    addFaceGeometry(geometry);
}