#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 人字形房屋几何体类
class GableHouse3D_Geo : public Geo3D
{
public:
    GableHouse3D_Geo();
    virtual ~GableHouse3D_Geo() = default;

    // 获取人字形房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ {"确定基座范围", 2, 2},{"确定房屋高度", 1},{"确定屋顶高度", 1} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};