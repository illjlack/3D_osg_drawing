#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 线几何体类
class Line3D_Geo : public Geo3D
{
public:
    Line3D_Geo();
    virtual ~Line3D_Geo() = default;

    // 获取线的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定线段端点", 2, INT_INF} 
        };
        // 单阶段：至少确定两个端点，可以添加更多点形成折线，无约束
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


