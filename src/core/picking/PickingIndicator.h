#pragma once
#pragma execution_character_set("utf-8")

#include "PickingSystem.h"
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/AnimationPath>
#include <osg/Timer>
#include <memory>

class Geo3D;

// 单个指示器
class PickingIndicator : public osg::Referenced
{
public:
    PickingIndicator(IndicatorType type, const IndicatorConfig& config = IndicatorConfig());
    virtual ~PickingIndicator();
    
    // 设置位置和方向
    void setPosition(const glm::vec3& position);
    void setDirection(const glm::vec3& direction);  // 用于边指示器
    void setNormal(const glm::vec3& normal);        // 用于面指示器
    
    // 动画控制
    void startAnimation();
    void stopAnimation();
    void fadeIn(float duration = 0.3f);
    void fadeOut(float duration = 0.3f);
    
    // 访问器
    osg::Group* getNode() const { return m_rootNode.get(); }
    IndicatorType getType() const { return m_type; }
    bool isVisible() const { return m_visible; }
    
    // 更新
    void update(float deltaTime);

private:
    void createGeometry();
    void createVertexIndicator();
    void createEdgeIndicator();
    void createFaceIndicator();
    
    void updateTransform();
    void updateAnimation(float deltaTime);
    void updateFade(float deltaTime);

private:
    IndicatorType m_type;
    IndicatorConfig m_config;
    
    // 几何属性
    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::vec3 m_normal;
    
    // OSG节点
    osg::ref_ptr<osg::Group> m_rootNode;
    osg::ref_ptr<osg::MatrixTransform> m_transform;
    osg::ref_ptr<osg::Geometry> m_geometry;
    osg::ref_ptr<osg::StateSet> m_stateSet;
    
    // 动画状态
    bool m_visible;
    bool m_animating;
    float m_animationTime;
    float m_fadeTime;
    float m_fadeTarget;
    float m_currentAlpha;
    
    // 动画类型
    enum class AnimationType
    {
        NONE,
        ROTATE,
        SCALE,
        PULSE
    } m_animationType;
};

// 高亮系统
class HighlightSystem : public osg::Referenced
{
public:
    HighlightSystem();
    virtual ~HighlightSystem();
    
    // 初始化
    bool initialize();
    
    // 高亮对象
    void highlightObject(Geo3D* geo, const osg::Vec4& color = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
    void clearHighlight();
    
    // 访问器
    osg::Group* getHighlightRoot() const { return m_highlightRoot.get(); }

private:
    void createHighlightGeometry(Geo3D* geo, const osg::Vec4& color);
    void setupHighlightState(osg::StateSet* stateSet, const osg::Vec4& color);

private:
    osg::ref_ptr<osg::Group> m_highlightRoot;
    osg::ref_ptr<osg::Group> m_currentHighlight;
    Geo3D* m_highlightedObject;
};

// 拾取指示器管理器
class PickingIndicatorManager : public osg::Referenced
{
public:
    PickingIndicatorManager();
    virtual ~PickingIndicatorManager();
    
    // 初始化
    bool initialize();
    
    // 指示器管理
    void showIndicator(const PickingResult& result);
    void hideIndicator();
    void clearAll();
    
    // 高亮管理
    void highlightObject(Geo3D* geo);
    void clearHighlight();
    
    // 配置
    void setIndicatorConfig(IndicatorType type, const IndicatorConfig& config);
    const IndicatorConfig& getIndicatorConfig(IndicatorType type) const;
    
    // 访问器
    osg::Group* getIndicatorRoot() const { return m_indicatorRoot.get(); }
    osg::Group* getHighlightRoot() const { return m_highlightSystem->getHighlightRoot(); }
    
    // 更新
    void update(float deltaTime);
    
    // 事件处理
    void onPickingResult(const PickingResult& result);

private:
    void createIndicator(const PickingResult& result);
    IndicatorType getIndicatorType(PickingID64::TypeCode typeCode) const;

private:
    // 指示器系统
    osg::ref_ptr<osg::Group> m_indicatorRoot;
    osg::ref_ptr<PickingIndicator> m_currentIndicator;
    
    // 高亮系统
    osg::ref_ptr<HighlightSystem> m_highlightSystem;
    
    // 配置
    std::map<IndicatorType, IndicatorConfig> m_indicatorConfigs;
    
    // 状态
    PickingResult m_lastResult;
    bool m_indicatorVisible;
    
    // 性能优化
    float m_lastUpdateTime;
    float m_updateInterval;  // 更新间隔(秒)
};

// 动画辅助类
class IndicatorAnimation
{
public:
    // 创建旋转动画
    static osg::ref_ptr<osg::AnimationPath> createRotationAnimation(
        float duration, const osg::Vec3& axis = osg::Vec3(0, 0, 1));
    
    // 创建缩放动画
    static osg::ref_ptr<osg::AnimationPath> createScaleAnimation(
        float duration, float minScale = 0.8f, float maxScale = 1.2f);
    
    // 创建脉冲动画
    static osg::ref_ptr<osg::AnimationPath> createPulseAnimation(
        float duration, float minAlpha = 0.3f, float maxAlpha = 1.0f);
    
    // 创建淡入淡出动画
    static osg::ref_ptr<osg::AnimationPath> createFadeAnimation(
        float duration, float startAlpha = 0.0f, float endAlpha = 1.0f);
};

// 指示器工厂
class IndicatorFactory
{
public:
    // 创建顶点指示器几何体
    static osg::ref_ptr<osg::Geometry> createVertexIndicator(float size);
    
    // 创建边指示器几何体(三角箭头)
    static osg::ref_ptr<osg::Geometry> createEdgeIndicator(float size);
    
    // 创建面指示器几何体(圆环)
    static osg::ref_ptr<osg::Geometry> createFaceIndicator(float size);
    
    // 创建高亮轮廓
    static osg::ref_ptr<osg::Geometry> createHighlightOutline(osg::Geometry* originalGeometry);

private:
    // 辅助函数
    static osg::ref_ptr<osg::Vec3Array> createCircleVertices(float radius, int segments = 32);
    static osg::ref_ptr<osg::Vec3Array> createArrowVertices(float size);
    static osg::ref_ptr<osg::Vec3Array> createBoxVertices(float size);
}; 