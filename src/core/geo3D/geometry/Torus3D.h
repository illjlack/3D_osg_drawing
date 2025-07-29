#pragma once
#pragma execution_character_set("utf-8")

#include "GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 环面几何体类
class Torus3D_Geo : public Geo3D
{
public:
    Torus3D_Geo();
    virtual ~Torus3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取环面的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        using namespace constraint;
        static StageDescriptors stageDescriptors
        { 
            {"确定环面轴线", 2, 2},
            {"确定主圆", 1, 1, createConstraintCall(planeConstraint, {{0,0}, {0,1}, {1,0}})},
            {"确定内圆半径", 1, 1, createConstraintCall(planeConstraint, {{0,0}, {0,1}, {1,0}})} 
        };
        // 第一阶段：确定环面轴线（2个点定义轴线和主半径）
        // 第二阶段：确定主圆（第3个点与前两点确定平面，绘制主圆）
        // 第三阶段：确定内圆半径（第4个点确定内环半径，绘制完整圆环）
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


