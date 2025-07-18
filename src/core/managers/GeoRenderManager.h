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
 * @brief 渲染管理器
 * 直接设置几何对象的渲染状态和材质
 */
class GeoRenderManager
{
public:
    // 渲染模式
    enum RenderMode
    {
        RENDER_POINTS,           // 仅显示点
        RENDER_WIREFRAME,        // 线框模式
        RENDER_SOLID,            // 实体模式
        RENDER_ALL               // 点+线框+实体
    };

    explicit GeoRenderManager(Geo3D* parent);
    ~GeoRenderManager() = default;

    // 渲染模式设置
    void setRenderMode(RenderMode mode);
    void setShowPoints(bool show);
    void setShowEdges(bool show);
    void setShowFaces(bool show);
    void setVisible(bool visible);

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

private:
    void initializeRender();
    void updateRender();
    void applyMaterialPreset(MaterialType3D type);

private:
    Geo3D* m_parent;
    
    // 渲染状态
    bool m_showPoints;
    bool m_showEdges;
    bool m_showFaces;
    bool m_visible;
    bool m_wireframeMode;
    bool m_highlighted;
    
    // 材质属性
    Material3D m_material;
    
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