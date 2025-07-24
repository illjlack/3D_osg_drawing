#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../../util/MathUtils.h"

// 棱柱几何体类
class Prism3D_Geo : public Geo3D
{
public:
    Prism3D_Geo();
    virtual ~Prism3D_Geo() = default;

    // 获取棱柱的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
         { 
            //{"确定底面", 3, INT_INF, ConstraintSystem::flatDrawingConstraint()},
            //{"确定高度", 1, 1, ConstraintSystem::volumeConstraint()} 
        };
        // 第一阶段使用2D平面绘制约束，确保底面在同一平面
        // 第二阶段使用3D立体约束，确保高度点在垂直方向上
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 

