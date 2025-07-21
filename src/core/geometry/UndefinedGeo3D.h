#pragma once
#pragma execution_character_set("utf-8")

#include "../GeometryBase.h"

// 未定义几何体类（作为基类或placeholder）
class UndefinedGeo3D_Geo : public Geo3D
{
public:
    UndefinedGeo3D_Geo();
    virtual ~UndefinedGeo3D_Geo() = default;
    
    // ==================== 多阶段绘制支持 ====================
    virtual std::vector<StageDescriptor> getStageDescriptors() const override;
    virtual void mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

protected:
    virtual void buildVertexGeometries() override;
    virtual void buildEdgeGeometries() override;
    virtual void buildFaceGeometries() override;
    virtual void buildStageVertexGeometries(int stage) override;
    virtual void buildStageEdgeGeometries(int stage) override;
    virtual void buildStageFaceGeometries(int stage) override;
    virtual void buildCurrentStagePreviewGeometries() override;
    virtual bool isDrawingComplete() const override;
    virtual bool areControlPointsValid() const override;

private:
    void buildGenericGeometry();
    
private:
    // 占位符成员变量
    std::vector<Point3D> m_points;
};
