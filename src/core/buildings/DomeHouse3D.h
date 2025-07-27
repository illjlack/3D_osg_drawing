#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 穹顶房屋几何体类
class DomeHouse3D_Geo : public Geo3D
{
public:
    DomeHouse3D_Geo();
    virtual ~DomeHouse3D_Geo() = default;

    // 获取穹顶房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定基座", 3, 3},
            {"确定穹顶高度", 1, 1} 
        };
        // 第一阶段：确定圆形基座,三点确定一个圆
        // 第三阶段：确定穹顶高度，垂直于基座平面，形成半球形穹顶
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};

