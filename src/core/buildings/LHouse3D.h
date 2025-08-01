#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// L型房屋几何体类
class LHouse3D_Geo : public Geo3D
{
public:
    LHouse3D_Geo();
    virtual ~LHouse3D_Geo() = default;

    // 获取L型房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定第一个顶点", 1, 1},
            {"确定第二个顶点", 1, 1},
            {"确定第三个顶点", 1, 1},
            {"确定第四个顶点", 1, 1},
            {"确定第五个顶点", 1, 1},
            {"确定第六个顶点", 1, 1},
            {"确定墙体高度", 1, 1}
        };
        // 第一阶段：确定L型基座的第一个顶点，使用平面约束
        // 第二阶段：确定L型基座的第二个顶点，保持在同一平面
        // 第三阶段：确定L型基座的第三个顶点，保持在同一平面
        // 第四阶段：确定L型基座的第四个顶点，保持在同一平面
        // 第五阶段：确定L型基座的第五个顶点，保持在同一平面
        // 第六阶段：确定L型基座的第六个顶点，保持在同一平面，形成L型基座
        // 第七阶段：确定墙体高度，垂直于基座平面，完成L型房屋结构
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


