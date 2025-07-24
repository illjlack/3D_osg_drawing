#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 半球几何体类
class Hemisphere3D_Geo : public Geo3D
{
public:
    Hemisphere3D_Geo();
    virtual ~Hemisphere3D_Geo() = default;
 
    // 获取半球的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            //{"确定底面圆心", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定半径", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定半球朝向", 1, 1, ConstraintSystem::verticalToBaseConstraint()} 
        };
        // 第一阶段：确定底面圆心，使用平面约束
        // 第二阶段：确定半径，保持在同一平面
        // 第三阶段：确定半球的朝向（向上或向下），垂直于底面
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 

