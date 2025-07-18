#include "GeoRenderManager.h"
#include "../GeometryBase.h"
#include <osg/PolygonMode>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/BlendFunc>

GeoRenderManager::GeoRenderManager(Geo3D* parent)
    : m_parent(parent)
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
    m_lineWidth = new osg::LineWidth(1.0f);  // 与根节点保持一致
    m_pointSize = new osg::Point(3.0f);      // 与根节点保持一致
    
    // 1. 三套材质
    m_pointMaterial = new osg::Material(*m_osgMaterial);
    m_edgeMaterial  = new osg::Material(*m_osgMaterial);
    m_faceMaterial  = new osg::Material(*m_osgMaterial);

    // 2. 设置默认颜色
    m_pointMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1,0,0,1)); // 红点
    m_edgeMaterial ->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0,1,0,1)); // 绿线
    m_faceMaterial ->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0,0,1,1)); // 蓝面

    // 3. 拿到三组几何
    auto geomPts  = m_parent->mm_node()->getVertexGeometry();
    auto geomEdges= m_parent->mm_node()->getEdgeGeometry();
    auto geomFaces= m_parent->mm_node()->getFaceGeometry();

    // 4. 分别挂到它们自己的 StateSet
    if (geomPts)
    {
        auto ss = geomPts->getOrCreateStateSet();
        ss->setAttributeAndModes(m_pointMaterial.get(),osg::StateAttribute::ON);
        ss->setAttributeAndModes(m_pointSize .get(),osg::StateAttribute::ON);
        
        // 点抗锯齿设置（继承根节点的混合函数）
        ss->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
    }
    if (geomEdges)
    {
        auto ss = geomEdges->getOrCreateStateSet();
        ss->setAttributeAndModes(m_edgeMaterial.get(),osg::StateAttribute::ON);
        ss->setAttributeAndModes(m_lineWidth .get(),osg::StateAttribute::ON);
        
        // 线抗锯齿设置（继承根节点的混合函数）
        ss->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
        // 要虚线、加 LineStipple
    }
    if (geomFaces)
    {
        auto ss = geomFaces->getOrCreateStateSet();
        ss->setAttributeAndModes(m_faceMaterial.get(),osg::StateAttribute::ON);
        if (m_wireframeMode)
        {
            auto pm = new osg::PolygonMode();
            pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            ss->setAttributeAndModes(pm,osg::StateAttribute::ON);
            
            // 线框模式下的抗锯齿（继承根节点的混合函数）
            ss->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
        }
    }

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

void GeoRenderManager::setPointColor(const Color3D& color)
{
    if (m_pointMaterial.valid()) {
        m_pointMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, 
                                   osg::Vec4(color.r, color.g, color.b, color.a));
    }
}

void GeoRenderManager::setEdgeColor(const Color3D& color)
{
    if (m_edgeMaterial.valid()) {
        m_edgeMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, 
                                  osg::Vec4(color.r, color.g, color.b, color.a));
    }
}

void GeoRenderManager::setFaceColor(const Color3D& color)
{
    if (m_faceMaterial.valid()) {
        m_faceMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, 
                                  osg::Vec4(color.r, color.g, color.b, color.a));
    }
}
