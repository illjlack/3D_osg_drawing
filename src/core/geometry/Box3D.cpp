#include "Box3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>
#include <algorithm>
#include "../../util/MathUtils.h"

Box3D_Geo::Box3D_Geo()
    : m_size(1.0f, 1.0f, 1.0f)
{
    m_geoType = Geo_Box3D;
    // 确保基类正确初始化
    initialize();
}

void Box3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateDrawComplete())
    {
        // 添加控制点
        mm_controlPoint()->addControlPoint(Point3D(worldPos));
        
        const auto& controlPoints = mm_controlPoint()->getControlPoints();
        
        if (controlPoints.size() == 2)
        {
            // 计算长方体尺寸
            glm::vec3 diff = controlPoints[1].position - controlPoints[0].position;
            m_size = glm::abs(diff);
            mm_state()->setStateDrawComplete();
        }
        
        mm_state()->setControlPointsUpdated();
    }
}

void Box3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (!mm_state()->isStateDrawComplete() && controlPoints.size() < 2)
    {
        // 设置临时点用于预览
        mm_controlPoint()->setTempPoint(Point3D(worldPos));
        mm_state()->setTemporaryPointsUpdated();
    }
}

void Box3D_Geo::buildVertexGeometries()
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
    
    // 几何体已经通过mm_node()->getVertexGeometry()获取，直接使用
}

void Box3D_Geo::buildEdgeGeometries()
{
    mm_node()->clearEdgeGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getEdgeGeometry();
    if (!geometry.valid())
        return;
    
    // 创建边的几何体（长方体边界线）
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 使用lambda表达式计算长方体参数
    auto calculateBoxParams = [&]() -> MathUtils::BoxParameters {
        glm::vec3 min = glm::min(controlPoints[0].position, controlPoints[1].position);
        glm::vec3 max = glm::max(controlPoints[0].position, controlPoints[1].position);
        return MathUtils::calculateBoxParameters(min, max);
    };
    
    auto boxParams = calculateBoxParams();
    glm::vec3 center = boxParams.center;
    glm::vec3 size = boxParams.size;
    glm::vec3 halfSize = size * 0.5f;
    
    // 添加8个顶点
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z));
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z));
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z));
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z));
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z));
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z));
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z));
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z));
    
    // 为每个顶点设置颜色
    for (int i = 0; i < 8; ++i)
    {
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 绘制边界线
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    // 前面
    indices->push_back(0); indices->push_back(1);
    indices->push_back(1); indices->push_back(2);
    indices->push_back(2); indices->push_back(3);
    indices->push_back(3); indices->push_back(0);
    
    // 后面
    indices->push_back(4); indices->push_back(5);
    indices->push_back(5); indices->push_back(6);
    indices->push_back(6); indices->push_back(7);
    indices->push_back(7); indices->push_back(4);
    
    // 连接前后
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
}

void Box3D_Geo::buildFaceGeometries()
{
    mm_node()->clearFaceGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> geometry = mm_node()->getFaceGeometry();
    if (!geometry.valid())
        return;
    
    // 创建面的几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 使用lambda表达式计算长方体参数
    auto calculateBoxParams = [&]() -> MathUtils::BoxParameters {
        glm::vec3 min = glm::min(controlPoints[0].position, controlPoints[1].position);
        glm::vec3 max = glm::max(controlPoints[0].position, controlPoints[1].position);
        return MathUtils::calculateBoxParameters(min, max);
    };
    
    auto boxParams = calculateBoxParams();
    glm::vec3 center = boxParams.center;
    glm::vec3 size = boxParams.size;
    glm::vec3 halfSize = size * 0.5f;
    
    // 添加8个顶点
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z));
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z));
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z));
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z));
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z));
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z));
    vertices->push_back(osg::Vec3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z));
    vertices->push_back(osg::Vec3(center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z));
    
    // 为每个顶点设置颜色
    for (int i = 0; i < 8; ++i)
    {
        colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                   m_parameters.fillColor.b, m_parameters.fillColor.a));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 绘制6个面（每个面2个三角形）
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
    for (int i = 0; i < 8; ++i)
    {
        normals->push_back(osg::Vec3(0, 0, 1)); // 简化法线
    }
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
}

