#include "GeoRenderManager.h"
#include "../GeometryBase.h"
#include <osg/PolygonMode>
#include <osg/Depth>
#include <osg/CullFace>

GeoRenderManager::GeoRenderManager(Geo3D* parent)
    : m_parent(parent)
    , m_showPoints(true)
    , m_showEdges(true)
    , m_showFaces(true)
    , m_visible(true)
    , m_wireframeMode(false)
    , m_highlighted(false)
    , m_highlightColor(1.0f, 1.0f, 0.0f, 1.0f)
    , m_blendingEnabled(false)
{
    initializeRender();
}

void GeoRenderManager::initializeRender()
{
    // 初始化默认材质
    m_material = Material3D();
    
    // 创建OSG渲染状态
    m_stateSet = new osg::StateSet();
    m_osgMaterial = new osg::Material();
    m_blendFunc = new osg::BlendFunc();
    m_lineWidth = new osg::LineWidth(2.0f);
    m_pointSize = new osg::Point(5.0f);
    
    // 设置到几何节点
    if (m_parent && m_parent->mm_node()) {
        auto node = m_parent->mm_node()->getOSGNode();
        if (node.valid()) {
            node->setStateSet(m_stateSet.get());
        }
    }
    
    if (!m_stateSet.valid()) return;
    
    if (m_osgMaterial.valid()) {
        m_osgMaterial->setAmbient(osg::Material::FRONT_AND_BACK, 
                                 osg::Vec4(m_material.ambient.r, m_material.ambient.g, 
                                          m_material.ambient.b, m_material.ambient.a));
        m_osgMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, 
                                 osg::Vec4(m_material.diffuse.r, m_material.diffuse.g, 
                                          m_material.diffuse.b, m_material.diffuse.a));
        m_osgMaterial->setSpecular(osg::Material::FRONT_AND_BACK, 
                                  osg::Vec4(m_material.specular.r, m_material.specular.g, 
                                           m_material.specular.b, m_material.specular.a));
        m_osgMaterial->setShininess(osg::Material::FRONT_AND_BACK, m_material.shininess);
        
        m_stateSet->setAttributeAndModes(m_osgMaterial.get(), osg::StateAttribute::ON);
    }
    
    if (m_blendingEnabled) {
        if (m_blendFunc.valid()) {
            m_blendFunc->setSource(static_cast<osg::BlendFunc::BlendFuncMode>(GL_SRC_ALPHA));
            m_blendFunc->setDestination(static_cast<osg::BlendFunc::BlendFuncMode>(GL_ONE_MINUS_SRC_ALPHA));
            m_stateSet->setAttributeAndModes(m_blendFunc.get(), osg::StateAttribute::ON);
            m_stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        }
    } else {
        m_stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
    }
    
    if (m_lineWidth.valid()) {
        m_stateSet->setAttributeAndModes(m_lineWidth.get(), osg::StateAttribute::ON);
    }
    
    if (m_pointSize.valid()) {
        m_stateSet->setAttributeAndModes(m_pointSize.get(), osg::StateAttribute::ON);
    }
    
    if (m_wireframeMode) {
        osg::ref_ptr<osg::PolygonMode> polygonMode = new osg::PolygonMode();
        polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
        m_stateSet->setAttributeAndModes(polygonMode.get(), osg::StateAttribute::ON);
    }
}

void GeoRenderManager::setRenderMode(RenderMode mode)
{
    switch (mode) {
        case RENDER_POINTS:
            setShowPoints(true);
            setShowEdges(false);
            setShowFaces(false);
            break;
        case RENDER_WIREFRAME:
            setShowPoints(false);
            setShowEdges(true);
            setShowFaces(false);
            break;
        case RENDER_SOLID:
            setShowPoints(false);
            setShowEdges(false);
            setShowFaces(true);
            break;
        case RENDER_ALL:
            setShowPoints(true);
            setShowEdges(true);
            setShowFaces(true);
            break;
    }
}

void GeoRenderManager::setShowPoints(bool show)
{
    m_showPoints = show;
    if (m_parent && m_parent->mm_node()) {
        m_parent->mm_node()->setVertexVisible(show);
    }
}

void GeoRenderManager::setShowEdges(bool show)
{
    m_showEdges = show;
    if (m_parent && m_parent->mm_node()) {
        m_parent->mm_node()->setEdgeVisible(show);
    }
}

void GeoRenderManager::setShowFaces(bool show)
{
    m_showFaces = show;
    if (m_parent && m_parent->mm_node()) {
        m_parent->mm_node()->setFaceVisible(show);
    }
}

void GeoRenderManager::setVisible(bool visible)
{
    m_visible = visible;
    if (m_parent && m_parent->mm_node()) {
        auto node = m_parent->mm_node()->getOSGNode();
        if (node.valid()) {
            node->setNodeMask(visible ? 0xffffffff : 0x0);
        }
    }
}

void GeoRenderManager::setMaterial(const Material3D& material)
{
    m_material = material;
}

void GeoRenderManager::setMaterialType(MaterialType3D type)
{
    applyMaterialPreset(type);
}

void GeoRenderManager::setColor(const Color3D& color)
{
    m_material.diffuse = color;
}

void GeoRenderManager::setTransparency(float transparency)
{
    transparency = std::max(0.0f, std::min(1.0f, transparency));
    m_material.transparency = transparency;
    
    if (transparency < 1.0f) {
        m_blendingEnabled = true;
    }
}

void GeoRenderManager::setLineWidth(float width)
{
    if (m_lineWidth.valid()) {
        m_lineWidth->setWidth(width);
    }
}

void GeoRenderManager::setPointSize(float size)
{
    if (m_pointSize.valid()) {
        m_pointSize->setSize(size);
    }
}

void GeoRenderManager::setWireframeMode(bool enable)
{
    m_wireframeMode = enable;
}

void GeoRenderManager::setHighlighted(bool highlighted)
{
    m_highlighted = highlighted;
    if (highlighted) {
        m_material.diffuse = m_highlightColor;
    }
}

void GeoRenderManager::setHighlightColor(const Color3D& color)
{
    m_highlightColor = color;
    if (m_highlighted) {
        m_material.diffuse = color;
    }
}

void GeoRenderManager::applyMaterialPreset(MaterialType3D type)
{
    switch (type) {
        case Material_Basic3D:
            m_material.ambient = Color3D(0.2f, 0.2f, 0.2f, 1.0f);
            m_material.diffuse = Color3D(0.8f, 0.8f, 0.8f, 1.0f);
            m_material.specular = Color3D(0.0f, 0.0f, 0.0f, 1.0f);
            m_material.shininess = 0.0f;
            break;
            
        case Material_Phong3D:
            m_material.ambient = Color3D(0.2f, 0.2f, 0.2f, 1.0f);
            m_material.diffuse = Color3D(0.8f, 0.8f, 0.8f, 1.0f);
            m_material.specular = Color3D(1.0f, 1.0f, 1.0f, 1.0f);
            m_material.shininess = 32.0f;
            break;
            
        case Material_Blinn3D:
            m_material.ambient = Color3D(0.1f, 0.1f, 0.1f, 1.0f);
            m_material.diffuse = Color3D(0.7f, 0.7f, 0.7f, 1.0f);
            m_material.specular = Color3D(0.8f, 0.8f, 0.8f, 1.0f);
            m_material.shininess = 64.0f;
            break;
            
        case Material_Lambert3D:
            m_material.ambient = Color3D(0.3f, 0.3f, 0.3f, 1.0f);
            m_material.diffuse = Color3D(0.9f, 0.9f, 0.9f, 1.0f);
            m_material.specular = Color3D(0.0f, 0.0f, 0.0f, 1.0f);
            m_material.shininess = 0.0f;
            break;
            
        case Material_PBR3D:
            m_material.ambient = Color3D(0.04f, 0.04f, 0.04f, 1.0f);
            m_material.diffuse = Color3D(0.5f, 0.5f, 0.5f, 1.0f);
            m_material.specular = Color3D(0.04f, 0.04f, 0.04f, 1.0f);
            m_material.shininess = 128.0f;
            break;
    }
} 