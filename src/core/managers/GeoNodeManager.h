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

// 前向声明
class Geo3D;

class GeoNodeManager : public QObject
{
    Q_OBJECT
public:
    explicit GeoNodeManager(osg::ref_ptr<Geo3D> parent);
    ~GeoNodeManager() = default;

    // ============= 节点访问 =============
    osg::ref_ptr<osg::Group> getOSGNode() const { return m_osgNode; }
    osg::ref_ptr<osg::MatrixTransform> getTransformNode() const { return m_transformNode; }
    
    // ============= 几何体访问 =============
    osg::ref_ptr<osg::Geometry> getVertexGeometry() const { return m_vertexGeometry; }
    osg::ref_ptr<osg::Geometry> getEdgeGeometry() const { return m_edgeGeometry; }
    osg::ref_ptr<osg::Geometry> getFaceGeometry() const { return m_faceGeometry; }
    osg::ref_ptr<osg::Geometry> getControlPointsGeometry() const { return m_controlPointsGeometry; }
    
    // ============= 节点设置 =============
    void setOSGNode(osg::ref_ptr<osg::Node> node);  // 加载外部节点
    
    // ============= 几何体管理 =============
    void clearVertexGeometry();
    void clearEdgeGeometry();
    void clearFaceGeometry();
    void clearControlPointsGeometry();
    void updateGeometries();
    
    // ============= 选中状态管理 =============
    void setSelected(bool selected);  // 自动控制包围盒和控制点显示
    bool isSelected() const { return m_selected; }
    
public slots:
    void onDrawingCompleted();  // 处理绘制完成后的节点设置
    
signals:
    void geometryChanged();     // 用于更新材质和渲染
    void transformChanged();    // 用于更新渲染

private:
    // ============= 初始化 =============
    void initializeNodes();
    
    // ============= 外部节点处理 =============
    void findAndAssignNodeComponents(osg::Node* node);  // 基于标记查找组件
    
    // ============= 空间索引管理 =============
    void updateSpatialIndex();
    void clearSpatialIndex();
    void buildKdTreeForGeometry(osg::Geometry* geometry);
    
    // ============= 包围盒管理 =============
    void updateBoundingBoxGeometry();
    void createBoundingBoxGeometry(const osg::BoundingBox& boundingBox);
    
    // ============= 渲染设置 =============
    void setupControlPointsRendering();
    void setupBoundingBoxRendering();

    // ============= 成员变量 =============
    osg::ref_ptr<Geo3D> m_parent;
    
    // 节点层次结构
    osg::ref_ptr<osg::Group> m_osgNode;
    osg::ref_ptr<osg::MatrixTransform> m_transformNode;
    
    // 几何体节点
    osg::ref_ptr<osg::Geometry> m_vertexGeometry;
    osg::ref_ptr<osg::Geometry> m_edgeGeometry;
    osg::ref_ptr<osg::Geometry> m_faceGeometry;
    osg::ref_ptr<osg::Geometry> m_controlPointsGeometry;
    osg::ref_ptr<osg::Geometry> m_boundingBoxGeometry;
    
    // 状态标志
    bool m_initialized;
    bool m_selected;
}; 

