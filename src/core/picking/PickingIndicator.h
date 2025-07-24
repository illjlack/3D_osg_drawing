#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "PickingTypes.h"
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/PositionAttitudeTransform>
#include <glm/glm.hpp>

// 前向声明
class CameraController;

// 指示器配置
struct IndicatorConfig
{
    double pickingPixelRadius;       // 拾取像素半径（屏幕固定大小）
    osg::Vec4 color;               // 颜色
    double lineWidth;               // 线宽
    double animationSpeed;          // 动画速度
    bool enableAnimation;          // 是否启用动画
    double fadeTime;                // 淡入淡出时间
    
    IndicatorConfig()
        : pickingPixelRadius(10.0)          // 与拾取系统的cylinderRadius保持一致
        , color(1.0, 1.0, 0.0, 1.0)     // 黄色
        , lineWidth(3.0)                    // 线宽
        , animationSpeed(2.0)
        , enableAnimation(true)
        , fadeTime(0.3)
    {}
};

// 拾取指示器 - 用于显示拾取位置和类型
class PickingIndicator : public osg::Referenced
{
public:
    PickingIndicator();
    virtual ~PickingIndicator();
    
    // 初始化指示器
    bool initialize();
    
    // 关闭指示器
    void shutdown();
    
    // 显示指示器
    void showIndicator(const glm::dvec3& position, PickFeatureType featureType, const glm::dvec3& normal = glm::dvec3(0.0, 0.0, 1.0));
    
    // 隐藏指示器
    void hideIndicator();
    
    // 更新指示器动画
    void updateAnimation(double currentTime);
    
    // 获取指示器根节点
    osg::Group* getIndicatorRoot() const { return m_indicatorRoot.get(); }
    
    // 配置
    void setConfig(const IndicatorConfig& config) { m_config = config; }
    const IndicatorConfig& getConfig() const { return m_config; }
    
    // 状态查询
    bool isVisible() const { return m_isVisible; }

private:
    // 创建指示器几何体
    void createIndicatorGeometry();
    void createVertexIndicator();
    void createEdgeIndicator(); 
    void createFaceIndicator();
    
    // 内部状态
    bool m_initialized = false;
    bool m_isVisible = false;
    PickFeatureType m_currentFeatureType = PickFeatureType::NONE;
    double m_animationStartTime = 0.0;
    glm::dvec3 m_currentPosition{0.0};  // 当前指示器位置（用于动画）
    
    // 配置
    IndicatorConfig m_config;
    
    // OSG节点
    osg::ref_ptr<osg::Group> m_indicatorRoot;
    osg::ref_ptr<osg::Group> m_vertexIndicator;
    osg::ref_ptr<osg::Group> m_edgeIndicator;
    osg::ref_ptr<osg::Group> m_faceIndicator;
    
    // 变换节点（用于位置更新）
    osg::ref_ptr<osg::PositionAttitudeTransform> m_indicatorTransform;
    
    // 面指示器的法向量变换节点
    osg::ref_ptr<osg::PositionAttitudeTransform> m_faceTransform;
}; 




