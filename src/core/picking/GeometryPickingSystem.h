#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include "PickingTypes.h"
#include <osg/Camera>
#include <osg/Group>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/PolytopeIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osgGA/GUIEventHandler>
#include <glm/glm.hpp>
#include <vector>
#include <functional>

// 前向声明
class Geo3D;

// 简化的拾取配置
struct PickConfig {
    double cylinderRadius = 10.0;     // 圆柱形拾取半径（像素）
    bool enableVertexPicking = true;  // 启用顶点拾取
    bool enableEdgePicking = true;    // 启用边拾取  
    bool enableFacePicking = true;    // 启用面拾取
};

// 单个拾取结果存储结构（每次只有一个最近的结果）
struct SinglePickingResults {
    bool hasFaceResult = false;
    bool hasVertexResult = false;
    bool hasEdgeResult = false;
    
    osgUtil::LineSegmentIntersector::Intersection faceIntersection;
    osgUtil::PolytopeIntersector::Intersection vertexIntersection;
    osgUtil::PolytopeIntersector::Intersection edgeIntersection;
    
    void clear() {
        hasFaceResult = false;
        hasVertexResult = false;
        hasEdgeResult = false;
    }
};

// 拾取系统 - 使用射线和几何拾取器
class GeometryPickingSystem : public osg::Referenced
{
public:
    GeometryPickingSystem();
    virtual ~GeometryPickingSystem();
    
    // 初始化和关闭
    bool initialize(osg::Camera* camera, osg::Group* sceneRoot);
    void shutdown();
    
    // 配置
    void setConfig(const PickConfig& config) { m_config = config; }
    const PickConfig& getConfig() const { return m_config; }
    
    // 主要拾取接口 - 使用IntersectorGroup进行混合拾取
    PickResult pickGeometry(int mouseX, int mouseY);
    
    // 回调设置
    void setPickingCallback(std::function<void(const PickResult&)> callback);
    
    // 状态查询
    bool isInitialized() const { return m_initialized; }
    
    // 坐标转换（公开访问）
    glm::dvec2 worldToScreen(const glm::dvec3& worldPos);

private:
    // 选择最佳的单个结果 - 比较距离和优先级
    PickResult selectBestSingleResult();
    
    // 分析单个面拾取交点
    PickResult analyzeFaceIntersection(const osgUtil::LineSegmentIntersector::Intersection& intersection);
    
    // 分析单个顶点/边拾取交点
    PickResult analyzePolytopeIntersection(const osgUtil::PolytopeIntersector::Intersection& intersection, PickFeatureType featureType);
    
    // 几何体匹配 - 通过遍历节点路径查找
    Geo3D* findGeometryFromNodePath(const osg::NodePath& nodePath);
    
    // NodeMask 获取
    unsigned int getPickingMask() const;
    
    // 内部状态
    bool m_initialized = false;
    PickConfig m_config;
    
    // OSG组件
    osg::ref_ptr<osg::Camera> m_camera;
    osg::ref_ptr<osg::Group> m_sceneRoot;
    
    // 回调
    std::function<void(const PickResult&)> m_pickingCallback;
    
    // 单个拾取结果存储
    SinglePickingResults m_singleResults;
};





