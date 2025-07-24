#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 球体几何体类
class Sphere3D_Geo : public Geo3D
{
public:
    Sphere3D_Geo();
    virtual ~Sphere3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取球体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        // 根据球面的四个点确定球
        using namespace constraint;
        static StageDescriptors stageDescriptors
        {
            {"三个点确定球的一个截面", 3, 3},
            {"确定球", 1, 1, }
        };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


