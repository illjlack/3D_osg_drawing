#include "GeoBoundingBoxManager.h"
#include "../GeometryBase.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

GeoBoundingBoxManager::GeoBoundingBoxManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
{
    initializeBoundingBox();
}

void GeoBoundingBoxManager::initializeBoundingBox()
{
    m_boundingBox = osg::BoundingBox();
}

void GeoBoundingBoxManager::updateBoundingBox()
{
    // 优先从几何体更新
    updateFromGeometry();
    
    // 如果几何体没有包围盒，则从控制点更新
    if (!m_boundingBox.valid()) {
        updateFromControlPoints();
    }
    
    emit boundingBoxChanged();
}

void GeoBoundingBoxManager::updateFromGeometry()
{
    if (!m_parent) return;
    
    auto* nodeManager = m_parent->mm_node();
    if (!nodeManager) return;
    
    auto geometry = nodeManager->getVertexGeometry();
    if (!geometry.valid()) return;
    
    // 直接使用OSG的包围盒
    m_boundingBox = geometry->getBoundingBox();
}

void GeoBoundingBoxManager::updateFromControlPoints()
{
    if (!m_parent) return;
    
    auto* controlManager = m_parent->mm_controlPoint();
    if (!controlManager || !controlManager->hasControlPoints()) return;
    
    const auto& controlPoints = controlManager->getControlPoints();
    if (controlPoints.empty()) return;
    
    // 从控制点计算包围盒
    m_boundingBox = osg::BoundingBox();
    for (const auto& cp : controlPoints) {
        m_boundingBox.expandBy(osg::Vec3(cp.position.x, cp.position.y, cp.position.z));
    }
}

glm::vec3 GeoBoundingBoxManager::getCenter() const
{
    if (m_boundingBox.valid()) {
        osg::Vec3 center = m_boundingBox.center();
        return glm::vec3(center.x(), center.y(), center.z());
    }
    return glm::vec3(0.0f);
}

glm::vec3 GeoBoundingBoxManager::getSize() const
{
    if (m_boundingBox.valid()) {
        osg::Vec3 size = m_boundingBox._max - m_boundingBox._min;
        return glm::vec3(size.x(), size.y(), size.z());
    }
    return glm::vec3(0.0f);
}

void GeoBoundingBoxManager::calculateBoundingBoxFromOSGGeometry()
{
    updateFromGeometry();
} 