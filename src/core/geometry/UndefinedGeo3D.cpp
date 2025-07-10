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
    LOG_INFO("创建未定义几何体", "几何体");
}

void UndefinedGeo3D::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos.x, worldPos.y, worldPos.z));
        
        if (getControlPoints().size() >= 1)
        {
            completeDrawing();
        }
    }
}

void UndefinedGeo3D::completeDrawing()
{
    setStateComplete();
    clearStateEditing();
    updateGeometry();
    
    LOG_INFO("完成未定义几何体绘制", "几何体");
}

void UndefinedGeo3D::updateGeometry()
{
    if (isGeometryDirty())
    {
        updateOSGNode();
    }
}

std::vector<FeatureType> UndefinedGeo3D::getSupportedFeatureTypes() const
{
    // 未定义几何体支持所有Feature类型
    return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> UndefinedGeo3D::createGeometry()
{
    return createDefaultGeometry();
}

osg::ref_ptr<osg::Geometry> UndefinedGeo3D::createDefaultGeometry()
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    
    if (getControlPoints().empty())
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
        const auto& controlPoints = getControlPoints();
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

std::vector<PickingFeature> UndefinedGeo3D::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    const auto& controlPoints = getControlPoints();
    
    for (size_t i = 0; i < controlPoints.size(); ++i)
    {
        PickingFeature feature(FeatureType::VERTEX, static_cast<uint32_t>(i));
        feature.center = osg::Vec3(controlPoints[i].x(), controlPoints[i].y(), controlPoints[i].z());
        feature.size = 0.1f; // 设置合适的大小
        
        features.push_back(feature);
    }
    
    return features;
}

std::vector<PickingFeature> UndefinedGeo3D::extractEdgeFeatures() const
{
    std::vector<PickingFeature> features;
    const auto& controlPoints = getControlPoints();
    
    if (controlPoints.size() >= 2)
    {
        for (size_t i = 0; i < controlPoints.size() - 1; ++i)
        {
            PickingFeature feature(FeatureType::EDGE, static_cast<uint32_t>(i));
            
            // 计算边的中点
            glm::vec3 midPoint = (controlPoints[i].position + controlPoints[i + 1].position) * 0.5f;
            feature.center = osg::Vec3(midPoint.x, midPoint.y, midPoint.z);
            feature.size = glm::length(controlPoints[i + 1].position - controlPoints[i].position);
            
            features.push_back(feature);
        }
    }
    
    return features;
}

std::vector<PickingFeature> UndefinedGeo3D::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    // 未定义几何体默认有一个面
    if (!getControlPoints().empty())
    {
        PickingFeature feature(FeatureType::FACE, 0);
        feature.center = osg::Vec3(getControlPoints()[0].x(), getControlPoints()[0].y(), getControlPoints()[0].z());
        feature.size = 1.0f; // 设置合适的大小
        
        features.push_back(feature);
    }
    
    return features;
} 