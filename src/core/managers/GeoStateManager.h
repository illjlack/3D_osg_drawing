#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <map>
#include <vector>
#include <functional>

// 前向声明
class Geo3D;

/**
 * @brief 几何对象状态管理器 - 智能状态机
 * 
 * 功能特性：
 * 1. 状态依赖管理：自动处理状态间的依赖关系
 * 2. 更新链传递：状态变化时自动触发相关更新
 * 3. 状态验证：确保状态组合的有效性
 * 4. 批量操作：支持批量状态设置和清除
 * 5. 信号通知：状态变化时发送相应的信号
 * 
 * 状态分类：
 * - 主动更新状态：参数更新、临时点更新、控制点更新（用户主动触发）
 * - 被动失效状态：几何体、八叉树、包围盒、材料等失效（系统自动设置）
 * - 清除失效状态：更新完成状态（系统自动清除）
 * 
 * 更新流程示例：
 * - 参数更新 → 控制点更新 → 几何体更新 → 包围盒更新 → 显示列表更新
 * - 临时点更新 → 控制点更新 → 几何体更新
 * - 变换更新 → 包围盒更新 → 显示列表更新
 */
class GeoStateManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoStateManager(Geo3D* parent);
    ~GeoStateManager() = default;

    // ==================== 状态查询接口 ====================
    
    // 基础状态查询
    bool isStateInitialized() const { return m_geoState & GeoState_Initialized3D; }
    bool isStateComplete() const { return m_geoState & GeoState_Complete3D; }
    bool isStateInvalid() const { return m_geoState & GeoState_Invalid3D; }
    bool isStateSelected() const { return m_geoState & GeoState_Selected3D; }
    bool isStateEditing() const { return m_geoState & GeoState_Editing3D; }

    // ==================== 主动更新状态查询 ====================
    // 用户主动触发的更新状态
    bool isParametersUpdated() const { return m_geoState & GeoState_ParametersUpdated3D; }
    bool isTemporaryPointsUpdated() const { return m_geoState & GeoState_TemporaryPointsUpdated3D; }
    bool isControlPointsUpdated() const { return m_geoState & GeoState_ControlPointsInvalid3D; }

    // ==================== 被动失效状态查询 ====================
    // 系统自动设置的失效状态
    bool isOctreeInvalid() const { return m_geoState & GeoState_OctreeInvalid3D; }
    bool isVertexGeometryInvalid() const { return m_geoState & GeoState_VertexGeometryInvalid3D; }
    bool isEdgeGeometryInvalid() const { return m_geoState & GeoState_EdgeGeometryInvalid3D; }
    bool isFaceGeometryInvalid() const { return m_geoState & GeoState_FaceGeometryInvalid3D; }
    bool isGeometryInvalid() const { return m_geoState & GeoState_GeometryInvalid3D; }
    bool isBoundingBoxInvalid() const { return m_geoState & GeoState_BoundingBoxInvalid3D; }
    bool isDisplayListInvalid() const { return m_geoState & GeoState_DisplayListInvalid3D; }
    bool isTextureInvalid() const { return m_geoState & GeoState_TextureInvalid3D; }
    bool isMaterialInvalid() const { return m_geoState & GeoState_MaterialInvalid3D; }
    bool isTransformInvalid() const { return m_geoState & GeoState_TransformInvalid3D; }

    // ==================== 基础状态设置 ====================
    void setStateInitialized();
    void setStateComplete();
    void setStateInvalid();
    void setStateSelected();
    void setStateEditing();

    // ==================== 主动更新状态设置 ====================
    // 用户主动触发的更新状态（会触发相关失效状态）
    void setParametersUpdated();
    void setTemporaryPointsUpdated();
    void setControlPointsUpdated();

    // ==================== 被动失效状态设置 ====================
    // 系统自动设置的失效状态
    void setOctreeInvalid();
    void setVertexGeometryInvalid();
    void setEdgeGeometryInvalid();
    void setFaceGeometryInvalid();
    void setGeometryInvalid();
    void setBoundingBoxInvalid();
    void setDisplayListInvalid();
    void setTextureInvalid();
    void setMaterialInvalid();
    void setTransformInvalid();

    // ==================== 基础状态清除 ====================
    void clearStateComplete();
    void clearStateInvalid();
    void clearStateSelected();
    void clearStateEditing();

    // ==================== 主动更新状态清除 ====================
    // 清除用户主动触发的更新状态
    void clearParametersUpdated();
    void clearTemporaryPointsUpdated();
    void clearControlPointsUpdated();

    // ==================== 被动失效状态清除 ====================
    // 清除系统自动设置的失效状态（更新完成）
    void clearOctreeInvalid();
    void clearVertexGeometryInvalid();
    void clearEdgeGeometryInvalid();
    void clearFaceGeometryInvalid();
    void clearGeometryInvalid();
    void clearBoundingBoxInvalid();
    void clearDisplayListInvalid();
    void clearTextureInvalid();
    void clearMaterialInvalid();
    void clearTransformInvalid();

    // ==================== 批量操作接口 ====================
    
    // 批量设置主动更新状态
    void setAllUpdateStates();
    void clearAllUpdateStates();
    
    // 批量设置被动失效状态
    void setAllInvalidStates();
    void clearAllInvalidStates();

    // ==================== 状态管理接口 ====================
    
    // 获取完整状态
    int getState() const { return m_geoState; }
    void setState(int state);

    // 状态重置
    void reset();

    // 状态切换
    void toggleSelected();
    void toggleEditing();

signals:
    void stateInitialized();
    void stateCompleted();
    void stateInvalidated();
    void stateSelected();
    void stateDeselected();
    void editingStarted();
    void editingFinished();

    // ==================== 主动更新（已经更新完成）状态变化信号 ====================
    void parametersUpdated();
    void temporaryPointsUpdated();
    void controlPointsUpdated();

    // ==================== 被动失效（主动去更新）状态变化信号 ====================
    void octreeUpdate();
    void vertexGeometryUpdate();
    void edgeGeometryUpdate();
    void faceGeometryUpdate();
    void boundingBoxUpdate();
    void displayListUpdate();
    void textureUpdate();
    void materialUpdate();
    void transformUpdate();

private:
    Geo3D* m_parent;
    int m_geoState;
};