#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 圆锥体几何体类
class Cone3D_Geo : public Geo3D
{
public:
    Cone3D_Geo();
    virtual ~Cone3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取圆锥的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            //{"确定底面圆心", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定底面半径", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定锥体顶点", 1, 1, ConstraintSystem::verticalToBaseConstraint()} 
        };
        // 第一阶段：确定底面圆心，使用平面约束
        // 第二阶段：确定半径，保持在同一平面
        // 第三阶段：确定顶点位置，约束在垂直于底面的直线上
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
