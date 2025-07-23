#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 长方体几何体类
class Box3D_Geo : public Geo3D
{
public:
    Box3D_Geo();
    virtual ~Box3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取长方体的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors{ {"确定第一个角点", 1, 1},{"确定对角点", 1} };
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};
