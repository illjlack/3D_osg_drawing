#include "GeoMaterialManager.h"
#include "../GeometryBase.h"
#include <osg/PolygonMode>
#include <osg/Depth>
#include <osg/CullFace>

GeoMaterialManager::GeoMaterialManager(Geo3D* parent)
    : m_parent(parent)
    , m_blendingEnabled(false)
    , m_wireframeMode(false)
    , m_depthTest(true)
    , m_depthWrite(true)
    , m_materialDirty(true)
{
    initializeMaterial();
}

void GeoMaterialManager::initializeMaterial()
{
    // 初始化默认材质
    m_material = Material3D();
    
    // 创建OSG材质对象
    m_osgMaterial = new osg::Material();
    m_blendFunc = new osg::BlendFunc();
    m_lineWidth = new osg::LineWidth(2.0f);
    m_pointSize = new osg::Point(5.0f);
    
    createDefaultStateSet();
    updateOSGMaterial();
}

void GeoMaterialManager::setMaterial(const Material3D& material)
{
    if (m_material.ambient.r != material.ambient.r ||
        m_material.ambient.g != material.ambient.g ||
        m_material.ambient.b != material.ambient.b ||
        m_material.ambient.a != material.ambient.a ||
        m_material.diffuse.r != material.diffuse.r ||
        m_material.diffuse.g != material.diffuse.g ||
        m_material.diffuse.b != material.diffuse.b ||
        m_material.diffuse.a != material.diffuse.a ||
        m_material.specular.r != material.specular.r ||
        m_material.specular.g != material.specular.g ||
        m_material.specular.b != material.specular.b ||
        m_material.specular.a != material.specular.a ||
        m_material.shininess != material.shininess ||
        m_material.transparency != material.transparency ||
        m_material.type != material.type) {
        
        m_material = material;
        m_materialDirty = true;
        updateMaterialInternal();
    }
}

void GeoMaterialManager::updateMaterial()
{
    if (m_materialDirty) {
        updateMaterialInternal();
    }
}

void GeoMaterialManager::updateMaterialInternal()
{
    // 只进行内部更新，不调用任何外部函数
    updateOSGMaterial();
    updateRenderingAttributes();
    m_materialDirty = false;
    
    // 不调用任何外部函数，避免递归
    // 渲染更新由外部调用者负责
}

void GeoMaterialManager::resetMaterial()
{
    setMaterial(Material3D());
}

void GeoMaterialManager::setPointColor(const Color3D& color)
{
    // 直接设置材质属性，避免递归调用
    m_material.diffuse = color;
    m_materialDirty = true;
    updateMaterialInternal();
}

void GeoMaterialManager::setLineColor(const Color3D& color)
{
    // 直接设置材质属性，避免递归调用
    m_material.diffuse = color;
    m_materialDirty = true;
    updateMaterialInternal();
}

void GeoMaterialManager::setFaceColor(const Color3D& color)
{
    // 直接设置材质属性，避免递归调用
    m_material.diffuse = color;
    m_materialDirty = true;
    updateMaterialInternal();
}

void GeoMaterialManager::setLineWidth(float width)
{
    // 直接设置线宽，避免递归调用
    if (m_lineWidth.valid()) {
        m_lineWidth->setWidth(width);
    }
}

void GeoMaterialManager::setPointSize(float size)
{
    // 直接设置点大小，避免递归调用
    if (m_pointSize.valid()) {
        m_pointSize->setSize(size);
    }
}

void GeoMaterialManager::setTransparency(float transparency)
{
    transparency = std::max(0.0f, std::min(1.0f, transparency));
    
    if (m_material.transparency != transparency) {
        m_material.transparency = transparency;
        m_materialDirty = true;
        
        if (transparency < 1.0f) {
            m_blendingEnabled = true;
            if (m_blendFunc.valid()) {
                m_blendFunc->setSource(static_cast<osg::BlendFunc::BlendFuncMode>(GL_SRC_ALPHA));
                m_blendFunc->setDestination(static_cast<osg::BlendFunc::BlendFuncMode>(GL_ONE_MINUS_SRC_ALPHA));
            }
        }
        
        updateMaterialInternal();
    }
}

void GeoMaterialManager::setMaterialType(MaterialType3D type)
{
    if (m_material.type != type) {
        m_material.type = type;
        applyMaterialPreset(type);
    }
}

void GeoMaterialManager::setAmbient(const Color3D& ambient)
{
    m_material.ambient = ambient;
    m_materialDirty = true;
    updateMaterialInternal();
}

void GeoMaterialManager::setDiffuse(const Color3D& diffuse)
{
    m_material.diffuse = diffuse;
    m_materialDirty = true;
    updateMaterialInternal();
}

void GeoMaterialManager::setSpecular(const Color3D& specular)
{
    m_material.specular = specular;
    m_materialDirty = true;
    updateMaterialInternal();
}

void GeoMaterialManager::setShininess(float shininess)
{
    m_material.shininess = shininess;
    m_materialDirty = true;
    updateMaterialInternal();
}

void GeoMaterialManager::setWireframeMode(bool enable)
{
    if (m_wireframeMode != enable) {
        m_wireframeMode = enable;
    }
}

void GeoMaterialManager::updateOSGMaterial()
{
    if (!m_osgMaterial.valid()) return;
    
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
}

void GeoMaterialManager::updateRenderingAttributes()
{
    // 根据材质类型和设置更新渲染属性
    if (m_material.transparency < 1.0f) {
        m_blendingEnabled = true;
        if (m_blendFunc.valid()) {
            m_blendFunc->setSource(static_cast<osg::BlendFunc::BlendFuncMode>(GL_SRC_ALPHA));
            m_blendFunc->setDestination(static_cast<osg::BlendFunc::BlendFuncMode>(GL_ONE_MINUS_SRC_ALPHA));
        }
    }
}

void GeoMaterialManager::createDefaultStateSet()
{
    m_stateSet = new osg::StateSet();
}



void GeoMaterialManager::applyMaterialPreset(MaterialType3D type)
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
    
    m_materialDirty = true;
    updateMaterialInternal();
} 