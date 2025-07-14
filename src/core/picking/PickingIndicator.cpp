#include "PickingIndicator.h"
#include "../GeometryBase.h"
#include <osg/ShapeDrawable>
#include <osg/PrimitiveSet>
#include <osg/CullFace>
#include <osg/LightModel>
#include <cmath>
#include "../../util/LogManager.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// PickingIndicator Implementation
// ============================================================================

PickingIndicator::PickingIndicator(IndicatorType type, const IndicatorConfig& config)
    : m_type(type)
    , m_config(config)
    , m_position(0.0f)
    , m_direction(0.0f, 1.0f, 0.0f)
    , m_normal(0.0f, 0.0f, 1.0f)
    , m_visible(false)
    , m_animating(false)
    , m_animationTime(0.0f)
    , m_fadeTime(0.0f)
    , m_fadeTarget(1.0f)
    , m_currentAlpha(0.0f)
    , m_animationType(AnimationType::ROTATE)
{
    m_rootNode = new osg::Group;
    m_transform = new osg::MatrixTransform;
    m_rootNode->addChild(m_transform);
    
    createGeometry();
    
    // 设置动画类型
    switch (m_type)
    {
    case IndicatorType::VERTEX_INDICATOR:
        m_animationType = AnimationType::SCALE;
        break;
    case IndicatorType::EDGE_INDICATOR:
        m_animationType = AnimationType::PULSE;
        break;
    case IndicatorType::FACE_INDICATOR:
        m_animationType = AnimationType::ROTATE;
        break;
    }
}

PickingIndicator::~PickingIndicator()
{
}

void PickingIndicator::setPosition(const glm::vec3& position)
{
    m_position = position;
    updateTransform();
}

void PickingIndicator::setDirection(const glm::vec3& direction)
{
    m_direction = glm::normalize(direction);
    updateTransform();
}

void PickingIndicator::setNormal(const glm::vec3& normal)
{
    m_normal = glm::normalize(normal);
    updateTransform();
}

void PickingIndicator::startAnimation()
{
    m_animating = true;
    m_animationTime = 0.0f;
}

void PickingIndicator::stopAnimation()
{
    m_animating = false;
}

void PickingIndicator::fadeIn(float duration)
{
    m_fadeTime = 0.0f;
    m_fadeTarget = 1.0f;
    m_config.fadeTime = duration;
}

void PickingIndicator::fadeOut(float duration)
{
    m_fadeTime = 0.0f;
    m_fadeTarget = 0.0f;
    m_config.fadeTime = duration;
}

void PickingIndicator::update(float deltaTime)
{
    updateAnimation(deltaTime);
    updateFade(deltaTime);
}

void PickingIndicator::createGeometry()
{
    switch (m_type)
    {
    case IndicatorType::VERTEX_INDICATOR:
        createVertexIndicator();
        break;
    case IndicatorType::EDGE_INDICATOR:
        createEdgeIndicator();
        break;
    case IndicatorType::FACE_INDICATOR:
        createFaceIndicator();
        break;
    }
}

void PickingIndicator::createVertexIndicator()
{
    m_geometry = IndicatorFactory::createVertexIndicator(m_config.size);
    m_transform->addChild(m_geometry);
    
    // 设置状态
    m_stateSet = m_geometry->getOrCreateStateSet();
    m_stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    m_stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    m_stateSet->setAttributeAndModes(new osg::LineWidth(m_config.lineWidth));
    m_stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    // 设置材质
    osg::Material* material = new osg::Material;
    material->setDiffuse(osg::Material::FRONT_AND_BACK, m_config.color);
    material->setAmbient(osg::Material::FRONT_AND_BACK, m_config.color * 0.3f);
    m_stateSet->setAttributeAndModes(material);
}

void PickingIndicator::createEdgeIndicator()
{
    m_geometry = IndicatorFactory::createEdgeIndicator(m_config.size);
    m_transform->addChild(m_geometry);
    
    m_stateSet = m_geometry->getOrCreateStateSet();
    m_stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    m_stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    m_stateSet->setAttributeAndModes(new osg::LineWidth(m_config.lineWidth));
    m_stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    osg::Material* material = new osg::Material;
    material->setDiffuse(osg::Material::FRONT_AND_BACK, m_config.color);
    material->setAmbient(osg::Material::FRONT_AND_BACK, m_config.color * 0.3f);
    m_stateSet->setAttributeAndModes(material);
}

void PickingIndicator::createFaceIndicator()
{
    m_geometry = IndicatorFactory::createFaceIndicator(m_config.size);
    m_transform->addChild(m_geometry);
    
    m_stateSet = m_geometry->getOrCreateStateSet();
    m_stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    m_stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    m_stateSet->setAttributeAndModes(new osg::LineWidth(m_config.lineWidth));
    m_stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    osg::Material* material = new osg::Material;
    material->setDiffuse(osg::Material::FRONT_AND_BACK, m_config.color);
    material->setAmbient(osg::Material::FRONT_AND_BACK, m_config.color * 0.3f);
    m_stateSet->setAttributeAndModes(material);
}

void PickingIndicator::updateTransform()
{
    osg::Matrix matrix;
    matrix.makeTranslate(osg::Vec3(m_position.x, m_position.y, m_position.z));
    
    // 根据类型设置方向
    if (m_type == IndicatorType::EDGE_INDICATOR)
    {
        // 边指示器：沿边方向
        osg::Vec3 dir(m_direction.x, m_direction.y, m_direction.z);
        osg::Vec3 up(0, 0, 1);
        if (fabs(dir * up) > 0.9f)
            up = osg::Vec3(0, 1, 0);
        
        osg::Vec3 right = dir ^ up;
        right.normalize();
        up = right ^ dir;
        up.normalize();
        
        osg::Matrix rotation;
        rotation.set(right.x(), right.y(), right.z(), 0,
                    up.x(), up.y(), up.z(), 0,
                    dir.x(), dir.y(), dir.z(), 0,
                    0, 0, 0, 1);
        
        matrix = rotation * matrix;
    }
    else if (m_type == IndicatorType::FACE_INDICATOR)
    {
        // 面指示器：沿法线方向
        osg::Vec3 normal(m_normal.x, m_normal.y, m_normal.z);
        osg::Vec3 up(0, 0, 1);
        if (fabs(normal * up) > 0.9f)
            up = osg::Vec3(0, 1, 0);
        
        osg::Vec3 right = normal ^ up;
        right.normalize();
        up = right ^ normal;
        up.normalize();
        
        osg::Matrix rotation;
        rotation.set(right.x(), right.y(), right.z(), 0,
                    up.x(), up.y(), up.z(), 0,
                    normal.x(), normal.y(), normal.z(), 0,
                    0, 0, 0, 1);
        
        matrix = rotation * matrix;
    }
    
    m_transform->setMatrix(matrix);
}

void PickingIndicator::updateAnimation(float deltaTime)
{
    if (!m_animating || !m_config.enableAnimation)
        return;
    
    m_animationTime += deltaTime * m_config.animationSpeed;
    
    osg::Matrix baseMatrix = m_transform->getMatrix();
    osg::Matrix animMatrix;
    
    switch (m_animationType)
    {
    case AnimationType::ROTATE:
        {
            float angle = m_animationTime * 2.0f * M_PI;
            animMatrix.makeRotate(angle, osg::Vec3(0, 0, 1));
            break;
        }
    case AnimationType::SCALE:
        {
            float scale = 1.0f + 0.2f * sin(m_animationTime * 4.0f * M_PI);
            animMatrix.makeScale(scale, scale, scale);
            break;
        }
    case AnimationType::PULSE:
        {
            float alpha = 0.5f + 0.5f * sin(m_animationTime * 3.0f * M_PI);
            osg::Vec4 color = m_config.color;
            color.a() = alpha * m_currentAlpha;
            
            osg::Material* material = dynamic_cast<osg::Material*>(
                m_stateSet->getAttribute(osg::StateAttribute::MATERIAL));
            if (material)
            {
                material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
            }
            return;
        }
    default:
        return;
    }
    
    m_transform->setMatrix(animMatrix * baseMatrix);
}

void PickingIndicator::updateFade(float deltaTime)
{
    if (m_fadeTime >= m_config.fadeTime)
        return;
    
    m_fadeTime += deltaTime;
    float t = std::min(m_fadeTime / m_config.fadeTime, 1.0f);
    
    // 使用平滑插值
    t = t * t * (3.0f - 2.0f * t);
    
    m_currentAlpha = m_currentAlpha * (1.0f - t) + m_fadeTarget * t;
    
    // 更新材质透明度
    osg::Vec4 color = m_config.color;
    color.a() = m_currentAlpha;
    
    osg::Material* material = dynamic_cast<osg::Material*>(
        m_stateSet->getAttribute(osg::StateAttribute::MATERIAL));
    if (material)
    {
        material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
    }
    
    // 更新可见性
    m_visible = (m_currentAlpha > 0.01f);
    m_rootNode->setNodeMask(m_visible ? 0xFFFFFFFF : 0x0);
}

// ============================================================================
// HighlightSystem Implementation
// ============================================================================

HighlightSystem::HighlightSystem()
    : m_highlightedObject(nullptr)
{
    m_highlightRoot = new osg::Group;
}

HighlightSystem::~HighlightSystem()
{
}

bool HighlightSystem::initialize()
{
    return true;
}

void HighlightSystem::highlightObject(Geo3D* geo, const osg::Vec4& color)
{
    if (!geo)
        return;
    
    // 清除之前的高亮
    clearHighlight();
    
    // 创建高亮几何体
    createHighlightGeometry(geo, color);
    
    m_highlightedObject = geo;
}

void HighlightSystem::clearHighlight()
{
    if (m_currentHighlight)
    {
        m_highlightRoot->removeChild(m_currentHighlight);
        m_currentHighlight = nullptr;
    }
    
    m_highlightedObject = nullptr;
}

void HighlightSystem::createHighlightGeometry(Geo3D* geo, const osg::Vec4& color)
{
    if (!geo || !geo->getOSGNode())
        return;
    
    m_currentHighlight = new osg::Group;
    m_highlightRoot->addChild(m_currentHighlight);
    
    // 遍历几何对象的子节点
    osg::Group* geoGroup = geo->getOSGNode()->asGroup();
    if (geoGroup)
    {
        for (unsigned int i = 0; i < geoGroup->getNumChildren(); ++i)
        {
            osg::Node* child = geoGroup->getChild(i);
            osg::Geometry* geometry = child->asGeometry();
            
            if (geometry)
            {
                // 创建高亮轮廓
                osg::ref_ptr<osg::Geometry> highlightGeometry = 
                    IndicatorFactory::createHighlightOutline(geometry);
                
                if (highlightGeometry)
                {
                    m_currentHighlight->addChild(highlightGeometry);
                    setupHighlightState(highlightGeometry->getOrCreateStateSet(), color);
                }
            }
        }
    }
}

void HighlightSystem::setupHighlightState(osg::StateSet* stateSet, const osg::Vec4& color)
{
    if (!stateSet)
        return;
    
    // 设置线框模式
    osg::PolygonMode* polygonMode = new osg::PolygonMode;
    polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    stateSet->setAttributeAndModes(polygonMode);
    
    // 设置线宽
    stateSet->setAttributeAndModes(new osg::LineWidth(3.0f));
    
    // 禁用光照
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    
    // 设置深度测试
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    // 设置多边形偏移，避免Z-fighting
    osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
    polygonOffset->setFactor(-1.0f);
    polygonOffset->setUnits(-1.0f);
    stateSet->setAttributeAndModes(polygonOffset);
    
    // 设置材质
    osg::Material* material = new osg::Material;
    material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
    material->setAmbient(osg::Material::FRONT_AND_BACK, color * 0.3f);
    stateSet->setAttributeAndModes(material);
    
    // 设置渲染顺序
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
}

// ============================================================================
// PickingIndicatorManager Implementation
// ============================================================================

PickingIndicatorManager::PickingIndicatorManager()
    : m_indicatorVisible(false)
    , m_lastUpdateTime(0.0f)
    , m_updateInterval(1.0f / 60.0f)  // 60 FPS
{
    m_indicatorRoot = new osg::Group;
    m_indicatorRoot->setName("PickingIndicatorRoot");
    
    // 设置指示器根节点状态
    osg::StateSet* stateSet = m_indicatorRoot->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    m_highlightSystem = new HighlightSystem;
}

PickingIndicatorManager::~PickingIndicatorManager()
{
}

bool PickingIndicatorManager::initialize()
{
    if (!m_highlightSystem) {
        LOG_ERROR("Failed to create highlight system", "拾取");
        return false;
    }
    
    if (!m_highlightSystem->initialize()) {
        LOG_WARNING("Highlight system initialization failed, but continuing", "拾取");
        // 不返回false，因为指示器功能仍然可以工作
    }
    
    // 设置默认指示器配置
    IndicatorConfig vertexConfig;
    vertexConfig.size = 0.05f;
    vertexConfig.color = osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f);  // 红色
    vertexConfig.lineWidth = 2.0f;
    m_indicatorConfigs[IndicatorType::VERTEX_INDICATOR] = vertexConfig;
    
    IndicatorConfig edgeConfig;
    edgeConfig.size = 0.08f;
    edgeConfig.color = osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f);  // 绿色
    edgeConfig.lineWidth = 3.0f;
    m_indicatorConfigs[IndicatorType::EDGE_INDICATOR] = edgeConfig;
    
    IndicatorConfig faceConfig;
    faceConfig.size = 0.1f;
    faceConfig.color = osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f);  // 蓝色
    faceConfig.lineWidth = 2.0f;
    m_indicatorConfigs[IndicatorType::FACE_INDICATOR] = faceConfig;
    
    LOG_SUCCESS("PickingIndicatorManager initialized successfully", "拾取");
    return true;
}

void PickingIndicatorManager::showIndicator(const PickingResult& result)
{
    if (!result.hasResult)
    {
        hideIndicator();
        return;
    }
    
    // 检查是否需要创建新指示器
    if (!m_currentIndicator || 
        m_lastResult.id.pack() != result.id.pack() ||
        glm::distance(m_lastResult.worldPos, result.worldPos) > 0.001f)
    {
        createIndicator(result);
        m_lastResult = result;
    }
    
    // 高亮对象
    if (result.geometry)
    {
        highlightObject(result.geometry);
    }
    
    m_indicatorVisible = true;
}

void PickingIndicatorManager::hideIndicator()
{
    if (m_currentIndicator)
    {
        m_currentIndicator->fadeOut();
    }
    
    m_indicatorVisible = false;
}

void PickingIndicatorManager::clearAll()
{
    hideIndicator();
    clearHighlight();
    
    if (m_currentIndicator)
    {
        m_indicatorRoot->removeChild(m_currentIndicator->getNode());
        m_currentIndicator = nullptr;
    }
}

void PickingIndicatorManager::highlightObject(Geo3D* geo)
{
    m_highlightSystem->highlightObject(geo);
}

void PickingIndicatorManager::clearHighlight()
{
    m_highlightSystem->clearHighlight();
}

void PickingIndicatorManager::setIndicatorConfig(IndicatorType type, const IndicatorConfig& config)
{
    m_indicatorConfigs[type] = config;
}

const IndicatorConfig& PickingIndicatorManager::getIndicatorConfig(IndicatorType type) const
{
    auto it = m_indicatorConfigs.find(type);
    if (it != m_indicatorConfigs.end())
        return it->second;
    
    static IndicatorConfig defaultConfig;
    return defaultConfig;
}

void PickingIndicatorManager::update(float deltaTime)
{
    m_lastUpdateTime += deltaTime;
    
    if (m_lastUpdateTime >= m_updateInterval)
    {
        if (m_currentIndicator)
        {
            m_currentIndicator->update(m_lastUpdateTime);
        }
        
        m_lastUpdateTime = 0.0f;
    }
}

void PickingIndicatorManager::onPickingResult(const PickingResult& result)
{
    showIndicator(result);
}

void PickingIndicatorManager::createIndicator(const PickingResult& result)
{
    // 移除旧指示器
    if (m_currentIndicator)
    {
        m_indicatorRoot->removeChild(m_currentIndicator->getNode());
    }
    
    // 创建新指示器
    IndicatorType type = getIndicatorType(result.id.getTypeCode());
    const IndicatorConfig& config = getIndicatorConfig(type);
    
    m_currentIndicator = new PickingIndicator(type, config);
    m_currentIndicator->setPosition(result.worldPos);
    
    // 设置方向（如果适用）
    if (type == IndicatorType::EDGE_INDICATOR)
    {
        // 这里可以根据边的方向设置指示器方向
        // 简化实现，使用默认方向
        m_currentIndicator->setDirection(glm::vec3(1.0f, 0.0f, 0.0f));
    }
    
    // 添加到场景图
    m_indicatorRoot->addChild(m_currentIndicator->getNode());
    
    // 启动动画
    m_currentIndicator->fadeIn();
    m_currentIndicator->startAnimation();
}

IndicatorType PickingIndicatorManager::getIndicatorType(PickingID64::TypeCode typeCode) const
{
    switch (typeCode)
    {
    case PickingID64::TYPE_VERTEX:
        return IndicatorType::VERTEX_INDICATOR;
    case PickingID64::TYPE_EDGE:
        return IndicatorType::EDGE_INDICATOR;
    case PickingID64::TYPE_FACE:
        return IndicatorType::FACE_INDICATOR;
    default:
        return IndicatorType::FACE_INDICATOR;
    }
}

// ============================================================================
// IndicatorFactory Implementation
// ============================================================================

osg::ref_ptr<osg::Geometry> IndicatorFactory::createVertexIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建方框顶点
    osg::ref_ptr<osg::Vec3Array> vertices = createBoxVertices(size);
    geometry->setVertexArray(vertices);
    
    // 创建线框
    osg::ref_ptr<osg::DrawElementsUInt> lines = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    // 方框的12条边
    int edges[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,  // 底面
        4, 5, 5, 6, 6, 7, 7, 4,  // 顶面
        0, 4, 1, 5, 2, 6, 3, 7   // 连接边
    };
    
    for (int i = 0; i < 24; ++i)
    {
        lines->push_back(edges[i]);
    }
    
    geometry->addPrimitiveSet(lines);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> IndicatorFactory::createEdgeIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建箭头顶点
    osg::ref_ptr<osg::Vec3Array> vertices = createArrowVertices(size);
    geometry->setVertexArray(vertices);
    
    // 创建三角形
    osg::ref_ptr<osg::DrawElementsUInt> triangles = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
    
    // 箭头由两个三角形组成
    triangles->push_back(0); triangles->push_back(1); triangles->push_back(2);
    triangles->push_back(0); triangles->push_back(2); triangles->push_back(3);
    
    geometry->addPrimitiveSet(triangles);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> IndicatorFactory::createFaceIndicator(float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    
    // 创建圆环顶点
    osg::ref_ptr<osg::Vec3Array> vertices = createCircleVertices(size, 32);
    geometry->setVertexArray(vertices);
    
    // 创建线环
    osg::ref_ptr<osg::DrawElementsUInt> lineLoop = new osg::DrawElementsUInt(osg::PrimitiveSet::LINE_LOOP);
    
    for (unsigned int i = 0; i < vertices->size(); ++i)
    {
        lineLoop->push_back(i);
    }
    
    geometry->addPrimitiveSet(lineLoop);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> IndicatorFactory::createHighlightOutline(osg::Geometry* originalGeometry)
{
    if (!originalGeometry)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> highlightGeometry = new osg::Geometry;
    
    // 复制顶点数组
    highlightGeometry->setVertexArray(originalGeometry->getVertexArray());
    
    // 复制图元集
    for (unsigned int i = 0; i < originalGeometry->getNumPrimitiveSets(); ++i)
    {
        highlightGeometry->addPrimitiveSet(originalGeometry->getPrimitiveSet(i));
    }
    
    return highlightGeometry;
}

osg::ref_ptr<osg::Vec3Array> IndicatorFactory::createCircleVertices(float radius, int segments)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    for (int i = 0; i < segments; ++i)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        vertices->push_back(osg::Vec3(x, y, 0.0f));
    }
    
    return vertices;
}

osg::ref_ptr<osg::Vec3Array> IndicatorFactory::createArrowVertices(float size)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    // 三角形箭头
    vertices->push_back(osg::Vec3(0.0f, size, 0.0f));      // 顶点
    vertices->push_back(osg::Vec3(-size * 0.5f, 0.0f, 0.0f)); // 左下
    vertices->push_back(osg::Vec3(size * 0.5f, 0.0f, 0.0f));  // 右下
    vertices->push_back(osg::Vec3(0.0f, -size * 0.3f, 0.0f)); // 尾部
    
    return vertices;
}

osg::ref_ptr<osg::Vec3Array> IndicatorFactory::createBoxVertices(float size)
{
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    
    float half = size * 0.5f;
    
    // 立方体的8个顶点
    vertices->push_back(osg::Vec3(-half, -half, -half));
    vertices->push_back(osg::Vec3( half, -half, -half));
    vertices->push_back(osg::Vec3( half,  half, -half));
    vertices->push_back(osg::Vec3(-half,  half, -half));
    vertices->push_back(osg::Vec3(-half, -half,  half));
    vertices->push_back(osg::Vec3( half, -half,  half));
    vertices->push_back(osg::Vec3( half,  half,  half));
    vertices->push_back(osg::Vec3(-half,  half,  half));
    
    return vertices;
} 