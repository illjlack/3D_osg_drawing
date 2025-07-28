#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../../util/MathUtils.h"

// 棱柱几何体类
class Prism3D_Geo : public Geo3D
{
public:
    Prism3D_Geo();
    virtual ~Prism3D_Geo() = default;

    // 获取棱柱的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        using namespace constraint;
        static StageDescriptors stageDescriptors
         { 
            {"确定多边形顶点", 3, INT_INF, createConstraintCall(planeConstraint, {{0,0}, {0,1},{0,2}})},
            {"确定高", 1, 1, createConstraintCall(perpendicularToLastTwoPointsConstraint, {{0,0}, {0,1}, {0,2}})}
        };
        // 第一阶段使用2D平面绘制约束，确保底面在同一平面
        // 第二阶段：确定高，垂直于三点确定的底面平面（直接使用平面法向量约束）
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 

