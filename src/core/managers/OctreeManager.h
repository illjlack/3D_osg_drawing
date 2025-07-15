#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ref_ptr>
#include <osg/KdTree>
#include <osg/NodeVisitor>
#include <QObject>
#include <vector>
#include <memory>
#include <limits>

// 前向声明
class Geo3D;

/**
 * @brief 几何体八叉树节点信息
 */
struct GeoOctreeNodeInfo
{
    osg::ref_ptr<osg::Drawable> drawable;
    osg::ref_ptr<osg::Node> node;
    Geo3D* geoObject;
    int geometryType; // 0:点, 1:线, 2:面
    int index;
    osg::BoundingBox boundingBox;
    
    GeoOctreeNodeInfo() : geoObject(nullptr), geometryType(0), index(-1) {}
};

/**
 * @brief 八叉树节点
 */
struct OctreeNode
{
    osg::BoundingBox bounds;
    std::vector<GeoOctreeNodeInfo> geometries;
    std::vector<std::unique_ptr<OctreeNode>> children;
    bool isLeaf;
    int maxGeometries;
    int maxDepth;
    int currentDepth;
    
    OctreeNode(const osg::BoundingBox& bb, int maxGeo = 8, int maxD = 8, int depth = 0)
        : bounds(bb), isLeaf(true), maxGeometries(maxGeo), maxDepth(maxD), currentDepth(depth) {}
};

/**
 * @brief 八叉树管理器
 * 负责管理几何对象的八叉树结构，提供快速空间查询功能
 */
class OctreeManager : public QObject
{
    Q_OBJECT

public:
    explicit OctreeManager(Geo3D* parent);
    ~OctreeManager() = default;

    // 八叉树构建和管理
    void buildOctree();
    void updateOctree();
    void clearOctree();
    void rebuildOctree();
    
    // 几何体数据收集
    void collectGeometryData();
    void addGeometryData(const GeoOctreeNodeInfo& info);
    void clearGeometryData();
    
    // 快速查询功能
    std::vector<GeoOctreeNodeInfo> queryOctree(const osg::Vec3& point, float radius = 0.1f);
    std::vector<GeoOctreeNodeInfo> queryOctreeRay(const osg::Vec3& start, const osg::Vec3& direction, float maxDistance = 1000.0f);
    std::vector<GeoOctreeNodeInfo> queryOctreeBox(const osg::BoundingBox& box);
    GeoOctreeNodeInfo findClosestGeometry(const osg::Vec3& point);
    
    // 几何体可见性管理
    void setGeometryVisible(int index, bool visible);
    bool isGeometryVisible(int index) const;
    std::vector<GeoOctreeNodeInfo> getVisibleGeometries() const;
    
    // 八叉树参数设置
    void setMaxGeometriesPerNode(int maxGeo);
    void setMaxDepth(int maxDepth);
    int getMaxGeometriesPerNode() const { return m_maxGeometriesPerNode; }
    int getMaxDepth() const { return m_maxDepth; }
    
    // 统计信息
    int getOctreeDepth() const;
    int getTotalNodes() const;
    int getLeafNodes() const;
    int getTotalGeometries() const;
    
    // 调试和可视化
    void printOctreeStats() const;
    osg::ref_ptr<osg::Node> createOctreeVisualization() const;

signals:
    void octreeUpdated();
    void geometryDataChanged();
    void visibilityChanged();

public:
    // 几何体中心计算
    osg::Vec3 getGeometryCenter(osg::Drawable* drawable) const;
    osg::BoundingBox getGeometryBoundingBox(osg::Drawable* drawable) const;

private:
    // 八叉树构建相关
    void buildOctreeRecursive(std::unique_ptr<OctreeNode>& node, const std::vector<GeoOctreeNodeInfo>& geometries);
    void splitNode(std::unique_ptr<OctreeNode>& node, const std::vector<GeoOctreeNodeInfo>& geometries);
    std::vector<osg::BoundingBox> calculateChildBounds(const osg::BoundingBox& parentBounds);
    
    // 查询相关
    void queryOctreeRecursive(const OctreeNode* node, const osg::Vec3& point, float radius, std::vector<GeoOctreeNodeInfo>& results) const;
    void queryOctreeRayRecursive(const OctreeNode* node, const osg::Vec3& start, const osg::Vec3& direction, float maxDistance, std::vector<GeoOctreeNodeInfo>& results) const;
    void queryOctreeBoxRecursive(const OctreeNode* node, const osg::BoundingBox& box, std::vector<GeoOctreeNodeInfo>& results) const;
    
    // 辅助函数
    bool isPointInBox(const osg::Vec3& point, const osg::BoundingBox& box) const;
    bool isBoxIntersectBox(const osg::BoundingBox& box1, const osg::BoundingBox& box2) const;
    bool isRayIntersectBox(const osg::Vec3& start, const osg::Vec3& direction, const osg::BoundingBox& box) const;
    float calculateDistance(const osg::Vec3& point1, const osg::Vec3& point2) const;
    
    // 统计信息计算
    void calculateOctreeStats(const OctreeNode* node, int& totalNodes, int& leafNodes) const;
    
    // 可视化相关
    osg::ref_ptr<osg::Node> createNodeVisualization(const OctreeNode* node) const;
    void createNodeVisualization(const OctreeNode* node, osg::Group* parent) const;

private:
    Geo3D* m_parent;
    
    // 八叉树根节点
    std::unique_ptr<OctreeNode> m_octreeRoot;
    
    // 几何体数据
    std::vector<GeoOctreeNodeInfo> m_geometryInfos;
    std::vector<bool> m_geometryVisibility;
    
    // 八叉树参数
    int m_maxGeometriesPerNode;
    int m_maxDepth;
    bool m_octreeDirty;
    
    // 统计信息
    mutable int m_cachedTotalNodes;
    mutable int m_cachedLeafNodes;
    mutable bool m_statsDirty;
}; 