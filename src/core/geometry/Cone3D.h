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

    // 获取圆锥的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        using namespace constraint;
        static StageDescriptors stageDescriptors
        { 
            {"确定底面圆心半径", 2, 2},
            {"确定底面", 1, 1, createConstraintCall(circleConstraint, {{0,0}, {0,1}})},
            {"确定锥体顶点", 1, 1, createConstraintCall(perpendicularToCirclePlaneConstraint, {{0,0}, {0,1}, {1,0}})}
        };
        return stageDescriptors;
    }

protected:
    // 第一阶段：确定底面圆心半径 (1-2个点)
    //   - 点A：圆心
    //   - 点B：圆周上的点（确定半径）
    //   顶点显示：显示圆心点A，如果有点B则同时显示点B
    //   边线显示：如果有两个点，绘制从圆心A到半径点B的线段
    //   面几何：无
    // 
    // 第二阶段：确定底面 (第一阶段2个点 + 第二阶段1个点)
    //   - 点A：圆心（stage1[0]）
    //   - 点B：圆周上的点（stage1[1]）
    //   - 点C：圆周上的点（stage2[0]）
    //   顶点显示：只显示圆心点A
    //   边线显示：绘制完整圆周线（处理三点共线情况：A-B和A-C共线时退化为直线）
    //   面几何：绘制底面圆形（三角形扇形）
    // 
    // 第三阶段：确定锥体顶点 (前面确定的圆 + 锥顶点)
    //   - 圆：由A、B、C三点确定（A为圆心）
    //   - 点D：锥顶点（stage3[0]）
    //   顶点显示：只显示锥顶点D
    //   边线显示：绘制圆周线（不绘制母线）
    //   面几何：绘制完整圆锥（底面圆形 + 侧面三角形）
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
};
