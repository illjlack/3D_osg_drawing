#include "GeoRenderManager.h"
#include "../GeometryBase.h"
#include "GeoNodeManager.h"
#include <osg/PolygonMode>
#include <osg/Depth>
#include <osg/BlendFunc>

GeoRenderManager::GeoRenderManager(osg::ref_ptr<Geo3D> parent)
    : m_parent(parent)
{
    initializeRenderStates();
}

void GeoRenderManager::initializeRenderStates()
{
    if (!m_parent || !m_parent->mm_node()) return;
    
    // 创建渲染状态对象
    m_pointMaterial = new osg::Material();
    m_lineMaterial = new osg::Material();
    m_surfaceMaterial = new osg::Material();
    
    m_pointSize = new osg::Point(5.0);
    m_lineWidth = new osg::LineWidth(2.0);
    m_lineStipple = new osg::LineStipple();
    m_blendFunc = new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 设置默认渲染状态
    auto nodeManager = m_parent->mm_node();
    
    // 为点几何设置状态
    if (auto pointGeom = nodeManager->getVertexGeometry()) {
        auto stateSet = pointGeom->getOrCreateStateSet();
        stateSet->setAttributeAndModes(m_pointMaterial.get(), osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(m_pointSize.get(), osg::StateAttribute::ON);
        stateSet->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);
    }
    
    // 为线几何设置状态
    if (auto lineGeom = nodeManager->getEdgeGeometry()) {
        auto stateSet = lineGeom->getOrCreateStateSet();
        stateSet->setAttributeAndModes(m_lineMaterial.get(), osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(m_lineWidth.get(), osg::StateAttribute::ON);
        stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    }
    
    // 为面几何设置状态
    if (auto faceGeom = nodeManager->getFaceGeometry()) {
        auto stateSet = faceGeom->getOrCreateStateSet();
        stateSet->setAttributeAndModes(m_surfaceMaterial.get(), osg::StateAttribute::ON);
    }
}

void GeoRenderManager::updateRenderingParameters(const GeoParameters3D& params)
{
    if (!m_parent || !m_parent->mm_node()) return;
    
    // 保存当前参数
    m_currentParams = params;
    
    // 更新各组件的渲染参数
    updatePointRendering(params);
    updateLineRendering(params);
    updateSurfaceRendering(params);
    updateVisibilityStates(params);
}

void GeoRenderManager::updatePointRendering(const GeoParameters3D& params)
{
    if (!m_pointMaterial.valid() || !m_pointSize.valid()) return;
    
    // 设置点颜色（包含透明度）
    osg::Vec4 pointColor = colorToOsgVec4(params.pointColor);
    m_pointMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, pointColor);
    m_pointMaterial->setAmbient(osg::Material::FRONT_AND_BACK, pointColor * 0.3f);
    
    // 设置点大小
    m_pointSize->setSize(static_cast<float>(params.pointSize));
    
    // 处理透明度
    if (params.pointColor.a < 1.0) {
        auto nodeManager = m_parent->mm_node();
        if (auto pointGeom = nodeManager->getVertexGeometry()) {
            auto stateSet = pointGeom->getOrCreateStateSet();
            stateSet->setAttributeAndModes(m_blendFunc.get(), osg::StateAttribute::ON);
            stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
            stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        }
    }
}

void GeoRenderManager::updateLineRendering(const GeoParameters3D& params)
{
    if (!m_lineMaterial.valid() || !m_lineWidth.valid()) return;
    
    auto nodeManager = m_parent->mm_node();
    auto lineGeom = nodeManager->getEdgeGeometry();
    if (!lineGeom) return;
    
    auto stateSet = lineGeom->getOrCreateStateSet();
    
    // 设置线颜色（包含透明度）
    osg::Vec4 lineColor = colorToOsgVec4(params.lineColor);
    m_lineMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, lineColor);
    m_lineMaterial->setAmbient(osg::Material::FRONT_AND_BACK, lineColor * 0.3f);
    
    // 设置线宽
    m_lineWidth->setWidth(static_cast<float>(params.lineWidth));
    
    // 设置线型
    applyLineStyle(params.lineStyle, params.lineDashPattern);
    
    // 处理透明度
    if (params.lineColor.a < 1.0) {
        stateSet->setAttributeAndModes(m_blendFunc.get(), osg::StateAttribute::ON);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
}

void GeoRenderManager::updateSurfaceRendering(const GeoParameters3D& params)
{
    if (!m_surfaceMaterial.valid()) return;
    
    auto nodeManager = m_parent->mm_node();
    auto faceGeom = nodeManager->getFaceGeometry();
    if (!faceGeom) return;
    
    auto stateSet = faceGeom->getOrCreateStateSet();
    
    // 设置面颜色（包含透明度）
    osg::Vec4 fillColor = colorToOsgVec4(params.fillColor);
    m_surfaceMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, fillColor);
    m_surfaceMaterial->setAmbient(osg::Material::FRONT_AND_BACK, fillColor * 0.3f);
    
    // 处理透明度
    if (params.fillColor.a < 1.0) {
        stateSet->setAttributeAndModes(m_blendFunc.get(), osg::StateAttribute::ON);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
}

void GeoRenderManager::updateVisibilityStates(const GeoParameters3D& params)
{
    if (!m_parent || !m_parent->mm_node()) return;
    
    auto nodeManager = m_parent->mm_node();
    
    // 强制执行可见性约束：至少有一个组件可见
    bool hasVisible = params.showPoints || params.showEdges || params.showFaces;
    if (!hasVisible) {
        // 如果全部隐藏，强制显示线框
        if (auto lineGeom = nodeManager->getEdgeGeometry()) {
            lineGeom->setNodeMask(NODE_MASK_EDGE);
        }
        if (auto pointGeom = nodeManager->getVertexGeometry()) {
            pointGeom->setNodeMask(NODE_MASK_NONE);
        }
        if (auto faceGeom = nodeManager->getFaceGeometry()) {
            faceGeom->setNodeMask(NODE_MASK_NONE);
        }
        return;
    }
    
    // 控制点的显示/隐藏
    if (auto pointGeom = nodeManager->getVertexGeometry()) {
        uint32_t mask = params.showPoints ? NODE_MASK_VERTEX : NODE_MASK_NONE;
        pointGeom->setNodeMask(mask);
    }
    
    // 控制边的显示/隐藏
    if (auto lineGeom = nodeManager->getEdgeGeometry()) {
        uint32_t mask = params.showEdges ? NODE_MASK_EDGE : NODE_MASK_NONE;
        lineGeom->setNodeMask(mask);
    }
    
    // 控制面的显示/隐藏
    if (auto faceGeom = nodeManager->getFaceGeometry()) {
        uint32_t mask = params.showFaces ? NODE_MASK_FACE : NODE_MASK_NONE;
        faceGeom->setNodeMask(mask);
    }
}

void GeoRenderManager::applyLineStyle(LineStyle3D style, double dashPattern)
{
    if (!m_parent || !m_parent->mm_node()) return;
    
    auto lineGeom = m_parent->mm_node()->getEdgeGeometry();
    if (!lineGeom) return;
    
    auto stateSet = lineGeom->getOrCreateStateSet();
    
    switch (style) {
        case Line_Solid3D:
            stateSet->setMode(GL_LINE_STIPPLE, osg::StateAttribute::OFF);
            break;
            
        case Line_Dashed3D:
            m_lineStipple->setPattern(0xF0F0);  // 短虚线
            m_lineStipple->setFactor(1);
            stateSet->setAttributeAndModes(m_lineStipple.get(), osg::StateAttribute::ON);
            break;
            
        case Line_Dotted3D:
            m_lineStipple->setPattern(0xAAAA);  // 点线
            m_lineStipple->setFactor(1);
            stateSet->setAttributeAndModes(m_lineStipple.get(), osg::StateAttribute::ON);
            break;
            
        case Line_DashDot3D:
            m_lineStipple->setPattern(0xFF18);  // 点划线
            m_lineStipple->setFactor(1);
            stateSet->setAttributeAndModes(m_lineStipple.get(), osg::StateAttribute::ON);
            break;
            
        case Line_DashDotDot3D:
            m_lineStipple->setPattern(0xFE38);  // 双点划线
            m_lineStipple->setFactor(1);
            stateSet->setAttributeAndModes(m_lineStipple.get(), osg::StateAttribute::ON);
            break;
            
        case Line_Custom3D:
            // 自定义虚线模式，使用dashPattern参数
            {
                int factor = static_cast<int>(std::max(1.0, dashPattern));
                m_lineStipple->setPattern(0xF0F0);  // 基础虚线模式
                m_lineStipple->setFactor(factor);
                stateSet->setAttributeAndModes(m_lineStipple.get(), osg::StateAttribute::ON);
            }
            break;
            
        default:
            stateSet->setMode(GL_LINE_STIPPLE, osg::StateAttribute::OFF);
            break;
    }
}

osg::Vec4 GeoRenderManager::colorToOsgVec4(const Color3D& color) const
{
    return osg::Vec4(
        static_cast<float>(color.r),
        static_cast<float>(color.g),
        static_cast<float>(color.b),
        static_cast<float>(color.a)
    );
}




