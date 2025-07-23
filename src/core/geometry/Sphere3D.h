#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 球体几何体类
class Sphere3D_Geo : public Geo3D
{
public:
    Sphere3D_Geo();
    virtual ~Sphere3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取球体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            //{"确定球心", 1, 1, ConstraintSystem::noConstraint},
            //{"确定半径", 1, 1, ConstraintSystem::noConstraint} 
        };
        // 第一阶段：确定球心，无约束
        // 第二阶段：确定球面上一点以确定半径，无约束
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
