#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../ConstraintSystem.h"

// 三角形几何体类
class Triangle3D_Geo : public Geo3D
{
public:
    Triangle3D_Geo();
    virtual ~Triangle3D_Geo() = default;

    // 获取三角形的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定三角形顶点", 3, 3} 
        };
        // 单阶段：确定三个顶点
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


