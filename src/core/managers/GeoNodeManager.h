#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <osg/Group>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/ref_ptr>
#include <osg/KdTree>
#include <osg/BoundingBox>
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
    // 控制点，只是用来可视化和操作
    osg::ref_ptr<osg::Geometry> getControlPointsGeometry() const { return m_controlPointsGeometry; }
    // 包围盒，用来可视化，是点、线、面的包围盒对应的立方体
    osg::ref_ptr<osg::Geometry> getBoundingBoxGeometry() const { return m_boundingBoxGeometry; }

    // 几何体清理
    void clearVertexGeometry();
    void clearEdgeGeometry();
    void clearFaceGeometry();
    void clearControlPointsGeometry();
    void clearBoundingBoxGeometry();
    void clearAllGeometries();

    // 变换管理
    void setTransformMatrix(const osg::Matrix& matrix);
    osg::Matrix getTransformMatrix() const;
    void resetTransform();

    // 可见性管理
    void setVisible(bool visible);
    bool isVisible() const;
    void setVertexVisible(bool visible);
    void setEdgeVisible(bool visible);
    void setFaceVisible(bool visible);
    void setControlPointsVisible(bool visible);
    void setBoundingBoxVisible(bool visible);
    bool isVertexVisible() const;
    bool isEdgeVisible() const;
    bool isFaceVisible() const;
    bool isControlPointsVisible() const;
    bool isBoundingBoxVisible() const;

    // 更新节点和包围盒、kdtree
    void updateGeometries();
    
    // 更新包围盒可见性状态
    void updateBoundingBoxVisibility();
signals:
    // 用于更新材质和渲染
    void geometryChanged();

    // 用于更新渲染
    void transformChanged();

private:
    void initializeNodes();
    void buildKdTreeForGeometry(osg::Geometry* geometry);
    void createBoundingBoxGeometry(const osg::BoundingBox& boundingBox);

    // 空间索引管理
    void updateSpatialIndex();
    void clearSpatialIndex();
    // 包围盒更新
    void updateBoundingBoxGeometry();

    Geo3D* m_parent;
    
    // 节点层次结构
    osg::ref_ptr<osg::Group> m_osgNode;
    osg::ref_ptr<osg::MatrixTransform> m_transformNode;
    
    // 几何体节点
    osg::ref_ptr<osg::Geometry> m_vertexGeometry;
    osg::ref_ptr<osg::Geometry> m_edgeGeometry;
    osg::ref_ptr<osg::Geometry> m_faceGeometry;
    osg::ref_ptr<osg::Geometry> m_controlPointsGeometry;
    osg::ref_ptr<osg::Geometry> m_boundingBoxGeometry;
    
    bool m_initialized;
}; 