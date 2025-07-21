#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "../GeometryBase.h"
#include <osg/Camera>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/Billboard>
#include <glm/glm.hpp>
#include <memory>

// 前向声明
class Geo3D;

// 拾取特征类型
enum class PickFeatureType {
    NONE = 0,
    VERTEX = 1,    // 顶点
    EDGE = 2,       // 边
    FACE = 3        // 面
};

// 拾取指示器配置
struct PickingIndicatorConfig {
    float indicatorSize = 0.2f;           // 指示器大小
    bool enableIndicator = true;          // 是否启用指示器
    bool enableHighlight = true;          // 是否启用高亮
    
    // 颜色配置
    osg::Vec4 vertexColor = osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f);   // 顶点指示器颜色（红色）
    osg::Vec4 edgeColor = osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f);     // 边指示器颜色（绿色）
    osg::Vec4 faceColor = osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f);     // 面指示器颜色（蓝色）
    osg::Vec4 highlightColor = osg::Vec4(1.0f, 1.0f, 0.0f, 0.3f); // 高亮颜色（半透明黄色）
    osg::Vec4 selectionColor = osg::Vec4(1.0f, 1.0f, 0.0f, 0.8f); // 选择高亮颜色（黄色）
};

// 拾取指示器管理器
class PickingIndicatorManager
{
public:
    PickingIndicatorManager();
    virtual ~PickingIndicatorManager();
    
    // 初始化
    bool initialize(osg::Camera* camera);
    void shutdown();
    
    // 配置
    void setConfig(const PickingIndicatorConfig& config);
    const PickingIndicatorConfig& getConfig() const { return m_config; }
    
    // 获取根节点（用于添加到场景图）
    osg::Group* getIndicatorRoot() const { return m_indicatorRoot.get(); }
    
    // 指示器管理
    void showIndicator(const glm::vec3& position, PickFeatureType featureType);
    void hideIndicator();
    void updateIndicatorPosition(const glm::vec3& position, PickFeatureType featureType);
    
    // 高亮管理
    void showHighlight(Geo3D* geometry);
    void hideHighlight();
    void showSelectionHighlight(Geo3D* geometry);
    void hideSelectionHighlight();
    
    // 状态查询
    bool isInitialized() const { return m_initialized; }
    
private:
    // 创建指示器几何体
    osg::ref_ptr<osg::Geometry> createVertexIndicator(float size);
    osg::ref_ptr<osg::Geometry> createEdgeIndicator(float size);
    osg::ref_ptr<osg::Geometry> createFaceIndicator(float size);
    
    // 创建高亮几何体
    osg::ref_ptr<osg::Geometry> createHighlightGeometry(Geo3D* geometry);
    osg::ref_ptr<osg::Geometry> createControlPointHighlightGeometry(Geo3D* geometry);
    
    // 坐标转换
    glm::vec2 worldToScreen(const glm::vec3& worldPos);
    
    // 内部状态
    bool m_initialized = false;
    PickingIndicatorConfig m_config;
    
    // OSG组件
    osg::ref_ptr<osg::Camera> m_camera;
    osg::ref_ptr<osg::Group> m_indicatorRoot;
    
    // 指示器和高亮
    osg::ref_ptr<osg::MatrixTransform> m_indicator;
    osg::ref_ptr<osg::Group> m_highlightNode;
    Geo3D* m_highlightedGeometry = nullptr;
    
    // 指示器几何体缓存
    osg::ref_ptr<osg::Geometry> m_vertexIndicator;
    osg::ref_ptr<osg::Geometry> m_edgeIndicator;
    osg::ref_ptr<osg::Geometry> m_faceIndicator;
};

// 全局指示器管理器（单例）
class GlobalPickingIndicatorManager
{
public:
    static GlobalPickingIndicatorManager& getInstance();
    
    // 委托给内部管理器的方法
    bool initialize(osg::Camera* camera);
    void shutdown();
    void setConfig(const PickingIndicatorConfig& config);
    const PickingIndicatorConfig& getConfig() const;
    osg::Group* getIndicatorRoot() const;
    
    void showIndicator(const glm::vec3& position, PickFeatureType featureType);
    void hideIndicator();
    void updateIndicatorPosition(const glm::vec3& position, PickFeatureType featureType);
    
    void showHighlight(Geo3D* geometry);
    void hideHighlight();
    void showSelectionHighlight(Geo3D* geometry);
    void hideSelectionHighlight();
    
    bool isInitialized() const;

private:
    GlobalPickingIndicatorManager();
    ~GlobalPickingIndicatorManager() = default;
    GlobalPickingIndicatorManager(const GlobalPickingIndicatorManager&) = delete;
    GlobalPickingIndicatorManager& operator=(const GlobalPickingIndicatorManager&) = delete;
    
    std::unique_ptr<PickingIndicatorManager> m_indicatorManager;
}; 