#pragma once
#pragma execution_character_set("utf-8")

#include "GeometryBase.h"

// 人字形房屋几何体类
class GableHouse3D_Geo : public Geo3D
{
public:
    GableHouse3D_Geo();
    virtual ~GableHouse3D_Geo() = default;

    // 获取人字形房屋的阶段描述符
    virtual const StageDescriptors& getStageDescriptors() const
    {
        static StageDescriptors stageDescriptors
        { 
            {"底面ABCD", 4, 4},
            {"屋脊E", 1, 1},
            {"屋脊F", 1, 1}
        };
        // 第一阶段, ABCD连接成四边形
        // 第二阶段, ABCD连接到E
        // 第三阶段, CD连接到F
        return stageDescriptors;
    }

protected:
    //不需要顶点
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;

private:
    
private:
};

