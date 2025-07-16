#include "BezierCurve3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <QKeyEvent>
#include "../../util/MathUtils.h"

BezierCurve3D_Geo::BezierCurve3D_Geo()
{
    m_geoType = Geo_BezierCurve3D;
    // 确保基类正确初始化
    initialize();
}

void BezierCurve3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        
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
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (!mm_state()->isStateComplete() && !controlPoints.empty())
    {
        // 设置临时点用于预览
        // 这里需要实现临时点机制
        updateGeometry();
    }
}

void BezierCurve3D_Geo::keyPressEvent(QKeyEvent* event)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (controlPoints.size() >= 2)
        {
            mm_state()->setStateComplete();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!controlPoints.empty())
        {
            mm_controlPoint()->removeControlPoint(controlPoints.size() - 1);
            updateGeometry();
        }
    }
}

void BezierCurve3D_Geo::updateGeometry()
{
    // 清除点线面节点
    mm_node()->clearAllGeometries();
    
    // 构建点线面几何体
    buildVertexGeometries();
    buildEdgeGeometries();
    buildFaceGeometries();
    
    // 更新OSG节点
    updateOSGNode();
    
    // 更新捕捉点
    mm_snapPoint()->updateSnapPoints();
    
    // 更新包围盒
    mm_boundingBox()->updateBoundingBox();
    
    // 更新空间索引
    mm_node()->updateSpatialIndex();
}



osg::ref_ptr<osg::Geometry> BezierCurve3D_Geo::createGeometry()
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
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
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    return geometry;
}

void BezierCurve3D_Geo::buildVertexGeometries()
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
    
    mm_node()->setVertexGeometry(geometry);
}

void BezierCurve3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建贝塞尔曲线边界线几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 使用lambda表达式生成贝塞尔曲线点
    auto generateBezierPoints = [&]() {
        std::vector<glm::vec3> controlVecs;
        for (const Point3D& cp : controlPoints)
        {
            controlVecs.push_back(cp.position);
        }
        return MathUtils::generateBezierCurve(controlVecs, m_parameters.steps > 0 ? m_parameters.steps : 50);
    };
    
    auto bezierPoints = generateBezierPoints();
    
    // 更新成员变量
    m_bezierPoints.clear();
    for (const auto& point : bezierPoints)
    {
        m_bezierPoints.push_back(Point3D(point));
    }
    
    // 添加贝塞尔曲线点作为边
    for (const Point3D& point : m_bezierPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 如果正在绘制且有临时点，计算包含临时点的贝塞尔曲线
    if (!mm_state()->isStateComplete() && mm_controlPoint()->getTempPoint().position != glm::vec3(0))
    {
        std::vector<Point3D> tempControlPoints = controlPoints;
        tempControlPoints.push_back(mm_controlPoint()->getTempPoint());
        
        // 使用lambda表达式生成临时贝塞尔曲线点
        auto generateTempBezierPoints = [&]() {
            std::vector<glm::vec3> tempVecs;
            for (const Point3D& cp : tempControlPoints)
            {
                tempVecs.push_back(cp.position);
            }
            return MathUtils::generateBezierCurve(tempVecs, m_parameters.steps > 0 ? m_parameters.steps : 50);
        };
        
        auto tempBezierPoints = generateTempBezierPoints();
        
        // 添加临时点（半透明）
        for (const auto& point : tempBezierPoints)
        {
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
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
    
    // 通过状态管理器清除边几何体无效状态
    mm_state()->clearEdgeGeometryInvalid();
}

void BezierCurve3D_Geo::buildFaceGeometries()
{
    clearFaceGeometries();
    // 贝塞尔曲线没有面几何体
}

 
