#pragma once
#pragma execution_character_set("utf-8")

#include "GeometryBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 椭球几何体类
class Ellipsoid3D_Geo : public Geo3D
{
public:
    Ellipsoid3D_Geo();
    virtual ~Ellipsoid3D_Geo() = default;

    // ==================== 多阶段绘制支持 ====================
    
    // 获取椭球的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"确定椭球中心", 1, 1},
            {"确定第一轴端点", 1, 1},
            {"确定第二轴端点", 1, 1},
            {"确定第三轴端点", 1, 1} 
        };
        // 第一阶段：确定椭球中心，无约束
        // 第二阶段：确定第一轴（长轴）的端点，无约束
        // 第三阶段：确定第二轴端点，构成椭球的三个轴
        // 第四阶段：确定第三轴端点，完成椭球定义
        return stageDescriptors;
    }

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
}; 

