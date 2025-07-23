#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../ConstraintSystem.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 圆弧几何体类
class Arc3D_Geo : public Geo3D
{
public:
    Arc3D_Geo();
    virtual ~Arc3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取圆弧的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"圆弧", 3, INT_INF, ConstraintSystem::noConstraint}
        };
        // 前三个点确定圆弧, 后面的每个点和前面一个点和它的往前一个小的增量的点，三点确定圆弧
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 