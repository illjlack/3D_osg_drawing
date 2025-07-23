#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 未定义几何体类
class UndefinedGeo3D_Geo : public Geo3D
{
public:
    UndefinedGeo3D_Geo();
    virtual ~UndefinedGeo3D_Geo() = default;

    // 获取未定义几何体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{};
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
