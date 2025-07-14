#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <vector>
#include <map>
#include <glm/glm.hpp>

// 前向声明
class Geo3D;

/**
 * @brief 捕捉点管理器
 * 负责管理几何对象的捕捉点系统，包括自动生成、手动添加、查询最近点等
 */
class GeoSnapPointManager : public QObject
{
    Q_OBJECT

public:
    // 捕捉点类型
    enum SnapPointType
    {
        SNAP_CONTROL_POINT,      // 控制点
        SNAP_VERTEX,             // 顶点
        SNAP_EDGE_ENDPOINT,      // 边的端点
        SNAP_EDGE_MIDPOINT,      // 边的中点
        SNAP_EDGE_QUARTER,       // 边的1/4和3/4点
        SNAP_FACE_CENTER,        // 面的中心
        SNAP_BOUNDING_BOX,       // 包围盒角点
        SNAP_CUSTOM              // 自定义捕捉点
    };

    // 捕捉点信息
    struct SnapPoint
    {
        glm::vec3 position;
        SnapPointType type;
        int featureIndex;        // 特征索引（如第几个顶点、第几条边等）
        float priority;          // 优先级（数值越小优先级越高）
        bool enabled;            // 是否启用
        
        SnapPoint(const glm::vec3& pos, SnapPointType t, int index = -1, float prio = 1.0f)
            : position(pos), type(t), featureIndex(index), priority(prio), enabled(true) {}
    };

    explicit GeoSnapPointManager(Geo3D* parent);
    ~GeoSnapPointManager() = default;

    // 捕捉点访问
    const std::vector<SnapPoint>& getSnapPoints() const { return m_snapPoints; }
    const std::vector<glm::vec3>& getSnapPositions() const { return m_snapPositions; }
    int getSnapPointCount() const { return static_cast<int>(m_snapPoints.size()); }

    // 捕捉点操作
    void addSnapPoint(const glm::vec3& position, SnapPointType type = SNAP_CUSTOM, int featureIndex = -1);
    void addSnapPoint(const SnapPoint& snapPoint);
    void removeSnapPoint(int index);
    void clearSnapPoints();
    void clearSnapPointsByType(SnapPointType type);

    // 自动生成捕捉点
    void updateSnapPoints();
    void generateControlPointSnaps();
    void generateVertexSnaps();
    void generateEdgeSnaps();
    void generateFaceSnaps();
    void generateBoundingBoxSnaps();

    // 捕捉查询
    glm::vec3 findNearestSnapPoint(const glm::vec3& position, float threshold = 0.15f) const;
    int findNearestSnapPointIndex(const glm::vec3& position, float threshold = 0.15f) const;
    bool hasSnapPointNear(const glm::vec3& position, float threshold = 0.15f) const;
    std::vector<int> findSnapPointsInRange(const glm::vec3& position, float range) const;

    // 捕捉设置
    void setSnapThreshold(float threshold);
    float getSnapThreshold() const { return m_snapThreshold; }
    void setSnapEnabled(bool enabled);
    bool isSnapEnabled() const { return m_snapEnabled; }

    // 捕捉点类型控制
    void setSnapTypeEnabled(SnapPointType type, bool enabled);
    bool isSnapTypeEnabled(SnapPointType type) const;
    void enableAllSnapTypes();
    void disableAllSnapTypes();

    // 捕捉点优先级
    void setSnapPointPriority(int index, float priority);
    float getSnapPointPriority(int index) const;
    void sortSnapPointsByPriority();

    // 捕捉点可见性
    void setSnapPointsVisible(bool visible);
    bool areSnapPointsVisible() const { return m_snapPointsVisible; }
    void setSnapPointSize(float size);
    float getSnapPointSize() const { return m_snapPointSize; }
    void setSnapPointColor(const Color3D& color);
    const Color3D& getSnapPointColor() const { return m_snapPointColor; }

    // 捕捉点验证
    bool validateSnapPoints() const;
    void removeInvalidSnapPoints();
    void removeDuplicateSnapPoints(float tolerance = 0.001f);

    // 捕捉点统计
    int getSnapPointCountByType(SnapPointType type) const;
    std::vector<SnapPointType> getActiveSnapTypes() const;

    // 捕捉点变换
    void transformSnapPoints(const glm::mat4& matrix);
    void translateSnapPoints(const glm::vec3& offset);

signals:
    void snapPointAdded(int index, const SnapPoint& snapPoint);
    void snapPointRemoved(int index);
    void snapPointsCleared();
    void snapPointsUpdated();
    void snapThresholdChanged(float threshold);
    void snapEnabledChanged(bool enabled);
    void snapTypeEnabledChanged(SnapPointType type, bool enabled);
    void snapPointsVisibilityChanged(bool visible);

private:
    void initializeSnapSettings();
    void updateSnapPositions();
    void generateSnapPointsFromGeometry();
    void addEdgeSnapPoints(const std::vector<glm::vec3>& vertices);
    void addFaceSnapPoints(const std::vector<glm::vec3>& vertices);
    bool isSnapTypeActive(SnapPointType type) const;
    void updateSnapPointVisualization();

private:
    Geo3D* m_parent;
    
    // 捕捉点数据
    std::vector<SnapPoint> m_snapPoints;
    std::vector<glm::vec3> m_snapPositions;  // 用于快速查询的位置列表
    
    // 捕捉设置
    float m_snapThreshold;
    bool m_snapEnabled;
    bool m_snapPointsVisible;
    float m_snapPointSize;
    Color3D m_snapPointColor;
    
    // 捕捉点类型启用状态
    std::map<SnapPointType, bool> m_snapTypeEnabled;
    
    // 自动更新设置
    bool m_autoUpdateEnabled;
    bool m_needsUpdate;
}; 