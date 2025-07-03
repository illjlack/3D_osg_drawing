#include "IndicatorFactory.h"
#include "../core/Enums3D.h"
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/MatrixTransform>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/PolygonMode>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 默认颜色设置
const Color3D IndicatorFactory::DefaultSettings::VERTEX_COLOR = Color3D(1.0f, 1.0f, 0.0f, 1.0f);
const Color3D IndicatorFactory::DefaultSettings::EDGE_COLOR = Color3D(0.0f, 1.0f, 0.0f, 1.0f);
const Color3D IndicatorFactory::DefaultSettings::FACE_COLOR = Color3D(0.0f, 0.0f, 1.0f, 1.0f);
const Color3D IndicatorFactory::DefaultSettings::VOLUME_COLOR = Color3D(1.0f, 0.0f, 1.0f, 1.0f);
const Color3D IndicatorFactory::DefaultSettings::HIGHLIGHT_COLOR = Color3D(1.0f, 0.5f, 0.0f, 1.0f);
const Color3D IndicatorFactory::DefaultSettings::SELECTION_COLOR = Color3D(1.0f, 0.0f, 0.0f, 1.0f);

osg::ref_ptr<osg::Node> IndicatorFactory::createVertexIndicator(const osg::Vec3& position, float size, const Color3D& color)
{
    osg::ref_ptr<osg::Geometry> geometry = createSphereGeometry(size);
    osg::ref_ptr<osg::Geode> geode = createIndicatorGeode(geometry, color);
    
    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();
    transform->setMatrix(osg::Matrix::translate(position));
    transform->addChild(geode);
    
    return transform;
}

osg::ref_ptr<osg::Node> IndicatorFactory::createEdgeIndicator(const osg::Vec3& center, const osg::Vec3& direction, 
                                                             float size, const Color3D& color)
{
    osg::ref_ptr<osg::Geometry> geometry = createBoxGeometry(size);
    osg::ref_ptr<osg::Geode> geode = createIndicatorGeode(geometry, color);
    
    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();
    transform->setMatrix(osg::Matrix::translate(center));
    transform->addChild(geode);
    
    return transform;
}

osg::ref_ptr<osg::Node> IndicatorFactory::createFaceIndicator(const osg::Vec3& center, const osg::Vec3& normal, 
                                                             float size, const Color3D& color)
{
    osg::ref_ptr<osg::Geometry> geometry = createPlaneGeometry(size);
    osg::ref_ptr<osg::Geode> geode = createIndicatorGeode(geometry, color, true);
    
    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();
    transform->setMatrix(osg::Matrix::translate(center));
    transform->addChild(geode);
    
    return transform;
}

osg::ref_ptr<osg::Node> IndicatorFactory::createVolumeIndicator(const osg::Vec3& center, float size, const Color3D& color)
{
    osg::ref_ptr<osg::Geometry> geometry = createBoxGeometry(size);
    osg::ref_ptr<osg::Geode> geode = createIndicatorGeode(geometry, color, true);
    
    osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();
    transform->setMatrix(osg::Matrix::translate(center));
    transform->addChild(geode);
    
    return transform;
}

osg::ref_ptr<osg::Node> IndicatorFactory::createGeneralIndicator(IndicatorType type, const osg::Vec3& position, 
                                                                float size, const Color3D& color)
{
    switch (type)
    {
    case IndicatorType::VERTEX_INDICATOR:
        return createVertexIndicator(position, size, color);
    case IndicatorType::EDGE_INDICATOR:
        return createEdgeIndicator(position, osg::Vec3(0, 0, 1), size, color);
    case IndicatorType::FACE_INDICATOR:
        return createFaceIndicator(position, osg::Vec3(0, 0, 1), size, color);
    case IndicatorType::VOLUME_INDICATOR:
        return createVolumeIndicator(position, size, color);
    default:
        return createVertexIndicator(position, size, color);
    }
}

osg::ref_ptr<osg::Node> IndicatorFactory::createHighlightIndicator(FeatureType featureType, const osg::Vec3& position, 
                                                                  float size, const Color3D& color)
{
    return createGeneralIndicator(static_cast<IndicatorType>(featureType), position, size * 1.2f, color);
}

osg::ref_ptr<osg::Node> IndicatorFactory::createSelectionIndicator(FeatureType featureType, const osg::Vec3& position, 
                                                                  float size, const Color3D& color)
{
    return createGeneralIndicator(static_cast<IndicatorType>(featureType), position, size * 1.5f, color);
}

osg::ref_ptr<osg::Node> IndicatorFactory::createAnimatedIndicator(IndicatorType type, const osg::Vec3& position, 
                                                                 float size, const Color3D& color)
{
    return createGeneralIndicator(type, position, size, color);
}

// 基本几何体创建方法
osg::ref_ptr<osg::Geometry> IndicatorFactory::createSphereGeometry(float radius, int segments)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    // 简单的球体生成
    for (int lat = 0; lat <= segments; ++lat)
    {
        float theta = static_cast<float>(M_PI * lat / segments);
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= segments; ++lon)
        {
            float phi = static_cast<float>(2.0 * M_PI * lon / segments);
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            osg::Vec3 normal(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            osg::Vec3 vertex = normal * radius;
            
            vertices->push_back(vertex);
            normals->push_back(normal);
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> IndicatorFactory::createBoxGeometry(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    float halfSize = size * 0.5f;
    
    // 简单的立方体
    vertices->push_back(osg::Vec3(-halfSize, -halfSize, -halfSize));
    vertices->push_back(osg::Vec3( halfSize, -halfSize, -halfSize));
    vertices->push_back(osg::Vec3( halfSize,  halfSize, -halfSize));
    vertices->push_back(osg::Vec3(-halfSize,  halfSize, -halfSize));
    vertices->push_back(osg::Vec3(-halfSize, -halfSize,  halfSize));
    vertices->push_back(osg::Vec3( halfSize, -halfSize,  halfSize));
    vertices->push_back(osg::Vec3( halfSize,  halfSize,  halfSize));
    vertices->push_back(osg::Vec3(-halfSize,  halfSize,  halfSize));
    
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> IndicatorFactory::createArrowGeometry(float length, float width)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    // 简单的箭头
    vertices->push_back(osg::Vec3(0, 0, 0));
    vertices->push_back(osg::Vec3(0, 0, length));
    
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> IndicatorFactory::createPlaneGeometry(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    float halfSize = size * 0.5f;
    
    // 简单的平面
    vertices->push_back(osg::Vec3(-halfSize, -halfSize, 0));
    vertices->push_back(osg::Vec3( halfSize, -halfSize, 0));
    vertices->push_back(osg::Vec3( halfSize,  halfSize, 0));
    vertices->push_back(osg::Vec3(-halfSize,  halfSize, 0));
    
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::StateSet> IndicatorFactory::createIndicatorStateSet(const Color3D& color, bool transparent)
{
    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
    
    osg::ref_ptr<osg::Material> material = createIndicatorMaterial(color);
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
    
    if (transparent)
    {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
    
    return stateSet;
}

osg::ref_ptr<osg::Material> IndicatorFactory::createIndicatorMaterial(const Color3D& color)
{
    osg::ref_ptr<osg::Material> material = new osg::Material();
    osg::Vec4 colorVec(color.r, color.g, color.b, color.a);
    
    material->setDiffuse(osg::Material::FRONT_AND_BACK, colorVec);
    material->setAmbient(osg::Material::FRONT_AND_BACK, colorVec * 0.3f);
    
    return material;
}

osg::ref_ptr<osg::Geode> IndicatorFactory::createIndicatorGeode(osg::ref_ptr<osg::Geometry> geometry, 
                                                               const Color3D& color, bool transparent)
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable(geometry);
    
    osg::ref_ptr<osg::StateSet> stateSet = createIndicatorStateSet(color, transparent);
    geode->setStateSet(stateSet);
    
    return geode;
}

void IndicatorFactory::updateIndicatorColor(osg::Node* indicator, const Color3D& color)
{
    if (!indicator) return;
    
    osg::ref_ptr<osg::StateSet> stateSet = indicator->getOrCreateStateSet();
    osg::ref_ptr<osg::Material> material = createIndicatorMaterial(color);
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
}

void IndicatorFactory::updateIndicatorSize(osg::Node* indicator, float size)
{
    // TODO: 实现大小更新
}

void IndicatorFactory::updateIndicatorPosition(osg::Node* indicator, const osg::Vec3& position)
{
    if (!indicator) return;
    
    osg::MatrixTransform* transform = dynamic_cast<osg::MatrixTransform*>(indicator);
    if (transform)
    {
        transform->setMatrix(osg::Matrix::translate(position));
    }
}

void IndicatorFactory::setupIndicatorTransform(osg::Node* node, const osg::Vec3& position)
{
    osg::MatrixTransform* transform = dynamic_cast<osg::MatrixTransform*>(node);
    if (transform)
    {
        transform->setMatrix(osg::Matrix::translate(position));
    }
} 