﻿#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 未定义几何体类
class UndefinedGeo3D : public Geo3D
{
public:
    UndefinedGeo3D();
    virtual ~UndefinedGeo3D() = default;
    
    // 设置自定义数据
    void setCustomData(const QVariantMap& data) { m_customData = data; }
    QVariantMap getCustomData() const { return m_customData; }
    
    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void completeDrawing() override;
    virtual void updateGeometry() override;

    // 拾取Feature支持
    virtual std::vector<FeatureType> getSupportedFeatureTypes() const override;

protected:
    virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
    
    // Feature抽取
    virtual std::vector<PickingFeature> extractVertexFeatures() const override;
    virtual std::vector<PickingFeature> extractEdgeFeatures() const override;
    virtual std::vector<PickingFeature> extractFaceFeatures() const override;
    
private:
    QVariantMap m_customData;  // 自定义数据
    osg::ref_ptr<osg::Geometry> createDefaultGeometry();
}; 