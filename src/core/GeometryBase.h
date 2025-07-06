#pragma once

#include "Common3D.h"
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
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <algorithm>

// 前向声明
class Geo3D;

// 几何对象工厂
Geo3D* createGeo3D(DrawMode3D mode);

// 拾取Provider接口
class IPickingProvider
{
public:
    virtual ~IPickingProvider() = default;
    
    // 获取支持的Feature类型
    virtual std::vector<FeatureType> getSupportedFeatureTypes() const = 0;
    
    // 获取指定类型的所有Feature
    virtual std::vector<PickingFeature> getFeatures(FeatureType type) const = 0;
    
    // 获取指定Feature的详细信息(可选实现)
    virtual bool getFeatureInfo(FeatureType type, uint32_t index, PickingFeature& feature) const { return false; }
    
    // 判断是否需要重新抽取Feature(当几何体发生变化时)
    virtual bool needsFeatureUpdate() const = 0;
    
    // 标记Feature已更新
    virtual void markFeatureUpdated() = 0;
};

// 三维几何对象基类
class Geo3D : public osg::Referenced, public IPickingProvider
{
public:
    Geo3D();
    virtual ~Geo3D();

    // 类型相关
    GeoType3D getGeoType() const { return m_geoType; }
    void setGeoType(GeoType3D type) { m_geoType = type; }

    // 状态管理
    bool isStateInitialized() const { return m_geoState & GeoState_Initialized3D; }
    bool isStateComplete() const { return m_geoState & GeoState_Complete3D; }
    bool isStateInvalid() const { return m_geoState & GeoState_Invalid3D; }
    bool isStateSelected() const { return m_geoState & GeoState_Selected3D; }
    bool isStateEditing() const { return m_geoState & GeoState_Editing3D; }

    void setStateInitialized() { m_geoState |= GeoState_Initialized3D; }
    void setStateComplete() { m_geoState |= GeoState_Complete3D; }
    void setStateInvalid() { m_geoState |= GeoState_Invalid3D; }
    void setStateSelected() { m_geoState |= GeoState_Selected3D; }
    void setStateEditing() { m_geoState |= GeoState_Editing3D; }

    void clearStateComplete() { m_geoState &= ~GeoState_Complete3D; }
    void clearStateInvalid() { m_geoState &= ~GeoState_Invalid3D; }
    void clearStateSelected() { m_geoState &= ~GeoState_Selected3D; }
    void clearStateEditing() { m_geoState &= ~GeoState_Editing3D; }

    // 参数管理
    const GeoParameters3D& getParameters() const { return m_parameters; }
    void setParameters(const GeoParameters3D& params);

    // 控制点管理
    const std::vector<Point3D>& getControlPoints() const { return m_controlPoints; }
    void addControlPoint(const Point3D& point);
    void setControlPoint(int index, const Point3D& point);
    void removeControlPoint(int index);
    void clearControlPoints();
    bool hasControlPoints() const { return !m_controlPoints.empty(); }
    
    // 临时点（用于绘制过程中的预览）
    const Point3D& getTempPoint() const { return m_tempPoint; }
    void setTempPoint(const Point3D& point) { m_tempPoint = point; }

    // 变换
    const Transform3D& getTransform() const { return m_transform; }
    void setTransform(const Transform3D& transform);

    // 包围盒
    const BoundingBox3D& getBoundingBox() const { return m_boundingBox; }

    // OSG节点
    osg::ref_ptr<osg::Group> getOSGNode() const { return m_osgNode; }

    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void mouseReleaseEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    // 拾取测试
    virtual bool hitTest(const Ray3D& ray, PickResult3D& result) const;

    // 绘制完成
    virtual void completeDrawing();
    
    // 更新几何体
    virtual void updateGeometry() = 0;

    // 实现IPickingProvider接口
    virtual std::vector<FeatureType> getSupportedFeatureTypes() const override;
    virtual std::vector<PickingFeature> getFeatures(FeatureType type) const override;
    virtual bool needsFeatureUpdate() const override { return m_featuresDirty; }
    virtual void markFeatureUpdated() override { m_featuresDirty = false; }

protected:
    // 初始化（第一次调用时）
    virtual void initialize();
    
    // 更新OSG节点
    virtual void updateOSGNode();
    virtual void updateMaterial();
    virtual void updateControlPointsVisualization();
    
    // 创建几何体（子类实现）
    virtual osg::ref_ptr<osg::Geometry> createGeometry() = 0;
    
    // 辅助函数
    osg::Vec3 glmToOsgVec3(const glm::vec3& v) const;
    osg::Vec4 glmToOsgVec4(const glm::vec4& v) const;
    glm::vec3 osgToGlmVec3(const osg::Vec3& v) const;
    glm::vec4 osgToGlmVec4(const osg::Vec4& v) const;
    
    void updateBoundingBox();
    
    void markGeometryDirty() { m_geometryDirty = true; }
    bool isGeometryDirty() const { return m_geometryDirty; }
    void clearGeometryDirty() { m_geometryDirty = false; }

    // 子类实现具体的Feature抽取逻辑
    virtual std::vector<PickingFeature> extractFaceFeatures() const { return {}; }
    virtual std::vector<PickingFeature> extractEdgeFeatures() const { return {}; }
    virtual std::vector<PickingFeature> extractVertexFeatures() const { return {}; }
    
    // 标记Feature需要更新
    void markFeaturesDirty() { m_featuresDirty = true; }

protected:
    GeoType3D m_geoType;
    int m_geoState;
    GeoParameters3D m_parameters;
    
    std::vector<Point3D> m_controlPoints;
    Point3D m_tempPoint;
    Transform3D m_transform;
    BoundingBox3D m_boundingBox;
    
    // OSG相关 - 使用现代OSG API (Group+Drawable而不是Geode)
    osg::ref_ptr<osg::Group> m_osgNode;
    osg::ref_ptr<osg::Group> m_drawableGroup;  // 替代Geode
    osg::ref_ptr<osg::Geometry> m_geometry;
    osg::ref_ptr<osg::MatrixTransform> m_transformNode;
    osg::ref_ptr<osg::Group> m_controlPointsNode;
    
    bool m_geometryDirty;
    bool m_initialized;
    bool m_parametersChanged;  // 新增：跟踪参数是否发生变化

    mutable bool m_featuresDirty;
    mutable std::map<FeatureType, std::vector<PickingFeature>> m_cachedFeatures;
    
    // 缓存Feature以提高性能
    std::vector<PickingFeature> getCachedFeatures(FeatureType type) const;
};

// 规则几何体基类
class RegularGeo3D : public Geo3D
{
public:
    RegularGeo3D();
    virtual ~RegularGeo3D() = default;
    
    // 规则几何体默认支持所有Feature类型
    virtual std::vector<FeatureType> getSupportedFeatureTypes() const override
    {
        return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
    }

protected:
    // 规则几何体可以精确计算Feature
    virtual std::vector<PickingFeature> extractFaceFeatures() const override;
    virtual std::vector<PickingFeature> extractEdgeFeatures() const override; 
    virtual std::vector<PickingFeature> extractVertexFeatures() const override;
};

// 网格几何体基类
class MeshGeo3D : public Geo3D
{
public:
    MeshGeo3D();
    virtual ~MeshGeo3D() = default;
    
    // 网格对象通常只提供面Feature
    virtual std::vector<FeatureType> getSupportedFeatureTypes() const override
    {
        return {FeatureType::FACE};
    }
    
    // 设置三角网格数据
    void setMeshData(osg::ref_ptr<osg::Geometry> geometry);

protected:
    virtual std::vector<PickingFeature> extractFaceFeatures() const override;

private:
    osg::ref_ptr<osg::Geometry> m_meshGeometry;
};

// 复合几何体(由多个基本几何体组成)
class CompositeGeo3D : public Geo3D
{
public:
    CompositeGeo3D();
    virtual ~CompositeGeo3D() = default;
    
    // 复合对象支持所有Feature类型
    virtual std::vector<FeatureType> getSupportedFeatureTypes() const override
    {
        return {FeatureType::FACE, FeatureType::EDGE, FeatureType::VERTEX};
    }
    
    // 组件管理
    void addComponent(osg::ref_ptr<Geo3D> component);
    void removeComponent(osg::ref_ptr<Geo3D> component);
    void clearComponents();
    
    // 从所有组件收集Feature
    virtual std::vector<PickingFeature> getFeatures(FeatureType type) const override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    virtual void updateGeometry() override;

private:
    std::vector<osg::ref_ptr<Geo3D>> m_components;
}; 