#include "PickingIndicator.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/PolygonOffset>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Point>
#include <osg/PrimitiveSet>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/Billboard>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// PickingIndicatorManager Implementation
// ============================================================================

PickingIndicatorManager::PickingIndicatorManager()
{
    // 创建指示器根节点
    m_indicatorRoot = new osg::Group;
    m_indicatorRoot->setName("PickingIndicatorRoot");
    
    // 设置指示器根节点渲染状态
    osg::StateSet* stateSet = m_indicatorRoot->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
}

PickingIndicatorManager::~PickingIndicatorManager()
{
    shutdown();
}

bool PickingIndicatorManager::initialize(osg::Camera* camera)
{
    if (!camera) {
        LOG_ERROR("指示器管理器初始化参数无效", "指示器");
        return false;
    }
    
    m_camera = camera;
    
    // 创建指示器
    m_indicator = new osg::MatrixTransform;
    m_indicator->setName("PickingIndicator");
    
    // 创建指示器几何体缓存
    m_vertexIndicator = createVertexIndicator(m_config.indicatorSize);
    m_edgeIndicator = createEdgeIndicator(m_config.indicatorSize);
    m_faceIndicator = createFaceIndicator(m_config.indicatorSize);
    
    if (m_vertexIndicator) {
        m_indicator->addChild(m_vertexIndicator);
    }
    m_indicatorRoot->addChild(m_indicator);
    
    // 创建高亮节点
    m_highlightNode = new osg::Group;
    m_highlightNode->setName("PickingHighlight");
    m_indicatorRoot->addChild(m_highlightNode);
    
    // 初始时隐藏指示器
    m_indicator->setNodeMask(0);
    
    m_initialized = true;
    
    LOG_SUCCESS("指示器管理器初始化成功", "指示器");
    return true;
}

void PickingIndicatorManager::shutdown()
{
    if (!m_initialized) return;
    
    hideIndicator();
    hideHighlight();
    
    m_camera = nullptr;
    m_indicator = nullptr;
    m_highlightNode = nullptr;
    m_indicatorRoot = nullptr;
    m_highlightedGeometry = nullptr;
    
    m_vertexIndicator = nullptr;
    m_edgeIndicator = nullptr;
    m_faceIndicator = nullptr;
    
    m_initialized = false;
    
    LOG_INFO("指示器管理器已关闭", "指示器");
}

void PickingIndicatorManager::setConfig(const PickingIndicatorConfig& config)
{
    m_config = config;
    
    // 重新创建指示器几何体缓存
    if (m_initialized) {
        m_vertexIndicator = createVertexIndicator(m_config.indicatorSize);
        m_edgeIndicator = createEdgeIndicator(m_config.indicatorSize);
        m_faceIndicator = createFaceIndicator(m_config.indicatorSize);
    }
}

void PickingIndicatorManager::showIndicator(const glm::vec3& position, PickFeatureType featureType)
{
    if (!m_indicator || !m_config.enableIndicator) return;
    
    // 将世界坐标转换为屏幕坐标
    glm::vec2 screenPos = worldToScreen(position);
    
    // 设置指示器位置（使用屏幕坐标）
    osg::Matrix matrix;
    matrix.makeTranslate(osg::Vec3(screenPos.x, screenPos.y, 0.0f));
    m_indicator->setMatrix(matrix);
    
    // 根据特征类型设置指示器几何体
    m_indicator->removeChildren(0, m_indicator->getNumChildren());
    
    osg::ref_ptr<osg::Geometry> indicatorGeometry = nullptr;
    
    switch (featureType) {
        case PickFeatureType::VERTEX:
            indicatorGeometry = m_vertexIndicator;
            break;
        case PickFeatureType::EDGE:
            indicatorGeometry = m_edgeIndicator;
            break;
        case PickFeatureType::FACE:
            indicatorGeometry = m_faceIndicator;
            break;
        default:
            return;
    }
    
    if (indicatorGeometry) {
        // 创建Billboard节点，使2D指示器始终面向相机
        osg::ref_ptr<osg::Billboard> billboard = new osg::Billboard;
        billboard->setMode(osg::Billboard::POINT_ROT_EYE);
        billboard->addDrawable(indicatorGeometry);
        
        m_indicator->addChild(billboard);
    }
    
    // 显示指示器
    m_indicator->setNodeMask(0xFFFFFFFF);
}

void PickingIndicatorManager::hideIndicator()
{
    if (m_indicator) {
        m_indicator->setNodeMask(0);
    }
}

void PickingIndicatorManager::updateIndicatorPosition(const glm::vec3& position, PickFeatureType featureType)
{
    showIndicator(position, featureType);
}

void PickingIndicatorManager::showHighlight(Geo3D* geometry)
{
    if (!m_highlightNode || !geometry || !m_config.enableHighlight) return;
    
    hideHighlight();
    
    // 创建高亮几何体
    osg::ref_ptr<osg::Geometry> highlightGeometry = createHighlightGeometry(geometry);
    if (highlightGeometry) {
        m_highlightNode->addChild(highlightGeometry);
        m_highlightedGeometry = geometry;
    }
}

void PickingIndicatorManager::hideHighlight()
{
    if (m_highlightNode) {
        m_highlightNode->removeChildren(0, m_highlightNode->getNumChildren());
        m_highlightedGeometry = nullptr;
    }
}

void PickingIndicatorManager::showSelectionHighlight(Geo3D* geometry)
{
    if (!m_highlightNode || !geometry || !m_config.enableHighlight) return;
    
    hideSelectionHighlight();
    
    // 创建控制点高亮几何体
    osg::ref_ptr<osg::Geometry> highlightGeometry = createControlPointHighlightGeometry(geometry);
    if (highlightGeometry) {
        m_highlightNode->addChild(highlightGeometry);
        m_highlightedGeometry = geometry;
    }
}

void PickingIndicatorManager::hideSelectionHighlight()
{
    if (m_highlightNode) {
        m_highlightNode->removeChildren(0, m_highlightNode->getNumChildren());
        m_highlightedGeometry = nullptr;
    }
}

osg::ref_ptr<osg::Geometry> PickingIndicatorManager::createVertexIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建2D圆形指示器（顶点）
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 创建圆形
    const int segments = 16;
    for (int i = 0; i <= segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        float x = size * 0.5f * cos(angle);
        float y = size * 0.5f * sin(angle);
        vertices->push_back(osg::Vec3(x, y, 0.0f));
        colors->push_back(m_config.vertexColor);
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 创建三角形扇形
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置渲染状态
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    // 设置混合函数
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> PickingIndicatorManager::createEdgeIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建2D矩形指示器（边）
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 创建矩形
    float halfWidth = size * 0.3f;
    float halfHeight = size * 0.1f;
    
    vertices->push_back(osg::Vec3(-halfWidth, -halfHeight, 0.0f));
    vertices->push_back(osg::Vec3(halfWidth, -halfHeight, 0.0f));
    vertices->push_back(osg::Vec3(halfWidth, halfHeight, 0.0f));
    vertices->push_back(osg::Vec3(-halfWidth, halfHeight, 0.0f));
    
    for (int i = 0; i < 4; ++i) {
        colors->push_back(m_config.edgeColor);
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 创建四边形
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置渲染状态
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    // 设置混合函数
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> PickingIndicatorManager::createFaceIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建2D正方形指示器（面）
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 创建正方形
    float halfSize = size * 0.4f;
    
    vertices->push_back(osg::Vec3(-halfSize, -halfSize, 0.0f));
    vertices->push_back(osg::Vec3(halfSize, -halfSize, 0.0f));
    vertices->push_back(osg::Vec3(halfSize, halfSize, 0.0f));
    vertices->push_back(osg::Vec3(-halfSize, halfSize, 0.0f));
    
    for (int i = 0; i < 4; ++i) {
        colors->push_back(m_config.faceColor);
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 创建四边形
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    
    // 设置渲染状态
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    // 设置混合函数
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> PickingIndicatorManager::createHighlightGeometry(Geo3D* geometry)
{
    if (!geometry) return nullptr;
    
    // 根据几何体类型创建不同的高亮效果
    osg::ref_ptr<osg::Geometry> highlightGeometry = new osg::Geometry;
    
    // 设置高亮材质
    osg::StateSet* stateSet = highlightGeometry->getOrCreateStateSet();
    osg::Material* material = new osg::Material;
    material->setDiffuse(osg::Material::FRONT_AND_BACK, m_config.highlightColor);
    material->setAmbient(osg::Material::FRONT_AND_BACK, m_config.highlightColor);
    stateSet->setAttributeAndModes(material);
    
    // 启用混合
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    // 设置深度测试
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    // 创建线框效果
    osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
    polygonOffset->setFactor(-1.0f);
    polygonOffset->setUnits(-1.0f);
    stateSet->setAttributeAndModes(polygonOffset);
    
    return highlightGeometry;
}

osg::ref_ptr<osg::Geometry> PickingIndicatorManager::createControlPointHighlightGeometry(Geo3D* geometry)
{
    if (!geometry) return nullptr;
    
    osg::ref_ptr<osg::Geometry> highlightGeometry = new osg::Geometry;
    
    // 获取控制点
    const auto& controlPoints = geometry->mm_controlPoint()->getControlPoints();
    if (controlPoints.empty()) return nullptr;
    
    // 创建顶点数组
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    for (const auto& point : controlPoints) {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
    }
    highlightGeometry->setVertexArray(vertices);
    
    // 创建颜色数组
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(m_config.selectionColor);
    highlightGeometry->setColorArray(colors);
    highlightGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    
    // 设置渲染状态
    osg::StateSet* stateSet = highlightGeometry->getOrCreateStateSet();
    
    // 设置材质
    osg::Material* material = new osg::Material;
    material->setDiffuse(osg::Material::FRONT_AND_BACK, m_config.selectionColor);
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(
        m_config.selectionColor.x() * 0.3f,
        m_config.selectionColor.y() * 0.3f,
        m_config.selectionColor.z() * 0.3f,
        m_config.selectionColor.w()
    ));
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(
        m_config.selectionColor.x() * 0.2f,
        m_config.selectionColor.y() * 0.2f,
        m_config.selectionColor.z() * 0.2f,
        m_config.selectionColor.w()
    ));
    stateSet->setAttributeAndModes(material);
    
    // 启用混合
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
    blendFunc->setDestination(osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    // 设置深度测试
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    // 创建点绘制
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, vertices->size());
    highlightGeometry->addPrimitiveSet(drawArrays);
    
    // 设置点大小
    osg::Point* point = new osg::Point;
    point->setSize(8.0f);
    stateSet->setAttributeAndModes(point);
    
    return highlightGeometry;
}

glm::vec2 PickingIndicatorManager::worldToScreen(const glm::vec3& worldPos)
{
    if (!m_camera) return glm::vec2(0.0f, 0.0f);
    
    // 获取相机矩阵
    osg::Matrix viewMatrix = m_camera->getViewMatrix();
    osg::Matrix projectionMatrix = m_camera->getProjectionMatrix();
    osg::Matrix viewportMatrix = m_camera->getViewport()->computeWindowMatrix();
    
    // 组合矩阵
    osg::Matrix modelViewProjectionMatrix = viewMatrix * projectionMatrix * viewportMatrix;
    
    // 转换世界坐标到屏幕坐标
    osg::Vec3 screenPos = osg::Vec3(worldPos.x, worldPos.y, worldPos.z) * modelViewProjectionMatrix;
    
    return glm::vec2(screenPos.x(), screenPos.y());
}

// ============================================================================
// GlobalPickingIndicatorManager Implementation
// ============================================================================

GlobalPickingIndicatorManager::GlobalPickingIndicatorManager()
{
    m_indicatorManager = std::make_unique<PickingIndicatorManager>();
}

GlobalPickingIndicatorManager& GlobalPickingIndicatorManager::getInstance()
{
    static GlobalPickingIndicatorManager instance;
    return instance;
}

bool GlobalPickingIndicatorManager::initialize(osg::Camera* camera)
{
    return m_indicatorManager->initialize(camera);
}

void GlobalPickingIndicatorManager::shutdown()
{
    m_indicatorManager->shutdown();
}

void GlobalPickingIndicatorManager::setConfig(const PickingIndicatorConfig& config)
{
    m_indicatorManager->setConfig(config);
}

const PickingIndicatorConfig& GlobalPickingIndicatorManager::getConfig() const
{
    return m_indicatorManager->getConfig();
}

osg::Group* GlobalPickingIndicatorManager::getIndicatorRoot() const
{
    return m_indicatorManager->getIndicatorRoot();
}

void GlobalPickingIndicatorManager::showIndicator(const glm::vec3& position, PickFeatureType featureType)
{
    m_indicatorManager->showIndicator(position, featureType);
}

void GlobalPickingIndicatorManager::hideIndicator()
{
    m_indicatorManager->hideIndicator();
}

void GlobalPickingIndicatorManager::updateIndicatorPosition(const glm::vec3& position, PickFeatureType featureType)
{
    m_indicatorManager->updateIndicatorPosition(position, featureType);
}

void GlobalPickingIndicatorManager::showHighlight(Geo3D* geometry)
{
    m_indicatorManager->showHighlight(geometry);
}

void GlobalPickingIndicatorManager::hideHighlight()
{
    m_indicatorManager->hideHighlight();
}

void GlobalPickingIndicatorManager::showSelectionHighlight(Geo3D* geometry)
{
    m_indicatorManager->showSelectionHighlight(geometry);
}

void GlobalPickingIndicatorManager::hideSelectionHighlight()
{
    m_indicatorManager->hideSelectionHighlight();
}

bool GlobalPickingIndicatorManager::isInitialized() const
{
    return m_indicatorManager->isInitialized();
} 