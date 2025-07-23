#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// L型房屋几何体类
class LHouse3D_Geo : public Geo3D
{
public:
    LHouse3D_Geo();
    virtual ~LHouse3D_Geo() = default;

    // 获取L型房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            //{"确定主体基座第一角点", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定主体基座对角点", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定扩展部分位置", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定房屋高度", 1, 1, ConstraintSystem::verticalToBaseConstraint()} 
        };
        // 第一阶段：确定主体矩形基座的第一个角点，使用平面约束
        // 第二阶段：确定主体基座的对角点，保持在同一平面，形成主体矩形
        // 第三阶段：确定扩展部分的位置，保持在同一平面，形成L型基座
        // 第四阶段：确定整个L型房屋的高度，垂直于基座平面
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
