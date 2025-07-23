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
        static StageDescriptors stageDescriptors{ {"确定基座范围", 2, 2},{"确定房屋高度", 1},{"确定尖顶高度", 1} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};