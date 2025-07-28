#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 圆柱体几何体类
class Cylinder3D_Geo : public Geo3D
{
public:
    Cylinder3D_Geo();
    virtual ~Cylinder3D_Geo() = default;

    // 获取圆柱体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        using namespace constraint;
        static StageDescriptors stageDescriptors
        { 
            {"确定底面圆", 3, 3},
            {"确定高", 1, 1, createConstraintCall(perpendicularToLastTwoPointsConstraint, {{0,0}, {0,1}, {0,2}})}
        };
        // 第一阶段：基于圆上的三个点确定圆
        // 第二阶段：确定高，垂直于三点确定的圆平面（直接使用平面法向量约束）
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


