#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 平顶房屋几何体类
class FlatHouse3D_Geo : public Geo3D
{
public:
    FlatHouse3D_Geo();
    virtual ~FlatHouse3D_Geo() = default;

    // 获取平顶房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定基座第一角点", 1, 1},
            {"确定基座对角点", 1, 1},
            {"确定房屋高度", 1, 1} 
        };
        // 第一阶段：确定基座的第一个角点，使用平面约束
        // 第二阶段：确定基座的对角点，保持在同一平面，形成矩形基座
        // 第三阶段：确定房屋高度，垂直于基座平面
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};

