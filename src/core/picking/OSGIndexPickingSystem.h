#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "../GeometryBase.h"
#include "PickingIndicator.h"
#include <osg/Camera>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/Point>
#include <osg/Shape>
#include <osg/ShapeDrawable>
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

// 拾取结果
struct OSGIndexPickResult {
    bool hasResult = false;
    Geo3D* geometry = nullptr;
    glm::vec3 worldPosition{0.0f};
    glm::vec3 surfaceNormal{0.0f};
    float distance = FLT_MAX;
    int screenX = 0;
    int screenY = 0;
    
    // 特征信息
    PickFeatureType featureType = PickFeatureType::NONE;
    int vertexIndex = -1;      // 顶点索引
    int edgeIndex = -1;        // 边索引
    int faceIndex = -1;        // 面索引
    
    // 是否为捕捉点
    bool isSnapped = false;
    glm::vec3 snapPosition{0.0f};
    
    // 指示器信息
    glm::vec3 indicatorPosition{0.0f};
    float indicatorSize = 0.2f;
};

// 拾取配置
struct OSGIndexPickConfig {
    int pickingRadius = 5;        // 拾取半径（像素）
    float snapThreshold = 0.15f;  // 捕捉阈值（世界坐标）
    bool enableSnapping = true;   // 是否启用捕捉
    bool enableIndicator = true;  // 是否启用指示器
    bool enableHighlight = true;  // 是否启用高亮
    float indicatorSize = 0.2f;   // 指示器大小
    double pickingFrequency = 60.0; // 拾取频率（Hz）
    
    // 拾取优先级
    bool pickVertexFirst = true;  // 优先拾取顶点
    bool pickEdgeSecond = true;   // 其次拾取边
    bool pickFaceLast = true;     // 最后拾取面
};

// 基于OSG索引的拾取系统
class OSGIndexPickingSystem : public osg::Referenced
{
public:
    OSGIndexPickingSystem();
    virtual ~OSGIndexPickingSystem();
    
    // 初始化
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void shutdown();
    
    // 配置
    void setConfig(const OSGIndexPickConfig& config) { m_config = config; }
    const OSGIndexPickConfig& getConfig() const { return m_config; }
    
    // 相机访问
    osg::Camera* getCamera() const { return m_camera.get(); }
    
    // 几何体管理
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void updateGeometry(Geo3D* geometry);
    void clearAllGeometries();
    
    // 拾取操作
    OSGIndexPickResult pick(int mouseX, int mouseY);
    
    // 设置回调
    void setPickingCallback(std::function<void(const OSGIndexPickResult&)> callback);
    
    // 获取根节点（用于添加到场景图） - 现在委托给指示器管理器
    osg::Group* getIndicatorRoot() const;
    
    // 状态查询
    bool isInitialized() const { return m_initialized; }
    int getGeometryCount() const { return static_cast<int>(m_geometries.size()); }
    
    // 调试
    void setDebugMode(bool enabled) { m_debugMode = enabled; }
    bool isDebugMode() const { return m_debugMode; }
    
    // 获取几何体列表和最后结果
    const std::vector<osg::ref_ptr<Geo3D>>& getGeometries() const { return m_geometries; }
    OSGIndexPickResult getLastResult() const { return m_lastResult; }

    // 高亮管理 - 委托给指示器管理器
    void showHighlight(Geo3D* geometry);
    void hideHighlight();
    void showSelectionHighlight(Geo3D* geometry);
    void hideSelectionHighlight();

private:
    // 核心拾取算法
    OSGIndexPickResult performOSGIndexPicking(int mouseX, int mouseY);
    
    // 按特征类型拾取
    OSGIndexPickResult pickVertex(int mouseX, int mouseY);
    OSGIndexPickResult pickEdge(int mouseX, int mouseY);
    OSGIndexPickResult pickFace(int mouseX, int mouseY);
    
    // 捕捉计算
    OSGIndexPickResult calculateSnapping(const OSGIndexPickResult& result);
    std::vector<glm::vec3> extractSnapPoints(Geo3D* geometry);
    
    // 几何体匹配
    Geo3D* findGeometryFromIntersection(const osgUtil::LineSegmentIntersector::Intersection& intersection);
    
    // 坐标转换
    glm::vec2 worldToScreen(const glm::vec3& worldPos);
    glm::vec3 screenToWorld(int screenX, int screenY, float depth = 0.0f);
    
    // 缓存机制辅助方法
    bool isCacheValid(int mouseX, int mouseY);
    void updateCameraState();
    void invalidateCache();
    void markSceneChanged();
    
    // 内部状态
    bool m_initialized = false;
    bool m_debugMode = false;
    OSGIndexPickConfig m_config;
    
    // OSG组件
    osg::ref_ptr<osg::Camera> m_camera;
    osg::ref_ptr<osg::Group> m_sceneRoot;
    
    // 几何体管理
    std::vector<osg::ref_ptr<Geo3D>> m_geometries;
    std::unordered_map<Geo3D*, std::vector<glm::vec3>> m_snapPointsCache;
    
    // 回调
    std::function<void(const OSGIndexPickResult&)> m_pickingCallback;
    
    // 上次结果缓存
    OSGIndexPickResult m_lastResult;
    double m_lastPickTime = 0.0;
    
    // 增强缓存机制 - 相机状态和鼠标位置缓存
    int m_lastMouseX = -1;
    int m_lastMouseY = -1;
    osg::Matrix m_lastViewMatrix;
    osg::Matrix m_lastProjectionMatrix;
    osg::Vec4 m_lastViewport;
    bool m_cameraStateValid = false;
    
    // 场景内容变化标记
    int m_sceneVersionNumber = 0;
    int m_lastSceneVersion = -1;
};

// 拾取事件处理器
class OSGIndexPickingEventHandler : public osgGA::GUIEventHandler
{
public:
    OSGIndexPickingEventHandler(OSGIndexPickingSystem* pickingSystem);
    virtual ~OSGIndexPickingEventHandler();
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    void setPickingCallback(std::function<void(const OSGIndexPickResult&)> callback);

private:
    void processPicking(int x, int y);
    
    osg::ref_ptr<OSGIndexPickingSystem> m_pickingSystem;
    std::function<void(const OSGIndexPickResult&)> m_pickingCallback;
    
    bool m_enabled = true;
    double m_lastPickTime = 0.0;
    int m_lastX = -1;
    int m_lastY = -1;
};

// 拾取系统管理器（单例）
class OSGIndexPickingSystemManager
{
public:
    static OSGIndexPickingSystemManager& getInstance();
    
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void shutdown();
    
    // 配置管理
    void setConfig(const OSGIndexPickConfig& config);
    const OSGIndexPickConfig& getConfig() const;
    
    // 相机访问
    osg::Camera* getCamera() const { return m_pickingSystem->getCamera(); }
    
    // 几何体管理
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void updateGeometry(Geo3D* geometry);
    void clearAllGeometries();
    
    // 拾取操作
    OSGIndexPickResult pick(int mouseX, int mouseY);
    
    // 高亮管理
    void showSelectionHighlight(Geo3D* geometry);
    void hideSelectionHighlight();
    
    // 回调设置
    void setPickingCallback(std::function<void(const OSGIndexPickResult&)> callback);
    
    // 状态查询
    bool isInitialized() const;
    QString getSystemInfo() const;
    
    // 事件处理器
    osgGA::GUIEventHandler* getEventHandler() const { return m_eventHandler.get(); }
    osg::Group* getIndicatorRoot() const { return m_pickingSystem->getIndicatorRoot(); }

private:
    OSGIndexPickingSystemManager();
    ~OSGIndexPickingSystemManager() = default;
    OSGIndexPickingSystemManager(const OSGIndexPickingSystemManager&) = delete;
    OSGIndexPickingSystemManager& operator=(const OSGIndexPickingSystemManager&) = delete;
    
    osg::ref_ptr<OSGIndexPickingSystem> m_pickingSystem;
    osg::ref_ptr<OSGIndexPickingEventHandler> m_eventHandler;
}; 
