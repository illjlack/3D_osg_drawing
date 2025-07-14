#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <glm/glm.hpp>
#include <vector>

// 前向声明
class Geo3D;

/**
 * @brief 包围盒管理器
 * 负责管理几何对象的包围盒计算、更新和查询
 */
class GeoBoundingBoxManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoBoundingBoxManager(Geo3D* parent);
    ~GeoBoundingBoxManager() = default;

    // 包围盒访问
    const BoundingBox3D& getBoundingBox() const { return m_boundingBox; }
    void setBoundingBox(const BoundingBox3D& box);

    // 包围盒计算
    void updateBoundingBox();
    void updateFromControlPoints();
    void updateFromGeometry();
    void updateFromChildren();
    void forceUpdate();

    // 包围盒查询
    glm::vec3 getCenter() const;
    glm::vec3 getSize() const;
    glm::vec3 getMin() const { return m_boundingBox.min; }
    glm::vec3 getMax() const { return m_boundingBox.max; }
    float getRadius() const;
    float getDiagonal() const;

    // 包围盒操作
    void expand(const glm::vec3& point);
    void expand(const BoundingBox3D& other);
    void expand(float margin);
    void contract(float margin);
    void reset();

    // 包围盒变换
    void transform(const glm::mat4& matrix);
    void translate(const glm::vec3& offset);
    void scale(const glm::vec3& scale);
    void scale(float uniformScale);

    // 包围盒测试
    bool isValid() const;
    bool isEmpty() const;
    bool contains(const glm::vec3& point) const;
    bool contains(const BoundingBox3D& other) const;
    bool intersects(const BoundingBox3D& other) const;
    bool intersects(const Ray3D& ray) const;

    // 包围盒距离计算
    float distanceToPoint(const glm::vec3& point) const;
    float distanceToBoundingBox(const BoundingBox3D& other) const;
    glm::vec3 closestPointTo(const glm::vec3& point) const;

    // 包围盒角点
    std::vector<glm::vec3> getCorners() const;
    glm::vec3 getCorner(int index) const;

    // 包围盒面
    std::vector<glm::vec3> getFaceCenter(int faceIndex) const;
    std::vector<glm::vec3> getFaceNormal(int faceIndex) const;

    // 包围盒边
    std::vector<std::pair<glm::vec3, glm::vec3>> getEdges() const;
    std::pair<glm::vec3, glm::vec3> getEdge(int edgeIndex) const;

    // 自动更新控制
    void setAutoUpdate(bool enabled);
    bool isAutoUpdateEnabled() const { return m_autoUpdate; }
    void setUpdateMode(int mode);
    int getUpdateMode() const { return m_updateMode; }

    // 包围盒可见性
    void setVisible(bool visible);
    bool isVisible() const { return m_visible; }
    void setWireframeColor(const Color3D& color);
    const Color3D& getWireframeColor() const { return m_wireframeColor; }
    void setWireframeWidth(float width);
    float getWireframeWidth() const { return m_wireframeWidth; }

    // 包围盒统计
    float getVolume() const;
    float getSurfaceArea() const;
    glm::vec3 getExtent() const;
    float getAspectRatio() const;

    // 包围盒验证
    bool validateBoundingBox() const;
    void correctBoundingBox();

    // 包围盒序列化
    QString toString() const;
    void fromString(const QString& str);

signals:
    void boundingBoxChanged();
    void boundingBoxUpdated();
    void boundingBoxInvalidated();
    void visibilityChanged(bool visible);
    void colorChanged();

private:
    void initializeBoundingBox();
    void calculateBoundingBoxFromPoints(const std::vector<glm::vec3>& points);
    void calculateBoundingBoxFromOSGGeometry();
    void updateBoundingBoxVisualization();
    void createBoundingBoxWireframe();
    void markDirty();
    bool isDirty() const { return m_dirty; }
    void clearDirty() { m_dirty = false; }

private:
    Geo3D* m_parent;
    
    // 包围盒数据
    BoundingBox3D m_boundingBox;
    
    // 更新控制
    bool m_autoUpdate;
    int m_updateMode;  // 0: 控制点, 1: 几何体, 2: 子对象, 3: 全部
    bool m_dirty;
    
    // 可视化
    bool m_visible;
    Color3D m_wireframeColor;
    float m_wireframeWidth;
    
    // 缓存
    mutable bool m_centerCached;
    mutable glm::vec3 m_cachedCenter;
    mutable bool m_sizeCached;
    mutable glm::vec3 m_cachedSize;
    mutable bool m_radiusCached;
    mutable float m_cachedRadius;
}; 