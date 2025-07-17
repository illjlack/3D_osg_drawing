#include "GeoRenderManager.h"
#include "../GeometryBase.h"
#include <algorithm>

GeoRenderManager::GeoRenderManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_showPoints(true)
    , m_showEdges(true)
    , m_showFaces(true)
    , m_visible(true)
    , m_alpha(1.0f)
    , m_renderMode(RENDER_ALL)
    , m_lodEnabled(false)
    , m_lodDistance(100.0f)
    , m_lodScale(1.0f)
    , m_frustumCulling(true)
    , m_backfaceCulling(false)
    , m_occlusionCulling(false)
    , m_renderQuality(1) // 中等质量
    , m_antiAliasing(false)
    , m_highlighted(false)
    , m_highlightColor(1.0f, 1.0f, 0.0f, 1.0f) // 黄色高亮
    , m_highlightWidth(3.0f)
    , m_animationEnabled(false)
    , m_animationSpeed(1.0f)
    , m_needsRenderUpdate(true)
    , m_renderDataValid(false)
    , m_cachedVertexCount(0)
    , m_cachedTriangleCount(0)
    , m_statisticsDirty(true)
{
    initializeRenderSettings();
}

void GeoRenderManager::initializeRenderSettings()
{
    validateRenderSettings();
    updateRenderSettings();
}

void GeoRenderManager::setShowPoints(bool show)
{
    if (m_showPoints != show) {
        m_showPoints = show;
        updateFeatureVisibility();
        emit renderModeChanged(m_renderMode);
    }
}

void GeoRenderManager::setShowEdges(bool show)
{
    if (m_showEdges != show) {
        m_showEdges = show;
        updateFeatureVisibility();
        emit renderModeChanged(m_renderMode);
    }
}

void GeoRenderManager::setShowFaces(bool show)
{
    if (m_showFaces != show) {
        m_showFaces = show;
        updateFeatureVisibility();
        emit renderModeChanged(m_renderMode);
    }
}

void GeoRenderManager::setRenderMode(RenderMode mode)
{
    if (m_renderMode != mode) {
        m_renderMode = mode;
        applyRenderMode();
        emit renderModeChanged(mode);
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
            
        case RENDER_POINTS_WIREFRAME:
            setShowPoints(true);
            setShowEdges(true);
            setShowFaces(false);
            break;
            
        case RENDER_WIREFRAME_SOLID:
            setShowPoints(false);
            setShowEdges(true);
            setShowFaces(true);
            break;
            
        case RENDER_ALL:
        default:
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
        emit visibilityChanged(visible);
    }
}

void GeoRenderManager::setAlpha(float alpha)
{
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    if (m_alpha != alpha) {
        m_alpha = alpha;
        m_needsRenderUpdate = true;
        updateRenderSettings();
        emit renderUpdateRequired();
    }
}

void GeoRenderManager::setLODEnabled(bool enabled)
{
    if (m_lodEnabled != enabled) {
        m_lodEnabled = enabled;
        updateLODSettings();
        emit lodChanged(enabled);
    }
}

void GeoRenderManager::setLODDistance(float distance)
{
    if (m_lodDistance != distance) {
        m_lodDistance = std::max(0.1f, distance);
        updateLODSettings();
    }
}

void GeoRenderManager::setLODScale(float scale)
{
    if (m_lodScale != scale) {
        m_lodScale = std::max(0.1f, scale);
        updateLODSettings();
    }
}

void GeoRenderManager::setFrustumCulling(bool enabled)
{
    if (m_frustumCulling != enabled) {
        m_frustumCulling = enabled;
        applyRenderOptimizations();
    }
}

void GeoRenderManager::setBackfaceCulling(bool enabled)
{
    if (m_backfaceCulling != enabled) {
        m_backfaceCulling = enabled;
        applyRenderOptimizations();
    }
}

void GeoRenderManager::setOcclusionCulling(bool enabled)
{
    if (m_occlusionCulling != enabled) {
        m_occlusionCulling = enabled;
        applyRenderOptimizations();
    }
}

void GeoRenderManager::setRenderQuality(int quality)
{
    quality = std::clamp(quality, 0, 2);
    if (m_renderQuality != quality) {
        m_renderQuality = quality;
        updateRenderSettings();
        emit renderQualityChanged(quality);
    }
}

void GeoRenderManager::setAntiAliasing(bool enabled)
{
    if (m_antiAliasing != enabled) {
        m_antiAliasing = enabled;
        updateRenderSettings();
    }
}

void GeoRenderManager::setHighlighted(bool highlighted)
{
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        updateHighlightEffect();
        emit highlightChanged(highlighted);
    }
}

void GeoRenderManager::setHighlightColor(const Color3D& color)
{
    if (m_highlightColor.r != color.r || 
        m_highlightColor.g != color.g || 
        m_highlightColor.b != color.b || 
        m_highlightColor.a != color.a) {
        m_highlightColor = color;
        if (m_highlighted) {
            updateHighlightEffect();
        }
    }
}

void GeoRenderManager::setHighlightWidth(float width)
{
    if (m_highlightWidth != width) {
        m_highlightWidth = std::max(1.0f, width);
        if (m_highlighted) {
            updateHighlightEffect();
        }
    }
}

int GeoRenderManager::getVertexCount() const
{
    if (m_statisticsDirty) {
        // 计算顶点数
        m_cachedVertexCount = 0;
        
        if (m_parent && m_parent->mm_node()) {
            auto* nodeManager = m_parent->mm_node();
            auto geometry = nodeManager->getVertexGeometry();
            
            if (geometry.valid()) {
                auto vertices = geometry->getVertexArray();
                if (vertices) {
                    m_cachedVertexCount = static_cast<int>(vertices->getNumElements());
                }
            }
        }
        
        m_statisticsDirty = false;
    }
    
    return m_cachedVertexCount;
}

int GeoRenderManager::getTriangleCount() const
{
    if (m_statisticsDirty) {
        // 计算三角形数
        m_cachedTriangleCount = 0;
        
        if (m_parent && m_parent->mm_node()) {
            auto* nodeManager = m_parent->mm_node();
            auto geometry = nodeManager->getVertexGeometry();
            
            if (geometry.valid()) {
                for (unsigned int i = 0; i < geometry->getNumPrimitiveSets(); ++i) {
                    auto primitiveSet = geometry->getPrimitiveSet(i);
                    if (primitiveSet) {
                        int mode = primitiveSet->getMode();
                        if (mode == GL_TRIANGLES) {
                            m_cachedTriangleCount += primitiveSet->getNumIndices() / 3;
                        } else if (mode == GL_TRIANGLE_STRIP) {
                            m_cachedTriangleCount += std::max(0, static_cast<int>(primitiveSet->getNumIndices()) - 2);
                        } else if (mode == GL_TRIANGLE_FAN) {
                            m_cachedTriangleCount += std::max(0, static_cast<int>(primitiveSet->getNumIndices()) - 2);
                        }
                    }
                }
            }
        }
        
        m_statisticsDirty = false;
    }
    
    return m_cachedTriangleCount;
}

int GeoRenderManager::getDrawCallCount() const
{
    // 简化实现：假设每个几何体一个绘制调用
    return isRenderingEnabled() ? 1 : 0;
}

size_t GeoRenderManager::getMemoryUsage() const
{
    size_t usage = 0;
    
    if (m_parent && m_parent->mm_node()) {
        auto* nodeManager = m_parent->mm_node();
        auto geometry = nodeManager->getVertexGeometry();
        
        if (geometry.valid()) {
            // 估算顶点数据内存使用
            auto vertices = geometry->getVertexArray();
            if (vertices) {
                usage += vertices->getTotalDataSize();
            }
            
            // 估算索引数据内存使用
            for (unsigned int i = 0; i < geometry->getNumPrimitiveSets(); ++i) {
                auto primitiveSet = geometry->getPrimitiveSet(i);
                if (primitiveSet) {
                    // 估算索引大小
                    usage += primitiveSet->getNumIndices() * sizeof(unsigned int);
                }
            }
        }
    }
    
    return usage;
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
    m_statisticsDirty = true;
    updateRender();
    emit renderUpdateRequired();
}

void GeoRenderManager::invalidateRender()
{
    m_renderDataValid = false;
    m_needsRenderUpdate = true;
    m_statisticsDirty = true;
    emit renderUpdateRequired();
}

bool GeoRenderManager::needsOptimization() const
{
    // 检查是否需要优化
    int vertexCount = getVertexCount();
    int triangleCount = getTriangleCount();
    
    // 如果顶点数或三角形数过多，建议优化
    return vertexCount > 10000 || triangleCount > 5000;
}

void GeoRenderManager::optimizeRendering()
{
    if (needsOptimization()) {
        applyRenderOptimizations();
        emit renderOptimizationSuggested();
    }
}

void GeoRenderManager::compactRenderData()
{
    // 压缩渲染数据
    m_statisticsDirty = true;
    forceRenderUpdate();
}

bool GeoRenderManager::isRenderingEnabled() const
{
    return m_visible && (m_showPoints || m_showEdges || m_showFaces);
}

bool GeoRenderManager::isRenderDataValid() const
{
    return m_renderDataValid;
}

void GeoRenderManager::applyPerformancePreset()
{
    setRenderQuality(0);
    setAntiAliasing(false);
    setLODEnabled(true);
    setFrustumCulling(true);
    setBackfaceCulling(true);
    setOcclusionCulling(true);
}

void GeoRenderManager::applyQualityPreset()
{
    setRenderQuality(2);
    setAntiAliasing(true);
    setLODEnabled(false);
    setFrustumCulling(true);
    setBackfaceCulling(false);
    setOcclusionCulling(false);
}

void GeoRenderManager::applyBalancedPreset()
{
    setRenderQuality(1);
    setAntiAliasing(false);
    setLODEnabled(true);
    setFrustumCulling(true);
    setBackfaceCulling(false);
    setOcclusionCulling(false);
}

void GeoRenderManager::applyCustomPreset(const QString& presetName)
{
    // 这里可以根据预设名称应用不同的设置
    // 现在只是占位符实现
    if (presetName == "performance") {
        applyPerformancePreset();
    } else if (presetName == "quality") {
        applyQualityPreset();
    } else {
        applyBalancedPreset();
    }
}

void GeoRenderManager::setAnimationEnabled(bool enabled)
{
    if (m_animationEnabled != enabled) {
        m_animationEnabled = enabled;
        // 这里可以启动或停止动画
    }
}

void GeoRenderManager::setAnimationSpeed(float speed)
{
    if (m_animationSpeed != speed) {
        m_animationSpeed = std::max(0.1f, speed);
        // 这里可以调整动画速度
    }
}

void GeoRenderManager::updateFeatureVisibility()
{
    if (!m_parent || !m_parent->mm_node()) return;
    
    // 获取节点管理器
    auto nodeManager = m_parent->mm_node();
    
    // 更新点的可见性
    nodeManager->setVertexVisible(m_showPoints);
    
    // 更新边的可见性
    nodeManager->setEdgeVisible(m_showEdges);
    
    // 更新面的可见性
    nodeManager->setFaceVisible(m_showFaces);
    
    // 标记渲染需要更新
    m_needsRenderUpdate = true;
}

void GeoRenderManager::updateRenderSettings()
{
    // 根据渲染质量调整设置
    switch (m_renderQuality) {
        case 0: // 低质量
            // 使用简化设置
            break;
        case 1: // 中等质量
            // 使用平衡设置
            break;
        case 2: // 高质量
            // 使用最佳设置
            break;
    }
    
    // 应用透明度
    if (m_parent && m_parent->mm_material()) {
        m_parent->mm_material()->setTransparency(m_alpha);
    }
}

void GeoRenderManager::updateHighlightEffect()
{
    if (!m_parent) return;
    
    if (m_highlighted) {
        // 应用高亮效果
        // 这里可以修改材质或添加特殊渲染状态
        if (auto* materialManager = m_parent->mm_material()) {
                    // 临时保存原始颜色并应用高亮颜色
        materialManager->setDiffuse(m_highlightColor);
    }
} else {
    // 移除高亮效果，恢复原始外观
    if (auto* materialManager = m_parent->mm_material()) {
        // 恢复原始材质
        materialManager->resetMaterial();
    }
    }
}

void GeoRenderManager::updateLODSettings()
{
    if (!m_lodEnabled) return;
    
    // 这里可以根据LOD设置调整几何体的细节级别
    // 现在只是占位符实现
}

void GeoRenderManager::applyRenderOptimizations()
{
    // OSG 已经自动处理了大部分渲染优化，包括：
    // - 视锥体剔除 (CullVisitor 自动处理)
    // - 背面剔除 (通过 StateSet 自动处理)
    // - 包围体优化 (节点自动维护)
    
    // 只在真正需要时才更新空间索引
    if (m_parent && m_parent->mm_node()) {
        auto* nodeManager = m_parent->mm_node();
        
        // 只在几何体发生重大变化时才重建空间索引
        // 避免频繁的 KdTree 重建
        if (m_needsRenderUpdate && m_statisticsDirty) {
            // 这里可以添加更智能的判断逻辑
            // 比如检查几何体是否真的发生了变化
        }
    }
}

void GeoRenderManager::validateRenderSettings()
{
    // 验证和修正渲染设置
    m_alpha = std::clamp(m_alpha, 0.0f, 1.0f);
    m_renderQuality = std::clamp(m_renderQuality, 0, 2);
    m_lodDistance = std::max(0.1f, m_lodDistance);
    m_lodScale = std::max(0.1f, m_lodScale);
    m_highlightWidth = std::max(1.0f, m_highlightWidth);
    m_animationSpeed = std::max(0.1f, m_animationSpeed);
} 