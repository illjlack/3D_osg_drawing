#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 贝塞尔曲线几何体类
class BezierCurve3D_Geo : public Geo3D
{
public:
    BezierCurve3D_Geo();
    virtual ~BezierCurve3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取贝塞尔曲线的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ {"确定控制点", 3} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
