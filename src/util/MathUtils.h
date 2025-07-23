#pragma once
#pragma execution_character_set("utf-8")

#include <glm/glm.hpp>
#include <vector>
#include <osg/Vec3>
#include <osg/BoundingBox>

// 数学工具类，提供常用的数学计算功能
class MathUtils
{
public:
    // 角度转弧度
    static float degToRad(float degrees);
    
    // 弧度转角度
    static float radToDeg(float radians);
    
    // 向量相关
    static glm::vec3 normalize(const glm::vec3& vec);
    static float distance(const glm::vec3& a, const glm::vec3& b);
    static float distanceSquared(const glm::vec3& a, const glm::vec3& b);
    static glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t);
    static glm::vec3 slerp(const glm::vec3& a, const glm::vec3& b, float t);
    
    // 几何计算
    static glm::vec3 calculateNormal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);
    static glm::vec3 calculateCentroid(const std::vector<glm::vec3>& points);
    static float calculateArea(const std::vector<glm::vec3>& points);
    static float calculateVolume(const std::vector<glm::vec3>& points);
    
    // 包围盒计算
    static osg::BoundingBox calculateBoundingBox(const std::vector<glm::vec3>& points);
    
    // 投影和变换
    static glm::vec3 projectPointOnPlane(const glm::vec3& point, const glm::vec3& planeNormal, const glm::vec3& planePoint);
    static glm::vec3 projectPointOnLine(const glm::vec3& point, const glm::vec3& lineStart, const glm::vec3& lineEnd);
    
    // 相交检测
    static bool rayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                     const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                     float& t, glm::vec3& intersectionPoint);
    
    static bool rayIntersectsPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                  const glm::vec3& planeNormal, const glm::vec3& planePoint,
                                  float& t, glm::vec3& intersectionPoint);
    
    // 坐标转换
    static osg::Vec3 glmToOsg(const glm::vec3& vec);
    static glm::vec3 osgToGlm(const osg::Vec3& vec);
    
    // 曲线计算
    static glm::vec3 evaluateBezier(const std::vector<glm::vec3>& controlPoints, float t);
    static glm::vec3 evaluateSpline(const std::vector<glm::vec3>& controlPoints, float t);
    
    // 圆弧计算
    struct ArcParameters {
        glm::vec3 center;
        float radius;
        float startAngle;
        float endAngle;
        float sweepAngle;
        glm::vec3 normal;
        glm::vec3 uAxis;
        glm::vec3 vAxis;
    };
    
    static ArcParameters calculateArcFromThreePoints(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);
    static std::vector<glm::vec3> generateArcPoints(const ArcParameters& params, int segments = 50);
    
    // 贝塞尔曲线计算
    static glm::vec3 evaluateBezierPoint(const std::vector<glm::vec3>& controlPoints, float t);
    static std::vector<glm::vec3> generateBezierCurve(const std::vector<glm::vec3>& controlPoints, int steps = 50);
    
    // 圆锥体计算
    struct ConeParameters {
        glm::vec3 base;
        glm::vec3 apex;
        float radius;
        float height;
        glm::vec3 axis;
        glm::vec3 uAxis;
        glm::vec3 vAxis;
    };
    
    static ConeParameters calculateConeParameters(const glm::vec3& base, const glm::vec3& apex, float radius);
    static float calculateConeVolume(const ConeParameters& params);
    static float calculateConeSurfaceArea(const ConeParameters& params);
    static glm::vec3 calculateConeCenter(const ConeParameters& params);
    
    // 球体计算
    struct SphereParameters {
        glm::vec3 center;
        float radius;
        int segments;
    };
    
    static SphereParameters calculateSphereParameters(const glm::vec3& center, float radius, int segments = 16);
    static float calculateSphereVolume(const SphereParameters& params);
    static float calculateSphereSurfaceArea(const SphereParameters& params);
    static glm::vec3 calculateSphereCenter(const SphereParameters& params);
    
    // 长方体计算
    struct BoxParameters {
        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 size;
        glm::vec3 center;
    };
    
    static BoxParameters calculateBoxParameters(const glm::vec3& min, const glm::vec3& max);
    static float calculateBoxVolume(const BoxParameters& params);
    static float calculateBoxSurfaceArea(const BoxParameters& params);
    static glm::vec3 calculateBoxCenter(const BoxParameters& params);
    static glm::vec3 calculateBoxSize(const BoxParameters& params);
    
    // 圆柱体计算
    struct CylinderParameters {
        glm::vec3 base;
        glm::vec3 top;
        float radius;
        float height;
        glm::vec3 axis;
        glm::vec3 uAxis;
        glm::vec3 vAxis;
    };
    
    static CylinderParameters calculateCylinderParameters(const glm::vec3& base, const glm::vec3& top, float radius);
    static float calculateCylinderVolume(const CylinderParameters& params);
    static float calculateCylinderSurfaceArea(const CylinderParameters& params);
    static glm::vec3 calculateCylinderCenter(const CylinderParameters& params);
    
    // 圆环体计算
    struct TorusParameters {
        glm::vec3 center;
        float majorRadius;
        float minorRadius;
        glm::vec3 axis;
        glm::vec3 uAxis;
        glm::vec3 vAxis;
    };
    
    static TorusParameters calculateTorusParameters(const glm::vec3& center, float majorRadius, float minorRadius, const glm::vec3& axis = glm::vec3(0, 0, 1));
    static float calculateTorusVolume(const TorusParameters& params);
    static float calculateTorusSurfaceArea(const TorusParameters& params);
    static glm::vec3 calculateTorusCenter(const TorusParameters& params);
    
    // 三角形计算
    struct TriangleParameters {
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 v3;
        glm::vec3 center;
        glm::vec3 normal;
        float area;
    };
    
    static TriangleParameters calculateTriangleParameters(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3);
    static float calculateTriangleArea(const TriangleParameters& params);
    static glm::vec3 calculateTriangleCenter(const TriangleParameters& params);
    static glm::vec3 calculateTriangleNormal(const TriangleParameters& params);
    
    // 四边形计算
    struct QuadParameters {
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 v3;
        glm::vec3 v4;
        glm::vec3 center;
        glm::vec3 normal;
        float area;
    };
    
    static QuadParameters calculateQuadParameters(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4);
    static float calculateQuadArea(const QuadParameters& params);
    static glm::vec3 calculateQuadCenter(const QuadParameters& params);
    static glm::vec3 calculateQuadNormal(const QuadParameters& params);
    
    // 多边形计算
    struct PolygonParameters {
        std::vector<glm::vec3> vertices;
        glm::vec3 center;
        glm::vec3 normal;
        float area;
        std::vector<unsigned int> triangleIndices;
    };
    
    static PolygonParameters calculatePolygonParameters(const std::vector<glm::vec3>& vertices);
    static float calculatePolygonArea(const PolygonParameters& params);
    static glm::vec3 calculatePolygonCenter(const PolygonParameters& params);
    static glm::vec3 calculatePolygonNormal(const PolygonParameters& params);
    static std::vector<unsigned int> triangulatePolygon(const std::vector<glm::vec3>& vertices);
    
    // 线段计算
    struct LineParameters {
        glm::vec3 start;
        glm::vec3 end;
        glm::vec3 direction;
        float length;
        glm::vec3 center;
    };
    
    static LineParameters calculateLineParameters(const glm::vec3& start, const glm::vec3& end);
    static float calculateLineLength(const LineParameters& params);
    static glm::vec3 calculateLineCenter(const LineParameters& params);
    static glm::vec3 calculateLineDirection(const LineParameters& params);
    
    // 立方体计算
    struct CubeParameters {
        glm::vec3 center;
        float size;
        glm::vec3 min;
        glm::vec3 max;
    };
    
    static CubeParameters calculateCubeParameters(const glm::vec3& center, float size);
    static float calculateCubeVolume(const CubeParameters& params);
    static float calculateCubeSurfaceArea(const CubeParameters& params);
    static glm::vec3 calculateCubeCenter(const CubeParameters& params);
    static float calculateCubeSize(const CubeParameters& params);
    
    // 旋转和变换
    static glm::mat4 createRotationMatrix(const glm::vec3& axis, float angle);
    static glm::mat4 createTranslationMatrix(const glm::vec3& translation);
    static glm::mat4 createScaleMatrix(const glm::vec3& scale);
    
    // 常用常量
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float EPSILON = 1e-6f;
    static constexpr float DEG_TO_RAD = PI / 180.0f;
    static constexpr float RAD_TO_DEG = 180.0f / PI;
    
    // 比较函数
    static bool isEqual(float a, float b, float epsilon = EPSILON);
    static bool isZero(float value, float epsilon = EPSILON);
    static bool isEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = EPSILON);

    // ============= 基础几何计算函数 =============
    
    // 圆弧相关计算
    static bool calculateCircleCenterAndRadius(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, 
                                               glm::vec3& center, float& radius);
    static std::vector<glm::vec3> generateArcPointsFromThreePoints(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, int segments = 50);
    
    // 基础几何图形生成
    static glm::vec3 calculatePolygonNormal(const std::vector<glm::vec3>& vertices);
    static std::vector<glm::vec3> generateLineVertices(const glm::vec3& start, const glm::vec3& end);
    static std::vector<glm::vec3> generateRectangleVertices(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4);
    static std::vector<glm::vec3> generateTriangleVertices(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, glm::vec3& normal);
    static std::vector<glm::vec3> generateQuadVertices(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, std::vector<glm::vec3>& normals);

    // ============= 统一几何计算 =============
    
    // 几何计算结果结构
    struct GeometryResult {
        std::vector<glm::vec3> vertices;      // 顶点数据
        std::vector<glm::vec3> edgeVertices;  // 边顶点数据
        std::vector<glm::vec3> faceVertices;  // 面顶点数据
        std::vector<glm::vec3> normals;       // 面法向量
        std::vector<unsigned int> indices;    // 索引数据
        bool hasValidGeometry;                // 是否有有效几何体
    };

    // 根据控制点数据和几何类型计算几何图形
    static GeometryResult calculateGeometry(const std::vector<std::vector<glm::vec3>>& stageControlPoints, int geoType);

    // 具体几何图形的计算函数
    static GeometryResult calculatePointGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateLineGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateArcGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateBezierCurveGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateTriangleGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateQuadGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculatePolygonGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateBoxGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateCubeGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateCylinderGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateConeGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateSphereGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateTorusGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculatePrismGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateHemisphereGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateEllipsoidGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);

    // 建筑物几何计算函数
    static GeometryResult calculateFlatHouseGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateDomeHouseGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateSpireHouseGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateGableHouseGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);
    static GeometryResult calculateLHouseGeometry(const std::vector<std::vector<glm::vec3>>& controlPoints);

    // 辅助函数
    static std::vector<glm::vec3> generateCirclePoints(const glm::vec3& center, float radius, const glm::vec3& normal, int segments = 32);
    static std::vector<glm::vec3> generateBoxVertices(const glm::vec3& min, const glm::vec3& max);
    static std::vector<glm::vec3> generateBoxEdges(const glm::vec3& min, const glm::vec3& max);
    static std::vector<glm::vec3> generateBoxFaces(const glm::vec3& min, const glm::vec3& max, std::vector<glm::vec3>& normals);
    static std::vector<unsigned int> generateBoxIndices();
}; 