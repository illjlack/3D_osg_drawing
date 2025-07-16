#include "GeoMaterialManager.h"
#include "../GeometryBase.h"
#include <osg/PolygonMode>
#include <osg/Depth>
#include <osg/CullFace>

GeoMaterialManager::GeoMaterialManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_blendingEnabled(false)
    , m_wireframeMode(false)
    , m_pointMode(false)
    , m_depthTest(true)
    , m_depthWrite(true)
    , m_twoSided(false)
    , m_materialDirty(true)
    , m_stateSetDirty(true)
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
        m_material.emission.r != material.emission.r ||
        m_material.emission.g != material.emission.g ||
        m_material.emission.b != material.emission.b ||
        m_material.emission.a != material.emission.a ||
        m_material.shininess != material.shininess ||
        m_material.transparency != material.transparency ||
        m_material.type != material.type) {
        
        m_material = material;
        m_materialDirty = true;
        updateMaterial();
        emit materialChanged();
    }
}

void GeoMaterialManager::updateMaterial()
{
    if (m_materialDirty) {
        updateOSGMaterial();
        updateRenderingAttributes();
        updateStateSet();
        m_materialDirty = false;
    }
}

void GeoMaterialManager::resetMaterial()
{
    setMaterial(Material3D());
}

void GeoMaterialManager::setPointColor(const Color3D& color)
{
    if (m_parent && m_parent->getParameters().pointColor.r != color.r ||
        m_parent->getParameters().pointColor.g != color.g ||
        m_parent->getParameters().pointColor.b != color.b ||
        m_parent->getParameters().pointColor.a != color.a) {
        
        auto params = m_parent->getParameters();
        params.pointColor = color;
        m_parent->setParameters(params);
        
        emit colorChanged();
    }
}

void GeoMaterialManager::setLineColor(const Color3D& color)
{
    if (m_parent && m_parent->getParameters().lineColor.r != color.r ||
        m_parent->getParameters().lineColor.g != color.g ||
        m_parent->getParameters().lineColor.b != color.b ||
        m_parent->getParameters().lineColor.a != color.a) {
        
        auto params = m_parent->getParameters();
        params.lineColor = color;
        m_parent->setParameters(params);
        
        emit colorChanged();
    }
}

void GeoMaterialManager::setFaceColor(const Color3D& color)
{
    if (m_parent && m_parent->getParameters().fillColor.r != color.r ||
        m_parent->getParameters().fillColor.g != color.g ||
        m_parent->getParameters().fillColor.b != color.b ||
        m_parent->getParameters().fillColor.a != color.a) {
        
        auto params = m_parent->getParameters();
        params.fillColor = color;
        m_parent->setParameters(params);
        
        emit colorChanged();
    }
}

const Color3D& GeoMaterialManager::getPointColor() const
{
    return m_parent ? m_parent->getParameters().pointColor : Color3D();
}

const Color3D& GeoMaterialManager::getLineColor() const
{
    return m_parent ? m_parent->getParameters().lineColor : Color3D();
}

const Color3D& GeoMaterialManager::getFaceColor() const
{
    return m_parent ? m_parent->getParameters().fillColor : Color3D();
}

void GeoMaterialManager::setLineWidth(float width)
{
    if (m_parent && m_parent->getParameters().lineWidth != width) {
        auto params = m_parent->getParameters();
        params.lineWidth = width;
        m_parent->setParameters(params);
        
        if (m_lineWidth.valid()) {
            m_lineWidth->setWidth(width);
        }
        
        emit linePropertiesChanged();
    }
}

float GeoMaterialManager::getLineWidth() const
{
    return m_parent ? m_parent->getParameters().lineWidth : 1.0f;
}

void GeoMaterialManager::setLineStyle(LineStyle3D style)
{
    if (m_parent && m_parent->getParameters().lineStyle != style) {
        auto params = m_parent->getParameters();
        params.lineStyle = style;
        m_parent->setParameters(params);
        
        emit linePropertiesChanged();
    }
}

LineStyle3D GeoMaterialManager::getLineStyle() const
{
    return m_parent ? m_parent->getParameters().lineStyle : Line_Solid3D;
}

void GeoMaterialManager::setLineDashPattern(float pattern)
{
    if (m_parent && m_parent->getParameters().lineDashPattern != pattern) {
        auto params = m_parent->getParameters();
        params.lineDashPattern = pattern;
        m_parent->setParameters(params);
        
        emit linePropertiesChanged();
    }
}

float GeoMaterialManager::getLineDashPattern() const
{
    return m_parent ? m_parent->getParameters().lineDashPattern : 1.0f;
}

void GeoMaterialManager::setPointSize(float size)
{
    if (m_parent && m_parent->getParameters().pointSize != size) {
        auto params = m_parent->getParameters();
        params.pointSize = size;
        m_parent->setParameters(params);
        
        if (m_pointSize.valid()) {
            m_pointSize->setSize(size);
        }
        
        emit pointPropertiesChanged();
    }
}

float GeoMaterialManager::getPointSize() const
{
    return m_parent ? m_parent->getParameters().pointSize : 5.0f;
}

void GeoMaterialManager::setPointShape(PointShape3D shape)
{
    if (m_parent && m_parent->getParameters().pointShape != shape) {
        auto params = m_parent->getParameters();
        params.pointShape = shape;
        m_parent->setParameters(params);
        
        emit pointPropertiesChanged();
    }
}

PointShape3D GeoMaterialManager::getPointShape() const
{
    return m_parent ? m_parent->getParameters().pointShape : Point_Circle3D;
}

void GeoMaterialManager::setFillType(FillType3D type)
{
    if (m_parent && m_parent->getParameters().fillType != type) {
        auto params = m_parent->getParameters();
        params.fillType = type;
        m_parent->setParameters(params);
        
        emit facePropertiesChanged();
    }
}

FillType3D GeoMaterialManager::getFillType() const
{
    return m_parent ? m_parent->getParameters().fillType : Fill_Solid3D;
}

void GeoMaterialManager::setTransparency(float transparency)
{
    transparency = std::max(0.0f, std::min(1.0f, transparency));
    
    if (m_material.transparency != transparency) {
        m_material.transparency = transparency;
        m_materialDirty = true;
        
        // 根据透明度自动启用混合
        enableBlending(transparency < 1.0f);
        
        updateMaterial();
        emit materialChanged();
    }
}

float GeoMaterialManager::getTransparency() const
{
    return m_material.transparency;
}

void GeoMaterialManager::setMaterialType(MaterialType3D type)
{
    if (m_material.type != type) {
        m_material.type = type;
        applyMaterialPreset(type);
        emit materialChanged();
    }
}

MaterialType3D GeoMaterialManager::getMaterialType() const
{
    return m_material.type;
}

void GeoMaterialManager::setAmbient(const Color3D& ambient)
{
    m_material.ambient = ambient;
    m_materialDirty = true;
    updateMaterial();
}

void GeoMaterialManager::setDiffuse(const Color3D& diffuse)
{
    m_material.diffuse = diffuse;
    m_materialDirty = true;
    updateMaterial();
}

void GeoMaterialManager::setSpecular(const Color3D& specular)
{
    m_material.specular = specular;
    m_materialDirty = true;
    updateMaterial();
}

void GeoMaterialManager::setEmission(const Color3D& emission)
{
    m_material.emission = emission;
    m_materialDirty = true;
    updateMaterial();
}

void GeoMaterialManager::setShininess(float shininess)
{
    m_material.shininess = shininess;
    m_materialDirty = true;
    updateMaterial();
}

void GeoMaterialManager::setBlendMode(osg::BlendFunc::BlendFuncMode src, osg::BlendFunc::BlendFuncMode dst)
{
    if (m_blendFunc.valid()) {
        m_blendFunc->setSource(src);
        m_blendFunc->setDestination(dst);
        m_stateSetDirty = true;
        updateStateSet();
        emit blendingChanged();
    }
}

void GeoMaterialManager::enableBlending(bool enable)
{
    if (m_blendingEnabled != enable) {
        m_blendingEnabled = enable;
        m_stateSetDirty = true;
        updateStateSet();
        emit blendingChanged();
    }
}

bool GeoMaterialManager::isBlendingEnabled() const
{
    return m_blendingEnabled;
}

osg::ref_ptr<osg::StateSet> GeoMaterialManager::createStateSet()
{
    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
    
    // 设置材质
    if (m_osgMaterial.valid()) {
        stateSet->setAttributeAndModes(m_osgMaterial.get(), osg::StateAttribute::ON);
    }
    
    // 设置混合
    if (m_blendingEnabled && m_blendFunc.valid()) {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(m_blendFunc.get());
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
    
    // 设置线宽
    if (m_lineWidth.valid()) {
        stateSet->setAttributeAndModes(m_lineWidth.get(), osg::StateAttribute::ON);
    }
    
    // 设置点大小
    if (m_pointSize.valid()) {
        stateSet->setAttributeAndModes(m_pointSize.get(), osg::StateAttribute::ON);
    }
    
    return stateSet;
}

void GeoMaterialManager::applyStateSet(osg::ref_ptr<osg::StateSet> stateSet)
{
    m_stateSet = stateSet;
    
    // 应用到几何体
    if (m_parent && m_parent->mm_node()) {
        auto geometry = m_parent->mm_node()->getVertexGeometry();
        if (geometry.valid()) {
            geometry->setStateSet(stateSet.get());
        }
    }
}

void GeoMaterialManager::updateStateSet()
{
    if (m_stateSetDirty) {
        m_stateSet = createStateSet();
        applyStateSet(m_stateSet);
        m_stateSetDirty = false;
    }
}

void GeoMaterialManager::setWireframeMode(bool enable)
{
    if (m_wireframeMode != enable) {
        m_wireframeMode = enable;
        m_stateSetDirty = true;
        updateStateSet();
        emit renderModeChanged();
    }
}

bool GeoMaterialManager::isWireframeMode() const
{
    return m_wireframeMode;
}

void GeoMaterialManager::setPointMode(bool enable)
{
    if (m_pointMode != enable) {
        m_pointMode = enable;
        m_stateSetDirty = true;
        updateStateSet();
        emit renderModeChanged();
    }
}

bool GeoMaterialManager::isPointMode() const
{
    return m_pointMode;
}

void GeoMaterialManager::setDepthTest(bool enable)
{
    if (m_depthTest != enable) {
        m_depthTest = enable;
        m_stateSetDirty = true;
        updateStateSet();
    }
}

bool GeoMaterialManager::isDepthTestEnabled() const
{
    return m_depthTest;
}

void GeoMaterialManager::setDepthWrite(bool enable)
{
    if (m_depthWrite != enable) {
        m_depthWrite = enable;
        m_stateSetDirty = true;
        updateStateSet();
    }
}

bool GeoMaterialManager::isDepthWriteEnabled() const
{
    return m_depthWrite;
}

void GeoMaterialManager::setTwoSided(bool enable)
{
    if (m_twoSided != enable) {
        m_twoSided = enable;
        m_stateSetDirty = true;
        updateStateSet();
    }
}

bool GeoMaterialManager::isTwoSided() const
{
    return m_twoSided;
}

void GeoMaterialManager::applyBasicMaterial()
{
    setMaterialType(Material_Basic3D);
}

void GeoMaterialManager::applyPhongMaterial()
{
    setMaterialType(Material_Phong3D);
}

void GeoMaterialManager::applyBlinnMaterial()
{
    setMaterialType(Material_Blinn3D);
}

void GeoMaterialManager::applyLambertMaterial()
{
    setMaterialType(Material_Lambert3D);
}

void GeoMaterialManager::applyPBRMaterial()
{
    setMaterialType(Material_PBR3D);
}

bool GeoMaterialManager::validateMaterial() const
{
    // 检查材质属性是否有效
    return m_material.shininess >= 0.0f && 
           m_material.transparency >= 0.0f && 
           m_material.transparency <= 1.0f;
}

bool GeoMaterialManager::isMaterialValid() const
{
    return validateMaterial();
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
    m_osgMaterial->setEmission(osg::Material::FRONT_AND_BACK, 
                              osg::Vec4(m_material.emission.r, m_material.emission.g, 
                                       m_material.emission.b, m_material.emission.a));
    m_osgMaterial->setShininess(osg::Material::FRONT_AND_BACK, m_material.shininess);
    
    m_stateSetDirty = true;
}

void GeoMaterialManager::updateRenderingAttributes()
{
    // 根据材质类型和设置更新渲染属性
    if (m_material.transparency < 1.0f) {
        enableBlending(true);
        setBlendMode(static_cast<osg::BlendFunc::BlendFuncMode>(GL_SRC_ALPHA), 
                    static_cast<osg::BlendFunc::BlendFuncMode>(GL_ONE_MINUS_SRC_ALPHA));
    }
}

void GeoMaterialManager::createDefaultStateSet()
{
    m_stateSet = new osg::StateSet();
    m_stateSetDirty = true;
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
    updateMaterial();
} 