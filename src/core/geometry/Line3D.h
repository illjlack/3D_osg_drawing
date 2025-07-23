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
        static StageDescriptors stageDescriptors{ {"至少两个点", 2} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
