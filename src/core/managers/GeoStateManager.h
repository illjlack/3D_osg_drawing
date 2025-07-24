#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <osg/ref_ptr>

// 前向声明
class Geo3D;

/**
 * @brief 几何对象状态管理器
 * 负责管理几何对象的各种状态，包括初始化、完成、无效、选中、编辑等状态
 */
class GeoStateManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoStateManager(osg::ref_ptr<Geo3D> parent);
    ~GeoStateManager() = default;

    // 状态查询
    bool isStateInitialized() const { return m_geoState & GeoState_Initialized3D; }
    bool isStateComplete() const { return m_geoState & GeoState_Complete3D; }
    bool isStateInvalid() const { return m_geoState & GeoState_Invalid3D; }
    bool isStateSelected() const { return m_geoState & GeoState_Selected3D; }
    bool isStateEditing() const { return m_geoState & GeoState_Editing3D; }

    // 状态设置
    void setStateInitialized();
    void setStateComplete();
    void setStateInvalid();
    void setStateSelected();
    void setStateEditing();

    // 状态清除
    void clearStateComplete();
    void clearStateInvalid();
    void clearStateSelected();
    void clearStateEditing();

    // 获取完整状态
    int getState() const { return m_geoState; }
    void setState(int state);

    // 状态重置
    void reset();

    // 状态切换
    void toggleSelected();
    void toggleEditing();

    // 状态验证
    bool isValidState() const;
    bool canEdit() const;
    bool canSelect() const;

signals:
    void stateInitialized();
    void stateCompleted();
    void stateInvalidated();
    void stateSelected();
    void stateDeselected();
    void editingStarted();
    void editingFinished();

private:
    void emitStateChangedSignals(int oldState, int newState);
    bool hasStateChanged(int oldState, int newState, int stateMask) const;

private:
    osg::ref_ptr<Geo3D> m_parent;
    int m_geoState;
}; 

