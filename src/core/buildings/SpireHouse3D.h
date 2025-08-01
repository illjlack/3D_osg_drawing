#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 尖顶房屋几何体类
class SpireHouse3D_Geo : public Geo3D
{
public:
    SpireHouse3D_Geo();
    virtual ~SpireHouse3D_Geo() = default;

    // 获取尖顶房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定第一个角点", 1, 1},
            {"确定第二个角点", 1, 1},
            {"确定第三个角点", 1, 1},
            {"确定第四个角点", 1, 1},
            {"确定尖顶位置", 1, 1},
            {"确定墙体高度", 1, 1}
        };
        // 第一阶段：确定基座的第一个角点，使用平面约束
        // 第二阶段：确定基座的第二个角点，保持在同一平面
        // 第三阶段：确定基座的第三个角点，保持在同一平面
        // 第四阶段：确定基座的第四个角点，保持在同一平面，形成四边形基座
        // 第五阶段：确定尖顶位置，垂直于基座平面
        // 第六阶段：确定墙体高度，完成尖顶房屋结构
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};

