#include "BezierCurve3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>

BezierCurve3D_Geo::BezierCurve3D_Geo()
{
    m_geoType = Geo_BezierCurve3D;
    // 确保基类正确初始化
    initialize();
}

void BezierCurve3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        const auto& controlPoints = getControlPoints();
        
        if (controlPoints.size() >= 2)
        {
            generateBezierPoints();
            updateGeometry();
        }
        
        emit stateChanged(this);
    }
}

void BezierCurve3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = getControlPoints();
    if (!isStateComplete() && !controlPoints.empty())
    {
        setTempPoint(Point3D(worldPos));
        markGeometryDirty();
        updateGeometry();
    }
}

void BezierCurve3D_Geo::keyPressEvent(QKeyEvent* event)
{
    const auto& controlPoints = getControlPoints();
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (controlPoints.size() >= 2)
        {
            completeDrawing();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!controlPoints.empty())
        {
            removeControlPoint(controlPoints.size() - 1);
            updateGeometry();
        }
    }
}

void BezierCurve3D_Geo::updateGeometry()
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



osg::ref_ptr<osg::Geometry> BezierCurve3D_Geo::createGeometry()
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return nullptr;
    
    generateBezierPoints();
    
    if (m_bezierPoints.empty())
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    for (const Point3D& point : m_bezierPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 如果正在绘制且有临时点，计算包含临时点的贝塞尔曲线
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        std::vector<Point3D> tempControlPoints = controlPoints;
        tempControlPoints.push_back(getTempPoint());
        
        // 生成临时贝塞尔曲线点
        std::vector<Point3D> tempBezierPoints;
        int steps = m_parameters.steps > 0 ? m_parameters.steps : 50;
        
        for (int i = 0; i <= steps; ++i)
        {
            float t = static_cast<float>(i) / steps;
            // 临时计算贝塞尔点
            std::vector<glm::vec3> tempVecs;
            for (const Point3D& cp : tempControlPoints)
            {
                tempVecs.push_back(cp.position);
            }
            
            while (tempVecs.size() > 1)
            {
                std::vector<glm::vec3> newVecs;
                for (size_t j = 0; j < tempVecs.size() - 1; ++j)
                {
                    newVecs.push_back(glm::mix(tempVecs[j], tempVecs[j+1], t));
                }
                tempVecs = newVecs;
            }
            
            if (!tempVecs.empty())
            {
                tempBezierPoints.push_back(Point3D(tempVecs[0]));
            }
        }
        
        // 添加临时点（半透明）
        for (const Point3D& point : tempBezierPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    return geometry;
}

void BezierCurve3D_Geo::buildVertexGeometries()
{
    clearVertexGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 创建贝塞尔曲线顶点的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加控制点作为顶点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
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

void BezierCurve3D_Geo::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    if (m_bezierPoints.empty())
        return;
    
    // 创建贝塞尔曲线边的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加贝塞尔曲线点作为边
    for (const Point3D& point : m_bezierPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    edgeGeometry->setVertexArray(vertices.get());
    edgeGeometry->setColorArray(colors.get());
    edgeGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用线绘制
    edgeGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    // 设置线宽
    osg::ref_ptr<osg::StateSet> stateSet = edgeGeometry->getOrCreateStateSet();
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth();
    lineWidth->setWidth(m_parameters.lineWidth);
    stateSet->setAttribute(lineWidth.get());
    
    addEdgeGeometry(edgeGeometry.get());
}

void BezierCurve3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    // 贝塞尔曲线没有面几何体
}

void BezierCurve3D_Geo::generateBezierPoints()
{
    m_bezierPoints.clear();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    int steps = m_parameters.steps > 0 ? m_parameters.steps : 50;
    
    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec3 point = calculateBezierPoint(t);
        m_bezierPoints.push_back(Point3D(point));
    }
}

glm::vec3 BezierCurve3D_Geo::calculateBezierPoint(float t) const
{
    const auto& controlPoints = getControlPoints();
    if (controlPoints.empty())
        return glm::vec3(0);
    
    // De Casteljau算法
    std::vector<glm::vec3> tempPoints;
    for (const Point3D& cp : controlPoints)
    {
        tempPoints.push_back(cp.position);
    }
    
    while (tempPoints.size() > 1)
    {
        std::vector<glm::vec3> newPoints;
        for (size_t j = 0; j < tempPoints.size() - 1; ++j)
        {
            newPoints.push_back(glm::mix(tempPoints[j], tempPoints[j+1], t));
        }
        tempPoints = newPoints;
    }
    
    return tempPoints.empty() ? glm::vec3(0) : tempPoints[0];
} 