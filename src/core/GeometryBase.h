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

// 三维几何对象基类
class Geo3D : public QObject, public osg::Group
{
    Q_OBJECT
public:
    Geo3D();
    virtual ~Geo3D();

    // 反射类型
    GeoType3D getGeoType() const { return m_geoType; }
    void setGeoType(GeoType3D type) { m_geoType = type; }

    // 场景图管理方法
    void setParentNode(osg::Group* parentNode);
    void removeFromParent();
    osg::Group* getParentNode() const;
    bool isInScene() const;

    // 管理器直接访问接口(mm_开头：成员、管理器)
    /**
    * 控制点管理器负责绘制流程,修改
    * 节点管理器负责osg节点管理
    * 状态管理器负责阶段信号
    * 渲染管理器负责参数颜色材质的渲染
    */
    GeoStateManager*        mm_state() const { return m_stateManager.get(); }
    GeoNodeManager*         mm_node() const { return m_nodeManager.get(); }
    GeoRenderManager*       mm_render() const { return m_renderManager.get(); }
    GeoControlPointManager* mm_controlPoint() const { return m_controlPointManager.get(); }

    // 参数设置
    const GeoParameters3D& getParameters() const { return m_parameters; }
    void setParameters(const GeoParameters3D& params);
    
    // 序列化/反序列化参数描述 (用于文件保存/加载)
    virtual QString serialize() const;           // 序列化对象数据为字符串
    virtual bool deserialize(const QString& data); // 从字符串反序列化对象数据
    
    // 从文件加载完整恢复对象状态（按正确顺序：参数->节点->控制点->渲染）
    void restoreFromFileNode(osg::ref_ptr<osg::Node> node);
    
    // =============================== 控制点绘制相关 ==============================
    // 获取该几何图形的阶段描述符
    // 控制点管理器依据这个控制绘制阶段
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{};
        return stageDescriptors;
    }

protected:
    
    friend class GeoNodeManager;
    // 初始化
    virtual void initialize();
    // 点线面节点管理辅助方法
    virtual void buildControlPointGeometries();         // 子类实现具体的控制点构建(默认选择时所有控制点可见)
    virtual void buildVertexGeometries() = 0;           // 子类实现具体的顶点几何体构建
    virtual void buildEdgeGeometries() = 0;             // 子类实现具体的边几何体构建
    virtual void buildFaceGeometries() = 0;             // 子类实现具体的面几何体构建
    
protected:
    // 基本属性
    GeoType3D m_geoType;
    GeoParameters3D m_parameters;
    bool m_parametersChanged;
    // 管理器组件
    std::unique_ptr<GeoStateManager> m_stateManager;
    std::unique_ptr<GeoNodeManager> m_nodeManager;
    std::unique_ptr<GeoControlPointManager> m_controlPointManager;
    std::unique_ptr<GeoRenderManager> m_renderManager;

    // 创建管理器和连接信号
    void setupManagers();
    void connectManagerSignals();
};

