#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "../GeometryBase.h"
#include "PickingIndicator.h"
#include <osg/Camera>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Node>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osgGA/GUIEventHandler>
#include <osg/KdTree>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

// 前向声明
class Geo3D;
class PickingIndicatorManager;
class HighlightSystem;

// CPU拾取配置
struct CPUPickingConfig
{
    int pickingRadius = 5;          // 拾取半径（像素）
    int rayCount = 8;               // 射线数量
    float snapThreshold = 0.1f;     // 捕捉阈值（世界坐标）
    bool enableSnapping = true;     // 是否启用捕捉
    bool enableIndicator = true;    // 是否启用指示器
    bool enableHighlight = true;    // 是否启用高亮
    float indicatorSize = 0.2f;     // 指示器大小
    
    CPUPickingConfig() = default;
};

// 拾取特征类型
enum class PickingFeatureType
{
    VERTEX = 0,
    EDGE = 1,
    FACE = 2,
    NONE = 3
};

// 拾取结果
struct CPUPickingResult
{
    bool hasResult = false;
    Geo3D* geometry = nullptr;
    PickingFeatureType featureType = PickingFeatureType::NONE;
    glm::vec3 worldPosition{0.0f};
    glm::vec3 snapPosition{0.0f};    // 捕捉位置
    bool isSnapped = false;          // 是否发生了捕捉
    float distance = FLT_MAX;        // 距离相机的距离
    int screenX = 0;
    int screenY = 0;
    
    CPUPickingResult() = default;
};

// 射线信息
struct PickingRay
{
    glm::vec3 origin;
    glm::vec3 direction;
    int screenX;
    int screenY;
    
    PickingRay(const glm::vec3& orig, const glm::vec3& dir, int x, int y)
        : origin(orig), direction(dir), screenX(x), screenY(y) {}
};

// 捕捉点信息（直接存储在Geo3D中）
struct SnapPoint
{
    glm::vec3 position;
    PickingFeatureType type;
    float priority;  // 优先级，数值越小优先级越高
    
    SnapPoint(const glm::vec3& pos, PickingFeatureType t, float p = 0.0f)
        : position(pos), type(t), priority(p) {}
};

// 几何体拾取数据（简化版，使用OSG内置功能）
struct GeometryPickingData
{
    osg::ref_ptr<Geo3D> geometry;
    std::vector<SnapPoint> snapPoints;
    osg::BoundingSphere boundingSphere;
    osg::ref_ptr<osg::KdTree> kdTree;  // 使用OSG的KdTree
    
    GeometryPickingData(Geo3D* geo) : geometry(geo) {}
};

// CPU拾取系统主类
class CPUPickingSystem : public osg::Referenced
{
public:
    CPUPickingSystem();
    virtual ~CPUPickingSystem();
    
    // 初始化
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void setConfig(const CPUPickingConfig& config);
    const CPUPickingConfig& getConfig() const { return m_config; }
    
    // 对象管理
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void updateGeometry(Geo3D* geometry);
    void clearAllGeometries();
    
    // 拾取操作
    CPUPickingResult pick(int mouseX, int mouseY);
    
    // 配置接口
    void setPickingRadius(int radius) { m_config.pickingRadius = radius; }
    void setRayCount(int count) { m_config.rayCount = count; }
    void setSnapThreshold(float threshold) { m_config.snapThreshold = threshold; }
    void setEnableSnapping(bool enable) { m_config.enableSnapping = enable; }
    void setEnableIndicator(bool enable) { m_config.enableIndicator = enable; }
    void setEnableHighlight(bool enable) { m_config.enableHighlight = enable; }
    
    // 指示器和高亮系统
    void setIndicatorManager(PickingIndicatorManager* manager) { m_indicatorManager = manager; }
    void setHighlightSystem(HighlightSystem* system) { m_highlightSystem = system; }
    
private:
    // 射线生成（改进版）
    std::vector<PickingRay> generateRays(int mouseX, int mouseY);
    glm::vec3 screenToWorld(int x, int y, float depth = 0.0f);
    glm::vec2 worldToScreen(const glm::vec3& worldPos); // 添加世界到屏幕坐标转换
    
    // 相交检测（改进版）
    std::vector<osgUtil::LineSegmentIntersector::Intersection> 
        performRayIntersection(const std::vector<PickingRay>& rays);
    
    // 候选筛选（改进版）
    std::vector<CPUPickingResult> processCandidates(
        const std::vector<osgUtil::LineSegmentIntersector::Intersection>& intersections);
    
    // 捕捉计算（使用OSG KdTree）
    CPUPickingResult calculateSnapping(const CPUPickingResult& candidate);
    
    // 几何体数据管理
    void buildGeometryData(Geo3D* geometry);
    void updateSnapPoints(GeometryPickingData& data);
    
    // 结果选择
    CPUPickingResult selectBestResult(const std::vector<CPUPickingResult>& candidates);
    
    // 结果处理
    void processPickingResult(const CPUPickingResult& result);
    
    // 辅助函数
    Geo3D* findGeometryFromNode(osg::Node* node);
    PickingFeatureType determineFeatureType(osg::Node* node);
    
    // 指示器和高亮
    void showIndicator(const CPUPickingResult& result);
    void hideIndicator();
    void highlightGeometry(Geo3D* geometry);
    void clearHighlight();
    
    // OSG内置拾取功能
    void setupIntersectorSettings(osgUtil::LineSegmentIntersector* intersector);
    
private:
    CPUPickingConfig m_config;
    osg::ref_ptr<osg::Camera> m_camera;
    osg::ref_ptr<osg::Group> m_sceneRoot;
    
    // 几何体数据
    std::unordered_map<Geo3D*, std::unique_ptr<GeometryPickingData>> m_geometryData;
    
    // 指示器和高亮系统
    osg::ref_ptr<PickingIndicatorManager> m_indicatorManager;
    osg::ref_ptr<HighlightSystem> m_highlightSystem;
    
    // 统计信息
    int m_totalSnapPoints = 0;
    double m_lastPickTime = 0.0;
    
    // 是否已初始化
    bool m_initialized = false;
    
    // 上次拾取结果（用于指示器管理）
    CPUPickingResult m_lastResult;
};

// CPU拾取事件处理器
class CPUPickingEventHandler : public osgGA::GUIEventHandler
{
public:
    CPUPickingEventHandler(CPUPickingSystem* pickingSystem);
    virtual ~CPUPickingEventHandler();
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;
    
    // 回调设置
    void setPickingCallback(std::function<void(const CPUPickingResult&)> callback);
    
    // 配置
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    void setPickingFrequency(float frequency) { m_pickingFrequency = frequency; }
    
private:
    void processPicking(int x, int y);
    
private:
    osg::ref_ptr<CPUPickingSystem> m_pickingSystem;
    std::function<void(const CPUPickingResult&)> m_pickingCallback;
    
    bool m_enabled = true;
    float m_pickingFrequency = 60.0f;  // Hz
    double m_lastPickTime = 0.0;
    int m_lastX = -1;
    int m_lastY = -1;
};

// CPU拾取系统管理器（单例）
class CPUPickingSystemManager
{
public:
    static CPUPickingSystemManager& getInstance();
    
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void setConfig(const CPUPickingConfig& config);
    
    // 便捷接口
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void updateGeometry(Geo3D* geometry);
    CPUPickingResult pick(int mouseX, int mouseY);
    
    // 事件处理器
    CPUPickingEventHandler* getEventHandler() { return m_eventHandler.get(); }
    
    // 切换拾取系统
    void enableCPUPicking(bool enable) { m_cpuPickingEnabled = enable; }
    bool isCPUPickingEnabled() const { return m_cpuPickingEnabled; }
    
    // 指示器系统集成
    void initializeIndicatorSystem();
    
private:
    CPUPickingSystemManager();
    ~CPUPickingSystemManager();
    
    CPUPickingSystemManager(const CPUPickingSystemManager&) = delete;
    CPUPickingSystemManager& operator=(const CPUPickingSystemManager&) = delete;
    
    osg::ref_ptr<CPUPickingSystem> m_pickingSystem;
    osg::ref_ptr<CPUPickingEventHandler> m_eventHandler;
    bool m_cpuPickingEnabled = false;
}; 