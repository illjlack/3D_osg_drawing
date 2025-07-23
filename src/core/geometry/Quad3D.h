#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../../util/MathUtils.h"

// 四边形几何体类
class Quad3D_Geo : public Geo3D
{
public:
    Quad3D_Geo();
    virtual ~Quad3D_Geo() = default;

    // 获取四边形的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ 
            {"确定四个顶点", 4, 4, ConstraintSystem::flatDrawingConstraint()} 
        };
        // 使用Z平面约束，保持所有顶点在同一平面上
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
