#pragma once
#pragma execution_character_set("utf-8")

#include "Enums3D.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <cfloat>

namespace Geo3D {

// 三维点结构
struct Point3D {
    glm::dvec3 position;
    glm::dvec3 normal;
    glm::dvec2 texCoord;

    Point3D() : position(0.0), normal(0.0, 0.0, 1.0), texCoord(0.0) {}
    Point3D(const glm::dvec3& pos) : position(pos), normal(0.0, 0.0, 1.0), texCoord(0.0) {}
    Point3D(const glm::dvec3& pos, const glm::dvec3& norm) : position(pos), normal(norm), texCoord(0.0) {}
    Point3D(const glm::dvec3& pos, const glm::dvec3& norm, const glm::dvec2& tex) : position(pos), normal(norm), texCoord(tex) {}
};

// 颜色结构
struct Color3D {
    glm::dvec4 value;  // RGBA

    Color3D() : value(1.0, 1.0, 1.0, 1.0) {}
    Color3D(double r, double g, double b, double a = 1.0) : value(r, g, b, a) {}
    Color3D(const glm::dvec3& rgb) : value(rgb, 1.0) {}
    Color3D(const glm::dvec4& rgba) : value(rgba) {}

    glm::dvec3 rgb() const { return glm::dvec3(value); }
    double r() const { return value.r; }
    double g() const { return value.g; }
    double b() const { return value.b; }
    double a() const { return value.a; }
};

// 材质结构
struct Material3D {
    glm::dvec4 ambient;
    glm::dvec4 diffuse;
    glm::dvec4 specular;
    glm::dvec4 emission;
    double shininess;

    Material3D()
        : ambient(0.2, 0.2, 0.2, 1.0)
        , diffuse(0.8, 0.8, 0.8, 1.0)
        , specular(0.0, 0.0, 0.0, 1.0)
        , emission(0.0, 0.0, 0.0, 1.0)
        , shininess(0.0)
    {}
};

// 几何变换结构
struct Transform3D {
    glm::dmat4 matrix;
    glm::dvec3 translation;
    glm::dvec3 rotation;
    glm::dvec3 scale;

    Transform3D()
        : matrix(1.0)
        , translation(0.0)
        , rotation(0.0)
        , scale(1.0)
    {}

    void updateMatrix() {
        matrix = glm::mat4(1.0);
        matrix = glm::translate(matrix, translation);
        matrix = glm::rotate(matrix, rotation.x, glm::dvec3(1.0, 0.0, 0.0));
        matrix = glm::rotate(matrix, rotation.y, glm::dvec3(0.0, 1.0, 0.0));
        matrix = glm::rotate(matrix, rotation.z, glm::dvec3(0.0, 0.0, 1.0));
        matrix = glm::scale(matrix, scale);
    }
};

// 包围盒结构
struct BoundingBox3D {
    glm::dvec3 min;
    glm::dvec3 max;

    BoundingBox3D()
        : min(DBL_MAX)
        , max(-DBL_MAX)
    {}

    BoundingBox3D(const glm::dvec3& minPoint, const glm::dvec3& maxPoint)
        : min(minPoint)
        , max(maxPoint)
    {}

    void expandBy(const glm::dvec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    glm::dvec3 center() const {
        return (min + max) * 0.5;
    }

    glm::dvec3 size() const {
        return max - min;
    }

    bool contains(const glm::dvec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    bool intersects(const BoundingBox3D& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }
};

} // namespace Geo3D 
