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

    // 反射类型
    GeoType3D getGeoType() const { return m_geoType; }
    void setGeoType(GeoType3D type) { m_geoType = type; }

    // 管理器直接访问接口(mm_开头：成员、管理器)
    GeoStateManager* mm_state() const { return m_stateManager.get(); }
    GeoNodeManager* mm_node() const { return m_nodeManager.get(); }
    GeoMaterialManager* mm_material() const { return m_materialManager.get(); }
    GeoSnapPointManager* mm_snapPoint() const { return m_snapPointManager.get(); }
    GeoControlPointManager* mm_controlPoint() const { return m_controlPointManager.get(); }
    GeoBoundingBoxManager* mm_boundingBox() const { return m_boundingBoxManager.get(); }
    GeoRenderManager* mm_render() const { return m_renderManager.get(); }

    // 参数管理
    const GeoParameters3D& getParameters() const { return m_parameters; }
    void setParameters(const GeoParameters3D& params);

    // 事件处理虚函数
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

protected:
    // 初始化
    virtual void initialize();
    
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

    // 辅助函数
    osg::Vec3 glmToOsgVec3(const glm::vec3& v) const;
    osg::Vec4 glmToOsgVec4(const glm::vec4& v) const;
    glm::vec3 osgToGlmVec3(const osg::Vec3& v) const;
    glm::vec4 osgToGlmVec4(const osg::Vec4& v) const;
};