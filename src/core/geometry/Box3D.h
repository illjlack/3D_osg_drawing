#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../ConstraintSystem.h"

// 长方体几何体类
class Box3D_Geo : public Geo3D
{
public:
    Box3D_Geo();
    virtual ~Box3D_Geo() = default;

    // 获取长方体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            //{"确定一条边", 2, 2, ConstraintSystem::noConstraint},
            //{"确定底面", 1, 1, ConstraintSystem::perpendicularToLastTwoPointsConstraint},
            //{"确定长方体高度", 1, 1, ConstraintSystem::verticalToBaseConstraint} 
        };
        // 第一阶段：确定底面的第一个角点，使用平面约束
        // 第二阶段：确定底面的对角点，保持在同一平面，形成矩形底面
        // 第三阶段：确定长方体的高度，垂直于底面方向
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
