#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "../GeometryBase.h"
#include <osg/Camera>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/Material>
#include <osg/PolygonMode>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osgGA/GUIEventHandler>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

// 前向声明
class Geo3D;

// 简化的拾取结果
struct SimplePickingResult
{
    bool hasResult = false;
    Geo3D* geometry = nullptr;
    glm::vec3 worldPosition{0.0f};
    glm::vec3 surfaceNormal{0.0f};
    float distance = FLT_MAX;
    int screenX = 0;
    int screenY = 0;
    
    // 特征类型
    enum FeatureType {
        UNKNOWN = 0,
        VERTEX = 1,
        EDGE = 2,
        FACE = 3
    } featureType = UNKNOWN;
    
    // 是否为捕捉点
    bool isSnapped = false;
    glm::vec3 snapPosition{0.0f};
};

// 简化的拾取配置
struct SimplePickingConfig
{
    int pickingRadius = 5;        // 拾取半径（像素）
    float snapThreshold = 0.15f;  // 捕捉阈值（世界坐标）
    bool enableSnapping = true;   // 是否启用捕捉
    bool enableIndicator = true;  // 是否启用指示器
    bool enableHighlight = true;  // 是否启用高亮
    float indicatorSize = 0.2f;   // 指示器大小
    double pickingFrequency = 60.0; // 拾取频率（Hz）
};

// 简化的拾取系统
class SimplifiedPickingSystem : public osg::Referenced
{
public:
    SimplifiedPickingSystem();
    virtual ~SimplifiedPickingSystem();
    
    // 初始化
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void shutdown();
    
    // 配置
    void setConfig(const SimplePickingConfig& config) { m_config = config; }
    const SimplePickingConfig& getConfig() const { return m_config; }
    
    // 相机访问
    osg::Camera* getCamera() const { return m_camera.get(); }
    
    // 几何体管理
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void updateGeometry(Geo3D* geometry);
    void clearAllGeometries();
    
    // 拾取操作
    SimplePickingResult pick(int mouseX, int mouseY);
    
    // 设置回调
    void setPickingCallback(std::function<void(const SimplePickingResult&)> callback);
    
    // 获取根节点（用于添加到场景图）
    osg::Group* getIndicatorRoot() const { return m_indicatorRoot.get(); }
    
    // 状态查询
    bool isInitialized() const { return m_initialized; }
    int getGeometryCount() const { return static_cast<int>(m_geometries.size()); }
    
    // 调试
    void setDebugMode(bool enabled) { m_debugMode = enabled; }
    bool isDebugMode() const { return m_debugMode; }

    // 高亮管理
    void showHighlight(Geo3D* geometry);
    void hideHighlight();
    void showSelectionHighlight(Geo3D* geometry);
    void hideSelectionHighlight();

private:
    // 核心拾取算法
    SimplePickingResult performRayIntersection(int mouseX, int mouseY);
    
    // 捕捉计算
    SimplePickingResult calculateSnapping(const SimplePickingResult& result);
    std::vector<glm::vec3> extractSnapPoints(Geo3D* geometry);
    
    // 指示器管理
    void showIndicator(const SimplePickingResult& result);
    void hideIndicator();
    void updateIndicatorPosition(const glm::vec3& position);
    
    // 坐标转换
    glm::vec3 screenToWorld(int x, int y, float depth = 0.0f);
    glm::vec2 worldToScreen(const glm::vec3& worldPos);
    
    // 几何体工具
    osg::ref_ptr<osg::Geometry> createIndicatorGeometry(float size);
    osg::ref_ptr<osg::Geometry> createHighlightGeometry(Geo3D* geometry);
    osg::ref_ptr<osg::Geometry> createControlPointHighlightGeometry(Geo3D* geometry);
    
    // 内部状态
    bool m_initialized = false;
    bool m_debugMode = false;
    SimplePickingConfig m_config;
    
    // OSG组件
    osg::ref_ptr<osg::Camera> m_camera;
    osg::ref_ptr<osg::Group> m_sceneRoot;
    osg::ref_ptr<osg::Group> m_indicatorRoot;
    
    // 几何体管理
    std::vector<osg::ref_ptr<Geo3D>> m_geometries;
    std::unordered_map<Geo3D*, std::vector<glm::vec3>> m_snapPointsCache;
    
    // 指示器和高亮
    osg::ref_ptr<osg::MatrixTransform> m_indicator;
    osg::ref_ptr<osg::Group> m_highlightNode;
    Geo3D* m_highlightedGeometry = nullptr;
    
    // 回调
    std::function<void(const SimplePickingResult&)> m_pickingCallback;
    
    // 上次结果缓存
    SimplePickingResult m_lastResult;
    double m_lastPickTime = 0.0;
};

// 简化的拾取事件处理器
class SimplifiedPickingEventHandler : public osgGA::GUIEventHandler
{
public:
    SimplifiedPickingEventHandler(SimplifiedPickingSystem* pickingSystem);
    virtual ~SimplifiedPickingEventHandler();
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    void setPickingCallback(std::function<void(const SimplePickingResult&)> callback);

private:
    void processPicking(int x, int y);
    
    osg::ref_ptr<SimplifiedPickingSystem> m_pickingSystem;
    std::function<void(const SimplePickingResult&)> m_pickingCallback;
    
    bool m_enabled = true;
    double m_lastPickTime = 0.0;
    int m_lastX = -1;
    int m_lastY = -1;
};

// 简化的拾取系统管理器（单例）
class SimplifiedPickingSystemManager
{
public:
    static SimplifiedPickingSystemManager& getInstance();
    
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void shutdown();
    
    // 配置管理
    void setConfig(const SimplePickingConfig& config);
    const SimplePickingConfig& getConfig() const;
    
    // 相机访问
    osg::Camera* getCamera() const { return m_pickingSystem->getCamera(); }
    
    // 几何体管理
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void updateGeometry(Geo3D* geometry);
    void clearAllGeometries();
    
    // 拾取操作
    SimplePickingResult pick(int mouseX, int mouseY);
    
    // 选择高亮管理
    void showSelectionHighlight(Geo3D* geometry);
    void hideSelectionHighlight();
    
    // 事件处理器
    osgGA::GUIEventHandler* getEventHandler();
    void setPickingCallback(std::function<void(const SimplePickingResult&)> callback);
    
    // 获取根节点
    osg::Group* getIndicatorRoot();
    
    // 状态查询
    bool isInitialized() const;
    QString getSystemInfo() const;

private:
    SimplifiedPickingSystemManager();
    ~SimplifiedPickingSystemManager();
    
    SimplifiedPickingSystemManager(const SimplifiedPickingSystemManager&) = delete;
    SimplifiedPickingSystemManager& operator=(const SimplifiedPickingSystemManager&) = delete;
    
    osg::ref_ptr<SimplifiedPickingSystem> m_pickingSystem;
    osg::ref_ptr<SimplifiedPickingEventHandler> m_eventHandler;
}; 