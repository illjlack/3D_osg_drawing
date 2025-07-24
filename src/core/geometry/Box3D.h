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
        using namespace constraint;
        static StageDescriptors stageDescriptors = 
        {
            {"确定一条边", 2, 2},
            {"确定底面", 1, 1, createConstraintCall(perpendicularToLastTwoPointsConstraint, {{0,0}, {0,1}})},
            {"确定高", 1, 1, combineStageConstraints({
                    createConstraintCall(perpendicularToLastTwoPointsConstraint, {{0,0}, {0,1}}),
                    createConstraintCall(perpendicularToLastTwoPointsConstraint, {{1,0}, {0,1}})
                })}
        };
        // 第一阶段：确定底面的第一个角点，使用平面约束
        // 第二阶段：确定底面的对角点，保持在同一平面，形成矩形底面
        // 第三阶段：确定长方体的高度，垂直于两个底边切垂足为0,1
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


