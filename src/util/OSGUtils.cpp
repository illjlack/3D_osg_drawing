#include "OSGUtils.h"
#include <osg/Array>
#include <osg/BoundingBox>
#include <osg/BoundingSphere>
#include <osg/Material>
#include <osg/StateSet>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/PolygonMode>
#include <osg/NodeVisitor>
#include <osg/Geode>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Drawable>
#include <osg/PrimitiveSet>
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 类型转换工具

osg::Vec3 OSGUtils::glmToOsg(const glm::dvec3& vec)
{
    return osg::Vec3(static_cast<double>(vec.x), static_cast<double>(vec.y), static_cast<double>(vec.z));
}

glm::dvec3 OSGUtils::osgToGlm(const osg::Vec3& vec)
{
    return glm::dvec3(vec.x(), vec.y(), vec.z());
}

// 创建基本几何体

osg::ref_ptr<osg::Geometry> OSGUtils::createPoint(const glm::dvec3& position)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    vertices->push_back(glmToOsg(position));
    
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, 1));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createLine(const glm::dvec3& start, const glm::dvec3& end)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    vertices->push_back(glmToOsg(start));
    vertices->push_back(glmToOsg(end));
    
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createTriangle(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    vertices->push_back(glmToOsg(v1));
    vertices->push_back(glmToOsg(v2));
    vertices->push_back(glmToOsg(v3));
    
    osg::Vec3 normal = calculateTriangleNormal(glmToOsg(v1), glmToOsg(v2), glmToOsg(v3));
    for (int i = 0; i < 3; i++) {
        normals->push_back(normal);
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 3));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createQuad(const glm::dvec3& v1, const glm::dvec3& v2, const glm::dvec3& v3, const glm::dvec3& v4)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    vertices->push_back(glmToOsg(v1));
    vertices->push_back(glmToOsg(v2));
    vertices->push_back(glmToOsg(v3));
    vertices->push_back(glmToOsg(v4));
    
    osg::Vec3 normal = calculateTriangleNormal(glmToOsg(v1), glmToOsg(v2), glmToOsg(v3));
    for (int i = 0; i < 4; i++) {
        normals->push_back(normal);
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createBox(const glm::dvec3& center, const glm::dvec3& size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    double sx = size.x * 0.5;
    double sy = size.y * 0.5;
    double sz = size.z * 0.5;
    
    // 8个顶点
    std::vector<glm::dvec3> boxVertices = {
        center + glm::dvec3(-sx, -sy, -sz), // 0
        center + glm::dvec3( sx, -sy, -sz), // 1
        center + glm::dvec3( sx,  sy, -sz), // 2
        center + glm::dvec3(-sx,  sy, -sz), // 3
        center + glm::dvec3(-sx, -sy,  sz), // 4
        center + glm::dvec3( sx, -sy,  sz), // 5
        center + glm::dvec3( sx,  sy,  sz), // 6
        center + glm::dvec3(-sx,  sy,  sz)  // 7
    };
    
    // 6个面的索引和法向量
    std::vector<std::vector<int>> faces = {
        {0, 1, 2, 3}, // 底面
        {4, 7, 6, 5}, // 顶面
        {0, 4, 5, 1}, // 前面
        {2, 6, 7, 3}, // 后面
        {0, 3, 7, 4}, // 左面
        {1, 5, 6, 2}  // 右面
    };
    
    std::vector<glm::dvec3> faceNormals = {
        glm::dvec3(0, 0, -1), glm::dvec3(0, 0,  1),
        glm::dvec3(0, -1, 0), glm::dvec3(0,  1, 0),
        glm::dvec3(-1, 0, 0), glm::dvec3( 1, 0, 0)
    };
    
    // 为每个面添加顶点和法向量
    for (size_t f = 0; f < faces.size(); f++) {
        const auto& face = faces[f];
        const auto& normal = faceNormals[f];
        
        for (int i = 0; i < 4; i++) {
            vertices->push_back(glmToOsg(boxVertices[face[i]]));
            normals->push_back(glmToOsg(normal));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 为每个面添加四边形图元
    for (int f = 0; f < 6; f++) {
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, f * 4, 4));
    }
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createSphere(const glm::dvec3& center, double radius, int segments)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int rings = segments / 2;
    
    // 生成球面顶点
    for (int ring = 0; ring <= rings; ring++) {
        double phi = M_PI * ring / rings;
        double sinPhi = sin(phi);
        double cosPhi = cos(phi);
        
        for (int seg = 0; seg <= segments; seg++) {
            double theta = 2.0 * M_PI * seg / segments;
            double sinTheta = sin(theta);
            double cosTheta = cos(theta);
            
            glm::dvec3 normal(sinPhi * cosTheta, sinPhi * sinTheta, cosPhi);
            glm::dvec3 point = center + radius * normal;
            
            vertices->push_back(glmToOsg(point));
            normals->push_back(glmToOsg(normal));
        }
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    // 添加三角形条带
    for (int ring = 0; ring < rings; ring++) {
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLE_STRIP);
        
        for (int seg = 0; seg <= segments; seg++) {
            int curr = ring * (segments + 1) + seg;
            int next = (ring + 1) * (segments + 1) + seg;
            
            indices->push_back(curr);
            indices->push_back(next);
        }
        
        geometry->addPrimitiveSet(indices);
    }
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createCylinder(const glm::dvec3& center, double radius, double height, int segments)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    glm::dvec3 bottom = center - glm::dvec3(0, 0, height * 0.5);
    glm::dvec3 top = center + glm::dvec3(0, 0, height * 0.5);
    
    // 侧面顶点
    for (int i = 0; i < segments; i++) {
        double angle1 = 2.0 * M_PI * i / segments;
        double angle2 = 2.0 * M_PI * (i + 1) / segments;
        
        glm::dvec3 dir1(cos(angle1), sin(angle1), 0);
        glm::dvec3 dir2(cos(angle2), sin(angle2), 0);
        
        glm::dvec3 p1_bottom = bottom + radius * dir1;
        glm::dvec3 p2_bottom = bottom + radius * dir2;
        glm::dvec3 p1_top = top + radius * dir1;
        glm::dvec3 p2_top = top + radius * dir2;
        
        // 底面三角形
        vertices->push_back(glmToOsg(bottom));
        vertices->push_back(glmToOsg(p1_bottom));
        vertices->push_back(glmToOsg(p2_bottom));
        normals->push_back(osg::Vec3(0, 0, -1));
        normals->push_back(osg::Vec3(0, 0, -1));
        normals->push_back(osg::Vec3(0, 0, -1));
        
        // 顶面三角形
        vertices->push_back(glmToOsg(top));
        vertices->push_back(glmToOsg(p2_top));
        vertices->push_back(glmToOsg(p1_top));
        normals->push_back(osg::Vec3(0, 0, 1));
        normals->push_back(osg::Vec3(0, 0, 1));
        normals->push_back(osg::Vec3(0, 0, 1));
        
        // 侧面四边形 (拆分为两个三角形)
        // dir1 和 dir2 已经在循环开始处定义了，这里不需要重新定义
        
        // 第一个三角形
        vertices->push_back(glmToOsg(p1_bottom));
        vertices->push_back(glmToOsg(p1_top));
        vertices->push_back(glmToOsg(p2_bottom));
        normals->push_back(glmToOsg(dir1));
        normals->push_back(glmToOsg(dir1));
        normals->push_back(glmToOsg(dir2));
        
        // 第二个三角形
        vertices->push_back(glmToOsg(p2_bottom));
        vertices->push_back(glmToOsg(p1_top));
        vertices->push_back(glmToOsg(p2_top));
        normals->push_back(glmToOsg(dir2));
        normals->push_back(glmToOsg(dir1));
        normals->push_back(glmToOsg(dir2));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createCone(const glm::dvec3& base, double radius, double height, int segments)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    glm::dvec3 apex = base + glm::dvec3(0, 0, height);
    
    // 侧面
    for (int i = 0; i < segments; i++) {
        double angle1 = 2.0 * M_PI * i / segments;
        double angle2 = 2.0 * M_PI * (i + 1) / segments;
        
        glm::dvec3 dir1(cos(angle1), sin(angle1), 0);
        glm::dvec3 dir2(cos(angle2), sin(angle2), 0);
        
        glm::dvec3 p1 = base + radius * dir1;
        glm::dvec3 p2 = base + radius * dir2;
        
        vertices->push_back(glmToOsg(p1));
        vertices->push_back(glmToOsg(apex));
        vertices->push_back(glmToOsg(p2));
        
        glm::dvec3 edge1 = apex - p1;
        glm::dvec3 edge2 = p2 - p1;
        glm::dvec3 normal = glm::normalize(glm::cross(edge2, edge1));
        
        normals->push_back(glmToOsg(normal));
        normals->push_back(glmToOsg(normal));
        normals->push_back(glmToOsg(normal));
        
        // 底面三角形
        vertices->push_back(glmToOsg(base));
        vertices->push_back(glmToOsg(p2));
        vertices->push_back(glmToOsg(p1));
        normals->push_back(osg::Vec3(0, 0, -1));
        normals->push_back(osg::Vec3(0, 0, -1));
        normals->push_back(osg::Vec3(0, 0, -1));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createPlane(const glm::dvec3& center, const glm::dvec3& normal, double size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    double halfSize = size * 0.5;
    
    // 计算平面的两个正交向量
    glm::dvec3 u, v;
    if (abs(normal.z) < 0.9) {
        u = glm::normalize(glm::cross(normal, glm::dvec3(0, 0, 1)));
    } else {
        u = glm::normalize(glm::cross(normal, glm::dvec3(1, 0, 0)));
    }
    v = glm::normalize(glm::cross(normal, u));
    
    // 四个角点
    glm::dvec3 p1 = center + halfSize * (-u - v);
    glm::dvec3 p2 = center + halfSize * ( u - v);
    glm::dvec3 p3 = center + halfSize * ( u + v);
    glm::dvec3 p4 = center + halfSize * (-u + v);
    
    vertices->push_back(glmToOsg(p1));
    vertices->push_back(glmToOsg(p2));
    vertices->push_back(glmToOsg(p3));
    vertices->push_back(glmToOsg(p4));
    
    for (int i = 0; i < 4; i++) {
        normals->push_back(glmToOsg(normal));
    }
    
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    
    return geometry;
}

// 线框几何体

osg::ref_ptr<osg::Geometry> OSGUtils::createWireframeBox(const glm::dvec3& center, const glm::dvec3& size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    double sx = size.x * 0.5;
    double sy = size.y * 0.5;
    double sz = size.z * 0.5;
    
    // 8个顶点
    std::vector<glm::dvec3> boxVertices = {
        center + glm::dvec3(-sx, -sy, -sz), // 0
        center + glm::dvec3( sx, -sy, -sz), // 1
        center + glm::dvec3( sx,  sy, -sz), // 2
        center + glm::dvec3(-sx,  sy, -sz), // 3
        center + glm::dvec3(-sx, -sy,  sz), // 4
        center + glm::dvec3( sx, -sy,  sz), // 5
        center + glm::dvec3( sx,  sy,  sz), // 6
        center + glm::dvec3(-sx,  sy,  sz)  // 7
    };
    
    for (const auto& vertex : boxVertices) {
        vertices->push_back(glmToOsg(vertex));
    }
    
    geometry->setVertexArray(vertices);
    
    // 12条边
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    // 底面
    indices->push_back(0); indices->push_back(1);
    indices->push_back(1); indices->push_back(2);
    indices->push_back(2); indices->push_back(3);
    indices->push_back(3); indices->push_back(0);
    
    // 顶面
    indices->push_back(4); indices->push_back(5);
    indices->push_back(5); indices->push_back(6);
    indices->push_back(6); indices->push_back(7);
    indices->push_back(7); indices->push_back(4);
    
    // 垂直边
    indices->push_back(0); indices->push_back(4);
    indices->push_back(1); indices->push_back(5);
    indices->push_back(2); indices->push_back(6);
    indices->push_back(3); indices->push_back(7);
    
    geometry->addPrimitiveSet(indices);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createWireframeSphere(const glm::dvec3& center, double radius, int segments)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    
    int rings = segments / 2;
    
    // 生成球面顶点
    for (int ring = 0; ring <= rings; ring++) {
        double phi = M_PI * ring / rings;
        double sinPhi = sin(phi);
        double cosPhi = cos(phi);
        
        for (int seg = 0; seg <= segments; seg++) {
            double theta = 2.0 * M_PI * seg / segments;
            double sinTheta = sin(theta);
            double cosTheta = cos(theta);
            
            glm::dvec3 normal(sinPhi * cosTheta, sinPhi * sinTheta, cosPhi);
            glm::dvec3 point = center + radius * normal;
            
            vertices->push_back(glmToOsg(point));
        }
    }
    
    geometry->setVertexArray(vertices);
    
    // 添加线段索引
    for (int ring = 0; ring < rings; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            int curr = ring * (segments + 1) + seg;
            int next = (ring + 1) * (segments + 1) + seg;
            int nextSeg = ring * (segments + 1) + (seg + 1);
            
            // 纵向线
            indices->push_back(curr);
            indices->push_back(next);
            
            // 横向线
            indices->push_back(curr);
            indices->push_back(nextSeg);
        }
    }
    
    geometry->addPrimitiveSet(indices);
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createAxisArrows(const glm::dvec3& center, double length)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    // X轴 - 红色
    vertices->push_back(glmToOsg(center));
    vertices->push_back(glmToOsg(center + glm::dvec3(length, 0, 0)));
    colors->push_back(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    colors->push_back(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    
    // Y轴 - 绿色
    vertices->push_back(glmToOsg(center));
    vertices->push_back(glmToOsg(center + glm::dvec3(0, length, 0)));
    colors->push_back(osg::Vec4(0.0, 1.0, 0.0, 1.0));
    colors->push_back(osg::Vec4(0.0, 1.0, 0.0, 1.0));
    
    // Z轴 - 蓝色
    vertices->push_back(glmToOsg(center));
    vertices->push_back(glmToOsg(center + glm::dvec3(0, 0, length)));
    colors->push_back(osg::Vec4(0.0, 0.0, 1.0, 1.0));
    colors->push_back(osg::Vec4(0.0, 0.0, 1.0, 1.0));
    
    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 6));
    
    return geometry;
}

// 材质和渲染状态

osg::ref_ptr<osg::StateSet> OSGUtils::createBasicMaterial(const osg::Vec4& color)
{
    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
    osg::ref_ptr<osg::Material> material = new osg::Material();
    
    material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2, 0.2, 0.2, color.a()));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8, 0.8, 0.8, color.a()));
    material->setShininess(osg::Material::FRONT_AND_BACK, 64.0);
    
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
    
    return stateSet;
}

osg::ref_ptr<osg::StateSet> OSGUtils::createTransparentMaterial(const osg::Vec4& color)
{
    osg::ref_ptr<osg::StateSet> stateSet = createBasicMaterial(color);
    
    if (color.a() < 1.0) {
        setTransparency(stateSet, color.a());
    }
    
    return stateSet;
}

osg::ref_ptr<osg::StateSet> OSGUtils::createWireframeMaterial(const osg::Vec4& color, double lineWidth)
{
    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
    osg::ref_ptr<osg::Material> material = new osg::Material();
    
    material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
    material->setAmbient(osg::Material::FRONT_AND_BACK, color);
    
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
    stateSet->setAttributeAndModes(new osg::LineWidth(static_cast<double>(lineWidth)), osg::StateAttribute::ON);
    
    return stateSet;
}

osg::ref_ptr<osg::StateSet> OSGUtils::createPointMaterial(const osg::Vec4& color, double pointSize)
{
    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
    osg::ref_ptr<osg::Material> material = new osg::Material();
    
    material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
    material->setAmbient(osg::Material::FRONT_AND_BACK, color);
    
    stateSet->setAttributeAndModes(material, osg::StateAttribute::ON);
    stateSet->setAttributeAndModes(new osg::Point(static_cast<double>(pointSize)), osg::StateAttribute::ON);
    
    return stateSet;
}

void OSGUtils::setTransparency(osg::StateSet* stateSet, double alpha)
{
    if (alpha < 1.0) {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateSet->setAttributeAndModes(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false), osg::StateAttribute::ON);
    } else {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
        stateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
    }
}

void OSGUtils::setRenderOrder(osg::StateSet* stateSet, int order)
{
    stateSet->setRenderBinDetails(order, "RenderBin");
}

void OSGUtils::setDoubleSided(osg::StateSet* stateSet, bool doubleSided)
{
    if (doubleSided) {
        stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    } else {
        stateSet->setAttributeAndModes(new osg::CullFace(), osg::StateAttribute::ON);
    }
}

void OSGUtils::setDepthTest(osg::StateSet* stateSet, bool enable)
{
    if (enable) {
        stateSet->setAttributeAndModes(new osg::Depth(), osg::StateAttribute::ON);
    } else {
        stateSet->setAttributeAndModes(new osg::Depth(), osg::StateAttribute::OFF);
    }
}

// 辅助函数

osg::ref_ptr<osg::Vec4Array> OSGUtils::createColorArray(const osg::Vec4& color, size_t count)
{
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->resize(count, color);
    return colors;
}

osg::ref_ptr<osg::Vec3Array> OSGUtils::createNormalArray(const osg::Vec3& normal, size_t count)
{
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    normals->resize(count, normal);
    return normals;
}

osg::Vec3 OSGUtils::calculateTriangleNormal(const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3)
{
    osg::Vec3 edge1 = v2 - v1;
    osg::Vec3 edge2 = v3 - v1;
    osg::Vec3 normal = edge1 ^ edge2;
    normal.normalize();
    return normal;
}

osg::BoundingBox OSGUtils::calculateBoundingBox(const std::vector<osg::Vec3>& vertices)
{
    osg::BoundingBox bbox;
    for (const auto& vertex : vertices) {
        bbox.expandBy(vertex);
    }
    return bbox;
}

double OSGUtils::distance(const osg::Vec3& p1, const osg::Vec3& p2)
{
    return (p2 - p1).length();
}

double OSGUtils::distanceToPlane(const osg::Vec3& point, const osg::Vec3& planePoint, const osg::Vec3& planeNormal)
{
    osg::Vec3 normalizedNormal = planeNormal;
    normalizedNormal.normalize();
    return (point - planePoint) * normalizedNormal;
}

bool OSGUtils::rayIntersectsSphere(const osg::Vec3& rayOrigin, const osg::Vec3& rayDirection,
                                 const osg::Vec3& sphereCenter, double sphereRadius,
                                 double& t1, double& t2)
{
    osg::Vec3 oc = rayOrigin - sphereCenter;
    double a = rayDirection * rayDirection;
    double b = 2.0 * (oc * rayDirection);
    double c = (oc * oc) - sphereRadius * sphereRadius;
    
    double discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) {
        return false;
    }
    
    double sqrtDiscriminant = sqrt(discriminant);
    t1 = (-b - sqrtDiscriminant) / (2 * a);
    t2 = (-b + sqrtDiscriminant) / (2 * a);
    
    return true;
}

bool OSGUtils::rayIntersectsTriangle(const osg::Vec3& rayOrigin, const osg::Vec3& rayDirection,
                                   const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2,
                                   double& t, double& u, double& v)
{
    const double EPSILON = 1e-12;
    
    osg::Vec3 edge1 = v1 - v0;
    osg::Vec3 edge2 = v2 - v0;
    osg::Vec3 h = rayDirection ^ edge2;
    double a = edge1 * h;
    
    if (a > -EPSILON && a < EPSILON) {
        return false;
    }
    
    double f = 1.0 / a;
    osg::Vec3 s = rayOrigin - v0;
    u = f * (s * h);
    
    if (u < 0.0 || u > 1.0) {
        return false;
    }
    
    osg::Vec3 q = s ^ edge1;
    v = f * (rayDirection * q);
    
    if (v < 0.0 || u + v > 1.0) {
        return false;
    }
    
    t = f * (edge2 * q);
    
    return t > EPSILON;
}

// 创建调试和辅助对象

osg::ref_ptr<osg::Node> OSGUtils::createCoordinateSystem(double scale)
{
    return createAxisIndicator(scale);
}

osg::ref_ptr<osg::Group> OSGUtils::createAxisIndicator(double length)
{
    osg::ref_ptr<osg::Group> group = new osg::Group();
    
    // 创建三个轴的几何体
    osg::ref_ptr<osg::Geode> axisGeode = new osg::Geode();
    axisGeode->addDrawable(createAxisArrows(glm::dvec3(0, 0, 0), length));
    
    group->addChild(axisGeode);
    
    return group;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createGrid(double size, int divisions)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    double step = size / divisions;
    double halfSize = size * 0.5;
    
    // 创建网格线
    for (int i = 0; i <= divisions; i++) {
        double pos = -halfSize + i * step;
        
        // X方向的线
        vertices->push_back(osg::Vec3(static_cast<double>(-halfSize), static_cast<double>(pos), 0.0));
        vertices->push_back(osg::Vec3(static_cast<double>(halfSize), static_cast<double>(pos), 0.0));
        
        // Y方向的线
        vertices->push_back(osg::Vec3(static_cast<double>(pos), static_cast<double>(-halfSize), 0.0));
        vertices->push_back(osg::Vec3(static_cast<double>(pos), static_cast<double>(halfSize), 0.0));
    }
    
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));
    
    return geometry;
}

// 纹理坐标和索引

osg::ref_ptr<osg::Vec2Array> OSGUtils::createTextureCoords(const std::vector<osg::Vec2>& coords)
{
    osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array();
    for (const auto& coord : coords) {
        texCoords->push_back(coord);
    }
    return texCoords;
}

osg::ref_ptr<osg::DrawElementsUInt> OSGUtils::createTriangleIndices(const std::vector<unsigned int>& indices)
{
    osg::ref_ptr<osg::DrawElementsUInt> drawElements = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
    for (unsigned int index : indices) {
        drawElements->push_back(index);
    }
    return drawElements;
}

osg::ref_ptr<osg::DrawElementsUInt> OSGUtils::createLineIndices(const std::vector<unsigned int>& indices)
{
    osg::ref_ptr<osg::DrawElementsUInt> drawElements = new osg::DrawElementsUInt(osg::PrimitiveSet::LINES);
    for (unsigned int index : indices) {
        drawElements->push_back(index);
    }
    return drawElements;
}

// 法向量计算

void OSGUtils::calculateAndSetNormals(osg::Geometry* geometry)
{
    // 改用手动计算法向量的方式
    if (!geometry) return;
    
    osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
    if (!vertices) return;
    
    // 创建法向量数组
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    normals->resize(vertices->size());
    
    // 初始化所有法向量为零
    for (size_t i = 0; i < normals->size(); ++i) {
        (*normals)[i] = osg::Vec3(0.0f, 0.0f, 0.0f);
    }
    
    // 对每个图元计算法向量
    for (unsigned int primIndex = 0; primIndex < geometry->getNumPrimitiveSets(); ++primIndex) {
        osg::PrimitiveSet* primitiveSet = geometry->getPrimitiveSet(primIndex);
        if (primitiveSet->getMode() == osg::PrimitiveSet::TRIANGLES) {
            for (unsigned int i = 0; i < primitiveSet->getNumIndices(); i += 3) {
                unsigned int i0 = primitiveSet->index(i);
                unsigned int i1 = primitiveSet->index(i + 1);
                unsigned int i2 = primitiveSet->index(i + 2);
                
                if (i0 < vertices->size() && i1 < vertices->size() && i2 < vertices->size()) {
                    osg::Vec3 v0 = (*vertices)[i0];
                    osg::Vec3 v1 = (*vertices)[i1];
                    osg::Vec3 v2 = (*vertices)[i2];
                    
                    // 计算三角形法向量
                    osg::Vec3 normal = (v1 - v0) ^ (v2 - v0);
                    normal.normalize();
                    
                    // 累加到顶点法向量
                    (*normals)[i0] += normal;
                    (*normals)[i1] += normal;
                    (*normals)[i2] += normal;
                }
            }
        }
    }
    
    // 标准化所有法向量
    for (size_t i = 0; i < normals->size(); ++i) {
        (*normals)[i].normalize();
    }
    
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
}

void OSGUtils::setFlatNormals(osg::Geometry* geometry)
{
    // 为每个三角形计算单独的法向量
    osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
    if (!vertices) return;
    
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    for (size_t i = 0; i < vertices->size(); i += 3) {
        if (i + 2 < vertices->size()) {
            osg::Vec3 normal = calculateTriangleNormal((*vertices)[i], (*vertices)[i+1], (*vertices)[i+2]);
            normals->push_back(normal);
            normals->push_back(normal);
            normals->push_back(normal);
        }
    }
    
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
}

void OSGUtils::setSmoothNormals(osg::Geometry* geometry)
{
    calculateAndSetNormals(geometry);
}

// 几何体优化

void OSGUtils::optimizeGeometry(osg::Geometry* geometry)
{
    // 这里可以添加几何体优化逻辑
    // 例如移除重复顶点、优化索引等
}

void OSGUtils::mergeGeometries(osg::Group* group)
{
    // 这里可以添加合并几何体的逻辑
}

// 类型转换

osg::ref_ptr<osg::Vec3Array> OSGUtils::convertGlmToOsg(const std::vector<glm::dvec3>& glmVertices)
{
    osg::ref_ptr<osg::Vec3Array> osgVertices = new osg::Vec3Array();
    for (const auto& vertex : glmVertices) {
        osgVertices->push_back(glmToOsg(vertex));
    }
    return osgVertices;
}

std::vector<glm::dvec3> OSGUtils::convertOsgToGlm(const osg::Vec3Array* osgVertices)
{
    std::vector<glm::dvec3> glmVertices;
    if (osgVertices) {
        for (size_t i = 0; i < osgVertices->size(); i++) {
            glmVertices.push_back(osgToGlm((*osgVertices)[i]));
        }
    }
    return glmVertices;
} 




