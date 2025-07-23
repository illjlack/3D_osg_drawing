#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"
#include "../../util/MathUtils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 圆弧几何体类
class Arc3D_Geo : public Geo3D
{
public:
    Arc3D_Geo();
    virtual ~Arc3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取圆弧的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ {"确定两个起始点", 2, 2},{"点与前面的点构成圆弧", 1}};
        // 第三个点与前两个构成圆弧，之后的点与前一个点和前一个弧段上最近的一个点构成圆弧
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 