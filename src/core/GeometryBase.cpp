#include "GeometryBase.h"
#include <osg/Array>
#include <osg/Shape>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    m_drawableGroup = new osg::Group();
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
}

void Geo3D::addControlPoint(const Point3D& point)
{
    m_controlPoints.push_back(point);
    updateBoundingBox();
    markGeometryDirty();
    markFeaturesDirty();
}

void Geo3D::setControlPoint(int index, const Point3D& point)
{
    if (index >= 0 && index < static_cast<int>(m_controlPoints.size()))
    {
        m_controlPoints[index] = point;
        updateBoundingBox();
        markGeometryDirty();
        markFeaturesDirty();
    }
}

void Geo3D::removeControlPoint(int index)
{
    if (index >= 0 && index < static_cast<int>(m_controlPoints.size()))
    {
        m_controlPoints.erase(m_controlPoints.begin() + index);
        updateBoundingBox();
        markGeometryDirty();
        markFeaturesDirty();
    }
}

void Geo3D::clearControlPoints()
{
    m_controlPoints.clear();
    m_boundingBox = BoundingBox3D();
    markGeometryDirty();
    markFeaturesDirty();
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

// ============================================================================
// IPickingProvider Implementation
// ============================================================================

std::vector<FeatureType> Geo3D::getSupportedFeatureTypes() const
{
    return {};
}

std::vector<PickingFeature> Geo3D::getFeatures(FeatureType type) const
{
    return getCachedFeatures(type);
}

std::vector<PickingFeature> Geo3D::getCachedFeatures(FeatureType type) const
{
    auto it = m_cachedFeatures.find(type);
    if (it != m_cachedFeatures.end() && !m_featuresDirty)
    {
        return it->second;
    }
    
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
    
    m_cachedFeatures[type] = features;
    
    return features;
}

// ============================================================================
// RegularGeo3D Implementation
// ============================================================================

RegularGeo3D::RegularGeo3D()
{
}

std::vector<PickingFeature> RegularGeo3D::extractFaceFeatures() const
{
    return {};
}

std::vector<PickingFeature> RegularGeo3D::extractEdgeFeatures() const
{
    return {};
}

std::vector<PickingFeature> RegularGeo3D::extractVertexFeatures() const
{
    std::vector<PickingFeature> features;
    
    for (size_t i = 0; i < m_controlPoints.size(); ++i)
    {
        PickingFeature feature(FeatureType::VERTEX, static_cast<uint32_t>(i));
        feature.center = osg::Vec3(m_controlPoints[i].x(), m_controlPoints[i].y(), m_controlPoints[i].z());
        feature.size = 0.05f;
        features.push_back(feature);
    }
    
    return features;
}

// ============================================================================
// MeshGeo3D Implementation
// ============================================================================

MeshGeo3D::MeshGeo3D()
{
}

void MeshGeo3D::setMeshData(osg::ref_ptr<osg::Geometry> geometry)
{
    m_meshGeometry = geometry;
    markGeometryDirty();
    markFeaturesDirty();
}

std::vector<PickingFeature> MeshGeo3D::extractFaceFeatures() const
{
    std::vector<PickingFeature> features;
    
    if (!m_meshGeometry.valid())
        return features;
    
    // 从三角网格中提取面Feature的简化实现
    // 子类可以重写以提供更详细的实现
    
    return features;
}

// ============================================================================
// CompositeGeo3D Implementation
// ============================================================================

CompositeGeo3D::CompositeGeo3D()
{
}

void CompositeGeo3D::addComponent(osg::ref_ptr<Geo3D> component)
{
    if (component.valid())
    {
        m_components.push_back(component);
        m_drawableGroup->addChild(component->getOSGNode());
        markFeaturesDirty();
    }
}

void CompositeGeo3D::removeComponent(osg::ref_ptr<Geo3D> component)
{
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        m_drawableGroup->removeChild(component->getOSGNode());
        m_components.erase(it);
        markFeaturesDirty();
    }
}

void CompositeGeo3D::clearComponents()
{
    for (auto& component : m_components)
    {
        m_drawableGroup->removeChild(component->getOSGNode());
    }
    m_components.clear();
    markFeaturesDirty();
}

std::vector<PickingFeature> CompositeGeo3D::getFeatures(FeatureType type) const
{
    std::vector<PickingFeature> allFeatures;
    
    for (const auto& component : m_components)
    {
        std::vector<PickingFeature> componentFeatures = component->getFeatures(type);
        allFeatures.insert(allFeatures.end(), componentFeatures.begin(), componentFeatures.end());
    }
    
    return allFeatures;
}

osg::ref_ptr<osg::Geometry> CompositeGeo3D::createGeometry()
{
    return nullptr;
}

void CompositeGeo3D::updateGeometry()
{
    for (auto& component : m_components)
    {
        component->updateGeometry();
    }
}

// ============================================================================
// 工厂函数（临时实现）
// ============================================================================

#include "../util/GeometryFactory.h"

Geo3D* createGeo3D(DrawMode3D mode)
{
    return GeometryFactory::createGeometry(mode);
} 