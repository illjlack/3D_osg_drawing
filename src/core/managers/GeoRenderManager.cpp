#include "GeoRenderManager.h"
#include "../GeometryBase.h"
#include "GeoNodeManager.h"
#include "../../util/LogManager.h"
#include <osg/PolygonMode>
#include <osg/Depth>
#include <osg/BlendFunc>

GeoRenderManager::GeoRenderManager(osg::ref_ptr<Geo3D> parent)
    : m_parent(parent),
      m_pointMaterial(new osg::Material()),
      m_lineMaterial(new osg::Material()),
      m_surfaceMaterial(new osg::Material()),
      m_pointSize(new osg::Point(2.0)),
      m_lineWidth(new osg::LineWidth(1.0)),
      m_lineStipple(new osg::LineStipple()),
      m_blendFunc(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA))
{
    // 绑定到节点的steteSet
    initializeRenderStates();
}

void GeoRenderManager::initializeRenderStates()
{
    if (!m_parent || !m_parent->mm_node()) return;

    // 设置默认渲染状态
    auto nodeManager = m_parent->mm_node();
    
    // 为点几何设置状态（自定义形状点实际上是三角形面片，和面一样处理）
    if (auto pointGeom = nodeManager->getVertexGeometry()) {
        auto stateSet = pointGeom->getOrCreateStateSet();
        stateSet->setAttributeAndModes(m_pointMaterial, osg::StateAttribute::ON);
    }
    
    // 为线几何设置状态
    if (auto lineGeom = nodeManager->getEdgeGeometry()) {
        auto stateSet = lineGeom->getOrCreateStateSet();
        stateSet->setAttributeAndModes(m_lineMaterial, osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(m_lineWidth, osg::StateAttribute::ON);
        // 禁用LINE_SMOOTH以确保线宽设置生效
        stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::OFF);
    }
    
    // 为面几何设置状态
    if (auto faceGeom = nodeManager->getFaceGeometry()) {
        auto stateSet = faceGeom->getOrCreateStateSet();
        stateSet->setAttributeAndModes(m_surfaceMaterial, osg::StateAttribute::ON);
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

void GeoRenderManager::reinitializeRenderStates()
{
    LOG_INFO("开始重新初始化渲染状态", "渲染管理");
    assert(this);
    assert(m_parent);
    assert(m_parent && m_parent->mm_node());
    // 当节点全部更换时，直接重新初始化即可
    // 旧的StateSet已经废弃，不需要清理
    initializeRenderStates();
    
    // 应用当前参数
    updateRenderingParameters(m_currentParams);
    
    LOG_INFO("渲染状态重新初始化完成", "渲染管理");
}

void GeoRenderManager::updatePointRendering(const GeoParameters3D& params)
{
    if (!m_pointMaterial) return;
    
    auto nodeManager = m_parent->mm_node();
    auto pointGeom = nodeManager->getVertexGeometry();
    if (!pointGeom) return;
    
    auto stateSet = pointGeom->getOrCreateStateSet();
    
    // 检查primitive类型来判断是点还是面（三角网）
    bool isRealPoint = false;
    bool isTriangleMesh = false;
    
    for (unsigned int i = 0; i < pointGeom->getNumPrimitiveSets(); ++i) {
        osg::PrimitiveSet* ps = pointGeom->getPrimitiveSet(i);
        if (ps) {
            if (ps->getMode() == GL_POINTS) {
                isRealPoint = true;
            } else if (ps->getMode() == GL_TRIANGLES || ps->getMode() == GL_TRIANGLE_STRIP || ps->getMode() == GL_TRIANGLE_FAN) {
                isTriangleMesh = true;
            }
        }
    }
    
    if (isRealPoint) {
        // 设置真正的点参数
        setupPointParameters(params, stateSet);
    } else if (isTriangleMesh) {
        // 设置面（三角网）参数
        setupTriangleMeshParameters(params, stateSet);
    }
}

void GeoRenderManager::setupPointParameters(const GeoParameters3D& params, osg::StateSet* stateSet)
{
    // 设置点颜色（使用参数中的点颜色）
    osg::Vec4 pointColor = colorToOsgVec4(params.pointColor);
    m_pointMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, pointColor);
    m_pointMaterial->setAmbient(osg::Material::FRONT_AND_BACK, pointColor * 0.3f);
    
    // 设置点大小
    if (m_pointSize) {
        m_pointSize->setSize(static_cast<float>(params.pointSize));
        stateSet->setAttributeAndModes(m_pointSize, osg::StateAttribute::ON);
    }
    
    // 处理透明度
    if (params.pointColor.a < 1.0) {
        stateSet->setAttributeAndModes(m_blendFunc, osg::StateAttribute::ON);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
}

void GeoRenderManager::setupTriangleMeshParameters(const GeoParameters3D& params, osg::StateSet* stateSet)
{
    // 设置面颜色（使用参数中的填充颜色）
    osg::Vec4 meshColor = colorToOsgVec4(params.fillColor);
    m_pointMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, meshColor);
    m_pointMaterial->setAmbient(osg::Material::FRONT_AND_BACK, meshColor * 0.3f);
    
    // 处理透明度
    if (params.fillColor.a < 1.0) {
        stateSet->setAttributeAndModes(m_blendFunc, osg::StateAttribute::ON);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
}

void GeoRenderManager::updateLineRendering(const GeoParameters3D& params)
{
    if (!m_lineMaterial || !m_lineWidth) return;
    
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
    
    // 确保LINE_SMOOTH被禁用以保证线宽生效
    stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::OFF);
    
    // 设置线型
    applyLineStyle(params.lineStyle, params.lineDashPattern);
    
    // 处理透明度
    if (params.lineColor.a < 1.0) {
        stateSet->setAttributeAndModes(m_blendFunc, osg::StateAttribute::ON);
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
        stateSet->setAttributeAndModes(m_blendFunc, osg::StateAttribute::ON);
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
            stateSet->setAttributeAndModes(m_lineStipple, osg::StateAttribute::ON);
            break;
            
        case Line_Dotted3D:
            m_lineStipple->setPattern(0xAAAA);  // 点线
            m_lineStipple->setFactor(1);
            stateSet->setAttributeAndModes(m_lineStipple, osg::StateAttribute::ON);
            break;
            
        case Line_DashDot3D:
            m_lineStipple->setPattern(0xFF18);  // 点划线
            m_lineStipple->setFactor(1);
            stateSet->setAttributeAndModes(m_lineStipple, osg::StateAttribute::ON);
            break;
            
        case Line_DashDotDot3D:
            m_lineStipple->setPattern(0xFE38);  // 双点划线
            m_lineStipple->setFactor(1);
            stateSet->setAttributeAndModes(m_lineStipple, osg::StateAttribute::ON);
            break;
            
        case Line_Custom3D:
            // 自定义虚线模式，使用dashPattern参数
            {
                int factor = static_cast<int>(std::max(1.0, dashPattern));
                m_lineStipple->setPattern(0xF0F0);  // 基础虚线模式
                m_lineStipple->setFactor(factor);
                stateSet->setAttributeAndModes(m_lineStipple, osg::StateAttribute::ON);
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




