#pragma once
#pragma execution_character_set("utf-8")

#include "GeometryBase.h"

// 立方体几何体类
class Cube3D_Geo : public Geo3D
{
public:
    Cube3D_Geo();
    virtual ~Cube3D_Geo() = default;

    // 获取立方体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        using namespace constraint;
        static StageDescriptors stageDescriptors
        { 
            // {"确定一条边", 2, 2},
            // {"确定底面", 1, 1, combineStageConstraints({
            //         createConstraintCall(perpendicularToLastTwoPointsConstraint, {{0,0}, {0,1}}),
            //         createConstraintCall(equalLengthConstraint, {{0,0}, {0,1}, {0,1}})
            //     })},
            // {"确定高", 1, 1, combineStageConstraints({
            //         createConstraintCall(perpendicularToLastTwoPointsConstraint, {{0,0}, {0,1}}),
            //         createConstraintCall(perpendicularToLastTwoPointsConstraint, {{1,0}, {0,1}}),
            //         createConstraintCall(equalLengthConstraint, {{0,0}, {0,1}, {0,0}})
            //     })}
            // 第一阶段：确定底面的第一条边AB
            // 第二阶段：从B点出发，确定垂直于AB且等长于AB的点C，形成正方形底面
            // 第三阶段：从A点出发，确定垂直于底面且等长于AB的高度点，完成立方体

            // 因为都等长，两个点就行
            // 几何中心和第一个面中心
            //{"确定立方体", 2, 2},


            // 因为点的确定基于拾取，所有选取的点应该应该在几何表面
            {"确定一条边的轴", 2, 2},
            {"确定方向", 1, 1, combineStageConstraints({
                    createConstraintCall(perpendicularToLastTwoPointsConstraint, {{0,0}, {0,1}}),
                    createConstraintCall(equalLengthConstraint, {{0,0}, {0,1}, {0,1}})
                })}
        };

        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


