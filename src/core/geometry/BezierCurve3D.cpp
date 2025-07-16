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
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建贝塞尔曲线边界线几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 生成贝塞尔曲线点
    generateBezierPoints();
    
    // 添加贝塞尔曲线点作为边
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

bool BezierCurve3D_Geo::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 获取贝塞尔曲线的控制点
    const std::vector<Point3D>& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
    {
        return false;
    }
    
    // 射线-贝塞尔曲线相交测试（简化版本，使用包围盒）
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
            result.userData = const_cast<BezierCurve3D_Geo*>(this);
            result.point = ray.origin + t * rayDir;
            return true;
        }
    }
    
    return false;
} 