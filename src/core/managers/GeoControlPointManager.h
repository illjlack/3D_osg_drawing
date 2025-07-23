#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <vector>
#include <string>

// 前向声明
class Geo3D;

#ifndef INT_INF
#define INT_INF 0x3f3f3f3f
#endif

struct StageDescriptor 
{
    std::string stageName;      // 阶段名称
    int minControlPoints;       // 该阶段最少控制点数量
    int maxControlPoints;       // 该阶段最多控制点数量
    
    StageDescriptor() : minControlPoints(1), maxControlPoints(1) {}
    StageDescriptor(const std::string& name, int minPoints, int maxPoints = INT_INF)
        : stageName(name), minControlPoints(minPoints), maxControlPoints(maxPoints) 
    {
        // 至少能容纳一个
        assert(maxControlPoints >= 1);
    }
};

typedef std::vector<StageDescriptor> StageDescriptors;


class GeoControlPointManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoControlPointManager(Geo3D* parent);
    ~GeoControlPointManager() = default;

    /**
    * 提供的关于控制点的操作
    * 1.添加控制点（自动切换阶段，如果无上限或者有范围可能需要手动切换阶段）
    * 2.撤销上一控制点（可能要回到上一阶段，空则无操作）
    * 3.进入下一阶段（自动验证上一阶段是否完成，未完成则析构）
    * 4.移动临时点（临时点在整个绘制阶段都存在，预览用）
    * 5.修改控制点（这里的数据是其他地方的参考，需要这里修改然后通知其它地方，对外只有一个下标编号，里面按顺序排列）
    * 6.获得所有控制点（二维的两种）
    */
    
    // 1. 添加控制点（自动切换阶段）
    bool addControlPoint(const Point3D& point);
    
    // 2. 撤销上一控制点（可能回到上一阶段）
    bool undoLastControlPoint();
    
    // 3. 进入下一阶段（自动验证上一阶段是否完成）
    bool nextStage();
    
    // 4. 移动临时点（预览用）
    void setTempPoint(const Point3D& point);
    
    // 5. 修改控制点（通过全局索引）
    bool setControlPoint(int globalIndex, const Point3D& point);
    
    // 6. 获得所有控制点
    const std::vector<std::vector<Point3D>>& getAllStageControlPoints();

signals:
    void controlPointChanged();

private:

    typedef std::vector<Point3D> ControlPoints;
    typedef std::vector<ControlPoints> Stages;
    inline std::size_t stageSize()
    {
        return m_stages.size();
    }
    inline ControlPoints& currentStage()
    {
        return m_stages.back();
    }
    inline std::size_t currentStageIdx()
    {
        return m_stages.size() - 1;
    }
    inline std::size_t currentStagePointIdx()
    {
        return currentStage().size() - 1;
    }
    inline std::size_t currentStagePointSize()
    {
        return currentStage().size();
    }

    const StageDescriptors& getStageDescriptors() const;
    const StageDescriptor& getStageDescriptor(int idx) const;
    GeoStateManager* getState() const;

private:
    Geo3D* m_parent;
 
    Stages m_stages;
    Stages m_stagesTemp;
    Point3D m_tempPoint;
};
