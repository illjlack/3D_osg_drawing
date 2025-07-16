#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/Group>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/ref_ptr>
#include <osg/KdTree>
#include <QObject>

class Geo3D;

class GeoNodeManager : public QObject
{
    Q_OBJECT
public:
    explicit GeoNodeManager(Geo3D* parent);
    ~GeoNodeManager() = default;

    // 节点访问
    osg::ref_ptr<osg::Group> getOSGNode() const { return m_osgNode; }
    osg::ref_ptr<osg::MatrixTransform> getTransformNode() const { return m_transformNode; }

    // 几何体访问
    osg::ref_ptr<osg::Geometry> getVertexGeometry() const { return m_vertexGeometry; }
    osg::ref_ptr<osg::Geometry> getEdgeGeometry() const { return m_edgeGeometry; }
    osg::ref_ptr<osg::Geometry> getFaceGeometry() const { return m_faceGeometry; }
    osg::ref_ptr<osg::Geometry> getControlPointsGeometry() const { return m_controlPointsGeometry; }

    // 几何体清理
    void clearVertexGeometry();
    void clearEdgeGeometry();
    void clearFaceGeometry();
    void clearControlPointsGeometry();
    void clearAllGeometries();

    // 变换管理
    void setTransformMatrix(const osg::Matrix& matrix);
    osg::Matrix getTransformMatrix() const;
    void resetTransform();

    // 可见性管理 - 直接使用几何体节点的可见性
    void setVisible(bool visible);
    bool isVisible() const;
    void setVertexVisible(bool visible);
    void setEdgeVisible(bool visible);
    void setFaceVisible(bool visible);
    void setControlPointsVisible(bool visible);
    bool isVertexVisible() const;
    bool isEdgeVisible() const;
    bool isFaceVisible() const;
    bool isControlPointsVisible() const;

    // 空间索引管理 - 直接在几何体上构建KdTree
    void updateSpatialIndex();
    void clearSpatialIndex();

signals:
    void geometryChanged();
    void transformChanged();
    void visibilityChanged();

private:
    void initializeNodes();
    void setupNodeHierarchy();
    void buildKdTreeForGeometry(osg::Geometry* geometry);

    Geo3D* m_parent;
    
    // 节点层次结构
    osg::ref_ptr<osg::Group> m_osgNode;
    osg::ref_ptr<osg::MatrixTransform> m_transformNode;
    
    // 几何体节点
    osg::ref_ptr<osg::Geometry> m_vertexGeometry;
    osg::ref_ptr<osg::Geometry> m_edgeGeometry;
    osg::ref_ptr<osg::Geometry> m_faceGeometry;
    osg::ref_ptr<osg::Geometry> m_controlPointsGeometry;
    
    bool m_initialized;
}; 