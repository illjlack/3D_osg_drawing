#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 尖顶房屋几何体类
class SpireHouse3D_Geo : public Geo3D
{
public:
    SpireHouse3D_Geo();
    virtual ~SpireHouse3D_Geo() = default;

    // 获取尖顶房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            //{"确定基座第一角点", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定基座对角点", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定房屋高度", 1, 1, ConstraintSystem::verticalToBaseConstraint()},
            //{"确定尖顶高度", 1, 1, ConstraintSystem::verticalToBaseConstraint()} 
        };
        // 第一阶段：确定基座的第一个角点，使用平面约束
        // 第二阶段：确定基座的对角点，保持在同一平面，形成矩形基座
        // 第三阶段：确定房屋墙体高度，垂直于基座平面
        // 第四阶段：确定尖顶高度，垂直于基座平面，形成锥形尖顶
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};

