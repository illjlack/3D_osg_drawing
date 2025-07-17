#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/ref_ptr>
#include <osg/Group>

// 前向声明
class Geo3D;
class GeoNodeManager;
class GeoMaterialManager;

/**
 * @brief 渲染管理器
 * 负责管理几何对象的渲染控制
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

    // 基本显示控制
    void setShowPoints(bool show);
    void setShowEdges(bool show);
    void setShowFaces(bool show);
    
    // 获取显示状态
    bool isShowPoints() const { return m_showPoints; }
    bool isShowEdges() const { return m_showEdges; }
    bool isShowFaces() const { return m_showFaces; }

    // 渲染模式
    void setRenderMode(RenderMode mode);
    RenderMode getRenderMode() const { return m_renderMode; }
    void applyRenderMode();

    // 快速模式切换
    void showPointsOnly();
    void showWireframeOnly();
    void showSolidOnly();
    void showAll();
    void hideAll();

    // 可见性控制
    void setVisible(bool visible);
    bool isVisible() const { return m_visible; }
    void setAlpha(float alpha);

    // 选择高亮
    void setHighlighted(bool highlighted);
    bool isHighlighted() const { return m_highlighted; }
    void setHighlightColor(const Color3D& color);

    // 渲染更新
    void updateRender();
    void forceRenderUpdate();

    // 渲染状态
    bool isRenderingEnabled() const;
    bool needsRenderUpdate() const { return m_needsRenderUpdate; }

public:
    void updateFeatureVisibility();

private:
    void initializeRenderSettings();
    void updateRenderSettings();
    void updateHighlightEffect();

private:
    Geo3D* m_parent;
    
    // 基本显示控制
    bool m_showPoints;
    bool m_showEdges;
    bool m_showFaces;
    bool m_visible;
    float m_alpha;
    
    // 渲染模式
    RenderMode m_renderMode;
    
    // 选择高亮
    bool m_highlighted;
    Color3D m_highlightColor;
    float m_highlightWidth;
    
    // 渲染状态
    bool m_needsRenderUpdate;
    bool m_renderDataValid;
}; 