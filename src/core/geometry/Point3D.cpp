#include "Point3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Point3D_Geo::Point3D_Geo()
{
    m_geoType = Geo_Point3D;
}

void Point3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        markGeometryDirty();
        completeDrawing();
    }
}

void Point3D_Geo::completeDrawing()
{
    if (!m_controlPoints.empty())
    {
        Geo3D::completeDrawing();
    }
}

void Point3D_Geo::updateGeometry()
{
    updateOSGNode();
}

std::vector<FeatureType> Point3D_Geo::getSupportedFeatureTypes() const
{
    return {FeatureType::VERTEX};
}

osg::ref_ptr<osg::Geometry> Point3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    return createPointGeometry(m_parameters.pointShape, m_parameters.pointSize);
}

std::vector<PickingFeature> Point3D_Geo::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (!m_controlPoints.empty())
    {
        PickingFeature feature(FeatureType::VERTEX, 0);
        feature.center = osg::Vec3(m_controlPoints[0].x(), m_controlPoints[0].y(), m_controlPoints[0].z());
        feature.size = m_parameters.pointSize * 0.01f;
        features.push_back(feature);
    }
    
    return features;
}

osg::ref_ptr<osg::Geometry> Point3D_Geo::createPointGeometry(PointShape3D shape, float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    const Point3D& point = m_controlPoints[0];
    
    switch (shape)
    {
    case Point_Circle3D:
        {
            // 创建圆形点
            int segments = 16;
            float radius = size * 0.01f;
            for (int i = 0; i <= segments; ++i)
            {
                float angle = 2.0f * M_PI * i / segments;
                float x = point.x() + radius * cos(angle);
                float y = point.y() + radius * sin(angle);
                vertices->push_back(osg::Vec3(x, y, point.z()));
            }
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_FAN, 0, vertices->size()));
        }
        break;
        
    case Point_Square3D:
        {
            // 创建方形点
            float half = size * 0.01f;
            vertices->push_back(osg::Vec3(point.x() - half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() + half, point.z()));
            vertices->push_back(osg::Vec3(point.x() - half, point.y() + half, point.z()));
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));
        }
        break;
        
    case Point_Triangle3D:
        {
            // 创建三角形点
            float half = size * 0.01f;
            vertices->push_back(osg::Vec3(point.x(), point.y() + half, point.z()));
            vertices->push_back(osg::Vec3(point.x() - half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() - half, point.z()));
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
        }
        break;
        
    default:
        // 默认为简单点
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, 1));
        break;
    }
    
    // 设置颜色
    for (size_t i = 0; i < vertices->size(); ++i)
    {
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    return geometry;
} 