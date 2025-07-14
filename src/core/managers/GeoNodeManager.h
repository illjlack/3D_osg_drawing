#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/ref_ptr>
#include <osg/KdTree>
#include <osg/NodeVisitor>
#include <QObject>
#include <vector>
#include <memory>

// 前向声明
class Geo3D;

/**
 * @brief 几何体KDTree节点信息
 */
struct GeoKdTreeNodeInfo
{
    osg::ref_ptr<osg::Drawable> drawable;
    osg::ref_ptr<osg::Node> node;
    Geo3D* geoObject;
    int geometryType; // 0:点, 1:线, 2:面
    int index;
    
    GeoKdTreeNodeInfo() : geoObject(nullptr), geometryType(0), index(-1) {}
};

/**
 * @brief OSG节点管理器
 * 负责管理几何对象的OSG节点结构，包括主节点、变换节点、点线面节点等
 * 使用KDTree进行快速空间查询
 */
class GeoNodeManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoNodeManager(Geo3D* parent);
    ~GeoNodeManager() = default;

    // 节点访问
    osg::ref_ptr<osg::Group> getOSGNode() const { return m_osgNode; }
    osg::ref_ptr<osg::Group> getDrawableGroup() const { return m_drawableGroup; }
    osg::ref_ptr<osg::MatrixTransform> getTransformNode() const { return m_transformNode; }
    osg::ref_ptr<osg::Group> getControlPointsNode() const { return m_controlPointsNode; }

    // 点线面节点访问
    osg::ref_ptr<osg::Group> getVertexNode() const { return m_vertexNode; }
    osg::ref_ptr<osg::Group> getEdgeNode() const { return m_edgeNode; }
    osg::ref_ptr<osg::Group> getFaceNode() const { return m_faceNode; }

    // 节点操作
    void addChild(osg::ref_ptr<osg::Node> child);
    void removeChild(osg::ref_ptr<osg::Node> child);
    void clearChildren();

    // 几何体管理
    void setGeometry(osg::ref_ptr<osg::Geometry> geometry);
    osg::ref_ptr<osg::Geometry> getGeometry() const { return m_geometry; }
    void clearGeometry();

    // 点线面几何体管理
    void addVertexGeometry(osg::Drawable* drawable);
    void addEdgeGeometry(osg::Drawable* drawable);
    void addFaceGeometry(osg::Drawable* drawable);
    
    void clearVertexGeometries();
    void clearEdgeGeometries();
    void clearFaceGeometries();
    void clearAllGeometries();

    // KDTree管理
    void buildKdTree();
    void updateKdTree();
    void clearKdTree();
    
    // 快速查询功能
    std::vector<GeoKdTreeNodeInfo> queryKdTree(const osg::Vec3& point, float radius = 0.1f);
    std::vector<GeoKdTreeNodeInfo> queryKdTreeRay(const osg::Vec3& start, const osg::Vec3& direction, float maxDistance = 1000.0f);
    GeoKdTreeNodeInfo findClosestGeometry(const osg::Vec3& point);
    
    // 几何体可见性查询
    bool isGeometryVisible(int type, int index) const;
    void setGeometryVisible(int type, int index, bool visible);
    std::vector<GeoKdTreeNodeInfo> getVisibleGeometries() const;

    // 变换管理
    void setTransformMatrix(const osg::Matrix& matrix);
    osg::Matrix getTransformMatrix() const;
    void resetTransform();

    // 节点名称管理
    void setupNodeNames();
    void setNodeName(const std::string& name);
    std::string getNodeName() const;

    // 节点可见性
    void setVisible(bool visible);
    bool isVisible() const;
    void setVertexVisible(bool visible);
    void setEdgeVisible(bool visible);
    void setFaceVisible(bool visible);
    bool isVertexVisible() const;
    bool isEdgeVisible() const;
    bool isFaceVisible() const;

    // 节点更新
    void updateNodes();
    void updateControlPointsVisualization();
    void rebuildNodeStructure();

    // 节点查询
    int getChildCount() const;
    bool hasChildren() const;
    bool hasGeometry() const;

    // 节点优化
    void optimizeNodes();
    void compactNodes();

signals:
    void nodeStructureChanged();
    void geometryChanged();
    void transformChanged();
    void visibilityChanged();
    void controlPointsVisibilityChanged();
    void kdTreeUpdated();

private:
    void initializeNodes();
    void setupNodeHierarchy();
    void updateNodeVisibility();
    void createControlPointVisualization(const Point3D& point, float size, const Color3D& color);
    
    // KDTree相关私有方法
    void collectGeometryData();
    bool isGeometryInFrustum(const osg::Vec3& center, float radius) const;
    
public:
    // KDTree相关公共方法
    osg::Vec3 getGeometryCenter(osg::Drawable* drawable) const;

private:
    Geo3D* m_parent;
    
    // OSG节点
    osg::ref_ptr<osg::Group> m_osgNode;
    osg::ref_ptr<osg::Group> m_drawableGroup;
    osg::ref_ptr<osg::MatrixTransform> m_transformNode;
    osg::ref_ptr<osg::Group> m_controlPointsNode;
    
    // 点线面节点
    osg::ref_ptr<osg::Group> m_vertexNode;
    osg::ref_ptr<osg::Group> m_edgeNode;
    osg::ref_ptr<osg::Group> m_faceNode;
    
    // 几何体
    osg::ref_ptr<osg::Geometry> m_geometry;
    
    // KDTree相关
    osg::ref_ptr<osg::KdTree> m_kdTree;
    std::vector<GeoKdTreeNodeInfo> m_geometryInfos;
    std::vector<bool> m_geometryVisibility; // 对应几何体的可见性
    bool m_kdTreeDirty;
    
    // 状态
    bool m_initialized;
    bool m_visible;
    bool m_vertexVisible;
    bool m_edgeVisible;
    bool m_faceVisible;
}; 