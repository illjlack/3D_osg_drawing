#include "Geo3D.h"
#include <osg/Array>           // Vec3Array, Vec4Array
#include <osg/Shape>           // Sphere, Box, Cylinder, Cone, Capsule
#include <osg/PositionAttitudeTransform>
#include <osgUtil/Tessellator>
#include <QApplication>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ========================================= 工厂函数 =========================================
Geo3D* createGeo3D(DrawMode3D mode)
{
    switch (mode)
    {
    case DrawPoint3D:
        return new Point3D_Geo();
    case DrawLine3D:
        return new Line3D_Geo();
    case DrawArc3D:
    case DrawThreePointArc3D:
        return new Arc3D_Geo();
    case DrawBezierCurve3D:
        return new BezierCurve3D_Geo();
    case DrawTriangle3D:
        return new Triangle3D_Geo();
    case DrawQuad3D:
        return new Quad3D_Geo();
    case DrawPolygon3D:
        return new Polygon3D_Geo();
    case DrawBox3D:
        return new Box3D_Geo();
    case DrawCube3D:
        return new Cube3D_Geo();
    case DrawCylinder3D:
        return new Cylinder3D_Geo();
    case DrawCone3D:
        return new Cone3D_Geo();
    case DrawSphere3D:
        return new Sphere3D_Geo();
    case DrawTorus3D:
        return new Torus3D_Geo();
    default:
        return nullptr;
    }
}

// ========================================= 基类 Geo3D =========================================
Geo3D::Geo3D()
    : m_geoType(Geo_Undefined3D)
    , m_geoState(0)
    , m_tempPoint(0, 0, 0)
    , m_geometryDirty(true)
    , m_initialized(false)
    , m_featuresDirty(true)
{
    m_osgNode = new osg::Group();
    m_drawableGroup = new osg::Group();  // 替代Geode
    m_transformNode = new osg::MatrixTransform();
    m_controlPointsNode = new osg::Group();
    
    m_osgNode->addChild(m_transformNode.get());
    m_transformNode->addChild(m_drawableGroup.get());
    m_transformNode->addChild(m_controlPointsNode.get());
}

Geo3D::~Geo3D()
{
}

void Geo3D::setParameters(const GeoParameters3D& params)
{
    m_parameters = params;
    markGeometryDirty();
    updateGeometry();
}

void Geo3D::addControlPoint(const Point3D& point)
{
    m_controlPoints.push_back(point);
    updateBoundingBox();
    markGeometryDirty();
}

void Geo3D::setControlPoint(int index, const Point3D& point)
{
    if (index >= 0 && index < static_cast<int>(m_controlPoints.size()))
    {
        m_controlPoints[index] = point;
        updateBoundingBox();
        markGeometryDirty();
    }
}

void Geo3D::removeControlPoint(int index)
{
    if (index >= 0 && index < static_cast<int>(m_controlPoints.size()))
    {
        m_controlPoints.erase(m_controlPoints.begin() + index);
        updateBoundingBox();
        markGeometryDirty();
    }
}

void Geo3D::clearControlPoints()
{
    m_controlPoints.clear();
    m_boundingBox = BoundingBox3D();
    markGeometryDirty();
}

void Geo3D::setTransform(const Transform3D& transform)
{
    m_transform = transform;
    if (m_transformNode.valid())
    {
        osg::Matrix matrix;
        const glm::mat4& glmMat = transform.getMatrix();
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                matrix(i, j) = glmMat[j][i]; // GLM是列主序，OSG是行主序
            }
        }
        m_transformNode->setMatrix(matrix);
    }
}

void Geo3D::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 基类默认实现
}

void Geo3D::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 基类默认实现
}

void Geo3D::mouseReleaseEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    // 基类默认实现
}

void Geo3D::keyPressEvent(QKeyEvent* event)
{
    // 基类默认实现
}

void Geo3D::keyReleaseEvent(QKeyEvent* event)
{
    // 基类默认实现
}

bool Geo3D::hitTest(const Ray3D& ray, PickResult3D& result) const
{
    // 基类默认实现：简单的包围盒测试
    if (!m_boundingBox.isValid())
        return false;
    
    // 射线与AABB包围盒的相交测试
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t1 = (m_boundingBox.min - ray.origin) * invDir;
    glm::vec3 t2 = (m_boundingBox.max - ray.origin) * invDir;
    
    glm::vec3 tMin = glm::min(t1, t2);
    glm::vec3 tMax = glm::max(t1, t2);
    
    float tNear = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
    float tFar = glm::min(glm::min(tMax.x, tMax.y), tMax.z);
    
    if (tNear <= tFar && tFar >= 0)
    {
        result.hit = true;
        result.distance = tNear > 0 ? tNear : tFar;
        result.point = ray.pointAt(result.distance);
        result.userData = const_cast<Geo3D*>(this);
        return true;
    }
    
    return false;
}

void Geo3D::completeDrawing()
{
    setStateComplete();
    clearStateEditing();
    updateGeometry();
}

void Geo3D::initialize()
{
    if (!m_initialized)
    {
        m_parameters.resetToGlobal();
        setStateInitialized();
        m_initialized = true;
    }
}

void Geo3D::updateOSGNode()
{
    if (!m_initialized)
        initialize();
    
    if (isGeometryDirty())
    {
        // 清除旧的几何体
        m_drawableGroup->removeChildren(0, m_drawableGroup->getNumChildren());
        
        // 创建新的几何体
        m_geometry = createGeometry();
        if (m_geometry.valid())
        {
            // 创建Geode来包装Drawable，然后添加到Group中
            osg::ref_ptr<osg::Geode> geode = new osg::Geode();
            geode->addDrawable(m_geometry.get());
            m_drawableGroup->addChild(geode.get());
            updateMaterial();
        }
        
        clearGeometryDirty();
    }
    
    updateControlPointsVisualization();
}

void Geo3D::updateMaterial()
{
    if (!m_geometry.valid())
        return;
    
    osg::ref_ptr<osg::StateSet> stateSet = m_geometry->getOrCreateStateSet();
    
    // 设置材质
    osg::ref_ptr<osg::Material> material = new osg::Material();
    
    const Material3D& mat = m_parameters.material;
    material->setAmbient(osg::Material::FRONT_AND_BACK, 
                        osg::Vec4(mat.ambient.r, mat.ambient.g, mat.ambient.b, mat.ambient.a));
    material->setDiffuse(osg::Material::FRONT_AND_BACK, 
                        osg::Vec4(mat.diffuse.r, mat.diffuse.g, mat.diffuse.b, mat.diffuse.a));
    material->setSpecular(osg::Material::FRONT_AND_BACK, 
                         osg::Vec4(mat.specular.r, mat.specular.g, mat.specular.b, mat.specular.a));
    material->setEmission(osg::Material::FRONT_AND_BACK, 
                         osg::Vec4(mat.emission.r, mat.emission.g, mat.emission.b, mat.emission.a));
    material->setShininess(osg::Material::FRONT_AND_BACK, mat.shininess);
    
    stateSet->setAttributeAndModes(material.get(), osg::StateAttribute::ON);
    
    // 设置透明度
    if (mat.transparency < 1.0f)
    {
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }
    
    // 设置线宽
    if (m_geoType == Geo_Line3D || m_geoType == Geo_Arc3D || m_geoType == Geo_BezierCurve3D)
    {
        osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(m_parameters.lineWidth);
        stateSet->setAttributeAndModes(lineWidth.get(), osg::StateAttribute::ON);
    }
    
    // 设置点大小
    if (m_geoType == Geo_Point3D)
    {
        osg::ref_ptr<osg::Point> pointAttr = new osg::Point(m_parameters.pointSize);
        stateSet->setAttributeAndModes(pointAttr.get(), osg::StateAttribute::ON);
    }
    
    // 设置填充模式
    switch (m_parameters.fillType)
    {
    case Fill_Wireframe3D:
        {
            osg::ref_ptr<osg::PolygonMode> polyMode = new osg::PolygonMode();
            polyMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            stateSet->setAttributeAndModes(polyMode.get(), osg::StateAttribute::ON);
        }
        break;
    case Fill_Points3D:
        {
            osg::ref_ptr<osg::PolygonMode> polyMode = new osg::PolygonMode();
            polyMode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::POINT);
            stateSet->setAttributeAndModes(polyMode.get(), osg::StateAttribute::ON);
        }
        break;
    default:
        break;
    }
}

void Geo3D::updateControlPointsVisualization()
{
    m_controlPointsNode->removeChildren(0, m_controlPointsNode->getNumChildren());
    
    if (isStateEditing() && !m_controlPoints.empty())
    {
        for (const Point3D& point : m_controlPoints)
        {
            osg::ref_ptr<osg::Geode> controlPointGeode = new osg::Geode();
            osg::ref_ptr<osg::ShapeDrawable> sphere = new osg::ShapeDrawable(
                new osg::Sphere(osg::Vec3(point.x(), point.y(), point.z()), 0.1f));
            sphere->setColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f)); // 红色控制点
            controlPointGeode->addDrawable(sphere.get());
            m_controlPointsNode->addChild(controlPointGeode.get());
        }
    }
}

osg::Vec3 Geo3D::glmToOsgVec3(const glm::vec3& v) const
{
    return osg::Vec3(v.x, v.y, v.z);
}

osg::Vec4 Geo3D::glmToOsgVec4(const glm::vec4& v) const
{
    return osg::Vec4(v.x, v.y, v.z, v.w);
}

glm::vec3 Geo3D::osgToGlmVec3(const osg::Vec3& v) const
{
    return glm::vec3(v.x(), v.y(), v.z());
}

glm::vec4 Geo3D::osgToGlmVec4(const osg::Vec4& v) const
{
    return glm::vec4(v.x(), v.y(), v.z(), v.w());
}

void Geo3D::updateBoundingBox()
{
    m_boundingBox = BoundingBox3D();
    
    for (const Point3D& point : m_controlPoints)
    {
        m_boundingBox.expand(point.position);
    }
}

// ========================================= 点类 =========================================
Point3D_Geo::Point3D_Geo()
{
    m_geoType = Geo_Point3D;
}

void Point3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        completeDrawing();
    }
}

void Point3D_Geo::completeDrawing()
{
    if (!m_controlPoints.empty())
    {
        Geo3D::completeDrawing();
    }
}

osg::ref_ptr<osg::Geometry> Point3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    return createPointGeometry(m_parameters.pointShape, m_parameters.pointSize);
}

osg::ref_ptr<osg::Geometry> Point3D_Geo::createPointGeometry(PointShape3D shape, float size)
{
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    const Point3D& point = m_controlPoints[0];
    
    switch (shape)
    {
    case Point_Circle3D:
        {
            // 创建圆形点
            int segments = 16;
            float radius = size * 0.01f;
            for (int i = 0; i <= segments; ++i)
            {
                float angle = 2.0f * M_PI * i / segments;
                float x = point.x() + radius * cos(angle);
                float y = point.y() + radius * sin(angle);
                vertices->push_back(osg::Vec3(x, y, point.z()));
            }
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_FAN, 0, vertices->size()));
        }
        break;
        
    case Point_Square3D:
        {
            // 创建方形点
            float half = size * 0.01f;
            vertices->push_back(osg::Vec3(point.x() - half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() + half, point.z()));
            vertices->push_back(osg::Vec3(point.x() - half, point.y() + half, point.z()));
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));
        }
        break;
        
    case Point_Triangle3D:
        {
            // 创建三角形点
            float half = size * 0.01f;
            vertices->push_back(osg::Vec3(point.x(), point.y() + half, point.z()));
            vertices->push_back(osg::Vec3(point.x() - half, point.y() - half, point.z()));
            vertices->push_back(osg::Vec3(point.x() + half, point.y() - half, point.z()));
            
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
        }
        break;
        
    default:
        // 默认为简单点
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, 1));
        break;
    }
    
    // 设置颜色
    for (size_t i = 0; i < vertices->size(); ++i)
    {
        colors->push_back(osg::Vec4(m_parameters.pointColor.r, m_parameters.pointColor.g, 
                                   m_parameters.pointColor.b, m_parameters.pointColor.a));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    return geometry;
}

// ============================================================================
// IPickingProvider Implementation (Geo3D基类)
// ============================================================================

std::vector<FeatureType> Geo3D::getSupportedFeatureTypes() const
{
    // 默认不支持任何Feature类型，子类需要重写
    return {};
}

std::vector<PickingFeature> Geo3D::getFeatures(FeatureType type) const
{
    return getCachedFeatures(type);
}

std::vector<PickingFeature> Geo3D::getCachedFeatures(FeatureType type) const
{
    // 检查缓存
    auto it = m_cachedFeatures.find(type);
    if (it != m_cachedFeatures.end() && !m_featuresDirty)
    {
        return it->second;
    }
    
    // 重新抽取Feature
    std::vector<PickingFeature> features;
    switch (type)
    {
        case FeatureType::FACE:
            features = extractFaceFeatures();
            break;
        case FeatureType::EDGE:
            features = extractEdgeFeatures();
            break;
        case FeatureType::VERTEX:
            features = extractVertexFeatures();
            break;
        default:
            break;
    }
    
    // 更新缓存
    m_cachedFeatures[type] = features;
    
    return features;
}

// ============================================================================
// RegularGeo3D Implementation (规则几何体基类)
// ============================================================================

RegularGeo3D::RegularGeo3D()
{
    markFeaturesDirty();
}

std::vector<PickingFeature> RegularGeo3D::extractFaceFeatures() const
{
    // 规则几何体的子类需要实现具体的Face抽取逻辑
    return {};
}

std::vector<PickingFeature> RegularGeo3D::extractEdgeFeatures() const
{
    // 规则几何体的子类需要实现具体的Edge抽取逻辑
    return {};
}

std::vector<PickingFeature> RegularGeo3D::extractVertexFeatures() const
{
    // 规则几何体的子类需要实现具体的Vertex抽取逻辑
    return {};
}

// ============================================================================
// MeshGeo3D Implementation (三角网格几何体)
// ============================================================================

MeshGeo3D::MeshGeo3D()
{
    markFeaturesDirty();
}

void MeshGeo3D::setMeshData(osg::ref_ptr<osg::Geometry> geometry)
{
    m_meshGeometry = geometry;
    markFeaturesDirty();
}

std::vector<PickingFeature> MeshGeo3D::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (!m_meshGeometry)
        return features;
    
    // 获取基本几何数据
    const osg::Vec3Array* vertices = dynamic_cast<const osg::Vec3Array*>(
        m_meshGeometry->getVertexArray());
    
    if (!vertices || m_meshGeometry->getNumPrimitiveSets() == 0)
        return features;
    
    // 处理三角形面
    for (unsigned int i = 0; i < m_meshGeometry->getNumPrimitiveSets(); ++i)
    {
        const osg::PrimitiveSet* primitiveSet = m_meshGeometry->getPrimitiveSet(i);
        const osg::DrawElementsUInt* drawElements = 
            dynamic_cast<const osg::DrawElementsUInt*>(primitiveSet);
        
        if (drawElements && drawElements->getMode() == GL_TRIANGLES)
        {
            // 为每个三角形创建一个面Feature
            for (size_t j = 0; j < drawElements->size(); j += 3)
            {
                PickingFeature feature(FeatureType::FACE, static_cast<uint32_t>(j / 3));
                
                // 创建单个三角形的几何体
                osg::ref_ptr<osg::Geometry> faceGeometry = new osg::Geometry;
                faceGeometry->setVertexArray(const_cast<osg::Vec3Array*>(vertices));
                
                osg::ref_ptr<osg::DrawElementsUInt> faceElements = new osg::DrawElementsUInt(GL_TRIANGLES);
                faceElements->push_back((*drawElements)[j]);
                faceElements->push_back((*drawElements)[j + 1]);
                faceElements->push_back((*drawElements)[j + 2]);
                
                faceGeometry->addPrimitiveSet(faceElements);
                feature.geometry = faceGeometry;
                
                // 计算面的中心点
                const osg::Vec3& v0 = (*vertices)[(*drawElements)[j]];
                const osg::Vec3& v1 = (*vertices)[(*drawElements)[j + 1]];
                const osg::Vec3& v2 = (*vertices)[(*drawElements)[j + 2]];
                feature.center = (v0 + v1 + v2) / 3.0f;
                
                // 计算面的大小(最大边长)
                float d1 = (v1 - v0).length();
                float d2 = (v2 - v1).length(); 
                float d3 = (v0 - v2).length();
                feature.size = std::max({d1, d2, d3});
                
                features.push_back(feature);
            }
        }
    }
    
    return features;
}

// ============================================================================
// CompositeGeo3D Implementation (复合几何体)
// ============================================================================

CompositeGeo3D::CompositeGeo3D()
{
    markFeaturesDirty();
}

void CompositeGeo3D::addComponent(osg::ref_ptr<Geo3D> component)
{
    if (component)
    {
        m_components.push_back(component);
        markFeaturesDirty();
    }
}

void CompositeGeo3D::removeComponent(osg::ref_ptr<Geo3D> component)
{
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        m_components.erase(it);
        markFeaturesDirty();
    }
}

void CompositeGeo3D::clearComponents()
{
    m_components.clear();
    markFeaturesDirty();
}

std::vector<FeatureType> CompositeGeo3D::getSupportedFeatureTypes() const
{
    std::set<FeatureType> allTypes;
    
    // 收集所有子组件支持的Feature类型
    for (const auto& component : m_components)
    {
        if (component)
        {
            std::vector<FeatureType> componentTypes = component->getSupportedFeatureTypes();
            allTypes.insert(componentTypes.begin(), componentTypes.end());
        }
    }
    
    return std::vector<FeatureType>(allTypes.begin(), allTypes.end());
}

std::vector<PickingFeature> CompositeGeo3D::getFeatures(FeatureType type) const
{
    std::vector<PickingFeature> allFeatures;
    uint32_t featureOffset = 0;
    
    // 收集所有子组件的Feature
    for (const auto& component : m_components)
    {
        if (component)
        {
            std::vector<PickingFeature> componentFeatures = component->getFeatures(type);
            
            // 调整Feature索引，确保全局唯一
            for (auto& feature : componentFeatures)
            {
                feature.index += featureOffset;
                allFeatures.push_back(feature);
            }
            
            featureOffset += static_cast<uint32_t>(componentFeatures.size());
        }
    }
    
    return allFeatures;
}

void Point3D_Geo::updateGeometry()
{
    updateOSGNode();
}

// ========================================= 线类 =========================================
Line3D_Geo::Line3D_Geo()
{
    m_geoType = Geo_Line3D;
}

void Line3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() >= 2)
        {
            generatePolyline();
            updateGeometry();
        }
    }
}

void Line3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && !m_controlPoints.empty())
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

void Line3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (m_controlPoints.size() >= 2)
        {
            completeDrawing();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!m_controlPoints.empty())
        {
            removeControlPoint(m_controlPoints.size() - 1);
            updateGeometry();
        }
    }
}

osg::ref_ptr<osg::Geometry> Line3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2 && getTempPoint().position == glm::vec3(0))
        return nullptr;
    
    switch (m_parameters.nodeLineStyle)
    {
    case NodeLine_Polyline3D:
        generatePolyline();
        break;
    case NodeLine_Spline3D:
        generateSpline();
        break;
    case NodeLine_Bezier3D:
        generateBezierCurve();
        break;
    default:
        generatePolyline();
        break;
    }
    
    if (m_generatedPoints.empty())
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    for (const Point3D& point : m_generatedPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 如果正在绘制且有临时点，添加到最后一个点的线
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    return geometry;
}

void Line3D_Geo::generatePolyline()
{
    m_generatedPoints = m_controlPoints;
}

void Line3D_Geo::generateSpline()
{
    if (m_controlPoints.size() < 3)
    {
        generatePolyline();
        return;
    }
    
    m_generatedPoints.clear();
    
    int steps = m_parameters.steps > 0 ? m_parameters.steps : 20;
    
    // 简单的Catmull-Rom样条曲线实现
    for (int i = 0; i < static_cast<int>(m_controlPoints.size()) - 1; ++i)
    {
        glm::vec3 p0 = (i > 0) ? m_controlPoints[i-1].position : m_controlPoints[i].position;
        glm::vec3 p1 = m_controlPoints[i].position;
        glm::vec3 p2 = m_controlPoints[i+1].position;
        glm::vec3 p3 = (i+2 < static_cast<int>(m_controlPoints.size())) ? 
                       m_controlPoints[i+2].position : m_controlPoints[i+1].position;
        
        for (int j = 0; j < steps; ++j)
        {
            float t = static_cast<float>(j) / steps;
            float t2 = t * t;
            float t3 = t2 * t;
            
            glm::vec3 point = 0.5f * (
                (2.0f * p1) +
                (-p0 + p2) * t +
                (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
            );
            
            m_generatedPoints.push_back(Point3D(point));
        }
    }
    
    // 添加最后一个点
    m_generatedPoints.push_back(m_controlPoints.back());
}

void Line3D_Geo::generateBezierCurve()
{
    if (m_controlPoints.size() < 2)
        return;
    
    m_generatedPoints.clear();
    
    int steps = m_parameters.steps > 0 ? m_parameters.steps : 50;
    
    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec3 point = glm::vec3(0);
        
        // De Casteljau算法计算贝塞尔曲线点
        std::vector<glm::vec3> tempPoints;
        for (const Point3D& cp : m_controlPoints)
        {
            tempPoints.push_back(cp.position);
        }
        
        while (tempPoints.size() > 1)
        {
            std::vector<glm::vec3> newPoints;
            for (size_t j = 0; j < tempPoints.size() - 1; ++j)
            {
                newPoints.push_back(glm::mix(tempPoints[j], tempPoints[j+1], t));
            }
            tempPoints = newPoints;
        }
        
        if (!tempPoints.empty())
        {
            m_generatedPoints.push_back(Point3D(tempPoints[0]));
        }
    }
}

// ========================================= 圆弧类 =========================================
Arc3D_Geo::Arc3D_Geo()
    : m_radius(0.0f)
    , m_startAngle(0.0f)
    , m_endAngle(0.0f)
    , m_normal(0, 0, 1)
{
    m_geoType = Geo_Arc3D;
}

void Arc3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 3)
        {
            calculateArcFromThreePoints();
            generateArcPoints();
            completeDrawing();
        }
    }
}

void Arc3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 临时计算圆弧预览
            std::vector<Point3D> tempPoints = m_controlPoints;
            tempPoints.push_back(getTempPoint());
            
            // 临时保存当前控制点
            auto oldPoints = m_controlPoints;
            m_controlPoints = tempPoints;
            
            calculateArcFromThreePoints();
            generateArcPoints();
            
            // 恢复控制点
            m_controlPoints = oldPoints;
        }
        
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Arc3D_Geo::createGeometry()
{
    if (m_arcPoints.empty())
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    for (const Point3D& point : m_arcPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    return geometry;
}

void Arc3D_Geo::calculateArcFromThreePoints()
{
    if (m_controlPoints.size() < 3)
        return;
    
    glm::vec3 p1 = m_controlPoints[0].position;
    glm::vec3 p2 = m_controlPoints[1].position;
    glm::vec3 p3 = m_controlPoints[2].position;
    
    // 计算圆心
    glm::vec3 a = p2 - p1;
    glm::vec3 b = p3 - p2;
    
    // 计算法向量
    m_normal = glm::normalize(glm::cross(a, b));
    
    // 计算圆心
    glm::vec3 midAB = (p1 + p2) * 0.5f;
    glm::vec3 midBC = (p2 + p3) * 0.5f;
    
    glm::vec3 perpA = glm::cross(a, m_normal);
    glm::vec3 perpB = glm::cross(b, m_normal);
    
    // 解线性方程组找圆心
    float t = 0.0f;
    if (glm::length(perpA) > 1e-6 && glm::length(perpB) > 1e-6)
    {
        glm::vec3 diff = midBC - midAB;
        float denom = glm::dot(perpA, perpB);
        if (abs(denom) > 1e-6)
        {
            t = glm::dot(diff, perpB) / denom;
        }
    }
    
    m_center = midAB + t * perpA;
    m_radius = glm::length(p1 - m_center);
    
    // 计算起始和结束角度
    glm::vec3 ref = glm::normalize(p1 - m_center);
    glm::vec3 perpRef = glm::normalize(glm::cross(m_normal, ref));
    
    glm::vec3 v1 = glm::normalize(p1 - m_center);
    glm::vec3 v3 = glm::normalize(p3 - m_center);
    
    m_startAngle = atan2(glm::dot(v1, perpRef), glm::dot(v1, ref));
    m_endAngle = atan2(glm::dot(v3, perpRef), glm::dot(v3, ref));
    
    // 确保角度范围正确
    if (m_endAngle < m_startAngle)
        m_endAngle += 2.0f * M_PI;
}

void Arc3D_Geo::generateArcPoints()
{
    m_arcPoints.clear();
    
    if (m_radius <= 0)
        return;
    
    int segments = 50;
    float angleRange = m_endAngle - m_startAngle;
    
    for (int i = 0; i <= segments; ++i)
    {
        float t = static_cast<float>(i) / segments;
        float angle = m_startAngle + t * angleRange;
        
        glm::vec3 ref = glm::normalize(m_controlPoints[0].position - m_center);
        glm::vec3 perpRef = glm::normalize(glm::cross(m_normal, ref));
        
        glm::vec3 point = m_center + m_radius * (cos(angle) * ref + sin(angle) * perpRef);
        m_arcPoints.push_back(Point3D(point));
    }
}

void Arc3D_Geo::updateGeometry()
{
    updateOSGNode();
}

// ========================================= 贝塞尔曲线类 =========================================
BezierCurve3D_Geo::BezierCurve3D_Geo()
{
    m_geoType = Geo_BezierCurve3D;
}

void BezierCurve3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() >= 2)
        {
            generateBezierPoints();
            updateGeometry();
        }
    }
}

void BezierCurve3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && !m_controlPoints.empty())
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

void BezierCurve3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (m_controlPoints.size() >= 2)
        {
            completeDrawing();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!m_controlPoints.empty())
        {
            removeControlPoint(m_controlPoints.size() - 1);
            updateGeometry();
        }
    }
}

osg::ref_ptr<osg::Geometry> BezierCurve3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2)
        return nullptr;
    
    generateBezierPoints();
    
    if (m_bezierPoints.empty())
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    
    for (const Point3D& point : m_bezierPoints)
    {
        vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                   m_parameters.lineColor.b, m_parameters.lineColor.a));
    }
    
    // 如果正在绘制且有临时点，计算包含临时点的贝塞尔曲线
    if (!isStateComplete() && getTempPoint().position != glm::vec3(0))
    {
        std::vector<Point3D> tempControlPoints = m_controlPoints;
        tempControlPoints.push_back(getTempPoint());
        
        // 生成临时贝塞尔曲线点
        std::vector<Point3D> tempBezierPoints;
        int steps = m_parameters.steps > 0 ? m_parameters.steps : 50;
        
                 for (int i = 0; i <= steps; ++i)
         {
             float t = static_cast<float>(i) / steps;
             // 临时计算贝塞尔点
             std::vector<glm::vec3> tempVecs;
             for (const Point3D& cp : tempControlPoints)
             {
                 tempVecs.push_back(cp.position);
             }
             
             while (tempVecs.size() > 1)
             {
                 std::vector<glm::vec3> newVecs;
                 for (size_t j = 0; j < tempVecs.size() - 1; ++j)
                 {
                     newVecs.push_back(glm::mix(tempVecs[j], tempVecs[j+1], t));
                 }
                 tempVecs = newVecs;
             }
             
             if (!tempVecs.empty())
             {
                 tempBezierPoints.push_back(Point3D(tempVecs[0]));
             }
         }
        
        // 添加临时点（半透明）
        for (const Point3D& point : tempBezierPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
    
    return geometry;
}

void BezierCurve3D_Geo::generateBezierPoints()
{
    m_bezierPoints.clear();
    
    if (m_controlPoints.size() < 2)
        return;
    
    int steps = m_parameters.steps > 0 ? m_parameters.steps : 50;
    
    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec3 point = calculateBezierPoint(t);
        m_bezierPoints.push_back(Point3D(point));
    }
}

glm::vec3 BezierCurve3D_Geo::calculateBezierPoint(float t) const
{
    if (m_controlPoints.empty())
        return glm::vec3(0);
    
    // De Casteljau算法
    std::vector<glm::vec3> tempPoints;
    for (const Point3D& cp : m_controlPoints)
    {
        tempPoints.push_back(cp.position);
    }
    
    while (tempPoints.size() > 1)
    {
        std::vector<glm::vec3> newPoints;
        for (size_t j = 0; j < tempPoints.size() - 1; ++j)
        {
            newPoints.push_back(glm::mix(tempPoints[j], tempPoints[j+1], t));
        }
        tempPoints = newPoints;
    }
    
    return tempPoints.empty() ? glm::vec3(0) : tempPoints[0];
}

// ========================================= 三角形类 =========================================
Triangle3D_Geo::Triangle3D_Geo()
    : m_normal(0, 0, 1)
{
    m_geoType = Geo_Triangle3D;
}

void Triangle3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 3)
        {
            calculateNormal();
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Triangle3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Triangle3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    // 根据控制点数量决定绘制内容
    if (m_controlPoints.size() == 3 || (m_controlPoints.size() == 2 && getTempPoint().position != glm::vec3(0)))
    {
        // 三个点：绘制完整三角形
        Point3D p1 = m_controlPoints[0];
        Point3D p2 = m_controlPoints[1];
        Point3D p3 = (m_controlPoints.size() == 3) ? m_controlPoints[2] : getTempPoint();
        
        vertices->push_back(osg::Vec3(p1.x(), p1.y(), p1.z()));
        vertices->push_back(osg::Vec3(p2.x(), p2.y(), p2.z()));
        vertices->push_back(osg::Vec3(p3.x(), p3.y(), p3.z()));
        
        // 计算法向量
        glm::vec3 v1 = p2.position - p1.position;
        glm::vec3 v2 = p3.position - p1.position;
        glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
        
        for (int i = 0; i < 3; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
        }
        
        // 设置颜色
        Color3D color = isStateComplete() ? m_parameters.fillColor : 
                       Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                              m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
        
        for (int i = 0; i < 3; ++i)
        {
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
        
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
        
        // 如果需要显示边界
        if (m_parameters.showBorder)
        {
            osg::ref_ptr<osg::Vec4Array> borderColors = new osg::Vec4Array();
            for (int i = 0; i < 3; ++i)
            {
                borderColors->push_back(osg::Vec4(m_parameters.borderColor.r, m_parameters.borderColor.g, 
                                                 m_parameters.borderColor.b, m_parameters.borderColor.a));
            }
            
            // 创建边界线
            osg::ref_ptr<osg::DrawElementsUInt> borderIndices = new osg::DrawElementsUInt(GL_LINE_LOOP);
            borderIndices->push_back(0);
            borderIndices->push_back(1);
            borderIndices->push_back(2);
            
            geometry->addPrimitiveSet(borderIndices.get());
        }
    }
    else if (m_controlPoints.size() >= 1)
    {
        // 一个或两个点：绘制辅助线
        for (const Point3D& point : m_controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (getTempPoint().position != glm::vec3(0))
        {
            vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (vertices->size() >= 2)
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
        }
        else
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    if (!normals->empty())
    {
        geometry->setNormalArray(normals.get());
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    
    return geometry;
}

void Triangle3D_Geo::calculateNormal()
{
    if (m_controlPoints.size() >= 3)
    {
        glm::vec3 v1 = m_controlPoints[1].position - m_controlPoints[0].position;
        glm::vec3 v2 = m_controlPoints[2].position - m_controlPoints[0].position;
        m_normal = glm::normalize(glm::cross(v1, v2));
    }
}

// ========================================= 四边形类 =========================================
Quad3D_Geo::Quad3D_Geo()
    : m_normal(0, 0, 1)
{
    m_geoType = Geo_Quad3D;
}

void Quad3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 4)
        {
            calculateNormal();
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Quad3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Quad3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    if (m_controlPoints.size() == 4 || (m_controlPoints.size() == 3 && getTempPoint().position != glm::vec3(0)))
    {
        // 绘制四边形
        std::vector<Point3D> points = m_controlPoints;
        if (points.size() == 3)
            points.push_back(getTempPoint());
        
        // 添加顶点（按顺序）
        for (const Point3D& point : points)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
        }
        
        // 计算法向量
        glm::vec3 v1 = points[1].position - points[0].position;
        glm::vec3 v2 = points[2].position - points[0].position;
        glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
        
        for (int i = 0; i < 4; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
        }
        
        // 设置颜色
        Color3D color = isStateComplete() ? m_parameters.fillColor : 
                       Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                              m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
        
        for (int i = 0; i < 4; ++i)
        {
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
        
        // 将四边形三角化
        osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
        indices->push_back(0); indices->push_back(1); indices->push_back(2);
        indices->push_back(0); indices->push_back(2); indices->push_back(3);
        
        geometry->addPrimitiveSet(indices.get());
        
        // 边界线
        if (m_parameters.showBorder)
        {
            osg::ref_ptr<osg::DrawElementsUInt> borderIndices = new osg::DrawElementsUInt(GL_LINE_LOOP);
            borderIndices->push_back(0);
            borderIndices->push_back(1);
            borderIndices->push_back(2);
            borderIndices->push_back(3);
            
            geometry->addPrimitiveSet(borderIndices.get());
        }
    }
    else
    {
        // 绘制辅助线
        for (const Point3D& point : m_controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (getTempPoint().position != glm::vec3(0))
        {
            vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (vertices->size() >= 2)
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
        }
        else
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    if (!normals->empty())
    {
        geometry->setNormalArray(normals.get());
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    
    return geometry;
}

void Quad3D_Geo::calculateNormal()
{
    if (m_controlPoints.size() >= 3)
    {
        glm::vec3 v1 = m_controlPoints[1].position - m_controlPoints[0].position;
        glm::vec3 v2 = m_controlPoints[2].position - m_controlPoints[0].position;
        m_normal = glm::normalize(glm::cross(v1, v2));
    }
}

// ========================================= 多边形类 =========================================
Polygon3D_Geo::Polygon3D_Geo()
    : m_normal(0, 0, 1)
{
    m_geoType = Geo_Polygon3D;
}

void Polygon3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        updateGeometry();
    }
}

void Polygon3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

void Polygon3D_Geo::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (m_controlPoints.size() >= 3)
        {
            calculateNormal();
            triangulatePolygon();
            completeDrawing();
        }
    }
    else if (event->key() == Qt::Key_Escape)
    {
        if (!m_controlPoints.empty())
        {
            removeControlPoint(m_controlPoints.size() - 1);
            updateGeometry();
        }
    }
}

osg::ref_ptr<osg::Geometry> Polygon3D_Geo::createGeometry()
{
    if (m_controlPoints.size() < 2)
        return nullptr;
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    if (isStateComplete() && m_controlPoints.size() >= 3)
    {
        // 绘制完成的多边形
        for (const Point3D& point : m_controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            normals->push_back(osg::Vec3(m_normal.x, m_normal.y, m_normal.z));
            colors->push_back(osg::Vec4(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                                       m_parameters.fillColor.b, m_parameters.fillColor.a));
        }
        
        // 使用三角化的索引
        if (!m_triangleIndices.empty())
        {
            osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
            for (unsigned int index : m_triangleIndices)
            {
                indices->push_back(index);
            }
            geometry->addPrimitiveSet(indices.get());
        }
        else
        {
            // 简单扇形三角化
            for (size_t i = 1; i < m_controlPoints.size() - 1; ++i)
            {
                osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
                indices->push_back(0);
                indices->push_back(i);
                indices->push_back(i + 1);
                geometry->addPrimitiveSet(indices.get());
            }
        }
        
        // 边界线
        if (m_parameters.showBorder)
        {
            osg::ref_ptr<osg::DrawElementsUInt> borderIndices = new osg::DrawElementsUInt(GL_LINE_LOOP);
            for (size_t i = 0; i < m_controlPoints.size(); ++i)
            {
                borderIndices->push_back(i);
            }
            geometry->addPrimitiveSet(borderIndices.get());
        }
    }
    else
    {
        // 绘制过程中的辅助线
        for (const Point3D& point : m_controlPoints)
        {
            vertices->push_back(osg::Vec3(point.x(), point.y(), point.z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (getTempPoint().position != glm::vec3(0))
        {
            vertices->push_back(osg::Vec3(getTempPoint().x(), getTempPoint().y(), getTempPoint().z()));
            colors->push_back(osg::Vec4(m_parameters.lineColor.r, m_parameters.lineColor.g, 
                                       m_parameters.lineColor.b, m_parameters.lineColor.a * 0.5f));
            normals->push_back(osg::Vec3(0, 0, 1));
        }
        
        if (vertices->size() >= 2)
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, vertices->size()));
        }
        else
        {
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertices->size()));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    
    if (!normals->empty())
    {
        geometry->setNormalArray(normals.get());
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    }
    
    return geometry;
}

void Polygon3D_Geo::calculateNormal()
{
    if (m_controlPoints.size() >= 3)
    {
        glm::vec3 v1 = m_controlPoints[1].position - m_controlPoints[0].position;
        glm::vec3 v2 = m_controlPoints[2].position - m_controlPoints[0].position;
        m_normal = glm::normalize(glm::cross(v1, v2));
    }
}

void Polygon3D_Geo::triangulatePolygon()
{
    m_triangleIndices.clear();
    
    if (m_controlPoints.size() < 3)
        return;
    
    // 简单的耳切三角化算法
    // 这里使用简单的扇形三角化，更复杂的多边形可能需要更高级的算法
    for (size_t i = 1; i < m_controlPoints.size() - 1; ++i)
    {
        m_triangleIndices.push_back(0);
        m_triangleIndices.push_back(i);
        m_triangleIndices.push_back(i + 1);
    }
}

// ========================================= 长方体类 =========================================
Box3D_Geo::Box3D_Geo()
    : m_size(1.0f, 1.0f, 1.0f)
{
    m_geoType = Geo_Box3D;
}

void Box3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算长方体尺寸
            glm::vec3 diff = m_controlPoints[1].position - m_controlPoints[0].position;
            m_size = glm::abs(diff);
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Box3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Box3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    glm::vec3 size = m_size;
    glm::vec3 center = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        size = glm::abs(diff);
    }
    else if (m_controlPoints.size() == 2)
    {
        center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    float sx = size.x * 0.5f;
    float sy = size.y * 0.5f;
    float sz = size.z * 0.5f;
    
    // 定义长方体的8个顶点
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
    
    // 定义6个面的顶点索引和法向量
    std::vector<std::vector<int>> faces = {
        {0, 1, 2, 3}, // 底面 (z = -sz)
        {4, 7, 6, 5}, // 顶面 (z = sz)
        {0, 4, 5, 1}, // 前面 (y = -sy)
        {2, 6, 7, 3}, // 后面 (y = sy)
        {0, 3, 7, 4}, // 左面 (x = -sx)
        {1, 5, 6, 2}  // 右面 (x = sx)
    };
    
    std::vector<glm::vec3> faceNormals = {
        glm::vec3(0, 0, -1), // 底面
        glm::vec3(0, 0,  1), // 顶面
        glm::vec3(0, -1, 0), // 前面
        glm::vec3(0,  1, 0), // 后面
        glm::vec3(-1, 0, 0), // 左面
        glm::vec3( 1, 0, 0)  // 右面
    };
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 添加每个面的顶点
    for (size_t f = 0; f < faces.size(); ++f)
    {
        const std::vector<int>& face = faces[f];
        const glm::vec3& normal = faceNormals[f];
        
        // 三角化每个面 (四边形 -> 两个三角形)
        // 第一个三角形
        vertices->push_back(osg::Vec3(boxVertices[face[0]].x, boxVertices[face[0]].y, boxVertices[face[0]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[1]].x, boxVertices[face[1]].y, boxVertices[face[1]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[2]].x, boxVertices[face[2]].y, boxVertices[face[2]].z));
        
        // 第二个三角形
        vertices->push_back(osg::Vec3(boxVertices[face[0]].x, boxVertices[face[0]].y, boxVertices[face[0]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[2]].x, boxVertices[face[2]].y, boxVertices[face[2]].z));
        vertices->push_back(osg::Vec3(boxVertices[face[3]].x, boxVertices[face[3]].y, boxVertices[face[3]].z));
        
        // 法向量和颜色
        for (int i = 0; i < 6; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

// ========================================= 正方体类 =========================================
Cube3D_Geo::Cube3D_Geo()
    : m_size(1.0f)
{
    m_geoType = Geo_Cube3D;
}

void Cube3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算正方体边长（取最大边）
            glm::vec3 diff = m_controlPoints[1].position - m_controlPoints[0].position;
            m_size = std::max({abs(diff.x), abs(diff.y), abs(diff.z)});
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Cube3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Cube3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float size = m_size;
    glm::vec3 center = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        size = std::max({abs(diff.x), abs(diff.y), abs(diff.z)});
    }
    else if (m_controlPoints.size() == 2)
    {
        center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    }
    
    // 创建正方体几何体（复用长方体代码，但用相同的尺寸）
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    float s = size * 0.5f;
    
    // 定义正方体的8个顶点
    std::vector<glm::vec3> cubeVertices = {
        center + glm::vec3(-s, -s, -s), // 0
        center + glm::vec3( s, -s, -s), // 1
        center + glm::vec3( s,  s, -s), // 2
        center + glm::vec3(-s,  s, -s), // 3
        center + glm::vec3(-s, -s,  s), // 4
        center + glm::vec3( s, -s,  s), // 5
        center + glm::vec3( s,  s,  s), // 6
        center + glm::vec3(-s,  s,  s)  // 7
    };
    
    // 定义6个面的顶点索引和法向量
    std::vector<std::vector<int>> faces = {
        {0, 1, 2, 3}, // 底面
        {4, 7, 6, 5}, // 顶面
        {0, 4, 5, 1}, // 前面
        {2, 6, 7, 3}, // 后面
        {0, 3, 7, 4}, // 左面
        {1, 5, 6, 2}  // 右面
    };
    
    std::vector<glm::vec3> faceNormals = {
        glm::vec3(0, 0, -1), // 底面
        glm::vec3(0, 0,  1), // 顶面
        glm::vec3(0, -1, 0), // 前面
        glm::vec3(0,  1, 0), // 后面
        glm::vec3(-1, 0, 0), // 左面
        glm::vec3( 1, 0, 0)  // 右面
    };
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 添加每个面的顶点
    for (size_t f = 0; f < faces.size(); ++f)
    {
        const std::vector<int>& face = faces[f];
        const glm::vec3& normal = faceNormals[f];
        
        // 三角化每个面
        vertices->push_back(osg::Vec3(cubeVertices[face[0]].x, cubeVertices[face[0]].y, cubeVertices[face[0]].z));
        vertices->push_back(osg::Vec3(cubeVertices[face[1]].x, cubeVertices[face[1]].y, cubeVertices[face[1]].z));
        vertices->push_back(osg::Vec3(cubeVertices[face[2]].x, cubeVertices[face[2]].y, cubeVertices[face[2]].z));
        
        vertices->push_back(osg::Vec3(cubeVertices[face[0]].x, cubeVertices[face[0]].y, cubeVertices[face[0]].z));
        vertices->push_back(osg::Vec3(cubeVertices[face[2]].x, cubeVertices[face[2]].y, cubeVertices[face[2]].z));
        vertices->push_back(osg::Vec3(cubeVertices[face[3]].x, cubeVertices[face[3]].y, cubeVertices[face[3]].z));
        
        for (int i = 0; i < 6; ++i)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

// ========================================= 圆柱类 =========================================
Cylinder3D_Geo::Cylinder3D_Geo()
    : m_radius(1.0f)
    , m_height(2.0f)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Cylinder3D;
}

void Cylinder3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算圆柱参数
            glm::vec3 diff = m_controlPoints[1].position - m_controlPoints[0].position;
            m_height = glm::length(diff);
            if (m_height > 0)
                m_axis = glm::normalize(diff);
            m_radius = m_height * 0.3f; // 默认半径为高度的30%
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Cylinder3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Cylinder3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float radius = m_radius;
    float height = m_height;
    glm::vec3 axis = m_axis;
    glm::vec3 center = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - center;
        height = glm::length(diff);
        if (height > 0)
            axis = glm::normalize(diff);
        radius = height * 0.3f;
    }
    else if (m_controlPoints.size() == 2)
    {
        center = (m_controlPoints[0].position + m_controlPoints[1].position) * 0.5f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    
    // 计算圆柱的两个圆面中心
    glm::vec3 bottom = center - axis * (height * 0.5f);
    glm::vec3 top = center + axis * (height * 0.5f);
    
    // 计算垂直于轴的两个正交向量
    glm::vec3 u, v;
    if (abs(axis.z) < 0.9f)
    {
        u = glm::normalize(glm::cross(axis, glm::vec3(0, 0, 1)));
    }
    else
    {
        u = glm::normalize(glm::cross(axis, glm::vec3(1, 0, 0)));
    }
    v = glm::normalize(glm::cross(axis, u));
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 生成圆柱侧面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = cos(angle1) * u + sin(angle1) * v;
        glm::vec3 dir2 = cos(angle2) * u + sin(angle2) * v;
        
        glm::vec3 p1_bottom = bottom + radius * dir1;
        glm::vec3 p2_bottom = bottom + radius * dir2;
        glm::vec3 p1_top = top + radius * dir1;
        glm::vec3 p2_top = top + radius * dir2;
        
        // 第一个三角形
        vertices->push_back(osg::Vec3(p1_bottom.x, p1_bottom.y, p1_bottom.z));
        vertices->push_back(osg::Vec3(p2_bottom.x, p2_bottom.y, p2_bottom.z));
        vertices->push_back(osg::Vec3(p1_top.x, p1_top.y, p1_top.z));
        
        normals->push_back(osg::Vec3(dir1.x, dir1.y, dir1.z));
        normals->push_back(osg::Vec3(dir2.x, dir2.y, dir2.z));
        normals->push_back(osg::Vec3(dir1.x, dir1.y, dir1.z));
        
        // 第二个三角形
        vertices->push_back(osg::Vec3(p2_bottom.x, p2_bottom.y, p2_bottom.z));
        vertices->push_back(osg::Vec3(p2_top.x, p2_top.y, p2_top.z));
        vertices->push_back(osg::Vec3(p1_top.x, p1_top.y, p1_top.z));
        
        normals->push_back(osg::Vec3(dir2.x, dir2.y, dir2.z));
        normals->push_back(osg::Vec3(dir2.x, dir2.y, dir2.z));
        normals->push_back(osg::Vec3(dir1.x, dir1.y, dir1.z));
        
        // 添加颜色
        for (int j = 0; j < 6; ++j)
        {
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 生成底面和顶面
    // 底面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = cos(angle1) * u + sin(angle1) * v;
        glm::vec3 dir2 = cos(angle2) * u + sin(angle2) * v;
        
        glm::vec3 p1 = bottom + radius * dir1;
        glm::vec3 p2 = bottom + radius * dir2;
        
        vertices->push_back(osg::Vec3(bottom.x, bottom.y, bottom.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(-axis.x, -axis.y, -axis.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 顶面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = cos(angle1) * u + sin(angle1) * v;
        glm::vec3 dir2 = cos(angle2) * u + sin(angle2) * v;
        
        glm::vec3 p1 = top + radius * dir1;
        glm::vec3 p2 = top + radius * dir2;
        
        vertices->push_back(osg::Vec3(top.x, top.y, top.z));
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(axis.x, axis.y, axis.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    return geometry;
}

// ========================================= 圆锥类 =========================================
Cone3D_Geo::Cone3D_Geo()
    : m_radius(1.0f)
    , m_height(2.0f)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Cone3D;
}

void Cone3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算圆锥参数
            glm::vec3 diff = m_controlPoints[1].position - m_controlPoints[0].position;
            m_height = glm::length(diff);
            if (m_height > 0)
                m_axis = glm::normalize(diff);
            m_radius = m_height * 0.4f; // 默认半径为高度的40%
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Cone3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Cone3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float radius = m_radius;
    float height = m_height;
    glm::vec3 axis = m_axis;
    glm::vec3 base = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        glm::vec3 diff = getTempPoint().position - base;
        height = glm::length(diff);
        if (height > 0)
            axis = glm::normalize(diff);
        radius = height * 0.4f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int segments = static_cast<int>(m_parameters.subdivisionLevel);
    
    glm::vec3 apex = base + axis * height;
    
    // 计算垂直于轴的两个正交向量
    glm::vec3 u, v;
    if (abs(axis.z) < 0.9f)
    {
        u = glm::normalize(glm::cross(axis, glm::vec3(0, 0, 1)));
    }
    else
    {
        u = glm::normalize(glm::cross(axis, glm::vec3(1, 0, 0)));
    }
    v = glm::normalize(glm::cross(axis, u));
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 生成圆锥侧面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = cos(angle1) * u + sin(angle1) * v;
        glm::vec3 dir2 = cos(angle2) * u + sin(angle2) * v;
        
        glm::vec3 p1 = base + radius * dir1;
        glm::vec3 p2 = base + radius * dir2;
        
        // 计算侧面法向量（朝外）
        glm::vec3 edge1 = apex - p1;
        glm::vec3 edge2 = p2 - p1;
        glm::vec3 normal = glm::normalize(glm::cross(edge2, edge1));
        
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        vertices->push_back(osg::Vec3(apex.x, apex.y, apex.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 生成底面
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = 2.0f * M_PI * i / segments;
        float angle2 = 2.0f * M_PI * (i + 1) / segments;
        
        glm::vec3 dir1 = cos(angle1) * u + sin(angle1) * v;
        glm::vec3 dir2 = cos(angle2) * u + sin(angle2) * v;
        
        glm::vec3 p1 = base + radius * dir1;
        glm::vec3 p2 = base + radius * dir2;
        
        vertices->push_back(osg::Vec3(base.x, base.y, base.z));
        vertices->push_back(osg::Vec3(p2.x, p2.y, p2.z));
        vertices->push_back(osg::Vec3(p1.x, p1.y, p1.z));
        
        for (int j = 0; j < 3; ++j)
        {
            normals->push_back(osg::Vec3(-axis.x, -axis.y, -axis.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));
    
    return geometry;
} 

// ========================================= 球类 =========================================
Sphere3D_Geo::Sphere3D_Geo()
    : m_radius(1.0f)
{
    m_geoType = Geo_Sphere3D;
}

void Sphere3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算球的半径
            m_radius = glm::length(m_controlPoints[1].position - m_controlPoints[0].position);
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Sphere3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Sphere3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float radius = m_radius;
    glm::vec3 center = m_controlPoints[0].position;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        radius = glm::length(getTempPoint().position - center);
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int latSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int lonSegments = latSegments * 2;
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 生成球面顶点
    for (int lat = 0; lat <= latSegments; ++lat)
    {
        float theta = M_PI * lat / latSegments; // 纬度角 0 到 π
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= lonSegments; ++lon)
        {
            float phi = 2.0f * M_PI * lon / lonSegments; // 经度角 0 到 2π
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            // 球面坐标到笛卡尔坐标
            glm::vec3 normal(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            glm::vec3 point = center + radius * normal;
            
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
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
            
            // 第一个三角形
            indices->push_back(current);
            indices->push_back(next);
            indices->push_back(current + 1);
            
            // 第二个三角形
            indices->push_back(current + 1);
            indices->push_back(next);
            indices->push_back(next + 1);
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(indices.get());
    
    return geometry;
}

// ========================================= 圆环类 =========================================
Torus3D_Geo::Torus3D_Geo()
    : m_majorRadius(2.0f)
    , m_minorRadius(0.5f)
    , m_axis(0, 0, 1)
{
    m_geoType = Geo_Torus3D;
}

void Torus3D_Geo::mousePressEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete())
    {
        addControlPoint(Point3D(worldPos));
        
        if (m_controlPoints.size() == 2)
        {
            // 计算圆环参数
            float distance = glm::length(m_controlPoints[1].position - m_controlPoints[0].position);
            m_majorRadius = distance;
            m_minorRadius = distance * 0.2f; // 次半径为主半径的20%
            completeDrawing();
        }
        
        updateGeometry();
    }
}

void Torus3D_Geo::mouseMoveEvent(QMouseEvent* event, const glm::vec3& worldPos)
{
    if (!isStateComplete() && m_controlPoints.size() == 1)
    {
        setTempPoint(Point3D(worldPos));
        updateGeometry();
    }
}

osg::ref_ptr<osg::Geometry> Torus3D_Geo::createGeometry()
{
    if (m_controlPoints.empty())
        return nullptr;
    
    float majorRadius = m_majorRadius;
    float minorRadius = m_minorRadius;
    glm::vec3 center = m_controlPoints[0].position;
    glm::vec3 axis = m_axis;
    
    if (m_controlPoints.size() == 1 && getTempPoint().position != glm::vec3(0))
    {
        float distance = glm::length(getTempPoint().position - center);
        majorRadius = distance;
        minorRadius = distance * 0.2f;
    }
    
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array();
    
    int majorSegments = static_cast<int>(m_parameters.subdivisionLevel);
    int minorSegments = majorSegments / 2;
    
    // 计算垂直于轴的两个正交向量
    glm::vec3 u, v;
    if (abs(axis.z) < 0.9f)
    {
        u = glm::normalize(glm::cross(axis, glm::vec3(0, 0, 1)));
    }
    else
    {
        u = glm::normalize(glm::cross(axis, glm::vec3(1, 0, 0)));
    }
    v = glm::normalize(glm::cross(axis, u));
    
    Color3D color = isStateComplete() ? m_parameters.fillColor : 
                   Color3D(m_parameters.fillColor.r, m_parameters.fillColor.g, 
                          m_parameters.fillColor.b, m_parameters.fillColor.a * 0.5f);
    
    // 生成圆环顶点
    for (int i = 0; i <= majorSegments; ++i)
    {
        float majorAngle = 2.0f * M_PI * i / majorSegments;
        glm::vec3 majorCenter = center + majorRadius * (cos(majorAngle) * u + sin(majorAngle) * v);
        glm::vec3 majorTangent = glm::normalize(-sin(majorAngle) * u + cos(majorAngle) * v);
        
        for (int j = 0; j <= minorSegments; ++j)
        {
            float minorAngle = 2.0f * M_PI * j / minorSegments;
            
            // 在局部坐标系中计算次圆上的点
            glm::vec3 localPoint = minorRadius * (cos(minorAngle) * glm::normalize(glm::cross(majorTangent, axis)) + 
                                                 sin(minorAngle) * axis);
            glm::vec3 point = majorCenter + localPoint;
            
            // 计算法向量
            glm::vec3 normal = glm::normalize(localPoint);
            
            vertices->push_back(osg::Vec3(point.x, point.y, point.z));
            normals->push_back(osg::Vec3(normal.x, normal.y, normal.z));
            colors->push_back(osg::Vec4(color.r, color.g, color.b, color.a));
        }
    }
    
    // 生成三角形索引
    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES);
    
    for (int i = 0; i < majorSegments; ++i)
    {
        for (int j = 0; j < minorSegments; ++j)
        {
            int current = i * (minorSegments + 1) + j;
            int next = ((i + 1) % (majorSegments + 1)) * (minorSegments + 1) + j;
            
            // 第一个三角形
            indices->push_back(current);
            indices->push_back(next);
            indices->push_back(current + 1);
            
            // 第二个三角形
            indices->push_back(current + 1);
            indices->push_back(next);
            indices->push_back(next + 1);
        }
    }
    
    geometry->setVertexArray(vertices.get());
    geometry->setColorArray(colors.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalArray(normals.get());
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    
    geometry->addPrimitiveSet(indices.get());
    
    return geometry;
}

// ========================================= updateGeometry 实现 =========================================
void Line3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void BezierCurve3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Triangle3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Quad3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Polygon3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Box3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Cube3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Cylinder3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Cone3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Sphere3D_Geo::updateGeometry()
{
    updateOSGNode();
}

void Torus3D_Geo::updateGeometry()
{
    updateOSGNode();
}