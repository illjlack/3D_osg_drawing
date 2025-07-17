#include "GeoRenderManager.h"
#include "../GeometryBase.h"
#include <algorithm>

GeoRenderManager::GeoRenderManager(Geo3D* parent)
    : m_parent(parent)
    , m_showPoints(true)
    , m_showEdges(true)
    , m_showFaces(true)
    , m_visible(true)
    , m_alpha(1.0f)
    , m_renderMode(RENDER_ALL)
    , m_highlighted(false)
    , m_highlightColor(1.0f, 1.0f, 0.0f, 1.0f) // 黄色高亮
    , m_highlightWidth(3.0f)
    , m_needsRenderUpdate(true)
    , m_renderDataValid(false)
{
    initializeRenderSettings();
}

void GeoRenderManager::initializeRenderSettings()
{
    updateRenderSettings();
}

void GeoRenderManager::setShowPoints(bool show)
{
    if (m_showPoints != show) {
        m_showPoints = show;
        updateFeatureVisibility();
    }
}

void GeoRenderManager::setShowEdges(bool show)
{
    if (m_showEdges != show) {
        m_showEdges = show;
        updateFeatureVisibility();
    }
}

void GeoRenderManager::setShowFaces(bool show)
{
    if (m_showFaces != show) {
        m_showFaces = show;
        updateFeatureVisibility();
    }
}

void GeoRenderManager::setRenderMode(RenderMode mode)
{
    if (m_renderMode != mode) {
        m_renderMode = mode;
        applyRenderMode();
    }
}

void GeoRenderManager::applyRenderMode()
{
    switch (m_renderMode) {
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

void GeoRenderManager::showPointsOnly()
{
    setRenderMode(RENDER_POINTS);
}

void GeoRenderManager::showWireframeOnly()
{
    setRenderMode(RENDER_WIREFRAME);
}

void GeoRenderManager::showSolidOnly()
{
    setRenderMode(RENDER_SOLID);
}

void GeoRenderManager::showAll()
{
    setRenderMode(RENDER_ALL);
}

void GeoRenderManager::hideAll()
{
    setShowPoints(false);
    setShowEdges(false);
    setShowFaces(false);
}

void GeoRenderManager::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        updateFeatureVisibility();
    }
}

void GeoRenderManager::setAlpha(float alpha)
{
    alpha = std::max(0.0f, std::min(1.0f, alpha));
    if (m_alpha != alpha) {
        m_alpha = alpha;
        
        // 应用透明度到材质
        if (m_parent && m_parent->mm_material()) {
            m_parent->mm_material()->setTransparency(1.0f - alpha);
        }
    }
}

void GeoRenderManager::setHighlighted(bool highlighted)
{
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        updateHighlightEffect();
    }
}

void GeoRenderManager::setHighlightColor(const Color3D& color)
{
    m_highlightColor = color;
    if (m_highlighted) {
        updateHighlightEffect();
    }
}

void GeoRenderManager::updateRender()
{
    if (m_needsRenderUpdate) {
        updateRenderSettings();
        updateFeatureVisibility();
        m_needsRenderUpdate = false;
        m_renderDataValid = true;
    }
}

void GeoRenderManager::forceRenderUpdate()
{
    m_needsRenderUpdate = true;
    updateRender();
}

bool GeoRenderManager::isRenderingEnabled() const
{
    return m_visible && m_renderDataValid;
}

void GeoRenderManager::updateFeatureVisibility()
{
    if (!m_parent) return;
    
    // 获取节点管理器
    auto nodeManager = m_parent->mm_node();
    
    // 更新点的可见性
    nodeManager->setVertexVisible(m_showPoints);
    
    // 更新边的可见性
    nodeManager->setEdgeVisible(m_showEdges);
    
    // 更新面的可见性
    nodeManager->setFaceVisible(m_showFaces);
}

void GeoRenderManager::updateRenderSettings()
{
    // 应用透明度 - 避免递归调用
    if (m_parent && m_parent->mm_material()) {
        auto* materialManager = m_parent->mm_material();
        if (materialManager) {
            // 使用内部方法直接更新，避免触发updateMaterial
            materialManager->updateMaterialInternal();
        }
    }
}

void GeoRenderManager::updateHighlightEffect()
{
    if (!m_parent) return;
    
    if (m_highlighted) {
        // 应用高亮效果
        if (auto* materialManager = m_parent->mm_material()) {
            materialManager->setDiffuse(m_highlightColor);
        }
    } else {
        // 移除高亮效果，恢复原始外观
        if (auto* materialManager = m_parent->mm_material()) {
            materialManager->resetMaterial();
        }
    }
} 