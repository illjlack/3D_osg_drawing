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

// ============= 坐标转换 =============

osg::Vec3 OSGUtils::glmToOsg(const glm::vec3& vec)
{
    return osg::Vec3(vec.x, vec.y, vec.z);
}

glm::vec3 OSGUtils::osgToGlm(const osg::Vec3& vec)
{
    return glm::vec3(vec.x(), vec.y(), vec.z());
}

osg::Vec4 OSGUtils::glmToOsg(const glm::vec4& vec)
{
    return osg::Vec4(vec.x, vec.y, vec.z, vec.w);
}

glm::vec4 OSGUtils::osgToGlm(const osg::Vec4& vec)
{
    return glm::vec4(vec.x(), vec.y(), vec.z(), vec.w());
}

osg::Matrix OSGUtils::glmToOsg(const glm::mat4& mat)
{
    return osg::Matrix(
        mat[0][0], mat[1][0], mat[2][0], mat[3][0],
        mat[0][1], mat[1][1], mat[2][1], mat[3][1],
        mat[0][2], mat[1][2], mat[2][2], mat[3][2],
        mat[0][3], mat[1][3], mat[2][3], mat[3][3]
    );
}

glm::mat4 OSGUtils::osgToGlm(const osg::Matrix& mat)
{
    return glm::mat4(
        mat(0,0), mat(1,0), mat(2,0), mat(3,0),
        mat(0,1), mat(1,1), mat(2,1), mat(3,1),
        mat(0,2), mat(1,2), mat(2,2), mat(3,2),
        mat(0,3), mat(1,3), mat(2,3), mat(3,3)
    );
}

// ============= 几何体创建 =============

osg::ref_ptr<osg::Geometry> OSGUtils::createBox(const glm::vec3& center, const glm::vec3& size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    float sx = size.x * 0.5f;
    float sy = size.y * 0.5f;
    float sz = size.z * 0.5f;
    
    // 8个顶点
    std::vector<glm::vec3> boxVertices = {
        center + glm::vec3(-sx, -sy, -sz), // 0
        center + glm::vec3( sx, -sy, -sz), // 1
        center + glm::vec3( sx,  sy, -sz), // 2
        center + glm::vec3(-sx,  sy, -sz), // 3
        center + glm::vec3(-sx, -sy,  sz), // 4
        center + glm::vec3( sx, -sy,  sz), // 5
        center + glm::vec3( sx,  sy,  sz), // 6
        center + glm::vec3(-sx,  sy,  sz)  // 7
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
    
    std::vector<glm::vec3> faceNormals = {
        glm::vec3(0, 0, -1), glm::vec3(0, 0,  1),
        glm::vec3(0, -1, 0), glm::vec3(0,  1, 0),
        glm::vec3(-1, 0, 0), glm::vec3( 1, 0, 0)
    };
    
    for (size_t f = 0; f < faces.size(); ++f)
    {
        const auto& face = faces[f];
        const auto& normal = faceNormals[f];
        
        // 三角化每个面
        vertices->push_back(glmToOsg(boxVertices[face[0]]));
        vertices->push_back(glmToOsg(boxVertices[face[1]]));
        vertices->push_back(glmToOsg(boxVertices[face[2]]));
        
        vertices->push_back(glmToOsg(boxVertices[face[0]]));
        vertices->push_back(glmToOsg(boxVertices[face[2]]));
        vertices->push_back(glmToOsg(boxVertices[face[3]]));
        
        for (int i = 0; i < 6; ++i)
        {
            normals->push_back(glmToOsg(normal));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createSphere(const glm::vec3& center, float radius, int segments)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int latSegments = segments;
    int lonSegments = segments * 2;
    
    // 生成球面顶点
    for (int lat = 0; lat <= latSegments; ++lat)
    {
        float theta = M_PI * lat / latSegments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= lonSegments; ++lon)
        {
            float phi = 2.0f * M_PI * lon / lonSegments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            glm::vec3 normal(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            glm::vec3 point = center + radius * normal;
            
            vertices->push_back(glmToOsg(point));
            normals->push_back(glmToOsg(normal));
        }
    }
    
    // 生成三角形索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    for (int lat = 0; lat < latSegments; ++lat)
    {
        for (int lon = 0; lon < lonSegments; ++lon)
        {
            int current = lat * (lonSegments + 1) + lon;
            int next = current + lonSegments + 1;
            
            indices->push_back(current);
            indices->push_back(next);
            indices->push_back(current + 1);
            
            indices->push_back(current + 1);
            indices->push_back(next);
            indices->push_back(next + 1);
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(indices.get());
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createCylinder(const glm::vec3& center, float radius, float height, int segments)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    glm::vec3 bottom = center - glm::vec3(0, 0, height * 0.5f);
    glm::vec3 top = center + glm::vec3(0, 0, height * 0.5f);
    
    // 生成圆柱侧面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1(cos(angle1), sin(angle1), 0);
        glm::vec3 dir2(cos(angle2), sin(angle2), 0);
        
        glm::vec3 p1_bottom = bottom + radius * dir1;
        glm::vec3 p2_bottom = bottom + radius * dir2;
        glm::vec3 p1_top = top + radius * dir1;
        glm::vec3 p2_top = top + radius * dir2;
        
        // 第一个三角形
        vertices->push_back(glmToOsg(p1_bottom));
        vertices->push_back(glmToOsg(p2_bottom));
        vertices->push_back(glmToOsg(p1_top));
        
        normals->push_back(glmToOsg(dir1));
        normals->push_back(glmToOsg(dir2));
        normals->push_back(glmToOsg(dir1));
        
        // 第二个三角形
        vertices->push_back(glmToOsg(p2_bottom));
        vertices->push_back(glmToOsg(p2_top));
        vertices->push_back(glmToOsg(p1_top));
        
        normals->push_back(glmToOsg(dir2));
        normals->push_back(glmToOsg(dir2));
        normals->push_back(glmToOsg(dir1));
    }
    
    // 底面和顶面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1(cos(angle1), sin(angle1), 0);
        glm::vec3 dir2(cos(angle2), sin(angle2), 0);
        
        // 底面
        vertices->push_back(glmToOsg(bottom));
        vertices->push_back(glmToOsg(bottom + radius * dir2));
        vertices->push_back(glmToOsg(bottom + radius * dir1));
        
        normals->push_back(osg::Vec3(0, 0, -1));
        normals->push_back(osg::Vec3(0, 0, -1));
        normals->push_back(osg::Vec3(0, 0, -1));
        
        // 顶面
        vertices->push_back(glmToOsg(top));
        vertices->push_back(glmToOsg(top + radius * dir1));
        vertices->push_back(glmToOsg(top + radius * dir2));
        
        normals->push_back(osg::Vec3(0, 0, 1));
        normals->push_back(osg::Vec3(0, 0, 1));
        normals->push_back(osg::Vec3(0, 0, 1));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createCone(const glm::vec3& base, float radius, float height, int segments)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    glm::vec3 apex = base + glm::vec3(0, 0, height);
    
    // 生成圆锥侧面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1(cos(angle1), sin(angle1), 0);
        glm::vec3 dir2(cos(angle2), sin(angle2), 0);
        
        glm::vec3 p1 = base + radius * dir1;
        glm::vec3 p2 = base + radius * dir2;
        
        vertices->push_back(glmToOsg(p1));
        vertices->push_back(glmToOsg(p2));
        vertices->push_back(glmToOsg(apex));
        
        // 计算侧面法向量
        glm::vec3 edge1 = apex - p1;
        glm::vec3 edge2 = p2 - p1;
        glm::vec3 normal = glm::normalize(glm::cross(edge2, edge1));
        
        normals->push_back(glmToOsg(normal));
        normals->push_back(glmToOsg(normal));
        normals->push_back(glmToOsg(normal));
    }
    
    // 生成底面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1(cos(angle1), sin(angle1), 0);
        glm::vec3 dir2(cos(angle2), sin(angle2), 0);
        
        vertices->push_back(glmToOsg(base));
        vertices->push_back(glmToOsg(base + radius * dir2));
        vertices->push_back(glmToOsg(base + radius * dir1));
        
        normals->push_back(osg::Vec3(0, 0, -1));
        normals->push_back(osg::Vec3(0, 0, -1));
        normals->push_back(osg::Vec3(0, 0, -1));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createPlane(const glm::vec3& center, const glm::vec3& normal, float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    // 计算两个垂直于法向量的向量
    glm::vec3 u, v;
    if (abs(normal.z) < 0.9f)
    {
        u = glm::normalize(glm::cross(normal, glm::vec3(0, 0, 1)));
    }
    else
    {
        u = glm::normalize(glm::cross(normal, glm::vec3(1, 0, 0)));
    }
    v = glm::normalize(glm::cross(normal, u));
    
    float halfSize = size * 0.5f;
    
    // 四个顶点
    glm::vec3 p1 = center + halfSize * (-u - v);
    glm::vec3 p2 = center + halfSize * ( u - v);
    glm::vec3 p3 = center + halfSize * ( u + v);
    glm::vec3 p4 = center + halfSize * (-u + v);
    
    // 两个三角形
    vertices->push_back(glmToOsg(p1));
    vertices->push_back(glmToOsg(p2));
    vertices->push_back(glmToOsg(p3));
    
    vertices->push_back(glmToOsg(p1));
    vertices->push_back(glmToOsg(p3));
    vertices->push_back(glmToOsg(p4));
    
    for (int i = 0; i < 6; ++i)
    {
        normals->push_back(glmToOsg(normal));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 6));
    
    return geometry;
}

// ============= 材质和渲染状态 =============

osg::ref_ptr<osg::StateSet> OSGUtils::createMaterial(const osg::Vec4& color, bool transparent)
{
    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
    
    osg::ref_ptr<osg::Material> material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT_AND_BACK, color);
    material->setAmbient(osg::Material::FRONT_AND_BACK, color * 0.3f);
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
    material->setShininess(osg::Material::FRONT_AND_BACK, 32.0f);
    
    stateSet->setAttribute(material.get());
    
    if (transparent && color.w() < 1.0f)
    {
        setTransparency(stateSet.get(), color.w());
    }
    
    return stateSet;
}

osg::ref_ptr<osg::StateSet> OSGUtils::createWireframeMaterial(const osg::Vec4& color, float lineWidth)
{
    osg::ref_ptr<osg::StateSet> stateSet = createMaterial(color);
    
    osg::ref_ptr<osg::PolygonMode> polygonMode = new osg::PolygonMode();
    polygonMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
    stateSet->setAttribute(polygonMode.get());
    
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth();
    lw->setWidth(lineWidth);
    stateSet->setAttribute(lw.get());
    
    return stateSet;
}

osg::ref_ptr<osg::StateSet> OSGUtils::createPointMaterial(const osg::Vec4& color, float pointSize)
{
    osg::ref_ptr<osg::StateSet> stateSet = createMaterial(color);
    
    osg::ref_ptr<osg::Point> point = new osg::Point();
    point->setSize(pointSize);
    stateSet->setAttribute(point.get());
    
    return stateSet;
}

void OSGUtils::setTransparency(osg::StateSet* stateSet, float alpha)
{
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    
    osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc();
    blendFunc->setFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    stateSet->setAttribute(blendFunc.get());
}

void OSGUtils::setDepthTest(osg::StateSet* stateSet, bool enable)
{
    osg::ref_ptr<osg::Depth> depth = new osg::Depth();
    depth->setWriteMask(enable);
    stateSet->setAttribute(depth.get());
}

// ============= 数学工具 =============

osg::Vec3 OSGUtils::computeTriangleNormal(const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3)
{
    osg::Vec3 edge1 = v2 - v1;
    osg::Vec3 edge2 = v3 - v1;
    osg::Vec3 normal = edge1 ^ edge2;
    normal.normalize();
    return normal;
}

float OSGUtils::distance(const osg::Vec3& p1, const osg::Vec3& p2)
{
    return (p2 - p1).length();
}

float OSGUtils::distanceToPlane(const osg::Vec3& point, const osg::Vec3& planePoint, const osg::Vec3& planeNormal)
{
    return (point - planePoint) * planeNormal;
}

bool OSGUtils::rayIntersectSphere(const osg::Vec3& rayOrigin, const osg::Vec3& rayDirection,
                                 const osg::Vec3& sphereCenter, float sphereRadius,
                                 float& t1, float& t2)
{
    osg::Vec3 oc = rayOrigin - sphereCenter;
    float a = rayDirection * rayDirection;
    float b = 2.0f * (oc * rayDirection);
    float c = (oc * oc) - sphereRadius * sphereRadius;
    
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
        return false;
    
    float sqrtDiscriminant = sqrt(discriminant);
    t1 = (-b - sqrtDiscriminant) / (2.0f * a);
    t2 = (-b + sqrtDiscriminant) / (2.0f * a);
    
    return true;
}

bool OSGUtils::rayIntersectTriangle(const osg::Vec3& rayOrigin, const osg::Vec3& rayDirection,
                                   const osg::Vec3& v0, const osg::Vec3& v1, const osg::Vec3& v2,
                                   float& t, float& u, float& v)
{
    const float EPSILON = 0.0000001f;
    
    osg::Vec3 edge1 = v1 - v0;
    osg::Vec3 edge2 = v2 - v0;
    osg::Vec3 h = rayDirection ^ edge2;
    float a = edge1 * h;
    
    if (a > -EPSILON && a < EPSILON)
        return false;
    
    float f = 1.0f / a;
    osg::Vec3 s = rayOrigin - v0;
    u = f * (s * h);
    
    if (u < 0.0f || u > 1.0f)
        return false;
    
    osg::Vec3 q = s ^ edge1;
    v = f * (rayDirection * q);
    
    if (v < 0.0f || u + v > 1.0f)
        return false;
    
    t = f * (edge2 * q);
    
    return t > EPSILON;
}

// ============= 调试工具 =============

osg::ref_ptr<osg::Group> OSGUtils::createAxisIndicator(float length)
{
    osg::ref_ptr<osg::Group> group = new osg::Group();
    
    // X轴 - 红色
    osg::ref_ptr<osg::Geometry> xAxis = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> xVertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> xColors = new osg::Vec4Array();
    
    xVertices->push_back(osg::Vec3(0, 0, 0));
    xVertices->push_back(osg::Vec3(length, 0, 0));
    xColors->push_back(osg::Vec4(1, 0, 0, 1));
    xColors->push_back(osg::Vec4(1, 0, 0, 1));
    
    xAxis->setVertexArray(xVertices.get());
    xAxis->setColorArray(xColors.get());
    xAxis->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    xAxis->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 2));
    
    // Y轴 - 绿色
    osg::ref_ptr<osg::Geometry> yAxis = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> yVertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> yColors = new osg::Vec4Array();
    
    yVertices->push_back(osg::Vec3(0, 0, 0));
    yVertices->push_back(osg::Vec3(0, length, 0));
    yColors->push_back(osg::Vec4(0, 1, 0, 1));
    yColors->push_back(osg::Vec4(0, 1, 0, 1));
    
    yAxis->setVertexArray(yVertices.get());
    yAxis->setColorArray(yColors.get());
    yAxis->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    yAxis->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 2));
    
    // Z轴 - 蓝色
    osg::ref_ptr<osg::Geometry> zAxis = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> zVertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> zColors = new osg::Vec4Array();
    
    zVertices->push_back(osg::Vec3(0, 0, 0));
    zVertices->push_back(osg::Vec3(0, 0, length));
    zColors->push_back(osg::Vec4(0, 0, 1, 1));
    zColors->push_back(osg::Vec4(0, 0, 1, 1));
    
    zAxis->setVertexArray(zVertices.get());
    zAxis->setColorArray(zColors.get());
    zAxis->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    zAxis->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, 2));
    
    osg::ref_ptr<osg::Geode> xGeode = new osg::Geode();
    osg::ref_ptr<osg::Geode> yGeode = new osg::Geode();
    osg::ref_ptr<osg::Geode> zGeode = new osg::Geode();
    
    xGeode->addDrawable(xAxis.get());
    yGeode->addDrawable(yAxis.get());
    zGeode->addDrawable(zAxis.get());
    
    group->addChild(xGeode.get());
    group->addChild(yGeode.get());
    group->addChild(zGeode.get());
    
    return group;
}

osg::ref_ptr<osg::Geometry> OSGUtils::createGrid(float size, int divisions)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    float step = size / divisions;
    float halfSize = size * 0.5f;
    
    // 垂直线
    for (int i = 0; i <= divisions; ++i)
    {
        float x = -halfSize + i * step;
        vertices->push_back(osg::Vec3(x, -halfSize, 0));
        vertices->push_back(osg::Vec3(x,  halfSize, 0));
        colors->push_back(osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
        colors->push_back(osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
    }
    
    // 水平线
    for (int i = 0; i <= divisions; ++i)
    {
        float y = -halfSize + i * step;
        vertices->push_back(osg::Vec3(-halfSize, y, 0));
        vertices->push_back(osg::Vec3( halfSize, y, 0));
        colors->push_back(osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
        colors->push_back(osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, vertices->size()));
    
    return geometry;
}

void OSGUtils::printNodeInfo(osg::Node* node, int indent)
{
    if (!node) return;
    
    std::string indentStr(indent * 2, ' ');
    std::cout << indentStr << "Node: " << node->className() 
              << " (" << node->getName() << ")" << std::endl;
    
    osg::Group* group = node->asGroup();
    if (group)
    {
        for (unsigned int i = 0; i < group->getNumChildren(); ++i)
        {
            printNodeInfo(group->getChild(i), indent + 1);
        }
    }
}

void OSGUtils::printGeometryInfo(osg::Geometry* geometry)
{
    if (!geometry) return;
    
    std::cout << "Geometry Info:" << std::endl;
    std::cout << "  Vertices: " << (geometry->getVertexArray() ? geometry->getVertexArray()->getNumElements() : 0) << std::endl;
    std::cout << "  Primitive Sets: " << geometry->getNumPrimitiveSets() << std::endl;
    
    for (unsigned int i = 0; i < geometry->getNumPrimitiveSets(); ++i)
    {
        osg::PrimitiveSet* ps = geometry->getPrimitiveSet(i);
        std::cout << "    Set " << i << ": Mode=" << ps->getMode() 
                  << ", Elements=" << ps->getNumIndices() << std::endl;
    }
} 