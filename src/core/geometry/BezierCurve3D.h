#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../ConstraintSystem.h"

// 贝塞尔曲线几何体类
class BezierCurve3D_Geo : public Geo3D
{
public:
    BezierCurve3D_Geo();
    virtual ~BezierCurve3D_Geo() = default;

    // 获取贝塞尔曲线的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
         static StageDescriptors stageDescriptors
         { 
             //{"绘制贝塞尔曲线", 2, INT_INF, ConstraintSystem::noConstraint} 
         };
        // 单阶段：至少确定两个控制点，可以添加更多控制点
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
