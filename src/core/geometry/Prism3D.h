#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 棱柱几何体类
class Prism3D_Geo : public Geo3D
{
public:
    Prism3D_Geo();
    virtual ~Prism3D_Geo() = default;

    // 获取棱柱的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ {"确定底面", 3},{"确定高度", 1, 1} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 