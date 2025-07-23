#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 圆柱体几何体类
class Cylinder3D_Geo : public Geo3D
{
public:
    Cylinder3D_Geo();
    virtual ~Cylinder3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取圆柱体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ {"确定底面直径", 2, 2},{"确定高度", 1} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
