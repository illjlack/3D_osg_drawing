#pragma once
#pragma execution_character_set("utf-8")

#include "Common3D.h"
#include "managers/GeoStateManager.h"
#include "managers/GeoNodeManager.h"
#include "managers/GeoRenderManager.h"

#include "managers/GeoControlPointManager.h"
#include "managers/GeoRenderManager.h"
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
#include <QObject>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <algorithm>

// 前向声明
class Geo3D;

// 几何对象工厂
Geo3D* createGeo3D(DrawMode3D mode);

// 三维几何对象基类
class Geo3D : public QObject, public osg::Referenced
{
    Q_OBJECT

public:
    Geo3D();
    virtual ~Geo3D();

    // 反射类型
    GeoType3D getGeoType() const { return m_geoType; }
    void setGeoType(GeoType3D type) { m_geoType = type; }

    // 管理器直接访问接口(mm_开头：成员、管理器)
    GeoStateManager*        mm_state() const { return m_stateManager.get(); }
    GeoNodeManager*         mm_node() const { return m_nodeManager.get(); }
    GeoRenderManager*       mm_render() const { return m_renderManager.get(); }
    GeoControlPointManager* mm_controlPoint() const { return m_controlPointManager.get(); }

    // 参数设置
    const GeoParameters3D& getParameters() const { return m_parameters; }
    void setParameters(const GeoParameters3D& params);

    // ==================== 多阶段绘制接口 ====================
    
    // 获取该几何图形的阶段描述符（由派生类实现）
    virtual std::vector<StageDescriptor> getStageDescriptors() const = 0;
    
    // 当前阶段相关方法
    int getCurrentStage() const;
    bool nextStage(); // 进入下一阶段
    bool canAdvanceToNextStage() const;
    bool isCurrentStageComplete() const;
    bool isAllStagesComplete() const;
    const StageDescriptor* getCurrentStageDescriptor() const;
    
    // 阶段切换和验证
    bool validateCurrentStage() const; // 验证当前阶段的控制点是否有效
    void initializeStages(); // 初始化阶段描述符（在构造函数中调用）

    // 事件处理
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

    // 绘制完成检查和控制点验证
    virtual bool isDrawingComplete() const = 0;  // 检查是否绘制完成（检查控制点个数是否符合要求）
    virtual bool areControlPointsValid() const = 0;  // 检查控制点是否合法
    
    // 绘制完成检查和信号发送
    void checkAndEmitDrawingComplete();

    void updateGeometries();

protected:
    // 初始化
    virtual void initialize();
    
    // 点线面节点管理辅助方法
    virtual void buildVertexGeometries() = 0;  // 子类实现具体的顶点几何体构建
    virtual void buildEdgeGeometries() = 0;    // 子类实现具体的边几何体构建
    virtual void buildFaceGeometries() = 0;    // 子类实现具体的面几何体构建
    
    // ==================== 多阶段几何构建方法 ====================
    
    // 为不同阶段构建特定的几何体（由派生类重写以实现阶段特定的绘制逻辑）
    virtual void buildStageVertexGeometries(int stage) {} // 可选：阶段特定的顶点几何体构建
    virtual void buildStageEdgeGeometries(int stage) {}   // 可选：阶段特定的边几何体构建  
    virtual void buildStageFaceGeometries(int stage) {}   // 可选：阶段特定的面几何体构建
    
    // ==================== 多阶段临时点跟踪绘制 ====================
    
    // 构建当前阶段的临时预览几何体（包含临时点的预览绘制）
    virtual void buildCurrentStagePreviewGeometries() {} // 可选：当前阶段的预览绘制

protected:
    // 基本属性
    GeoType3D m_geoType;
    GeoParameters3D m_parameters;
    bool m_parametersChanged;
    bool m_stagesInitialized; // 阶段描述符是否已初始化

    // 管理器组件
    std::unique_ptr<GeoStateManager> m_stateManager;
    std::unique_ptr<GeoNodeManager> m_nodeManager;
    std::unique_ptr<GeoControlPointManager> m_controlPointManager;
    std::unique_ptr<GeoRenderManager> m_renderManager;

    // 创建管理器和连接信号
    void setupManagers();
    void connectManagerSignals();
};