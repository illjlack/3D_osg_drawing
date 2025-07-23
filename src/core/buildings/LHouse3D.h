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
        static StageDescriptors stageDescriptors{ {"确定主体基座", 2, 2},{"确定扩展部分", 1} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
