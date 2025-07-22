#include "PickingIndicator.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include "../managers/GeoNodeManager.h"
#include <osg/ShapeDrawable>
#include <osg/PolygonOffset>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Point>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Timer>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PickingIndicator::PickingIndicator()
{
    LOG_INFO("创建拾取指示器系统", "拾取");
}

PickingIndicator::~PickingIndicator()
{
    shutdown();
}

bool PickingIndicator::initialize()
{
    // 创建指示器根节点
    m_indicatorRoot = new osg::Group;
    m_indicatorRoot->setName("PickingIndicatorRoot");
    m_indicatorRoot->setNodeMask(NODE_MASK_PICKING_INDICATOR);
    
    // 创建位置变换节点
    m_indicatorTransform = new osg::PositionAttitudeTransform;
    m_indicatorTransform->setName("IndicatorTransform");
    m_indicatorRoot->addChild(m_indicatorTransform);
    
    // 创建指示器几何体
    createIndicatorGeometry();
    
    // 添加各种指示器到变换节点
    if (m_vertexIndicator) {
        m_indicatorTransform->addChild(m_vertexIndicator);
        m_vertexIndicator->setNodeMask(NODE_MASK_NONE); // 初始隐藏
    }
    
    if (m_edgeIndicator) {
        m_indicatorTransform->addChild(m_edgeIndicator);
        m_edgeIndicator->setNodeMask(NODE_MASK_NONE); // 初始隐藏
    }
    
    if (m_faceIndicator) {
        m_indicatorTransform->addChild(m_faceIndicator);
        m_faceIndicator->setNodeMask(NODE_MASK_NONE); // 初始隐藏
    }
    
    m_initialized = true;
    
    LOG_SUCCESS("拾取指示器系统初始化成功", "拾取");
    return true;
}

void PickingIndicator::shutdown()
{
    if (m_initialized) {
        hideIndicator();
        
        m_vertexIndicator = nullptr;
        m_edgeIndicator = nullptr;
        m_faceIndicator = nullptr;
        m_indicatorTransform = nullptr;
        m_indicatorRoot = nullptr;
        
        m_initialized = false;
        
        LOG_INFO("拾取指示器系统已关闭", "拾取");
    }
}

void PickingIndicator::showIndicator(const glm::vec3& position, PickFeatureType featureType)
{
    if (!m_initialized) return;
    
    // 更新当前特征类型和可见状态
    m_currentFeatureType = featureType;
    m_isVisible = true;
    m_currentPosition = position;  // 保存当前位置用于动画缩放计算
    
    // 先隐藏所有指示器
    if (m_vertexIndicator) m_vertexIndicator->setNodeMask(NODE_MASK_NONE);
    if (m_edgeIndicator) m_edgeIndicator->setNodeMask(NODE_MASK_NONE);
    if (m_faceIndicator) m_faceIndicator->setNodeMask(NODE_MASK_NONE);
    
    // 根据特征类型显示对应的指示器
    osg::ref_ptr<osg::Group> activeIndicator = nullptr;
    switch (featureType) {
        case PickFeatureType::VERTEX:
            activeIndicator = m_vertexIndicator;
            break;
        case PickFeatureType::EDGE:
            activeIndicator = m_edgeIndicator;
            break;
        case PickFeatureType::FACE:
            activeIndicator = m_faceIndicator;
            break;
        default:
            return;
    }
    
    if (!activeIndicator) return;
    
    // 显示选中的指示器
    activeIndicator->setNodeMask(NODE_MASK_PICKING_INDICATOR);
    
    // 更新指示器位置和缩放
    if (m_indicatorTransform) {
        m_indicatorTransform->setPosition(osg::Vec3(position.x, position.y, position.z));
        
        // 设置基础缩放，让指示器在不同距离下保持合适的大小
        float baseScale = 1.0f;
        
        // 启用基于距离的缩放调整（可选）
        baseScale = calculateDistanceBasedScale(position);
        
        m_indicatorTransform->setScale(osg::Vec3(baseScale, baseScale, baseScale));
    }
    
    // 记录动画开始时间
    m_animationStartTime = osg::Timer::instance()->time_s();
}

void PickingIndicator::hideIndicator()
{
    if (!m_initialized) return;
    
    // 隐藏所有指示器
    if (m_vertexIndicator) m_vertexIndicator->setNodeMask(NODE_MASK_NONE);
    if (m_edgeIndicator) m_edgeIndicator->setNodeMask(NODE_MASK_NONE);
    if (m_faceIndicator) m_faceIndicator->setNodeMask(NODE_MASK_NONE);
    
    m_isVisible = false;
    m_currentFeatureType = PickFeatureType::NONE;
}

void PickingIndicator::createIndicatorGeometry()
{
    // 创建三种指示器组件
    createVertexIndicator();
    createEdgeIndicator();
    createFaceIndicator();
}

void PickingIndicator::createVertexIndicator()
{
    m_vertexIndicator = new osg::Group;
    m_vertexIndicator->setName("VertexIndicator");
    
    // 创建圆形几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 创建圆形（用多边形近似）
    const int segments = 16;
    float radius = m_config.size; // 使用配置的大小
    
    for (int i = 0; i < segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        vertices->push_back(osg::Vec3(x, y, 0.0f));
        colors->push_back(osg::Vec4(m_config.color.r(), m_config.color.g(), m_config.color.b(), m_config.color.a()));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, vertices->size()));
    
    // 创建Geode并添加几何体
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry);
    
    // 设置材质
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    m_vertexIndicator->addChild(geode);
}

void PickingIndicator::createEdgeIndicator()
{
    m_edgeIndicator = new osg::Group;
    m_edgeIndicator->setName("EdgeIndicator");
    
    // 创建线段几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    float length = m_config.size;
    vertices->push_back(osg::Vec3(-length, 0.0f, 0.0f));
    vertices->push_back(osg::Vec3(length, 0.0f, 0.0f));
    
    colors->push_back(osg::Vec4(m_config.color.r(), m_config.color.g(), m_config.color.b(), m_config.color.a()));
    colors->push_back(osg::Vec4(m_config.color.r(), m_config.color.g(), m_config.color.b(), m_config.color.a()));
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));
    
    // 创建Geode并添加几何体
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry);
    
    // 设置材质
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(m_config.lineWidth);
    stateSet->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
    
    m_edgeIndicator->addChild(geode);
}

void PickingIndicator::createFaceIndicator()
{
    m_faceIndicator = new osg::Group;
    m_faceIndicator->setName("FaceIndicator");
    
    // 创建三角形几何体
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    float size = m_config.size;
    vertices->push_back(osg::Vec3(0.0f, size, 0.0f));
    vertices->push_back(osg::Vec3(-size * 0.866f, -size * 0.5f, 0.0f));
    vertices->push_back(osg::Vec3(size * 0.866f, -size * 0.5f, 0.0f));
    
    for (int i = 0; i < 3; ++i) {
        colors->push_back(osg::Vec4(m_config.color.r(), m_config.color.g(), m_config.color.b(), m_config.color.a()));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, vertices->size()));
    
    // 创建Geode并添加几何体
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry);
    
    // 设置材质
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    m_faceIndicator->addChild(geode);
}

void PickingIndicator::updateAnimation(double currentTime)
{
    if (!m_isVisible || !m_config.enableAnimation) return;
    
    double elapsed = currentTime - m_animationStartTime;
    
    // 简单的脉冲动画，基于距离相关的基础缩放进行
    float baseScale = calculateDistanceBasedScale(m_currentPosition);
    float animationScale = baseScale * (1.0f + 0.2f * sin(elapsed * m_config.animationSpeed));
    
    if (m_indicatorTransform) {
        m_indicatorTransform->setScale(osg::Vec3(animationScale, animationScale, animationScale));
    }
}

float PickingIndicator::calculateDistanceBasedScale(const glm::vec3& position) const
{
    // 简单的距离自适应缩放 - 可以根据需要调整
    // 这里假设有一个合理的基础距离和缩放因子
    
    const float baseDistance = 100.0f;  // 基础距离
    const float minScale = 0.5f;        // 最小缩放
    const float maxScale = 3.0f;        // 最大缩放
    
    // TODO: 如果需要基于相机距离的缩放，可以在这里实现
    // 目前返回固定值，用户可以根据需要修改
    
    // 示例：根据Z坐标调整缩放（简化的距离计算）
    float distance = std::abs(position.z);
    float scale = baseDistance / std::max(distance, 10.0f); // 避免除零
    
    // 限制缩放范围
    scale = std::max(minScale, std::min(maxScale, scale));
    
    return scale;
}

 