#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 点几何体类
class Point3D_Geo : public Geo3D
{
public:
    Point3D_Geo();
    virtual ~Point3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取点的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定点位置", 1, 1} 
        };
        // 单阶段：确定点的位置，无约束
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};


