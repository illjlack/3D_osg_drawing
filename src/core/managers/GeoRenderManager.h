#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <osg/ref_ptr>
#include <osg/Group>

// 前向声明
class Geo3D;
class GeoNodeManager;
class GeoMaterialManager;

/**
 * @brief 渲染管理器
 * 负责管理几何对象的渲染控制，包括点线面的显示/隐藏、渲染模式切换等
 */
class GeoRenderManager : public QObject
{
    Q_OBJECT

public:
    // 渲染模式
    enum RenderMode
    {
        RENDER_POINTS,           // 仅显示点
        RENDER_WIREFRAME,        // 线框模式
        RENDER_SOLID,            // 实体模式
        RENDER_POINTS_WIREFRAME, // 点+线框
        RENDER_WIREFRAME_SOLID,  // 线框+实体
        RENDER_ALL               // 点+线框+实体
    };

    explicit GeoRenderManager(Geo3D* parent);
    ~GeoRenderManager() = default;

    // 基本显示控制
    void setShowPoints(bool show);
    void setShowEdges(bool show);
    void setShowFaces(bool show);
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
    float getAlpha() const { return m_alpha; }

    // 细节层次控制(LOD)
    void setLODEnabled(bool enabled);
    bool isLODEnabled() const { return m_lodEnabled; }
    void setLODDistance(float distance);
    float getLODDistance() const { return m_lodDistance; }
    void setLODScale(float scale);
    float getLODScale() const { return m_lodScale; }

    // 渲染优化
    void setFrustumCulling(bool enabled);
    bool isFrustumCullingEnabled() const { return m_frustumCulling; }
    void setBackfaceCulling(bool enabled);
    bool isBackfaceCullingEnabled() const { return m_backfaceCulling; }
    void setOcclusionCulling(bool enabled);
    bool isOcclusionCullingEnabled() const { return m_occlusionCulling; }

    // 渲染质量
    void setRenderQuality(int quality);  // 0: 低, 1: 中, 2: 高
    int getRenderQuality() const { return m_renderQuality; }
    void setAntiAliasing(bool enabled);
    bool isAntiAliasingEnabled() const { return m_antiAliasing; }

    // 选择高亮
    void setHighlighted(bool highlighted);
    bool isHighlighted() const { return m_highlighted; }
    void setHighlightColor(const Color3D& color);
    const Color3D& getHighlightColor() const { return m_highlightColor; }
    void setHighlightWidth(float width);
    float getHighlightWidth() const { return m_highlightWidth; }

    // 渲染统计
    int getVertexCount() const;
    int getTriangleCount() const;
    int getDrawCallCount() const;
    size_t getMemoryUsage() const;

    // 渲染更新
    void updateRender();
    void forceRenderUpdate();
    void invalidateRender();

    // 渲染优化建议
    bool needsOptimization() const;
    void optimizeRendering();
    void compactRenderData();

    // 渲染状态
    bool isRenderingEnabled() const;
    bool isRenderDataValid() const;
    bool needsRenderUpdate() const { return m_needsRenderUpdate; }

    // 渲染预设
    void applyPerformancePreset();
    void applyQualityPreset();
    void applyBalancedPreset();
    void applyCustomPreset(const QString& presetName);

    // 动画渲染
    void setAnimationEnabled(bool enabled);
    bool isAnimationEnabled() const { return m_animationEnabled; }
    void setAnimationSpeed(float speed);
    float getAnimationSpeed() const { return m_animationSpeed; }

signals:
    void renderModeChanged(RenderMode mode);
    void visibilityChanged(bool visible);
    void highlightChanged(bool highlighted);
    void lodChanged(bool enabled);
    void renderQualityChanged(int quality);
    void renderUpdateRequired();
    void renderOptimizationSuggested();

public:
    void updateFeatureVisibility();

private:
    void initializeRenderSettings();
    void updateRenderSettings();
    void updateHighlightEffect();
    void updateLODSettings();
    void applyRenderOptimizations();
    void validateRenderSettings();

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
    
    // LOD控制
    bool m_lodEnabled;
    float m_lodDistance;
    float m_lodScale;
    
    // 渲染优化
    bool m_frustumCulling;
    bool m_backfaceCulling;
    bool m_occlusionCulling;
    
    // 渲染质量
    int m_renderQuality;
    bool m_antiAliasing;
    
    // 选择高亮
    bool m_highlighted;
    Color3D m_highlightColor;
    float m_highlightWidth;
    
    // 动画
    bool m_animationEnabled;
    float m_animationSpeed;
    
    // 渲染状态
    bool m_needsRenderUpdate;
    bool m_renderDataValid;
    
    // 渲染统计
    mutable int m_cachedVertexCount;
    mutable int m_cachedTriangleCount;
    mutable bool m_statisticsDirty;
}; 