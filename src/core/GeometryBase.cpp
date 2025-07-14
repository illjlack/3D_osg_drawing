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
    , m_parametersChanged(false)
{
    m_osgNode = new osg::Group();
    m_drawableGroup = new osg::Group();
    m_transformNode = new osg::MatrixTransform();
    m_controlPointsNode = new osg::Group();
    
    // 初始化点线面节点
    m_vertexNode = new osg::Group();
    m_edgeNode = new osg::Group();
    m_faceNode = new osg::Group();
    
    m_osgNode->addChild(m_transformNode.get());
    m_transformNode->addChild(m_drawableGroup.get());
    m_transformNode->addChild(m_controlPointsNode.get());
    
    // 将点线面节点添加到场景中
    m_transformNode->addChild(m_vertexNode.get());
    m_transformNode->addChild(m_edgeNode.get());
    m_transformNode->addChild(m_faceNode.get());
    
    // 初始化几何体状态
    initialize();
}

Geo3D::~Geo3D()
{
}

void Geo3D::setParameters(const GeoParameters3D& params)
{
    m_parameters = params;
    markGeometryDirty();
    emit parametersChanged(this);
}

void Geo3D::addControlPoint(const Point3D& point)
{
    m_controlPoints.push_back(point);
    updateBoundingBox();
    markGeometryDirty();
    emit geometryUpdated(this);
}

void Geo3D::setControlPoint(int index, const Point3D& point)
{
    if (index >= 0 && index < static_cast<int>(m_controlPoints.size()))
    {
        m_controlPoints[index] = point;
        updateBoundingBox();
        markGeometryDirty();
        emit geometryUpdated(this);
    }
}

void Geo3D::removeControlPoint(int index)
{
    if (index >= 0 && index < static_cast<int>(m_controlPoints.size()))
    {
        m_controlPoints.erase(m_controlPoints.begin() + index);
        updateBoundingBox();
        markGeometryDirty();
        emit geometryUpdated(this);
    }
}

void Geo3D::clearControlPoints()
{
    m_controlPoints.clear();
    m_boundingBox = BoundingBox3D();
    markGeometryDirty();
    emit geometryUpdated(this);
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
    emit drawingCompleted(this);
    emit stateChanged(this);
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
// RegularGeo3D Implementation
// ============================================================================

RegularGeo3D::RegularGeo3D()
{
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
}



// ============================================================================
// CompositeGeo3D Implementation
// ============================================================================

CompositeGeo3D::CompositeGeo3D()
{
}

void CompositeGeo3D::addComponent(osg::ref_ptr<Geo3D> component)
{
    if (component)
    {
        m_components.push_back(component);
    }
}

void CompositeGeo3D::removeComponent(osg::ref_ptr<Geo3D> component)
{
    auto it = std::find(m_components.begin(), m_components.end(), component);
    if (it != m_components.end())
    {
        m_components.erase(it);
    }
}

void CompositeGeo3D::clearComponents()
{
    m_components.clear();
}



osg::ref_ptr<osg::Geometry> CompositeGeo3D::createGeometry()
{
    // 复合几何体不直接创建几何体，而是由组件组成
    return nullptr;
}

void CompositeGeo3D::updateGeometry()
{
    // 更新所有组件
    for (auto& component : m_components)
    {
        if (component.valid())
        {
            component->updateGeometry();
        }
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

// ============================================================================
// 点线面节点管理方法实现
// ============================================================================

void Geo3D::addVertexGeometry(osg::ref_ptr<osg::Geometry> vertexGeo)
{
    if (vertexGeo.valid())
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode();
        geode->addDrawable(vertexGeo.get());
        m_vertexNode->addChild(geode.get());
    }
}

void Geo3D::addEdgeGeometry(osg::ref_ptr<osg::Geometry> edgeGeo)
{
    if (edgeGeo.valid())
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode();
        geode->addDrawable(edgeGeo.get());
        m_edgeNode->addChild(geode.get());
    }
}

void Geo3D::addFaceGeometry(osg::ref_ptr<osg::Geometry> faceGeo)
{
    if (faceGeo.valid())
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode();
        geode->addDrawable(faceGeo.get());
        m_faceNode->addChild(geode.get());
    }
}

void Geo3D::clearVertexGeometries()
{
    m_vertexNode->removeChildren(0, m_vertexNode->getNumChildren());
}

void Geo3D::clearEdgeGeometries()
{
    m_edgeNode->removeChildren(0, m_edgeNode->getNumChildren());
}

void Geo3D::clearFaceGeometries()
{
    m_faceNode->removeChildren(0, m_faceNode->getNumChildren());
}

void Geo3D::setShowPoints(bool show)
{
    m_parameters.showPoints = show;
    updateFeatureVisibility();
    emit parametersChanged(this);
}

void Geo3D::setShowEdges(bool show)
{
    m_parameters.showEdges = show;
    updateFeatureVisibility();
    emit parametersChanged(this);
}

void Geo3D::setShowFaces(bool show)
{
    m_parameters.showFaces = show;
    updateFeatureVisibility();
    emit parametersChanged(this);
}

void Geo3D::updateFeatureVisibility()
{
    // 更新顶点节点可见性
    m_vertexNode->setNodeMask(m_parameters.showPoints ? 0xffffffff : 0x0);
    
    // 更新边节点可见性
    m_edgeNode->setNodeMask(m_parameters.showEdges ? 0xffffffff : 0x0);
    
    // 更新面节点可见性
    m_faceNode->setNodeMask(m_parameters.showFaces ? 0xffffffff : 0x0);
} 