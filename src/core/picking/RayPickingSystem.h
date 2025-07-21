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
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osgUtil/RayIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osgGA/GUIEventHandler>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <memory>
#include <functional>

// 前向声明
class Geo3D;

// 拾取特征类型
enum class PickFeatureType {
    NONE = 0,
    VERTEX = 1,    // 顶点
    EDGE = 2,      // 边
    FACE = 3       // 面
};

// 拾取结果
struct PickResult {
    bool hasResult = false;
    Geo3D* geometry = nullptr;
    glm::vec3 worldPosition{0.0f};
    glm::vec3 surfaceNormal{0.0f};
    float distance = FLT_MAX;
    int screenX = 0;
    int screenY = 0;
    
    // 特征信息
    PickFeatureType featureType = PickFeatureType::NONE;
    int primitiveIndex = -1;    // 图元索引（顶点/边/面）
    
    // 捕捉信息
    bool isSnapped = false;
    glm::vec3 snapPosition{0.0f};
};

// 拾取配置
struct PickConfig {
    float pickRadius = 5.0f;        // 拾取半径（像素）
    float vertexPickRadius = 8.0f;  // 顶点拾取半径（像素）
    float edgePickRadius = 3.0f;    // 边拾取半径（像素）- 更小的容差
    float snapThreshold = 0.15f;    // 捕捉阈值（世界坐标）
    bool enableSnapping = true;     // 是否启用捕捉
    bool enableIndicator = true;    // 是否启用指示器
    bool enableHighlight = true;    // 是否启用高亮
    float indicatorSize = 0.2f;     // 指示器大小
    
    // 拾取优先级
    bool pickVertexFirst = true;    // 优先拾取顶点
    bool pickEdgeSecond = true;     // 其次拾取边
    bool pickFaceLast = true;       // 最后拾取面
};

// 拾取系统
class RayPickingSystem : public osg::Referenced
{
public:
    RayPickingSystem();
    virtual ~RayPickingSystem();
    
    // 初始化和关闭
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void shutdown();
    
    // 配置
    void setConfig(const PickConfig& config) { m_config = config; }
    const PickConfig& getConfig() const { return m_config; }
    
    // 几何体管理
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void clearAllGeometries();
    
    // 拾取操作
    PickResult pick(int mouseX, int mouseY);
    
    // 回调设置
    void setPickingCallback(std::function<void(const PickResult&)> callback);
    
    // 指示器管理
    void showIndicator(const glm::vec3& position, PickFeatureType featureType, const glm::vec3& normal = glm::vec3(0,0,1));
    void hideIndicator();
    
    // 获取指示器根节点（用于添加到场景图）
    osg::Group* getIndicatorRoot() const { return m_indicatorRoot.get(); }
    
    // 状态查询
    bool isInitialized() const { return m_initialized; }
    int getGeometryCount() const { return static_cast<int>(m_geometries.size()); }
    
    // 坐标转换（公开访问）
    glm::vec2 worldToScreen(const glm::vec3& worldPos);

private:
    // 拾取算法
    PickResult performRayPicking(int mouseX, int mouseY);
    PickResult pickVertex(const osg::Vec3& start, const osg::Vec3& end, int mouseX, int mouseY);
    PickResult pickEdge(const osg::Vec3& start, const osg::Vec3& end, int mouseX, int mouseY);
    PickResult pickFace(const osg::Vec3& start, const osg::Vec3& end);
    
    // 捕捉计算
    PickResult calculateSnapping(const PickResult& result);
    std::vector<glm::vec3> getGeometrySnapPoints(Geo3D* geometry);
    
    // 几何体匹配
    Geo3D* findGeometryFromNode(osg::Node* node);
    
    // 坐标转换
    osg::Vec3 screenToWorldRay(int screenX, int screenY, float rayLength = 1000.0f);
    float calculateDistanceScale(const glm::vec3& position);
    
    // 几何计算
    bool calculateRayToLineDistance(const osg::Vec3& rayStart, const osg::Vec3& rayDir, 
                                   const glm::vec3& lineStart, const glm::vec3& lineEnd,
                                   float& distance, glm::vec3& closestPoint);
    
    // 屏幕空间计算：计算鼠标在屏幕上到线段的垂点
    bool calculateScreenLineProjection(int mouseX, int mouseY, 
                                     const glm::vec3& lineStart, const glm::vec3& lineEnd,
                                     float& screenDistance, glm::vec3& projectedPoint);
    
    // 指示器创建
    void createIndicators();
    osg::ref_ptr<osg::Geometry> createVertexIndicator();
    osg::ref_ptr<osg::Geometry> createEdgeIndicator();
    osg::ref_ptr<osg::Geometry> createFaceIndicator();
    osg::Matrix calculateOrientationMatrix(const glm::vec3& normal);
    
    // 内部状态
    bool m_initialized = false;
    PickConfig m_config;
    
    // OSG组件
    osg::ref_ptr<osg::Camera> m_camera;
    osg::ref_ptr<osg::Group> m_sceneRoot;
    
    // 几何体管理（使用map简化查找）
    std::map<osg::Node*, Geo3D*> m_nodeToGeometry;
    std::vector<Geo3D*> m_geometries;
    
    // 回调
    std::function<void(const PickResult&)> m_pickingCallback;
    
    // 指示器
    osg::ref_ptr<osg::Group> m_indicatorRoot;
    osg::ref_ptr<osg::MatrixTransform> m_currentIndicator;
    
    // 预创建的指示器几何体
    osg::ref_ptr<osg::Geometry> m_vertexIndicator;
    osg::ref_ptr<osg::Geometry> m_edgeIndicator;
    osg::ref_ptr<osg::Geometry> m_faceIndicator;
    
    // 当前显示的指示器节点
    osg::ref_ptr<osg::Geode> m_currentIndicatorGeode;
};

// 拾取事件处理器
class PickingEventHandler : public osgGA::GUIEventHandler
{
public:
    PickingEventHandler(RayPickingSystem* pickingSystem);
    virtual ~PickingEventHandler() = default;
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // 吸附相关方法
    bool hasSnapPosition() const { return m_hasSnapPosition; }
    int getSnapScreenX() const { return m_snapScreenX; }
    int getSnapScreenY() const { return m_snapScreenY; }

private:
    osg::ref_ptr<RayPickingSystem> m_pickingSystem;
    bool m_enabled = true;
    int m_lastX = -1;
    int m_lastY = -1;
    
    // 吸附位置信息
    bool m_hasSnapPosition = false;
    int m_snapScreenX = 0;
    int m_snapScreenY = 0;
};

// 拾取系统管理器（单例，简化版）
class PickingSystemManager
{
public:
    static PickingSystemManager& getInstance();
    
    // 初始化和关闭
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void shutdown();
    
    // 委托到拾取系统的方法
    void setConfig(const PickConfig& config);
    const PickConfig& getConfig() const;
    void addGeometry(Geo3D* geometry);
    void removeGeometry(Geo3D* geometry);
    void clearAllGeometries();
    PickResult pick(int mouseX, int mouseY);
    void setPickingCallback(std::function<void(const PickResult&)> callback);
    
    // 获取组件
    osg::Group* getIndicatorRoot() const;
    osgGA::GUIEventHandler* getEventHandler() const;
    
    // 状态查询
    bool isInitialized() const;

private:
    PickingSystemManager() = default;
    ~PickingSystemManager() = default;
    PickingSystemManager(const PickingSystemManager&) = delete;
    PickingSystemManager& operator=(const PickingSystemManager&) = delete;
    
    osg::ref_ptr<RayPickingSystem> m_pickingSystem;
    osg::ref_ptr<PickingEventHandler> m_eventHandler;
}; 