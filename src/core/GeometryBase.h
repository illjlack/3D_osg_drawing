#pragma once
#pragma execution_character_set("utf-8")

#include "Common3D.h"
#include "managers/GeoStateManager.h"
#include "managers/GeoNodeManager.h"
#include "managers/GeoMaterialManager.h"
#include "managers/GeoSnapPointManager.h"
#include "managers/GeoControlPointManager.h"
#include "managers/GeoBoundingBoxManager.h"
#include "managers/GeoRenderManager.h"
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Drawable>
#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QObject>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <algorithm>

// 前向声明
class Geo3D;

// 几何对象工厂
Geo3D* createGeo3D(DrawMode3D mode);

// 三维几何对象基类
class Geo3D : public QObject, public osg::Referenced
{
    Q_OBJECT

public:
    Geo3D();
    virtual ~Geo3D();

    // 类型相关
    GeoType3D getGeoType() const { return m_geoType; }
    void setGeoType(GeoType3D type) { m_geoType = type; }

    // 状态管理（通过状态管理器）
    GeoStateManager* getStateManager() const { return m_stateManager.get(); }
    
    // 简化的状态访问接口
    bool isStateInitialized() const;
    bool isStateComplete() const;
    bool isStateInvalid() const;
    bool isStateSelected() const;
    bool isStateEditing() const;
    
    void setStateInitialized();
    void setStateComplete();
    void setStateInvalid();
    void setStateSelected();
    void setStateEditing();
    
    void clearStateComplete();
    void clearStateInvalid();
    void clearStateSelected();
    void clearStateEditing();

    // 参数管理
    const GeoParameters3D& getParameters() const { return m_parameters; }
    void setParameters(const GeoParameters3D& params);

    // 控制点管理（通过控制点管理器）
    GeoControlPointManager* getControlPointManager() const { return m_controlPointManager.get(); }
    
    // 简化的控制点访问接口
    const std::vector<Point3D>& getControlPoints() const;
    void addControlPoint(const Point3D& point);
    void setControlPoint(int index, const Point3D& point);
    void removeControlPoint(int index);
    void clearControlPoints();
    bool hasControlPoints() const;
    
    // 临时点（用于绘制过程中的预览）
    const Point3D& getTempPoint() const { return m_tempPoint; }
    void setTempPoint(const Point3D& point) { m_tempPoint = point; }

    // 变换
    const Transform3D& getTransform() const { return m_transform; }
    void setTransform(const Transform3D& transform);

    // 包围盒管理（通过包围盒管理器）
    GeoBoundingBoxManager* getBoundingBoxManager() const { return m_boundingBoxManager.get(); }
    const BoundingBox3D& getBoundingBox() const;
    
    // 几何体属性方法
    glm::vec3 getCenter() const;
    glm::vec3 getPosition() const;
    glm::vec3 getStartPoint() const;
    glm::vec3 getEndPoint() const;
    glm::vec3 getVertex(int index) const;
    float getRadius() const;
    float getHeight() const;
    float getStartAngle() const;
    float getEndAngle() const;
    float getMajorRadius() const;
    float getMinorRadius() const;

    // 节点管理（通过节点管理器）
    GeoNodeManager* getNodeManager() const { return m_nodeManager.get(); }
    
    // 简化的节点访问接口
    osg::ref_ptr<osg::Group> getOSGNode() const;
    osg::ref_ptr<osg::Group> getVertexNode() const;
    osg::ref_ptr<osg::Group> getEdgeNode() const;
    osg::ref_ptr<osg::Group> getFaceNode() const;

    // 材质管理（通过材质管理器）
    GeoMaterialManager* getMaterialManager() const { return m_materialManager.get(); }

    // 捕捉点管理（通过捕捉点管理器）
    GeoSnapPointManager* getSnapPointManager() const { return m_snapPointManager.get(); }
    
    // 简化的捕捉点访问接口
    const std::vector<glm::vec3>& getSnapPoints() const;
    void addSnapPoint(const glm::vec3& point);
    void clearSnapPoints();
    void updateSnapPoints();

    // 渲染管理（通过渲染管理器）
    GeoRenderManager* getRenderManager() const { return m_renderManager.get(); }
    
    // 简化的渲染接口
    void setShowPoints(bool show);
    void setShowEdges(bool show);
    void setShowFaces(bool show);
    bool isShowPoints() const;
    bool isShowEdges() const;
    bool isShowFaces() const;

    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void mouseReleaseEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const;
    
    // KDTree支持的快速拾取测试
    virtual bool hitTestWithKdTree(const Ray3D& ray, PickResult3D& result) const;
    virtual bool hitTestPoint(const osg::Vec3& point, float radius, PickResult3D& result) const;
    virtual bool hitTestVisible(const Ray3D& ray, PickResult3D& result) const;

    // 绘制完成
    virtual void completeDrawing();
    
    // 更新几何体
    virtual void updateGeometry() = 0;
    
    // 节点和渲染管理
    void setupNodeNames();
    void updateFeatureVisibility();
    
    // 点线面几何体管理
    void addVertexGeometry(osg::Drawable* drawable);
    void addEdgeGeometry(osg::Drawable* drawable);
    void addFaceGeometry(osg::Drawable* drawable);
    
    void clearVertexGeometries();
    void clearEdgeGeometries();
    void clearFaceGeometries();

signals:
    // 几何对象状态变化信号
    void stateChanged(Geo3D* geo);
    void drawingCompleted(Geo3D* geo);
    void geometryUpdated(Geo3D* geo);
    void parametersChanged(Geo3D* geo);

protected:
    // 初始化（第一次调用时）
    virtual void initialize();
    
    // 更新OSG节点
    virtual void updateOSGNode();
    
    // 创建几何体（子类实现）
    virtual osg::ref_ptr<osg::Geometry> createGeometry() = 0;
    
    // 辅助函数
    osg::Vec3 glmToOsgVec3(const glm::vec3& v) const;
    osg::Vec4 glmToOsgVec4(const glm::vec4& v) const;
    glm::vec3 osgToGlmVec3(const osg::Vec3& v) const;
    glm::vec4 osgToGlmVec4(const osg::Vec4& v) const;
    
    void markGeometryDirty() { m_geometryDirty = true; }
    bool isGeometryDirty() const { return m_geometryDirty; }
    void clearGeometryDirty() { m_geometryDirty = false; }
    
    // 点线面节点管理辅助方法
    virtual void buildVertexGeometries() = 0;  // 子类实现具体的顶点几何体构建
    virtual void buildEdgeGeometries() = 0;    // 子类实现具体的边几何体构建
    virtual void buildFaceGeometries() = 0;    // 子类实现具体的面几何体构建

    // 管理器友元类
    friend class GeoStateManager;
    friend class GeoNodeManager;
    friend class GeoMaterialManager;
    friend class GeoSnapPointManager;
    friend class GeoControlPointManager;
    friend class GeoBoundingBoxManager;
    friend class GeoRenderManager;

protected:
    // 基本属性
    GeoType3D m_geoType;
    GeoParameters3D m_parameters;
    Point3D m_tempPoint;
    Transform3D m_transform;
    bool m_geometryDirty;
    bool m_initialized;
    bool m_parametersChanged;

    // 管理器组件
    std::unique_ptr<GeoStateManager> m_stateManager;
    std::unique_ptr<GeoNodeManager> m_nodeManager;
    std::unique_ptr<GeoMaterialManager> m_materialManager;
    std::unique_ptr<GeoSnapPointManager> m_snapPointManager;
    std::unique_ptr<GeoControlPointManager> m_controlPointManager;
    std::unique_ptr<GeoBoundingBoxManager> m_boundingBoxManager;
    std::unique_ptr<GeoRenderManager> m_renderManager;

    // 内部方法
    void setupManagers();
    void connectManagerSignals();
};