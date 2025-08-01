#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 回型房屋几何体类
class CourtHouse3D_Geo : public Geo3D
{
public:
    CourtHouse3D_Geo();
    virtual ~CourtHouse3D_Geo() = default;

    // 获取回型房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定外围多边形", 4, 8},
            {"确定内围多边形", 3, 6},
            {"确定墙体高度", 1, 1}
        };
        // 第一阶段：确定外围多边形，4-8个顶点形成外围墙体
        // 第二阶段：确定内围多边形，3-6个顶点形成内部中庭边界
        // 第三阶段：确定墙体高度，完成回型房屋结构
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 