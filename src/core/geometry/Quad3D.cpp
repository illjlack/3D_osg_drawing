#include "Quad3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include "../../util/MathUtils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Quad3D_Geo::Quad3D_Geo()
    : m_area(0.0f)
    , m_normal(0, 0, 1)
{
    m_geoType = Geo_Quad3D;
    // 确保基类正确初始化
    initialize();
}

void Quad3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        if (controlPoints.size() == 4)
        {
            // 直接计算法向量和面积
            const auto& v1 = controlPoints[0].position;
            const auto& v2 = controlPoints[1].position;
            const auto& v3 = controlPoints[2].position;
            const auto& v4 = controlPoints[3].position;
            
            // 计算法向量（使用前三个点）
            glm::vec3 edge1 = v2 - v1;
            glm::vec3 edge2 = v3 - v1;
            m_normal = glm::normalize(glm::cross(edge1, edge2));
            
            // 计算面积（使用两个三角形）
            glm::vec3 cross1 = glm::cross(v2 - v1, v3 - v1);
            glm::vec3 cross2 = glm::cross(v3 - v1, v4 - v1);
            m_area = 0.5f * (glm::length(cross1) + glm::length(cross2));
            
            mm_state()->setStateComplete();
        }
        
        mm_state()->setControlPointsUpdated();
    }
}

void Quad3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (!mm_state()->isStateComplete() && controlPoints.size() < 4)
    {
        // 设置临时点用于预览
        mm_controlPoint()->setTempPoint(Point3D(worldPos));
        mm_state()->setTemporaryPointsUpdated();
    }
}

void Quad3D_Geo::buildVertexGeometries()
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
}

void Quad3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建四边形边界线几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 构建所有点（包括临时点）
    std::vector<Point3D> allPoints = controlPoints;
    if (!mm_state()->isStateComplete() && mm_controlPoint()->getTempPoint().position != glm::vec3(0))
    {
        allPoints.push_back(mm_controlPoint()->getTempPoint());
    }
    
    // 添加边线
    for (size_t i = 0; i < allPoints.size(); ++i)
    {
        vertices->push_back(osg::Vec3(allPoints[i].x(), allPoints[i].y(), allPoints[i].z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 如果有4个或更多点，形成闭合四边形
    if (allPoints.size() >= 4)
    {
        vertices->push_back(osg::Vec3(allPoints[0].x(), allPoints[0].y(), allPoints[0].z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
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
    
    // 几何体已经通过mm_node()->getEdgeGeometry()获取，直接使用
}

void Quad3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 使用lambda表达式计算四边形参数
    auto calculateQuadParams = [&]() -> MathUtils::QuadParameters {
        if (controlPoints.size() >= 4)
        {
            const auto& v1 = controlPoints[0].position;
            const auto& v2 = controlPoints[1].position;
            const auto& v3 = controlPoints[2].position;
            const auto& v4 = controlPoints[3].position;
            return MathUtils::calculateQuadParameters(v1, v2, v3, v4);
        }
        else if (controlPoints.size() >= 3)
        {
            // 如果只有3个点，用第3个点作为第4个点
            const auto& v1 = controlPoints[0].position;
            const auto& v2 = controlPoints[1].position;
            const auto& v3 = controlPoints[2].position;
            const auto& v4 = controlPoints[2].position;
            return MathUtils::calculateQuadParameters(v1, v2, v3, v4);
        }
        else
        {
            // 默认参数
            return MathUtils::QuadParameters{};
        }
    };
    
    auto quadParams = calculateQuadParams();
    
    // 更新成员变量
    m_normal = quadParams.normal;
    m_area = quadParams.area;
    
    // 创建四边形面几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    // 添加顶点
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                   m_parameters.fillColor.b, m_parameters.fillColor.a));
    }
    
    // 计算法向量
    for (int i = 0; i < controlPoints.size(); ++i)
    {
        normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角形绘制（两个三角形组成四边形）
    if (controlPoints.size() >= 4)
    {
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 6);
        (*indices)[0] = 0; (*indices)[1] = 1; (*indices)[2] = 2;
        (*indices)[3] = 0; (*indices)[4] = 2; (*indices)[5] = 3;
        geometry->addPrimitiveSet(indices);
    }
    else if (controlPoints.size() >= 3)
    {
        // 如果只有3个点，绘制三角形
        osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3);
        geometry->addPrimitiveSet(drawArrays);
    }
    
    // 几何体已经通过mm_node()->getFaceGeometry()获取，直接使用
} 
