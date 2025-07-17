#include "Polygon3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

Polygon3D_Geo::Polygon3D_Geo()
    : m_normal(0, 0, 1)
{
    m_geoType = Geo_Polygon3D;
    // 确保基类正确初始化
    initialize();
}

void Polygon3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateDrawComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        
        if (controlPoints.size() >= 3)
        {
            // 检查是否是双击完成多边形
            if (event && event->type() == QEvent::MouseButtonDblClick)
            {
                mm_state()->setStateDrawComplete();
            }
        }
        
        mm_state()->setControlPointsUpdated();
    }
}

void Polygon3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (!mm_state()->isStateDrawComplete() && !controlPoints.empty())
    {
        // 设置临时点用于预览
        mm_controlPoint()->setTempPoint(Point3D(worldPos));
        mm_state()->setTemporaryPointsUpdated();
    }
}

void Polygon3D_Geo::keyPressEvent(QKeyEvent* event)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (controlPoints.size() >= 3)
        {
            mm_state()->setStateDrawComplete();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!controlPoints.empty())
        {
            mm_controlPoint()->removeControlPoint(controlPoints.size() - 1);
        }
    }
}

void Polygon3D_Geo::buildVertexGeometries()
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
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    // 如果有临时点且几何体未完成，添加临时点
    if (!mm_state()->isStateDrawComplete() && mm_controlPoint()->getTempPoint().position != glm::vec3(0))
    {
        vertices->push_back(osg::Vec3(mm_controlPoint()->getTempPoint().x(), mm_controlPoint()->getTempPoint().y(), mm_controlPoint()->getTempPoint().z()));
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
    
    // 几何体已经通过mm_node()->getVertexGeometry()获取，直接使用
}

void Polygon3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建多边形边界线几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 构建所有点（包括临时点）
    std::vector<Point3D> allPoints = controlPoints;
    if (!mm_state()->isStateDrawComplete() && mm_controlPoint()->getTempPoint().position != glm::vec3(0))
    {
        allPoints.push_back(mm_controlPoint()->getTempPoint());
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
    
    // 几何体已经通过mm_node()->getEdgeGeometry()获取，直接使用
}

void Polygon3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 使用lambda表达式计算多边形参数
    auto calculatePolygonParams = [&]() -> MathUtils::PolygonParameters {
        std::vector<glm::vec3> vertices;
        for (const auto& point : controlPoints)
        {
            vertices.push_back(point.position);
        }
        return MathUtils::calculatePolygonParameters(vertices);
    };
    
    auto polygonParams = calculatePolygonParams();
    
    // 更新成员变量
    m_normal = polygonParams.normal;
    m_triangleIndices = polygonParams.triangleIndices;
    
    // 直接创建多边形面几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 添加顶点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                   m_parameters.fillColor.b, m_parameters.fillColor.a));
        normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角形索引绘制
    if (!m_triangleIndices.empty())
    {
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
        for (int index : m_triangleIndices)
        {
            indices->push_back(index);
        }
        geometry->addPrimitiveSet(indices);
    }
}
