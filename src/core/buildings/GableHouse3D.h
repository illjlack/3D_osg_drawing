#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 人字形房屋几何体类
class GableHouse3D_Geo : public Geo3D
{
public:
    GableHouse3D_Geo();
    virtual ~GableHouse3D_Geo() = default;

    // 获取人字形房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            //{"确定基座第一角点", 1, 1, ConstraintSystem::flatDrawingConstraint},
            //{"确定基座对角点", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定房屋高度", 1, 1, ConstraintSystem::verticalToBaseConstraint()},
            //{"确定屋脊高度", 1, 1, ConstraintSystem::verticalToBaseConstraint()} 
        };
        // 第一阶段：确定基座的第一个角点，使用平面约束
        // 第二阶段：确定基座的对角点，保持在同一平面，形成矩形基座
        // 第三阶段：确定房屋墙体高度，垂直于基座平面
        // 第四阶段：确定屋脊高度，垂直于基座平面，形成人字形屋顶
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};