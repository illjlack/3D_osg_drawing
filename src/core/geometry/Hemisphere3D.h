#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 半球几何体类
class Hemisphere3D_Geo : public Geo3D
{
public:
    Hemisphere3D_Geo();
    virtual ~Hemisphere3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取半球的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ {"确定球心", 1, 1},{"确定半径", 1} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 