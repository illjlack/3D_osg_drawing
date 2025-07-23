#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 立方体几何体类
class Cube3D_Geo : public Geo3D
{
public:
    Cube3D_Geo();
    virtual ~Cube3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取立方体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            //{"确定底面第一个角点", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定底面对角点", 1, 1, ConstraintSystem::flatDrawingConstraint()},
            //{"确定立方体高度", 1, 1, ConstraintSystem::verticalToBaseConstraint}
        };
        // 第一阶段：确定底面的第一个角点，使用平面约束
        // 第二阶段：确定底面的对角点，保持在同一平面，形成正方形底面
        // 第三阶段：确定立方体的高度，垂直于底面方向（边长等于底面边长）
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
