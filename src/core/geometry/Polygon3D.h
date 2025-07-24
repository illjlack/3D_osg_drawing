#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../../util/MathUtils.h"
#include "../../util/MathUtils.h"

// 多边形几何体类
class Polygon3D_Geo : public Geo3D
{
public:
    Polygon3D_Geo();
    virtual ~Polygon3D_Geo() = default;

    // 获取多边形的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        using namespace constraint;
        static StageDescriptors stageDescriptors
        { 
            {"确定多边形顶点", 3, INT_INF, createConstraintCall(planeConstraint, {{0,0}, {0,1},{0,2}})}
        };
        // 单阶段：至少确定三个顶点，可以添加更多顶点，使用平面约束保证所有点共面
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


