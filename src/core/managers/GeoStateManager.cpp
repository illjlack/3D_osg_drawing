#include "GeoStateManager.h"
#include "../GeometryBase.h"

GeoStateManager::GeoStateManager(Geo3D* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_geoState(0)
{
    // 初始化时设置为已初始化状态
    setStateInitialized();
}

void GeoStateManager::setStateInitialized()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Initialized3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
        emit stateInitialized();
    }
}

void GeoStateManager::setStateComplete()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Complete3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
        emit stateCompleted();
    }
}

void GeoStateManager::setStateInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Invalid3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
        emit stateInvalidated();
    }
}

void GeoStateManager::setStateSelected()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Selected3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
        emit stateSelected();
    }
}

void GeoStateManager::setStateEditing()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Editing3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
        emit editingStarted();
    }
}

void GeoStateManager::clearStateComplete()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_Complete3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
    }
}

void GeoStateManager::clearStateInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_Invalid3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
    }
}

void GeoStateManager::clearStateSelected()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_Selected3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
        emit stateDeselected();
    }
}

void GeoStateManager::clearStateEditing()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_Editing3D;
    
    if (oldState != m_geoState) {
        emit stateChanged(oldState, m_geoState);
        emit editingFinished();
    }
}

void GeoStateManager::setState(int state)
{
    int oldState = m_geoState;
    m_geoState = state;
    
    if (oldState != m_geoState) {
        emitStateChangedSignals(oldState, m_geoState);
    }
}

void GeoStateManager::reset()
{
    int oldState = m_geoState;
    m_geoState = GeoState_Initialized3D;
    
    if (oldState != m_geoState) {
        emitStateChangedSignals(oldState, m_geoState);
    }
}

void GeoStateManager::toggleSelected()
{
    if (isStateSelected()) {
        clearStateSelected();
    } else {
        setStateSelected();
    }
}

void GeoStateManager::toggleEditing()
{
    if (isStateEditing()) {
        clearStateEditing();
    } else {
        setStateEditing();
    }
}

bool GeoStateManager::isValidState() const
{
    // 检查状态是否有效
    if (isStateInvalid()) {
        return false;
    }
    
    // 编辑状态下不应该是完成状态
    if (isStateEditing() && isStateComplete()) {
        return false;
    }
    
    return true;
}

bool GeoStateManager::canEdit() const
{
    // 无效状态下不能编辑
    if (isStateInvalid()) {
        return false;
    }
    
    // 必须是已初始化状态
    if (!isStateInitialized()) {
        return false;
    }
    
    return true;
}

bool GeoStateManager::canSelect() const
{
    // 无效状态下不能选择
    if (isStateInvalid()) {
        return false;
    }
    
    // 必须是已初始化状态
    if (!isStateInitialized()) {
        return false;
    }
    
    return true;
}

void GeoStateManager::emitStateChangedSignals(int oldState, int newState)
{
    // 发送特定状态变化信号
    if (hasStateChanged(oldState, newState, GeoState_Initialized3D)) {
        if (newState & GeoState_Initialized3D) {
            emit stateInitialized();
        }
    }
    
    if (hasStateChanged(oldState, newState, GeoState_Complete3D)) {
        if (newState & GeoState_Complete3D) {
            emit stateCompleted();
        }
    }
    
    if (hasStateChanged(oldState, newState, GeoState_Invalid3D)) {
        if (newState & GeoState_Invalid3D) {
            emit stateInvalidated();
        }
    }
    
    if (hasStateChanged(oldState, newState, GeoState_Selected3D)) {
        if (newState & GeoState_Selected3D) {
            emit stateSelected();
        } else {
            emit stateDeselected();
        }
    }
    
    if (hasStateChanged(oldState, newState, GeoState_Editing3D)) {
        if (newState & GeoState_Editing3D) {
            emit editingStarted();
        } else {
            emit editingFinished();
        }
    }
}

bool GeoStateManager::hasStateChanged(int oldState, int newState, int stateMask) const
{
    return (oldState & stateMask) != (newState & stateMask);
}