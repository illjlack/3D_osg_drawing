#include "GeoStateManager.h"
#include "../GeometryBase.h"
#include "../../util/LogManager.h"

// ==================== 构造函数和析构函数 ====================

GeoStateManager::GeoStateManager(Geo3D* parent)
    : m_parent(parent)
    , m_geoState(0)
{
    // 初始状态设置为已初始化
    m_geoState = GeoState_Initialized3D;
    LOG_INFO("创建几何状态管理器", "状态管理");
}

// ==================== 基础状态设置接口 ====================

void GeoStateManager::setStateInitialized()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Initialized3D;
    
    if (oldState != m_geoState) {
        emit stateInitialized();
        LOG_DEBUG("设置状态：已初始化", "状态管理");
    }
}

void GeoStateManager::setStateComplete()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Complete3D;
    
    if (oldState != m_geoState) {
        emit stateCompleted();
        LOG_DEBUG("设置状态：绘制已完成", "状态管理");
    }
}

void GeoStateManager::setStateInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Invalid3D;
    
    if (oldState != m_geoState) {
        emit stateInvalidated();
        LOG_DEBUG("设置状态：已失效", "状态管理");
    }
}

void GeoStateManager::setStateSelected()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Selected3D;
    
    if (oldState != m_geoState) {
        emit stateSelected();
        LOG_DEBUG("设置状态：已选中", "状态管理");
    }
}

void GeoStateManager::setStateEditing()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_Editing3D;
    
    if (oldState != m_geoState) {
        emit editingStarted();
        LOG_DEBUG("设置状态：编辑中", "状态管理");
    }
}

// ==================== 主动更新状态设置接口 ====================

void GeoStateManager::setParametersUpdated()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_ParametersUpdated3D;
    
    if (oldState != m_geoState) {
        // 根据不同参数更新触发材料等更新：
        if(m_parent->getParameters().material.type == Material_Lambert3D)
        {
            setMaterialInvalid();
        }

        LOG_DEBUG("设置状态：参数更新", "状态管理");
    }
}

void GeoStateManager::setTemporaryPointsUpdated()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_TemporaryPointsUpdated3D;
    
    if (oldState != m_geoState) {
        setControlPointsUpdated();
        LOG_DEBUG("设置状态：临时点更新，控制点失效", "状态管理");
    }
}

void GeoStateManager::setControlPointsUpdated()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_ControlPointsInvalid3D;
    
    if (oldState != m_geoState) {
        setGeometryInvalid();
        LOG_DEBUG("设置状态：控制点更新，几何失效", "状态管理");
    }
}

// ==================== 被动失效状态设置接口 ====================

void GeoStateManager::setOctreeInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_OctreeInvalid3D;
    
    if (oldState != m_geoState) {
        emit octreeUpdate();
        LOG_DEBUG("设置状态：八叉树失效，触发八叉树更新", "状态管理");
    }
}

void GeoStateManager::setVertexGeometryInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_VertexGeometryInvalid3D;
    
    if (oldState != m_geoState) {
        emit vertexGeometryUpdate();
        LOG_DEBUG("设置状态：顶点几何体失效，触发顶点几何体更新", "状态管理");
    }
}

void GeoStateManager::setEdgeGeometryInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_EdgeGeometryInvalid3D;
    
    if (oldState != m_geoState) {
        emit edgeGeometryUpdate();
        LOG_DEBUG("设置状态：边失效，触发边更新", "状态管理");
    }
}

void GeoStateManager::setFaceGeometryInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_FaceGeometryInvalid3D;
    
    if (oldState != m_geoState) {
        emit faceGeometryUpdate();
        LOG_DEBUG("设置状态：面失效，触发面更新", "状态管理");
    }
}

void GeoStateManager::setGeometryInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_GeometryInvalid3D;
    
    if (oldState != m_geoState) {
        setVertexGeometryInvalid();
        setEdgeGeometryInvalid();
        setFaceGeometryInvalid();
        LOG_DEBUG("设置状态：几何体失效，点线面失效", "状态管理");
    }
}

void GeoStateManager::setBoundingBoxInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_BoundingBoxInvalid3D;
    
    if (oldState != m_geoState) {
        emit boundingBoxUpdate();
        LOG_DEBUG("设置状态：包围盒失效，触发包围盒更新", "状态管理");
    }
}

void GeoStateManager::setTextureInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_TextureInvalid3D;
    
    if (oldState != m_geoState) {
        emit textureUpdate();
        LOG_DEBUG("设置状态：纹理失效，触发纹理更新", "状态管理");
    }
}

void GeoStateManager::setMaterialInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_MaterialInvalid3D;
    
    if (oldState != m_geoState) {
        emit materialUpdate();
        LOG_DEBUG("设置状态：材质失效，触发材质更新", "状态管理");
    }
}

void GeoStateManager::setTransformInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_TransformInvalid3D;
    
    if (oldState != m_geoState) {
        emit transformUpdate();
        LOG_DEBUG("设置状态：变换失效，触发变换更新", "状态管理");
    }
}

// ==================== 基础状态清除接口 ====================

void GeoStateManager::clearStateComplete()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_Complete3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：已完成", "状态管理");
    }
}

void GeoStateManager::clearStateInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_Invalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：已失效", "状态管理");
    }
}

void GeoStateManager::clearStateSelected()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_Selected3D;
    
    if (oldState != m_geoState) {
        emit stateDeselected();
        LOG_DEBUG("清除状态：已选中，触发选中状态清除", "状态管理");
    }
}

void GeoStateManager::clearStateEditing()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_Editing3D;
    
    if (oldState != m_geoState) {
        emit editingFinished();
        LOG_DEBUG("清除状态：编辑中", "状态管理");
    }
}

// ==================== 主动更新状态清除接口 ====================

void GeoStateManager::clearParametersUpdated()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_ParametersUpdated3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：参数更新", "状态管理");
    }
}

void GeoStateManager::clearTemporaryPointsUpdated()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_TemporaryPointsUpdated3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：临时点更新", "状态管理");
    }
}

void GeoStateManager::clearControlPointsUpdated()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_ControlPointsInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：控制点更新", "状态管理");
    }
}

// ==================== 被动失效状态清除接口 ====================

void GeoStateManager::clearOctreeInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_OctreeInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：八叉树失效", "状态管理");
    }
}

void GeoStateManager::clearVertexGeometryInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_VertexGeometryInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：顶点几何体失效", "状态管理");
    }
}

void GeoStateManager::clearEdgeGeometryInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_EdgeGeometryInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：边几何体失效", "状态管理");
    }
}

void GeoStateManager::clearFaceGeometryInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_FaceGeometryInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：面几何体失效", "状态管理");
    }
}

void GeoStateManager::clearGeometryInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_GeometryInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：几何体失效", "状态管理");
    }
}



void GeoStateManager::clearBoundingBoxInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_BoundingBoxInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：包围盒失效", "状态管理");
    }
}

void GeoStateManager::clearDisplayListInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_DisplayListInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：显示列表失效", "状态管理");
    }
}

void GeoStateManager::clearTextureInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_TextureInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：纹理失效", "状态管理");
    }
}

void GeoStateManager::clearMaterialInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_MaterialInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：材质失效", "状态管理");
    }
}

void GeoStateManager::clearTransformInvalid()
{
    int oldState = m_geoState;
    m_geoState &= ~GeoState_TransformInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("清除状态：变换失效", "状态管理");
    }
}

// ==================== 批量操作接口 ====================

void GeoStateManager::setAllInvalidStates()
{
    int oldState = m_geoState;
    
    // 设置所有被动失效状态
    m_geoState |= GeoState_OctreeInvalid3D;
    m_geoState |= GeoState_VertexGeometryInvalid3D;
    m_geoState |= GeoState_EdgeGeometryInvalid3D;
    m_geoState |= GeoState_FaceGeometryInvalid3D;
    m_geoState |= GeoState_GeometryInvalid3D;
    m_geoState |= GeoState_BoundingBoxInvalid3D;
    m_geoState |= GeoState_DisplayListInvalid3D;
    m_geoState |= GeoState_TextureInvalid3D;
    m_geoState |= GeoState_MaterialInvalid3D;
    m_geoState |= GeoState_TransformInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("批量设置：所有被动失效状态", "状态管理");
    }
}

void GeoStateManager::clearAllInvalidStates()
{
    int oldState = m_geoState;
    
    // 清除所有被动失效状态
    m_geoState &= ~GeoState_OctreeInvalid3D;
    m_geoState &= ~GeoState_VertexGeometryInvalid3D;
    m_geoState &= ~GeoState_EdgeGeometryInvalid3D;
    m_geoState &= ~GeoState_FaceGeometryInvalid3D;
    m_geoState &= ~GeoState_GeometryInvalid3D;
    m_geoState &= ~GeoState_BoundingBoxInvalid3D;
    m_geoState &= ~GeoState_DisplayListInvalid3D;
    m_geoState &= ~GeoState_TextureInvalid3D;
    m_geoState &= ~GeoState_MaterialInvalid3D;
    m_geoState &= ~GeoState_TransformInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("批量清除：所有被动失效状态", "状态管理");
    }
}

void GeoStateManager::setAllUpdateStates()
{
    int oldState = m_geoState;
    
    // 设置所有主动更新状态
    m_geoState |= GeoState_ParametersUpdated3D;
    m_geoState |= GeoState_TemporaryPointsUpdated3D;
    m_geoState |= GeoState_ControlPointsInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("批量设置：所有主动更新状态", "状态管理");
    }
}

void GeoStateManager::clearAllUpdateStates()
{
    int oldState = m_geoState;
    
    // 清除所有主动更新状态
    m_geoState &= ~GeoState_ParametersUpdated3D;
    m_geoState &= ~GeoState_TemporaryPointsUpdated3D;
    m_geoState &= ~GeoState_ControlPointsInvalid3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("批量清除：所有主动更新状态", "状态管理");
    }
}

// ==================== 状态管理接口 ====================

void GeoStateManager::setState(int state)
{
    int oldState = m_geoState;
    m_geoState = state;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("设置完整状态", "状态管理");
    }
}

void GeoStateManager::reset()
{
    int oldState = m_geoState;
    m_geoState = GeoState_Initialized3D;
    
    if (oldState != m_geoState) {
        LOG_DEBUG("重置状态", "状态管理");
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

// ==================== 缺失的方法实现 ====================

void GeoStateManager::setDisplayListInvalid()
{
    int oldState = m_geoState;
    m_geoState |= GeoState_DisplayListInvalid3D;
    
    if (oldState != m_geoState) {
        emit displayListUpdate();
        LOG_DEBUG("设置状态：显示列表失效，触发显示列表更新", "状态管理");
    }
}


