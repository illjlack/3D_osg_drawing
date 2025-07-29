#pragma once
#pragma execution_character_set("utf-8")

#include "GeometryBase.h"
#include "util/MathUtils.h"

// 四边形几何体类
class Quad3D_Geo : public Geo3D
{
public:
    Quad3D_Geo();
    virtual ~Quad3D_Geo() = default;

    // 获取四边形的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        using namespace constraint;
        static StageDescriptors stageDescriptors
        { 
            {"确定四边形顶点", 4, 4, createConstraintCall(planeConstraint, {{0,0}, {0,1},{0,2}})}
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


