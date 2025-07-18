#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/Material>
#include <osg/StateSet>
#include <osg/BlendFunc>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/ref_ptr>

// 前向声明
class Geo3D;

/**
 * @brief 渲染管理器
 * 直接设置几何对象的渲染状态和材质
 */
class GeoRenderManager
{
public:

    explicit GeoRenderManager(Geo3D* parent);
    ~GeoRenderManager() = default;

    // 材质设置
    void setMaterial(const Material3D& material);
    void setMaterialType(MaterialType3D type);
    void setColor(const Color3D& color);
    void setTransparency(float transparency);

    // 渲染属性设置
    void setLineWidth(float width);
    void setPointSize(float size);
    void setWireframeMode(bool enable);

    // 高亮设置
    void setHighlighted(bool highlighted);
    void setHighlightColor(const Color3D& color);

    void setPointColor(const Color3D& color);
    void setEdgeColor(const Color3D& color);
    void setFaceColor(const Color3D& color);

private:
    void initializeRender();
    void applyMaterialPreset(MaterialType3D type);

private:
    Geo3D* m_parent;
    
    // 渲染状态
    bool m_wireframeMode;
    bool m_highlighted;
    
    // 材质属性
    Material3D m_material;
    
    // 三套材质
    osg::ref_ptr<osg::Material> m_pointMaterial;
    osg::ref_ptr<osg::Material> m_edgeMaterial;
    osg::ref_ptr<osg::Material> m_faceMaterial;
    
    // OSG渲染状态
    osg::ref_ptr<osg::StateSet> m_stateSet;
    osg::ref_ptr<osg::Material> m_osgMaterial;
    osg::ref_ptr<osg::BlendFunc> m_blendFunc;
    osg::ref_ptr<osg::LineWidth> m_lineWidth;
    osg::ref_ptr<osg::Point> m_pointSize;
    
    // 高亮
    Color3D m_highlightColor;
    bool m_blendingEnabled;
}; 