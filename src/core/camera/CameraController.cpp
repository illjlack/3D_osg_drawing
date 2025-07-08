#include "CameraController.h"
#include <osg/Matrix>
#include <osg/Math>
#include <osg/Camera>
#include <osg/Viewport>
#include <cmath>

CameraController::CameraController()
    : m_viewer(nullptr)
    , m_manipulator(new osgGA::TrackballManipulator)
    , m_projectionMode(ProjectionMode::Perspective)
    , m_perspectiveFOV(45.0)
    , m_perspectiveNear(0.1)
    , m_perspectiveFar(1000.0)
    , m_orthographicLeft(-10.0)
    , m_orthographicRight(10.0)
    , m_orthographicBottom(-10.0)
    , m_orthographicTop(10.0)
    , m_orthographicNear(-100.0)
    , m_orthographicFar(100.0)
    , m_directionCacheValid(false)
{
}

CameraController::~CameraController()
{
}

void CameraController::setViewer(osgViewer::Viewer* viewer)
{
    m_viewer = viewer;
    if (m_viewer)
    {
        m_viewer->setCameraManipulator(m_manipulator);
    }
}

osg::Vec3d CameraController::getEyePosition() const
{
    if (!m_manipulator) return osg::Vec3d(0, 0, 0);
    
    osg::Vec3d eye, center, up;
    m_manipulator->getInverseMatrix().getLookAt(eye, center, up);
    return eye;
}

osg::Vec3d CameraController::getCenterPosition() const
{
    if (!m_manipulator) return osg::Vec3d(0, 0, 0);
    
    osg::Vec3d eye, center, up;
    m_manipulator->getInverseMatrix().getLookAt(eye, center, up);
    return center;
}

osg::Vec3d CameraController::getCameraUpVector() const
{
    if (!m_manipulator) return osg::Vec3d(0, 0, 1);
    
    osg::Vec3d eye, center, up;
    m_manipulator->getInverseMatrix().getLookAt(eye, center, up);
    return up;
}

void CameraController::updateDirectionCache() const
{
    if (!m_manipulator) return;
    
    osg::Vec3d eye, center, up;
    m_manipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 计算摄像机的前方向（从eye指向center）
    m_cachedForward = center - eye;
    m_cachedForward.normalize();
    
    // 计算摄像机的右方向（前方向与上方向的叉积）
    m_cachedRight = m_cachedForward ^ up;
    m_cachedRight.normalize();
    
    // 计算摄像机的上方向（右方向与前方向的叉积，确保正交）
    m_cachedUp = m_cachedRight ^ m_cachedForward;
    m_cachedUp.normalize();
    
    m_directionCacheValid = true;
}

osg::Vec3d CameraController::getForwardVector() const
{
    if (!m_directionCacheValid)
    {
        updateDirectionCache();
    }
    return m_cachedForward;
}

osg::Vec3d CameraController::getRightVector() const
{
    if (!m_directionCacheValid)
    {
        updateDirectionCache();
    }
    return m_cachedRight;
}

osg::Vec3d CameraController::getUpVector() const
{
    if (!m_directionCacheValid)
    {
        updateDirectionCache();
    }
    return m_cachedUp;
}

void CameraController::moveForward(double distance)
{
    if (!m_manipulator) return;
    
    osg::Vec3d eye, center, up;
    m_manipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 计算摄像机的前方向（从eye指向center的方向）
    osg::Vec3d forward = center - eye;
    forward.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye + forward * distance;
    osg::Vec3d newCenter = center + forward * distance;
    
    m_manipulator->setHomePosition(newEye, newCenter, up);
    m_manipulator->home(0.0);
    
    m_directionCacheValid = false; // 使缓存失效
}

void CameraController::moveBackward(double distance)
{
    moveForward(-distance);
}

void CameraController::moveLeft(double distance)
{
    if (!m_manipulator) return;
    
    osg::Vec3d eye, center, up;
    m_manipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 计算摄像机的右方向（前方向与上方向的叉积）
    osg::Vec3d forward = center - eye;
    forward.normalize();
    osg::Vec3d right = forward ^ up;
    right.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye - right * distance;
    osg::Vec3d newCenter = center - right * distance;
    
    m_manipulator->setHomePosition(newEye, newCenter, up);
    m_manipulator->home(0.0);
    
    m_directionCacheValid = false;
}

void CameraController::moveRight(double distance)
{
    moveLeft(-distance);
}

void CameraController::moveUp(double distance)
{
    if (!m_manipulator) return;
    
    osg::Vec3d eye, center, up;
    m_manipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 使用当前的上方向向量
    osg::Vec3d upVector = up;
    upVector.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye + upVector * distance;
    osg::Vec3d newCenter = center + upVector * distance;
    
    m_manipulator->setHomePosition(newEye, newCenter, up);
    m_manipulator->home(0.0);
    
    m_directionCacheValid = false;
}

void CameraController::moveDown(double distance)
{
    moveUp(-distance);
}

void CameraController::rotateHorizontal(double angle)
{
    if (!m_manipulator) return;
    
    // 这里可以实现水平旋转，暂时使用OSG的默认旋转
    m_directionCacheValid = false;
}

void CameraController::rotateVertical(double angle)
{
    if (!m_manipulator) return;
    
    // 这里可以实现垂直旋转，暂时使用OSG的默认旋转
    m_directionCacheValid = false;
}

void CameraController::setPosition(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up)
{
    if (!m_manipulator) return;
    
    m_manipulator->setHomePosition(eye, center, up);
    m_manipulator->home(0.0);
    
    m_directionCacheValid = false;
}

void CameraController::setHomePosition(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up)
{
    if (!m_manipulator) return;
    
    m_manipulator->setHomePosition(eye, center, up);
    m_directionCacheValid = false;
}

void CameraController::home()
{
    if (!m_manipulator) return;
    
    m_manipulator->home(0.0);
    m_directionCacheValid = false;
}

osg::Matrix CameraController::getViewMatrix() const
{
    if (!m_manipulator) return osg::Matrix::identity();
    return m_manipulator->getInverseMatrix();
}

osg::Matrix CameraController::getInverseViewMatrix() const
{
    if (!m_manipulator) return osg::Matrix::identity();
    return m_manipulator->getInverseMatrix();
}

osg::Vec3d CameraController::screenToWorld(int screenX, int screenY, double depth, int viewportWidth, int viewportHeight) const
{
    if (!m_viewer || !m_viewer->getCamera()) return osg::Vec3d(0, 0, 0);
    
    osg::Camera* camera = m_viewer->getCamera();
    osg::Viewport* viewport = camera->getViewport();
    if (!viewport) return osg::Vec3d(0, 0, 0);
    
    osg::Matrix VPW = camera->getViewMatrix() * 
                     camera->getProjectionMatrix() * 
                     viewport->computeWindowMatrix();
    osg::Matrix invVPW;
    invVPW.invert(VPW);
    
    osg::Vec3f nearPoint = osg::Vec3f(screenX, viewportHeight - screenY, depth) * invVPW;
    return osg::Vec3d(nearPoint.x(), nearPoint.y(), nearPoint.z());
}

osg::Vec2d CameraController::worldToScreen(const osg::Vec3d& worldPos, int viewportWidth, int viewportHeight) const
{
    if (!m_viewer || !m_viewer->getCamera()) return osg::Vec2d(0, 0);
    
    osg::Camera* camera = m_viewer->getCamera();
    osg::Viewport* viewport = camera->getViewport();
    if (!viewport) return osg::Vec2d(0, 0);
    
    osg::Matrix VPW = camera->getViewMatrix() * 
                     camera->getProjectionMatrix() * 
                     viewport->computeWindowMatrix();
    
    osg::Vec3f world(worldPos.x(), worldPos.y(), worldPos.z());
    osg::Vec3f screen = world * VPW;
    return osg::Vec2d(screen.x(), viewportHeight - screen.y());
}

double CameraController::calculateAdaptiveMoveSpeed(double baseSpeed) const
{
    if (!m_viewer || !m_viewer->getCamera()) return baseSpeed;
    
    if (m_projectionMode == ProjectionMode::Orthographic)
    {
        // 正交投影模式：基于正交投影的大小计算移动速度
        double orthoWidth = m_orthographicRight - m_orthographicLeft;
        double orthoHeight = m_orthographicTop - m_orthographicBottom;
        double orthoSize = std::max(orthoWidth, orthoHeight);
        
        // 正交模式下，移动速度与正交投影大小成反比
        double adaptiveSpeed = baseSpeed * (10.0 / orthoSize); // 基准值10单位
        
        // 限制速度范围
        adaptiveSpeed = std::max(adaptiveSpeed, baseSpeed * 0.1); // 最小速度
        adaptiveSpeed = std::min(adaptiveSpeed, baseSpeed * 10.0); // 最大速度
        
        return adaptiveSpeed;
    }
    else
    {
        // 透视投影模式：使用原来的计算方法
        osg::Camera* camera = m_viewer->getCamera();
        osg::Viewport* viewport = camera->getViewport();
        if (!viewport) return baseSpeed;
        
        // 获取相机位置
        osg::Vec3d eye = getEyePosition();
        osg::Vec3d center = getCenterPosition();
        
        // 计算相机到中心的距离
        double distance = (eye - center).length();
        
        // 计算屏幕像素对应的世界单位
        double screenHeight = viewport->height();
        double fov = m_perspectiveFOV; // 使用当前设置的FOV
        double worldHeight = 2.0 * distance * tan(osg::DegreesToRadians(fov / 2.0));
        double pixelsPerUnit = screenHeight / worldHeight;
        
        // 根据比例尺调整移动速度
        // 当缩放越近（pixelsPerUnit越大）时，移动速度应该越快
        double adaptiveSpeed = baseSpeed * (pixelsPerUnit / 100.0); // 基准值100像素/单位
        
        // 限制速度范围
        adaptiveSpeed = std::max(adaptiveSpeed, baseSpeed * 0.1); // 最小速度
        adaptiveSpeed = std::min(adaptiveSpeed, baseSpeed * 10.0); // 最大速度
        
        return adaptiveSpeed;
    }
}

// ========================================= 投影模式相关方法 =========================================

void CameraController::setProjectionMode(ProjectionMode mode)
{
    m_projectionMode = mode;
    if (m_viewer && m_viewer->getCamera())
    {
        updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                              m_viewer->getCamera()->getViewport()->height());
    }
}

void CameraController::setPerspectiveFOV(double fov)
{
    m_perspectiveFOV = fov;
    if (m_projectionMode == ProjectionMode::Perspective && m_viewer && m_viewer->getCamera())
    {
        updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                              m_viewer->getCamera()->getViewport()->height());
    }
}

void CameraController::setPerspectiveNearFar(double near, double far)
{
    m_perspectiveNear = near;
    m_perspectiveFar = far;
    if (m_projectionMode == ProjectionMode::Perspective && m_viewer && m_viewer->getCamera())
    {
        updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                              m_viewer->getCamera()->getViewport()->height());
    }
}

void CameraController::setOrthographicSize(double left, double right, double bottom, double top)
{
    m_orthographicLeft = left;
    m_orthographicRight = right;
    m_orthographicBottom = bottom;
    m_orthographicTop = top;
    if (m_projectionMode == ProjectionMode::Orthographic && m_viewer && m_viewer->getCamera())
    {
        updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                              m_viewer->getCamera()->getViewport()->height());
    }
}

void CameraController::setOrthographicNearFar(double near, double far)
{
    m_orthographicNear = near;
    m_orthographicFar = far;
    if (m_projectionMode == ProjectionMode::Orthographic && m_viewer && m_viewer->getCamera())
    {
        updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                              m_viewer->getCamera()->getViewport()->height());
    }
}

void CameraController::updateProjectionMatrix(int viewportWidth, int viewportHeight)
{
    if (!m_viewer || !m_viewer->getCamera()) return;
    
    osg::Camera* camera = m_viewer->getCamera();
    
    if (m_projectionMode == ProjectionMode::Perspective)
    {
        // 透视投影
        double aspectRatio = static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight);
        camera->setProjectionMatrixAsPerspective(m_perspectiveFOV, aspectRatio, m_perspectiveNear, m_perspectiveFar);
    }
    else
    {
        // 正交投影
        // 根据视口宽高比调整正交投影的边界
        double aspectRatio = static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight);
        double currentWidth = m_orthographicRight - m_orthographicLeft;
        double currentHeight = m_orthographicTop - m_orthographicBottom;
        
        double targetWidth, targetHeight;
        if (aspectRatio > 1.0) // 宽屏
        {
            targetWidth = currentHeight * aspectRatio;
            targetHeight = currentHeight;
        }
        else // 竖屏
        {
            targetWidth = currentWidth;
            targetHeight = currentWidth / aspectRatio;
        }
        
        double centerX = (m_orthographicLeft + m_orthographicRight) * 0.5;
        double centerY = (m_orthographicBottom + m_orthographicTop) * 0.5;
        
        double left = centerX - targetWidth * 0.5;
        double right = centerX + targetWidth * 0.5;
        double bottom = centerY - targetHeight * 0.5;
        double top = centerY + targetHeight * 0.5;
        
        camera->setProjectionMatrixAsOrtho(left, right, bottom, top, m_orthographicNear, m_orthographicFar);
    }
} 