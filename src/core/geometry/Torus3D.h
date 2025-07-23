#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 环面几何体类
class Torus3D_Geo : public Geo3D
{
public:
    Torus3D_Geo();
    virtual ~Torus3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取环面的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ {"确定中心点", 1, 1},{"确定主半径", 1},{"确定次半径", 1} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
