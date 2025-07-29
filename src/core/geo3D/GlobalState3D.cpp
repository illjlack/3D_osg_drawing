#include "GlobalState3D.h"

namespace Geo3D {

// 全局状态变量（应用程序级别的状态）
DrawMode3D GlobalDrawMode3D = DrawMode3D::DRAW_SOLID;
PointShape3D GlobalPointShape3D = PointShape3D::POINT_CIRCLE;
double GlobalPointSize3D = 5.0;
glm::dvec3 GlobalPointColor3D(1.0, 0.0, 0.0);  // 红色

LineStyle3D GlobalLineStyle3D = LineStyle3D::LINE_SOLID;
double GlobalLineWidth3D = 1.0;
glm::dvec3 GlobalLineColor3D(0.0, 0.0, 0.0);  // 黑色
double GlobalLineDashPattern3D = 1.0;

FillType3D GlobalFillType3D = FillType3D::FILL_SOLID;
glm::dvec3 GlobalFillColor3D(0.8, 0.8, 0.8);  // 浅灰色

MaterialType3D GlobalMaterialType3D = MaterialType3D::MATERIAL_DEFAULT;
SubdivisionLevel3D GlobalSubdivisionLevel3D = SubdivisionLevel3D::SUBDIVISION_LOW;

// 显示控制全局变量
bool GlobalShowPoints3D = true;
bool GlobalShowEdges3D = true;
bool GlobalShowFaces3D = true;

void initializeGlobal3DSettings() {
    // 重置所有全局变量为默认值
    GlobalDrawMode3D = DrawMode3D::DRAW_SOLID;
    GlobalPointShape3D = PointShape3D::POINT_CIRCLE;
    GlobalPointSize3D = 5.0;
    GlobalPointColor3D = glm::dvec3(1.0, 0.0, 0.0);

    GlobalLineStyle3D = LineStyle3D::LINE_SOLID;
    GlobalLineWidth3D = 1.0;
    GlobalLineColor3D = glm::dvec3(0.0, 0.0, 0.0);
    GlobalLineDashPattern3D = 1.0;

    GlobalFillType3D = FillType3D::FILL_SOLID;
    GlobalFillColor3D = glm::dvec3(0.8, 0.8, 0.8);

    GlobalMaterialType3D = MaterialType3D::MATERIAL_DEFAULT;
    GlobalSubdivisionLevel3D = SubdivisionLevel3D::SUBDIVISION_LOW;

    GlobalShowPoints3D = true;
    GlobalShowEdges3D = true;
    GlobalShowFaces3D = true;
}

} // namespace Geo3D 