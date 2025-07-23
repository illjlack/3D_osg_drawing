#include "PickingIndicator.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"
#include "../managers/GeoNodeManager.h"
#include "../camera/CameraController.h"  // 添加CameraController头文件
#include <osg/ShapeDrawable>
#include <osg/PolygonOffset>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Point>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Timer>
#include <osg/AutoTransform>
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
        m_faceTransform = nullptr;
        m_indicatorTransform = nullptr;
        m_indicatorRoot = nullptr;
        
        m_initialized = false;
        
        LOG_INFO("拾取指示器系统已关闭", "拾取");
    }
}

void PickingIndicator::showIndicator(const glm::vec3& position, PickFeatureType featureType, const glm::vec3& normal)
{
    if (!m_initialized) return;
    
    // 更新当前特征类型和可见状态
    m_currentFeatureType = featureType;
    m_isVisible = true;
    m_currentPosition = position;  // 保存当前位置用于动画
    
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
            // 为面指示器设置法向量旋转
            if (m_faceTransform) {
                // 计算从默认Z轴到面法向量的旋转
                osg::Vec3 defaultNormal(0.0f, 0.0f, 1.0f);
                osg::Vec3 faceNormal(normal.x, normal.y, normal.z);
                faceNormal.normalize();
                
                // 计算旋转四元数
                osg::Quat rotation;
                rotation.makeRotate(defaultNormal, faceNormal);
                
                // 应用旋转到面变换节点
                m_faceTransform->setAttitude(rotation);
            }
            break;
        default:
            return;
    }
    
    if (!activeIndicator) return;
    
    // 显示选中的指示器
    activeIndicator->setNodeMask(NODE_MASK_PICKING_INDICATOR);
    
    // 更新指示器位置（不再手动设置缩放，由AutoTransform自动处理）
    if (m_indicatorTransform) {
        m_indicatorTransform->setPosition(osg::Vec3(position.x, position.y, position.z));
        // 移除手动缩放设置，让AutoTransform处理屏幕固定大小
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
    
    // 创建AutoTransform节点，设置为屏幕固定像素大小
    osg::ref_ptr<osg::AutoTransform> autoTransform = new osg::AutoTransform;
    autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
    autoTransform->setAutoScaleToScreen(true);  // 启用屏幕固定大小
    autoTransform->setAutoScaleTransitionWidthRatio(0.5);  // 设置过渡比例
    
    // 创建圆形几何体 - 使用基于像素半径的实际大小
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    // 创建圆形（用多边形近似）- 使用像素半径作为基础大小
    const int segments = 16;
    float radius = m_config.pickingPixelRadius;
    
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
    
    // 设置材质和线宽
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(m_config.lineWidth);
    stateSet->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
    
    // 将几何体添加到AutoTransform
    autoTransform->addChild(geode);
    
    // 将AutoTransform添加到指示器组
    m_vertexIndicator->addChild(autoTransform);
}

void PickingIndicator::createEdgeIndicator()
{
    m_edgeIndicator = new osg::Group;
    m_edgeIndicator->setName("EdgeIndicator");
    
    // 创建AutoTransform节点，设置为屏幕固定像素大小
    osg::ref_ptr<osg::AutoTransform> autoTransform = new osg::AutoTransform;
    autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
    autoTransform->setAutoScaleToScreen(true);  // 启用屏幕固定大小
    autoTransform->setAutoScaleTransitionWidthRatio(0.5);  // 设置过渡比例
    
    // 创建正方形几何体 - 使用基于像素半径的实际大小
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    float size = m_config.pickingPixelRadius; // 转换为合适的世界单位
    
    // 正方形的四个顶点
    vertices->push_back(osg::Vec3(-size, -size, 0.0f));
    vertices->push_back(osg::Vec3(size, -size, 0.0f));
    vertices->push_back(osg::Vec3(size, size, 0.0f));
    vertices->push_back(osg::Vec3(-size, size, 0.0f));
    
    for (int i = 0; i < 4; ++i) {
        colors->push_back(osg::Vec4(m_config.color.r(), m_config.color.g(), m_config.color.b(), m_config.color.a()));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_LOOP, 0, vertices->size()));
    
    // 创建Geode并添加几何体
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geometry);
    
    // 设置材质和线宽
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(m_config.lineWidth);
    stateSet->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
    
    // 将几何体添加到AutoTransform
    autoTransform->addChild(geode);
    
    // 将AutoTransform添加到指示器组
    m_edgeIndicator->addChild(autoTransform);
}

void PickingIndicator::createFaceIndicator()
{
    m_faceIndicator = new osg::Group;
    m_faceIndicator->setName("FaceIndicator");
    
    // 创建法向量变换节点用于根据面法向量旋转指示器
    m_faceTransform = new osg::PositionAttitudeTransform;
    m_faceTransform->setName("FaceTransform");
    
    // 在面变换内部添加AutoTransform用于屏幕固定大小
    osg::ref_ptr<osg::AutoTransform> autoTransform = new osg::AutoTransform;
    autoTransform->setAutoRotateMode(osg::AutoTransform::NO_ROTATION);  // 面指示器不需要朝向屏幕
    autoTransform->setAutoScaleToScreen(true);  // 启用屏幕固定大小
    autoTransform->setAutoScaleTransitionWidthRatio(0.5);  // 设置过渡比例
    
    // 创建三角形几何体 - 使用基于像素半径的实际大小
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    float size = m_config.pickingPixelRadius; // 转换为合适的世界单位（增大10倍）
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
    
    // 设置材质和线宽
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(m_config.lineWidth);
    stateSet->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
    
    // 将几何体添加到AutoTransform
    autoTransform->addChild(geode);
    
    // 将AutoTransform添加到面变换节点
    m_faceTransform->addChild(autoTransform);
    
    // 将变换节点添加到指示器组
    m_faceIndicator->addChild(m_faceTransform);
}

void PickingIndicator::updateAnimation(double currentTime)
{
    if (!m_isVisible || !m_config.enableAnimation) return;
    
    double elapsed = currentTime - m_animationStartTime;
    
    // 简单的脉冲动画 - 通过修改几何体顶点实现动画效果
    float animationScale = 1.0f + 0.2f * sin(elapsed * m_config.animationSpeed);
    
    // 注意：由于使用了AutoTransform，动画效果可能需要不同的实现方式
    // 这里暂时保留接口，可以在需要时实现具体的动画效果
}
 