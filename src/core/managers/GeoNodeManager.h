#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/ref_ptr>
#include <QObject>

// 前向声明
class Geo3D;

/**
 * @brief OSG节点管理器
 * 负责管理几何对象的OSG节点结构，包括主节点、变换节点、点线面节点等
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

private:
    void initializeNodes();
    void setupNodeHierarchy();
    void updateNodeVisibility();
    void createControlPointVisualization(const Point3D& point, float size, const Color3D& color);

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
    
    // 状态
    bool m_initialized;
    bool m_visible;
    bool m_vertexVisible;
    bool m_edgeVisible;
    bool m_faceVisible;
}; 