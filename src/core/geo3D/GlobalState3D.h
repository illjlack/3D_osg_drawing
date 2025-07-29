#pragma once
#pragma execution_character_set("utf-8")

#include "Enums3D.h"
#include <glm/glm.hpp>

namespace Geo3D {

// 全局状态变量（应用程序级别的状态）
extern DrawMode3D GlobalDrawMode3D;
extern PointShape3D GlobalPointShape3D;
extern double GlobalPointSize3D;
extern glm::dvec3 GlobalPointColor3D;

extern LineStyle3D GlobalLineStyle3D;
extern double GlobalLineWidth3D;
extern glm::dvec3 GlobalLineColor3D;
extern double GlobalLineDashPattern3D;

extern FillType3D GlobalFillType3D;
extern glm::dvec3 GlobalFillColor3D;

extern MaterialType3D GlobalMaterialType3D;
extern SubdivisionLevel3D GlobalSubdivisionLevel3D;

// 显示控制全局变量
extern bool GlobalShowPoints3D;
extern bool GlobalShowEdges3D;
extern bool GlobalShowFaces3D;

// 初始化全局设置
void initializeGlobal3DSettings();

} // namespace Geo3D 