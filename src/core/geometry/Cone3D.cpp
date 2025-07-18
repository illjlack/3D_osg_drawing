#include "Cone3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Cone3D_Geo::Cone3D_Geo()
    : m_radius(1.0f)
    , m_height(2.0f)
    , m_segments(16)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Cone3D;
    // 确保基类正确初始化
    initialize();
}

void Cone3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        // 使用新的检查方法
        if (isDrawingComplete() && areControlPointsValid())
        {
            // 计算圆锥参数
            const auto& controlPoints = mm_controlPoint()->getControlPoints();
            glm::vec3 diff = controlPoints[1].position - controlPoints[0].position;
            m_height = glm::length(diff);
            if (m_height > 0)
                m_axis = glm::normalize(diff);
            m_radius = m_height * 0.3f; // 默认半径为高度的30%
            mm_state()->setStateComplete();
        }
    }
}

void Cone3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (!mm_state()->isStateComplete() && controlPoints.size() == 1)
    {
        // 设置临时点用于预览
        mm_controlPoint()->setTempPoint(Point3D(worldPos));
    }
}

// ============================================================================
// 点线面几何体构建实现
// ============================================================================

void Cone3D_Geo::buildVertexGeometries()
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
    
    // 添加控制点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    
    geometry->setVertexArray(vertices);
    
    // 点绘制 - 控制点
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

void Cone3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建圆锥边界线几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    glm::vec3 base = controlPoints[0].position;
    glm::vec3 apex = controlPoints[1].position;
    float radius = m_radius;
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    if (segments < 8) segments = 16;
    
    // 使用lambda表达式计算圆锥体参数
    auto calculateConeParams = [&]() -> MathUtils::ConeParameters {
        return MathUtils::calculateConeParameters(base, apex, radius);
    };
    
    auto coneParams = calculateConeParams();
    glm::vec3 u = coneParams.uAxis;
    glm::vec3 v = coneParams.vAxis;
    
    // 生成底面边界线
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = base + radius * dir1;
        glm::vec3 p2 = base + radius * dir2;
        
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 生成侧边线（从底面到顶点）
    for (int i = 0; i < segments; i += segments / 4)  // 只显示4条主要的侧边线
    {
        float angle = 2.0f * M_PI * i / segments;
        glm::vec3 dir = static_cast<float>(cos(angle)) * u + static_cast<float>(sin(angle)) * v;
        glm::vec3 p = base + radius * dir;
        
        vertices->push_back(osg::Vec3(p.x, p.y, p.z));
        vertices->push_back(osg::Vec3(apex.x, apex.y, apex.z));
        
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
    lineWidth->setWidth(m_parameters.lineWidth);  // 使用参数中的线宽
    stateSet->setAttribute(lineWidth);
    
    // 几何体已经通过mm_node()->getEdgeGeometry()获取，直接使用
}

void Cone3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 创建圆锥底面和侧面几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    glm::vec3 base = controlPoints[0].position;
    glm::vec3 apex = controlPoints[1].position;
    float radius = m_radius;
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    if (segments < 8) segments = 16;
    
    // 使用lambda表达式计算圆锥体参数
    auto calculateConeParams = [&]() -> MathUtils::ConeParameters {
        return MathUtils::calculateConeParameters(base, apex, radius);
    };
    
    auto coneParams = calculateConeParams();
    glm::vec3 u = coneParams.uAxis;
    glm::vec3 v = coneParams.vAxis;
    glm::vec3 axis = coneParams.axis;
    
    // 生成底面三角形
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = base + radius * dir1;
        glm::vec3 p2 = base + radius * dir2;
        
        vertices->push_back(osg::Vec3(base.x, base.y, base.z));
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(-axis.x, -axis.y, -axis.z));  // 底面法向量
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
    }
    
    // 生成侧面三角形
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = static_cast<float>(cos(angle1)) * u + static_cast<float>(sin(angle1)) * v;
        glm::vec3 dir2 = static_cast<float>(cos(angle2)) * u + static_cast<float>(sin(angle2)) * v;
        
        glm::vec3 p1 = base + radius * dir1;
        glm::vec3 p2 = base + radius * dir2;
        
        // 第一个三角形：底面点到顶点到下一个底面点
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(apex.x, apex.y, apex.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        
        // 计算侧面法向量（从底面指向顶点的方向）
        glm::vec3 sideNormal = glm::normalize(glm::cross(p2 - p1, apex - p1));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(sideNormal.x, sideNormal.y, sideNormal.z));
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角面绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
}

// ==================== 绘制完成检查和控制点验证 ====================

bool Cone3D_Geo::isDrawingComplete() const
{
    // 圆锥体需要2个控制点（底面中心点和顶点）才能完成绘制
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    return controlPoints.size() >= 2;
}

bool Cone3D_Geo::areControlPointsValid() const
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    
    // 检查控制点数量
    if (controlPoints.size() < 2) {
        return false;
    }
    
    // 检查控制点是否重合（允许一定的误差）
    const float epsilon = 0.001f;
    glm::vec3 diff = controlPoints[1].position - controlPoints[0].position;
    float distance = glm::length(diff);
    
    if (distance < epsilon) {
        return false; // 两点重合，无效
    }
    
    // 检查控制点坐标是否有效（不是NaN或无穷大）
    for (const auto& point : controlPoints) {
        if (std::isnan(point.x()) || std::isnan(point.y()) || std::isnan(point.z()) ||
            std::isinf(point.x()) || std::isinf(point.y()) || std::isinf(point.z())) {
            return false;
        }
    }
    
    // 检查高度是否合理（不能太短）
    if (distance < 0.01f) {
        return false; // 高度太短，无效
    }
    
    return true;
}
