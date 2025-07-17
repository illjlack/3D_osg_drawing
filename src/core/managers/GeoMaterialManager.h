#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/Material>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/ref_ptr>

// 前向声明
class Geo3D;

/**
 * @brief 材质管理器
 * 负责管理几何对象的材质和渲染状态
 */
class GeoMaterialManager
{
public:
    explicit GeoMaterialManager(Geo3D* parent);
    ~GeoMaterialManager() = default;

    // 材质管理
    void setMaterial(const Material3D& material);
    void updateMaterial();
    void resetMaterial();

    // 颜色设置
    void setPointColor(const Color3D& color);
    void setLineColor(const Color3D& color);
    void setFaceColor(const Color3D& color);

    // 属性设置
    void setLineWidth(float width);
    void setPointSize(float size);
    void setTransparency(float transparency);
    void setMaterialType(MaterialType3D type);

    // 光照属性设置
    void setAmbient(const Color3D& ambient);
    void setDiffuse(const Color3D& diffuse);
    void setSpecular(const Color3D& specular);
    void setShininess(float shininess);

    // 渲染模式
    void setWireframeMode(bool enable);

    // 内部更新方法
    void updateOSGMaterial();
    void updateRenderingAttributes();
    
    // 内部方法，避免递归调用
    void updateMaterialInternal();

private:
    void initializeMaterial();
    void createDefaultStateSet();
    void applyMaterialPreset(MaterialType3D type);

private:
    Geo3D* m_parent;
    
    // 材质属性
    Material3D m_material;
    
    // 渲染状态
    osg::ref_ptr<osg::StateSet> m_stateSet;
    osg::ref_ptr<osg::Material> m_osgMaterial;
    osg::ref_ptr<osg::BlendFunc> m_blendFunc;
    osg::ref_ptr<osg::LineWidth> m_lineWidth;
    osg::ref_ptr<osg::Point> m_pointSize;
    
    // 渲染属性
    bool m_blendingEnabled;
    bool m_wireframeMode;
    bool m_depthTest;
    bool m_depthWrite;
    
    // 材质状态
    bool m_materialDirty;
}; 