#include "UndefinedGeo3D.h"
#include "../Common3D.h"
#include "../../util/LogManager.h"
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <GL/gl.h>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>

UndefinedGeo3D::UndefinedGeo3D()
{
    setGeoType(Geo_UndefinedGeo3D);
    // 确保基类正确初始化
    initialize();
    LOG_INFO("创建未定义几何体", "几何体");
}

void UndefinedGeo3D::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!mm_state()->isStateComplete())
    {
        mm_controlPoint()->addControlPoint(Point3D(worldPos.x, worldPos.y, worldPos.z));
        
        if (mm_controlPoint()->getControlPoints().size() >= 1)
        {
            mm_state()->setStateComplete();
        }
    }
}

void UndefinedGeo3D::completeDrawing()
{
    mm_state()->setStateComplete();
    updateGeometry();
    
    LOG_INFO("完成未定义几何体绘制", "几何体");
}

void UndefinedGeo3D::updateGeometry()
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



osg::ref_ptr<osg::Geometry> UndefinedGeo3D::createGeometry()
{
    return createDefaultGeometry();
}

osg::ref_ptr<osg::Geometry> UndefinedGeo3D::createDefaultGeometry()
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    
    if (controlPoint()->getControlPoints().empty())
    {
        // 如果没有控制点，创建一个默认的立方体
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
        
        // 创建一个简单的立方体
        float size = 1.0f;
        vertices->push_back(osg::Vec3(-size, -size, -size));
        vertices->push_back(osg::Vec3( size, -size, -size));
        vertices->push_back(osg::Vec3( size,  size, -size));
        vertices->push_back(osg::Vec3(-size,  size, -size));
        vertices->push_back(osg::Vec3(-size, -size,  size));
        vertices->push_back(osg::Vec3( size, -size,  size));
        vertices->push_back(osg::Vec3( size,  size,  size));
        vertices->push_back(osg::Vec3(-size,  size,  size));
        
        // 设置颜色
        osg::Vec4 color(0.8f, 0.8f, 0.8f, 1.0f);
        for (int i = 0; i < 8; ++i)
        {
            colors->push_back(color);
        }
        
        geometry->setVertexArray(vertices.get());
        geometry->setColorArray(colors.get());
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 添加线框绘制
        osg::ref_ptr<osg::DrawElementsUInt> lines = new osg::DrawElementsUInt(GL_LINES);
        // 立方体的12条边
        int indices[] = {
            0,1, 1,2, 2,3, 3,0,  // 底面
            4,5, 5,6, 6,7, 7,4,  // 顶面
            0,4, 1,5, 2,6, 3,7   // 竖边
        };
        for (int i = 0; i < 24; ++i)
        {
            lines->push_back(indices[i]);
        }
        geometry->addPrimitiveSet(lines.get());
        
        LOG_DEBUG("创建默认立方体几何体", "几何体");
    }
    else
    {
        // 根据控制点创建几何体
        const auto& controlPoints = controlPoint()->getControlPoints();
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
        
        for (const auto& point : controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f));
        }
        
        geometry->setVertexArray(vertices.get());
        geometry->setColorArray(colors.get());
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        
        // 绘制点
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
        
        LOG_DEBUG(QString("根据控制点创建几何体，点数: %1").arg(controlPoints.size()), "几何体");
    }
    
    return geometry;
} 

void UndefinedGeo3D::buildVertexGeometries()
{
    mm_node()->clearVertexGeometry();
    
    const auto& controlPoints = mm_controlPoint()->getControlPoints();
    if (controlPoints.empty())
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> vertexGeometry = mm_node()->getVertexGeometry();
    if (!vertexGeometry.valid())
        return;
    
    // 创建未定义几何体顶点的几何体
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
}

void UndefinedGeo3D::buildEdgeGeometries()
{
    clearEdgeGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 2)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> edgeGeometry = getEdgeGeometry();
    if (!edgeGeometry.valid())
        return;
    
    // 创建未定义几何体边的几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加所有边
    for (size_t i = 0; i < controlPoints.size() - 1; ++i)
    {
        vertices->push_back(osg::Vec3(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z()));
        vertices->push_back(osg::Vec3(controlPoints[i + 1].x(), controlPoints[i + 1].y(), controlPoints[i + 1].z()));
        
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
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

void UndefinedGeo3D::buildFaceGeometries()
{
    clearFaceGeometries();
    
    const auto& controlPoints = getControlPoints();
    if (controlPoints.size() < 3)
        return;
    
    // 获取现有的几何体
    osg::ref_ptr<osg::Geometry> faceGeometry = getFaceGeometry();
    if (!faceGeometry.valid())
        return;
    
    // 创建未定义几何体面的几何体
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // 添加所有顶点形成面
    for (const Point3D& point : controlPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                   m_parameters.fillColor.b, m_parameters.fillColor.a));
    }
    
    faceGeometry->setVertexArray(vertices.get());
    faceGeometry->setColorArray(colors.get());
    faceGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 使用三角形绘制（简单的扇形三角剖分）
    if (controlPoints.size() >= 3)
    {
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
        for (size_t i = 1; i < controlPoints.size() - 1; ++i)
        {
            indices->push_back(0);
            indices->push_back(i);
            indices->push_back(i + 1);
        }
        faceGeometry->addPrimitiveSet(indices.get());
    }
    
    addFaceGeometry(faceGeometry.get());
} 

bool UndefinedGeo3D::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 未定义几何体使用包围盒进行相交测试
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
            result.userData = const_cast<UndefinedGeo3D*>(this);
            result.point = ray.origin + t * rayDir;
            return true;
        }
    }
    
    return false;
} 