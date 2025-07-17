#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <glm/glm.hpp>
#include <osg/BoundingBox>
#include <vector>

// 前向声明
class Geo3D;

/**
 * @brief 包围盒管理器
 * 直接使用OSG的包围盒功能，提供简单的更新和查询接口
 */
class GeoBoundingBoxManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoBoundingBoxManager(Geo3D* parent);
    ~GeoBoundingBoxManager() = default;

    // 核心功能：获取OSG包围盒
    const osg::BoundingBox& getBoundingBox() const { return m_boundingBox; }

    // 更新包围盒
    void updateBoundingBox();
    void updateFromGeometry();
    void updateFromControlPoints();

    // 简单的查询接口
    glm::vec3 getCenter() const;
    glm::vec3 getSize() const;
    bool isValid() const { return m_boundingBox.valid(); }

signals:
    void boundingBoxChanged();

private:
    void initializeBoundingBox();
    void calculateBoundingBoxFromOSGGeometry();

private:
    Geo3D* m_parent;
    
    // OSG包围盒
    osg::BoundingBox m_boundingBox;
}; 