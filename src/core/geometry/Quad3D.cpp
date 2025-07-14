#include "Quad3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>

Quad3D_Geo::Quad3D_Geo()
    : m_normal(0, 0, 1)
{
    m_geoType = Geo_Quad3D;
}

void Quad3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 4)
        {
            calculateNormal();
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Quad3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void Quad3D_Geo::updateGeometry()
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



osg::ref_ptr<osg::Geometry> Quad3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    if (m_controlPoints.size() == 4 || (m_controlPoints.size() == 3 && getTempPoint().position != glm::vec3(0)))
    {
        // 绘制四边形
        std::vector<Point3D> points = m_controlPoints;
        if (points.size() == 3)
            points.push_back(getTempPoint());
        
        // 添加顶点（按顺序）
        for (const Point3D& point : points)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }
        
        // 计算法向量
        glm::vec3 v1 = points[1].position - points[0].position;
        glm::vec3 v2 = points[2].position - points[0].position;
        glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
        
        for (int i = 0; i < 4; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
        }
        
        // 设置颜色
        Color3D color = isStateComplete() ? m_parameters.fillColor : 
                       Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                              m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
        
        for (int i = 0; i < 4; ++i)
        {
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
        
        // 将四边形三角化
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
        indices->push_back(0); indices->push_back(1); indices->push_back(2);
        indices->push_back(0); indices->push_back(2); indices->push_back(3);
        
        geometry->addPrimitiveSet(indices.get());
        
        // 边界线
        if (m_parameters.showBorder)
        {
            osg::ref_ptr<osg::DrawElementsUInt> borderIndices = new osg::DrawElementsUInt(GL_LINE_LOOP);
            borderIndices->push_back(0);
            borderIndices->push_back(1);
            borderIndices->push_back(2);
            borderIndices->push_back(3);
            
            geometry->addPrimitiveSet(borderIndices.get());
        }
    }
    else
    {
        // 绘制辅助线
        for (const Point3D& point : m_controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (getTempPoint().position != glm::vec3(0))
        {
            vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (vertices->size() >= 2)
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
        }
        else
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
        }
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



void Quad3D_Geo::calculateNormal()
{
    if (m_controlPoints.size() >= 3)
    {
        glm::vec3 v1 = m_controlPoints[1].position - m_controlPoints[0].position;
        glm::vec3 v2 = m_controlPoints[2].position - m_controlPoints[0].position;
        m_normal = glm::normalize(glm::cross(v1, v2));
    }
} 

void Quad3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    if (m_controlPoints.empty())
        return;
    
    // 创建四边形顶点的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加控制点作为顶点
    for (const Point3D& point : m_controlPoints)
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
    
    vertexGeometry->setVertexArray(vertices.get());
    vertexGeometry->setColorArray(colors.get());
    vertexGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用点绘制
    vertexGeometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
    
    // 设置点大小
    osg::ref_ptr<osg::StateSet> stateSet = vertexGeometry->getOrCreateStateSet();
    osg::ref_ptr<osg::Point> point = new osg::Point();
    point->setSize(m_parameters.pointSize);
    stateSet->setAttribute(point.get());
    
    addVertexGeometry(vertexGeometry.get());
}

void Quad3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    if (m_controlPoints.size() < 2)
        return;
    
    // 创建四边形边的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 构建所有点（包括临时点）
    std::vector<Point3D> allPoints = m_controlPoints;
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        allPoints.push_back(getTempPoint());
    }
    
    // 添加边
    for (size_t i = 0; i < allPoints.size(); ++i)
    {
        const auto& point1 = allPoints[i];
        const auto& point2 = allPoints[(i + 1) % allPoints.size()];
        
        vertices->push_back(osg::Vec3(point1.x(), point1.y(), point1.z()));
        vertices->push_back(osg::Vec3(point2.x(), point2.y(), point2.z()));
        
        // 临时点的边使用半透明颜色
        float alpha = (i >= m_controlPoints.size() - 1 && !isStateComplete()) ? 0.5f : 1.0f;
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a * alpha));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a * alpha));
    }
    
    edgeGeometry->setVertexArray(vertices.get());
    edgeGeometry->setColorArray(colors.get());
    edgeGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用线绘制
    edgeGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, vertices->size()));
    
    // 设置线宽
    osg::ref_ptr<osg::StateSet> stateSet = edgeGeometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth();
    lineWidth->setWidth(m_parameters.lineWidth);
    stateSet->setAttribute(lineWidth.get());
    
    addEdgeGeometry(edgeGeometry.get());
}

void Quad3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    
    if (m_controlPoints.size() < 3)
        return;
    
    // 创建四边形面的几何体
    osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 构建所有点（包括临时点）
    std::vector<Point3D> allPoints = m_controlPoints;
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        allPoints.push_back(getTempPoint());
    }
    
    // 添加顶点
    for (const auto& point : allPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                   m_parameters.fillColor.b, m_parameters.fillColor.a));
    }
    
    faceGeometry->setVertexArray(vertices.get());
    faceGeometry->setColorArray(colors.get());
    faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角形绘制（两个三角形组成四边形）
    if (allPoints.size() >= 4)
    {
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES, 6);
        (*indices)[0] = 0; (*indices)[1] = 1; (*indices)[2] = 2;
        (*indices)[3] = 0; (*indices)[4] = 2; (*indices)[5] = 3;
        faceGeometry->addPrimitiveSet(indices.get());
    }
    else if (allPoints.size() >= 3)
    {
        // 如果只有3个点，绘制三角形
        faceGeometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
    }
    
    addFaceGeometry(faceGeometry.get());
} 