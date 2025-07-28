#include "CameraController.h"
#include <osg/Matrix>
#include <osg/Math>
#include <osg/Camera>
#include <osg/Viewport>
#include <cmath>
#include <QTimer>
#include <QDateTime>
#include "../../util/LogManager.h"

CameraController::CameraController()
    : m_viewer(nullptr)
    , m_currentManipulatorType(ManipulatorType::Trackball)
    , m_projectionMode(ProjectionMode::Perspective)
    , m_fov(45.0)
    , m_near(0.001)
    , m_far(10000.0)
    , m_left(-10.0)
    , m_right(10.0)
    , m_bottom(-10.0)
    , m_top(10.0)
    , m_directionCacheValid(false)
    , m_cameraMoveSpeed(0.2)
    , m_wheelMoveSensitivity(0.3)
    , m_accelerationRate(1.2)
    , m_maxAccelerationSpeed(5.0)
    , m_updateTimer(new QTimer(this))
    , m_lastMoveTime(0)
    , m_isMoving(false)
{
    LOG_INFO("创建相机控制器", "相机");
    
    // 初始化移动键状态
    for (int i = 0; i < 6; ++i) {
        m_cameraMoveKeys[i] = false;
        m_accelerationSpeeds[i] = 0.0;
    }
    
    // 初始化定时器（保留但不使用，以防将来需要）
    m_updateTimer->setInterval(16); // 约60FPS
    // connect(m_updateTimer, &QTimer::timeout, this, &CameraController::onUpdateTimer);
    
    // 初始化所有操控器
    initializeManipulators();
    
    // 相机控制器初始化完成（移除调试日志）
}

CameraController::~CameraController()
{
    LOG_INFO("销毁相机控制器", "相机");
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void CameraController::initializeManipulators()
{
    LOG_INFO("初始化相机操控器", "相机");
    
    // 创建各种操控器
    m_trackballManipulator = new osgGA::TrackballManipulator;
    m_firstPersonManipulator = new osgGA::FirstPersonManipulator;
    m_flightManipulator = new osgGA::FlightManipulator;
    m_driveManipulator = new osgGA::DriveManipulator;
    
    // 创建键盘切换操控器
    m_keySwitchManipulator = new osgGA::KeySwitchMatrixManipulator;
    m_keySwitchManipulator->addMatrixManipulator('1', "Trackball", m_trackballManipulator.get());
    m_keySwitchManipulator->addMatrixManipulator('2', "FirstPerson", m_firstPersonManipulator.get());
    m_keySwitchManipulator->addMatrixManipulator('3', "Flight", m_flightManipulator.get());
    m_keySwitchManipulator->addMatrixManipulator('4', "Drive", m_driveManipulator.get());
    
    // 设置默认操控器
    m_currentManipulator = static_cast<osgGA::CameraManipulator*>(m_trackballManipulator.get());
    m_keySwitchManipulator->selectMatrixManipulator(0);
    
    LOG_INFO("相机操控器初始化完成", "相机");
}

void CameraController::setViewer(osgViewer::Viewer* viewer)
{
    m_viewer = viewer;
    if (m_viewer && m_keySwitchManipulator)
    {
        m_viewer->setCameraManipulator(m_keySwitchManipulator);
        LOG_INFO("相机操控器已设置到OSG查看器", "相机");
    }
    else
    {
        LOG_WARNING("设置相机操控器失败：查看器或操控器为空", "相机");
    }
}

void CameraController::setManipulatorType(ManipulatorType type)
{
    if (m_currentManipulatorType != type) {
        QString oldType = getManipulatorTypeString(m_currentManipulatorType);
        QString newType = getManipulatorTypeString(type);
        
        LOG_INFO(QString("切换相机操控器: %1 -> %2").arg(oldType).arg(newType), "相机");
        
        switchManipulator(type);
        m_currentManipulatorType = type;
        emit manipulatorTypeChanged(type);
    }
}

void CameraController::switchManipulator(ManipulatorType type)
{
    if (!m_keySwitchManipulator) return;
    
    switch (type) {
        case ManipulatorType::Trackball:
            m_currentManipulator = static_cast<osgGA::CameraManipulator*>(m_trackballManipulator.get());
            m_keySwitchManipulator->selectMatrixManipulator(0);
            break;
        case ManipulatorType::FirstPerson:
            m_currentManipulator = static_cast<osgGA::CameraManipulator*>(m_firstPersonManipulator.get());
            m_keySwitchManipulator->selectMatrixManipulator(1);
            break;
        case ManipulatorType::Flight:
            m_currentManipulator = static_cast<osgGA::CameraManipulator*>(m_flightManipulator.get());
            m_keySwitchManipulator->selectMatrixManipulator(2);
            break;
        case ManipulatorType::Drive:
            m_currentManipulator = static_cast<osgGA::CameraManipulator*>(m_driveManipulator.get());
            m_keySwitchManipulator->selectMatrixManipulator(3);
            break;
    }
    
    // 切换到操控器（移除调试日志）
    invalidateDirectionCache();
}

void CameraController::switchToNextManipulator()
{
    int currentIndex = static_cast<int>(m_currentManipulatorType);
    int nextIndex = (currentIndex + 1) % 4;
    ManipulatorType nextType = static_cast<ManipulatorType>(nextIndex);
    
    LOG_INFO(QString("切换到下一个操控器: %1 -> %2")
             .arg(getManipulatorTypeString(m_currentManipulatorType))
             .arg(getManipulatorTypeString(nextType)), "相机");
    
    setManipulatorType(nextType);
}

void CameraController::switchToPreviousManipulator()
{
    int currentIndex = static_cast<int>(m_currentManipulatorType);
    int prevIndex = (currentIndex - 1 + 4) % 4;
    ManipulatorType prevType = static_cast<ManipulatorType>(prevIndex);
    
    LOG_INFO(QString("切换到上一个操控器: %1 -> %2")
             .arg(getManipulatorTypeString(m_currentManipulatorType))
             .arg(getManipulatorTypeString(prevType)), "相机");
    
    setManipulatorType(prevType);
}

osgGA::CameraManipulator* CameraController::getCurrentManipulator() const
{
    return m_currentManipulator;
}

osg::Vec3d CameraController::getEyePosition() const
{
    if (!m_currentManipulator) {
        LOG_WARNING("获取相机位置失败：当前操控器为空", "相机");
        return osg::Vec3d(0, 0, 0);
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 移除DEBUG日志，因为这个函数在每一帧都被调用
    return eye;
}

osg::Vec3d CameraController::getCenterPosition() const
{
    if (!m_currentManipulator) {
        LOG_WARNING("获取相机中心位置失败：当前操控器为空", "相机");
        return osg::Vec3d(0, 0, 0);
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 移除DEBUG日志，因为这个函数在每一帧都被调用
    return center;
}

osg::Vec3d CameraController::getCameraUpVector() const
{
    if (!m_currentManipulator) {
        LOG_WARNING("获取相机上方向失败：当前操控器为空", "相机");
        return osg::Vec3d(0, 0, 1);
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 移除DEBUG日志，因为这个函数在每一帧都被调用
    return up;
}

void CameraController::updateDirectionCache() const
{
    if (!m_currentManipulator) {
        LOG_WARNING("更新方向缓存失败：当前操控器为空", "相机");
        return;
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
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
    
    // 注释掉频繁的DEBUG日志
    // LOG_DEBUG(QString("更新方向缓存: 前方向(%1,%2,%3) 右方向(%4,%5,%6) 上方向(%7,%8,%9)")
    //           .arg(m_cachedForward.x(), 0, 'f', 3).arg(m_cachedForward.y(), 0, 'f', 3).arg(m_cachedForward.z(), 0, 'f', 3)
    //           .arg(m_cachedRight.x(), 0, 'f', 3).arg(m_cachedRight.y(), 0, 'f', 3).arg(m_cachedRight.z(), 0, 'f', 3)
    //           .arg(m_cachedUp.x(), 0, 'f', 3).arg(m_cachedUp.y(), 0, 'f', 3).arg(m_cachedUp.z(), 0, 'f', 3), "相机");
}

void CameraController::invalidateDirectionCache()
{
    if (m_directionCacheValid) {
        // 注释掉频繁的DEBUG日志
        // LOG_DEBUG("方向缓存已失效", "相机");
        m_directionCacheValid = false;
    }
}

osg::Vec3d CameraController::getForwardVector() const
{
    if (!m_directionCacheValid)
    {
        updateDirectionCache();
    }
    
    // 移除DEBUG日志，因为这个函数在每一帧都被调用
    return m_cachedForward;
}

osg::Vec3d CameraController::getRightVector() const
{
    if (!m_directionCacheValid)
    {
        updateDirectionCache();
    }
    
    // 移除DEBUG日志，因为这个函数在每一帧都被调用
    return m_cachedRight;
}

osg::Vec3d CameraController::getUpVector() const
{
    if (!m_directionCacheValid)
    {
        updateDirectionCache();
    }
    
    // 移除DEBUG日志，因为这个函数在每一帧都被调用
    return m_cachedUp;
}

void CameraController::moveForward(double distance)
{
    if (!m_currentManipulator) {
        LOG_WARNING("向前移动失败：当前操控器为空", "相机");
        return;
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 计算摄像机的前方向（从eye指向center的方向）
    osg::Vec3d forward = center - eye;
    forward.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye + forward * distance;
    osg::Vec3d newCenter = center + forward * distance;
    
    // 直接设置操控器的矩阵
    osg::Matrix lookAtMatrix = osg::Matrix::lookAt(newEye, newCenter, up);
    m_currentManipulator->setByInverseMatrix(lookAtMatrix);
    
    // 相机向前移动（移除频繁的调试日志）
    
    invalidateDirectionCache();
}

void CameraController::moveBackward(double distance)
{
    if (!m_currentManipulator) {
        LOG_WARNING("向后移动失败：当前操控器为空", "相机");
        return;
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 计算摄像机的前方向（从eye指向center的方向）
    osg::Vec3d forward = center - eye;
    forward.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye - forward * distance;
    osg::Vec3d newCenter = center - forward * distance;
    
    // 直接设置操控器的矩阵
    osg::Matrix lookAtMatrix = osg::Matrix::lookAt(newEye, newCenter, up);
    m_currentManipulator->setByInverseMatrix(lookAtMatrix);
    
    // 相机向后移动（移除频繁的调试日志）
    
    invalidateDirectionCache();
}

void CameraController::moveLeft(double distance)
{
    if (!m_currentManipulator) {
        LOG_WARNING("向左移动失败：当前操控器为空", "相机");
        return;
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 计算摄像机的右方向（前方向与上方向的叉积）
    osg::Vec3d forward = center - eye;
    forward.normalize();
    osg::Vec3d right = forward ^ up;
    right.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye - right * distance;
    osg::Vec3d newCenter = center - right * distance;
    
    // 直接设置操控器的矩阵
    osg::Matrix lookAtMatrix = osg::Matrix::lookAt(newEye, newCenter, up);
    m_currentManipulator->setByInverseMatrix(lookAtMatrix);
    
    // 相机向左移动（移除频繁的调试日志）
    
    invalidateDirectionCache();
}

void CameraController::moveRight(double distance)
{
    if (!m_currentManipulator) {
        LOG_WARNING("向右移动失败：当前操控器为空", "相机");
        return;
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 计算摄像机的右方向（前方向与上方向的叉积）
    osg::Vec3d forward = center - eye;
    forward.normalize();
    osg::Vec3d right = forward ^ up;
    right.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye + right * distance;
    osg::Vec3d newCenter = center + right * distance;
    
    // 直接设置操控器的矩阵
    osg::Matrix lookAtMatrix = osg::Matrix::lookAt(newEye, newCenter, up);
    m_currentManipulator->setByInverseMatrix(lookAtMatrix);
    
    // 相机向右移动（移除频繁的调试日志）
    
    invalidateDirectionCache();
}

void CameraController::moveUp(double distance)
{
    if (!m_currentManipulator) {
        LOG_WARNING("向上移动失败：当前操控器为空", "相机");
        return;
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 使用当前的上方向向量
    osg::Vec3d upVector = up;
    upVector.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye + upVector * distance;
    osg::Vec3d newCenter = center + upVector * distance;
    
    // 直接设置操控器的矩阵
    osg::Matrix lookAtMatrix = osg::Matrix::lookAt(newEye, newCenter, up);
    m_currentManipulator->setByInverseMatrix(lookAtMatrix);
    
    // 相机向上移动（移除频繁的调试日志）
    
    invalidateDirectionCache();
}

void CameraController::moveDown(double distance)
{
    if (!m_currentManipulator) {
        LOG_WARNING("向下移动失败：当前操控器为空", "相机");
        return;
    }
    
    osg::Vec3d eye, center, up;
    m_currentManipulator->getInverseMatrix().getLookAt(eye, center, up);
    
    // 使用当前的上方向向量
    osg::Vec3d upVector = up;
    upVector.normalize();
    
    // 同时移动eye和center，保持相对位置不变
    osg::Vec3d newEye = eye - upVector * distance;
    osg::Vec3d newCenter = center - upVector * distance;
    
    // 直接设置操控器的矩阵
    osg::Matrix lookAtMatrix = osg::Matrix::lookAt(newEye, newCenter, up);
    m_currentManipulator->setByInverseMatrix(lookAtMatrix);
    
    // 相机向下移动（移除频繁的调试日志）
    
    invalidateDirectionCache();
}



void CameraController::rotateHorizontal(double angle)
{
    if (!m_currentManipulator) {
        LOG_WARNING("水平旋转失败：当前操控器为空", "相机");
        return;
    }
    
    // 水平旋转（移除调试日志）
    
    // 这里可以实现水平旋转，暂时使用OSG的默认旋转
    invalidateDirectionCache();
}

void CameraController::rotateVertical(double angle)
{
    if (!m_currentManipulator) {
        LOG_WARNING("垂直旋转失败：当前操控器为空", "相机");
        return;
    }
    
    // 垂直旋转（移除调试日志）
    
    // 这里可以实现垂直旋转，暂时使用OSG的默认旋转
    m_directionCacheValid = false;
}

void CameraController::setPosition(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up)
{
    if (!m_currentManipulator) {
        LOG_WARNING("设置相机位置失败：当前操控器为空", "相机");
        return;
    }
    
    LOG_INFO(QString("设置相机位置: 眼睛(%1,%2,%3) 中心(%4,%5,%6) 上方向(%7,%8,%9)")
             .arg(eye.x(), 0, 'f', 2).arg(eye.y(), 0, 'f', 2).arg(eye.z(), 0, 'f', 2)
             .arg(center.x(), 0, 'f', 2).arg(center.y(), 0, 'f', 2).arg(center.z(), 0, 'f', 2)
             .arg(up.x(), 0, 'f', 2).arg(up.y(), 0, 'f', 2).arg(up.z(), 0, 'f', 2), "相机");
    
    m_currentManipulator->setHomePosition(eye, center, up);
    m_currentManipulator->home(0.0);
    
    m_directionCacheValid = false;
}

void CameraController::setHomePosition(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up)
{
    if (!m_currentManipulator) {
        LOG_WARNING("设置初始位置失败：当前操控器为空", "相机");
        return;
    }
    
    LOG_INFO(QString("设置相机初始位置: 眼睛(%1,%2,%3) 中心(%4,%5,%6) 上方向(%7,%8,%9)")
             .arg(eye.x(), 0, 'f', 2).arg(eye.y(), 0, 'f', 2).arg(eye.z(), 0, 'f', 2)
             .arg(center.x(), 0, 'f', 2).arg(center.y(), 0, 'f', 2).arg(center.z(), 0, 'f', 2)
             .arg(up.x(), 0, 'f', 2).arg(up.y(), 0, 'f', 2).arg(up.z(), 0, 'f', 2), "相机");
    
    m_currentManipulator->setHomePosition(eye, center, up);
    m_directionCacheValid = false;
}

void CameraController::home()
{
    if (!m_currentManipulator) {
        LOG_WARNING("回到初始位置失败：当前操控器为空", "相机");
        return;
    }
    
    LOG_INFO("相机回到初始位置", "相机");
    
    m_currentManipulator->home(0.0);
    m_directionCacheValid = false;
}

void CameraController::setRotationCenter(const osg::Vec3d& center)
{
    if (!m_currentManipulator) {
        LOG_WARNING("设置旋转中心失败：当前操控器为空", "相机");
        return;
    }
    
    // 主要针对轨道球操控器
    if (m_currentManipulatorType == ManipulatorType::Trackball && m_trackballManipulator) {
        // 获取当前相机位置和方向
        osg::Vec3d currentEye, currentCenter, currentUp;
        m_currentManipulator->getInverseMatrix().getLookAt(currentEye, currentCenter, currentUp);
        
        // 计算新的距离（从眼点到新中心的距离）
        double newDistance = (currentEye - center).length();
        
        // 只设置新的中心点和距离，保持眼点位置不变
        m_trackballManipulator->setCenter(center);
        m_trackballManipulator->setDistance(newDistance);
        
        LOG_INFO(QString("相机旋转中心已设置为: (%1, %2, %3), 新距离: %4")
                 .arg(center.x(), 0, 'f', 2)
                 .arg(center.y(), 0, 'f', 2)
                 .arg(center.z(), 0, 'f', 2)
                 .arg(newDistance, 0, 'f', 2), "相机");
    } else {
        // 对于其他操控器类型，使用通用方法
        // 获取当前状态
        osg::Vec3d currentEye, currentCenter, currentUp;
        m_currentManipulator->getInverseMatrix().getLookAt(currentEye, currentCenter, currentUp);
        
        // 保持眼点位置不变，只改变观察中心
        osg::Matrix lookAtMatrix = osg::Matrix::lookAt(currentEye, center, currentUp);
        m_currentManipulator->setByInverseMatrix(lookAtMatrix);
        
        LOG_INFO(QString("相机中心已设置为: (%1, %2, %3)，眼点位置保持不变")
                 .arg(center.x(), 0, 'f', 2)
                 .arg(center.y(), 0, 'f', 2)
                 .arg(center.z(), 0, 'f', 2), "相机");
    }
    
    m_directionCacheValid = false;
}

osg::Vec3d CameraController::getRotationCenter() const
{
    if (!m_currentManipulator) {
        LOG_WARNING("获取旋转中心失败：当前操控器为空", "相机");
        return osg::Vec3d(0, 0, 0);
    }
    
    // 对于轨道球操控器，直接获取中心点
    if (m_currentManipulatorType == ManipulatorType::Trackball && m_trackballManipulator) {
        return m_trackballManipulator->getCenter();
    } else {
        // 对于其他操控器，返回当前观察点
        return getCenterPosition();
    }
}

osg::Vec3d CameraController::screenToWorld(int screenX, int screenY, double depth, int viewportWidth, int viewportHeight) const
{
    if (!m_viewer || !m_viewer->getCamera()) {
        LOG_WARNING("screenToWorld失败：查看器或相机为空", "相机");
        return osg::Vec3d(0, 0, 0);
    }
    
    osg::Camera* camera = m_viewer->getCamera();
    osg::Viewport* viewport = camera->getViewport();
    if (!viewport) {
        LOG_WARNING("screenToWorld失败：视口为空", "相机");
        return osg::Vec3d(0, 0, 0);
    }
    
    osg::Matrix VPW = camera->getViewMatrix() * 
                     camera->getProjectionMatrix() * 
                     viewport->computeWindowMatrix();
    osg::Matrix invVPW;
    invVPW.invert(VPW);
    
    osg::Vec3f nearPoint = osg::Vec3f(screenX, viewportHeight - screenY, depth) * invVPW;
    osg::Vec3d result(nearPoint.x(), nearPoint.y(), nearPoint.z());
    
    // LOG_DEBUG(QString("屏幕坐标转世界坐标: 屏幕(%1,%2) 深度=%3 -> 世界(%4,%5,%6)")
    //           .arg(screenX).arg(screenY)
    //           .arg(depth, 0, 'f', 3)
    //           .arg(result.x(), 0, 'f', 3)
    //           .arg(result.y(), 0, 'f', 3)
    //           .arg(result.z(), 0, 'f', 3), "相机");
    
    return result;
}

osg::Vec2d CameraController::worldToScreen(const osg::Vec3d& worldPos, int viewportWidth, int viewportHeight) const
{
    if (!m_viewer || !m_viewer->getCamera()) {
        LOG_WARNING("worldToScreen失败：查看器或相机为空", "相机");
        return osg::Vec2d(0, 0);
    }
    
    osg::Camera* camera = m_viewer->getCamera();
    osg::Viewport* viewport = camera->getViewport();
    if (!viewport) {
        LOG_WARNING("worldToScreen失败：视口为空", "相机");
        return osg::Vec2d(0, 0);
    }
    
    osg::Matrix VPW = camera->getViewMatrix() * 
                     camera->getProjectionMatrix() * 
                     viewport->computeWindowMatrix();
    
    osg::Vec3f world(worldPos.x(), worldPos.y(), worldPos.z());
    osg::Vec3f screen = world * VPW;
    osg::Vec2d result(screen.x(), viewportHeight - screen.y());
    
    // LOG_DEBUG(QString("世界坐标转屏幕坐标: 世界(%1,%2,%3) -> 屏幕(%4,%5)")
    //           .arg(worldPos.x(), 0, 'f', 3)
    //           .arg(worldPos.y(), 0, 'f', 3)
    //           .arg(worldPos.z(), 0, 'f', 3)
    //           .arg(result.x(), 0, 'f', 1)
    //           .arg(result.y(), 0, 'f', 1), "相机");
    
    return result;
}

double CameraController::calculateAdaptiveMoveSpeed(double baseSpeed) const
{
    if (!m_viewer || !m_viewer->getCamera()) {
        LOG_WARNING("计算自适应移动速度失败：查看器或相机为空", "相机");
        return baseSpeed;
    }
    
    double adaptiveSpeed = baseSpeed;
    
    if (m_projectionMode == ProjectionMode::Orthographic)
    {
        // 正交投影模式：基于正交投影的大小计算移动速度
        double orthoWidth = m_right - m_left;
        double orthoHeight = m_top - m_bottom;
        double orthoSize = std::max(orthoWidth, orthoHeight);
        
        // 正交模式下，移动速度与正交投影大小成反比
        adaptiveSpeed = baseSpeed * (10.0 / orthoSize); // 基准值10单位
        
        // 限制速度范围
        adaptiveSpeed = std::max(adaptiveSpeed, baseSpeed * 0.1); // 最小速度
        adaptiveSpeed = std::min(adaptiveSpeed, baseSpeed * 10.0); // 最大速度
        
        // LOG_DEBUG(QString("正交模式自适应速度: 基础速度=%1, 正交大小=%2, 自适应速度=%3")
        //           .arg(baseSpeed, 0, 'f', 2)
        //           .arg(orthoSize, 0, 'f', 2)
        //           .arg(adaptiveSpeed, 0, 'f', 2), "相机");
    }
    else
    {
        // 透视投影模式：使用原来的计算方法
        osg::Camera* camera = m_viewer->getCamera();
        osg::Viewport* viewport = camera->getViewport();
        if (!viewport) {
            LOG_WARNING("计算透视模式自适应速度失败：视口为空", "相机");
            return baseSpeed;
        }
        
        // 获取相机位置
        osg::Vec3d eye = getEyePosition();
        osg::Vec3d center = getCenterPosition();
        
        // 计算相机到中心的距离
        double distance = (eye - center).length();
        
        // 计算屏幕像素对应的世界单位
        double screenHeight = viewport->height();
        double fov = m_fov; // 使用当前设置的FOV
        double worldHeight = 2.0 * distance * tan(osg::DegreesToRadians(fov / 2.0));
        double pixelsPerUnit = screenHeight / worldHeight;
        
        // 根据比例尺调整移动速度
        // 当缩放越近（pixelsPerUnit越大）时，移动速度应该越快
        adaptiveSpeed = baseSpeed * (pixelsPerUnit / 100.0); // 基准值100像素/单位
        
        // 限制速度范围
        adaptiveSpeed = std::max(adaptiveSpeed, baseSpeed * 0.1); // 最小速度
        adaptiveSpeed = std::min(adaptiveSpeed, baseSpeed * 10.0); // 最大速度
        
        // LOG_DEBUG(QString("透视模式自适应速度: 基础速度=%1, 距离=%2, FOV=%3°, 像素/单位=%4, 自适应速度=%5")
        //           .arg(baseSpeed, 0, 'f', 2)
        //           .arg(distance, 0, 'f', 2)
        //           .arg(fov, 0, 'f', 1)
        //           .arg(pixelsPerUnit, 0, 'f', 2)
        //           .arg(adaptiveSpeed, 0, 'f', 2), "相机");
    }
    
    return adaptiveSpeed;
}

// ========================================= 投影模式相关方法 =========================================

void CameraController::setProjectionMode(ProjectionMode mode)
{
    if (m_projectionMode != mode) {
        QString oldMode = (m_projectionMode == ProjectionMode::Perspective) ? "透视" : "正交";
        QString newMode = (mode == ProjectionMode::Perspective) ? "透视" : "正交";
        
        LOG_INFO(QString("切换投影模式: %1 -> %2").arg(oldMode).arg(newMode), "相机");
        
        m_projectionMode = mode;
        if (m_viewer && m_viewer->getCamera())
        {
            updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                                  m_viewer->getCamera()->getViewport()->height());
        }
    }
}

void CameraController::setProjectionModeSmooth(ProjectionMode mode)
{
    if (m_projectionMode == mode) return; // 已经是目标模式
    
    QString oldMode = (m_projectionMode == ProjectionMode::Perspective) ? "透视" : "正交";
    QString newMode = (mode == ProjectionMode::Perspective) ? "透视" : "正交";
    
    LOG_INFO(QString("平滑切换投影模式: %1 -> %2").arg(oldMode).arg(newMode), "相机");
    
    // 在切换前保存当前的视图状态
    osg::Vec3d currentEye = getEyePosition();
    osg::Vec3d currentCenter = getCenterPosition();
    osg::Vec3d currentUp = getCameraUpVector();
    
    // 切换到新模式
    m_projectionMode = mode;
    
    // 根据切换方向调整参数以保持视图连续性
    if (mode == ProjectionMode::Orthographic)
    {
        // 从透视切换到正交：根据当前FOV和距离计算合适的正交边界
        double distance = (currentEye - currentCenter).length();
        double fovRadians = osg::DegreesToRadians(m_fov);
        double halfHeight = distance * tan(fovRadians / 2.0);
        double halfWidth = halfHeight * 1.0; // 假设1:1的宽高比，实际会根据视口调整
        
        // 设置正交投影边界，保持视图中心不变
        m_left = -halfWidth;
        m_right = halfWidth;
        m_bottom = -halfHeight;
        m_top = halfHeight;
        
        LOG_DEBUG(QString("透视->正交: 距离=%1, FOV=%2°, 正交边界=[%3,%4]x[%5,%6]")
                  .arg(distance, 0, 'f', 2)
                  .arg(m_fov, 0, 'f', 1)
                  .arg(m_left, 0, 'f', 2).arg(m_right, 0, 'f', 2)
                  .arg(m_bottom, 0, 'f', 2).arg(m_top, 0, 'f', 2), "相机");
    }
    else
    {
        // 从正交切换到透视：根据当前正交边界计算合适的FOV
        double currentWidth = m_right - m_left;
        double currentHeight = m_top - m_bottom;
        double distance = (currentEye - currentCenter).length();
        
        // 计算等效的FOV
        double halfHeight = currentHeight / 2.0;
        double fovRadians = 2.0 * atan(halfHeight / distance);
        m_fov = osg::RadiansToDegrees(fovRadians);
        
        LOG_DEBUG(QString("正交->透视: 距离=%1, 正交边界=[%2,%3]x[%4,%5], 计算FOV=%6°")
                  .arg(distance, 0, 'f', 2)
                  .arg(m_left, 0, 'f', 2).arg(m_right, 0, 'f', 2)
                  .arg(m_bottom, 0, 'f', 2).arg(m_top, 0, 'f', 2)
                  .arg(m_fov, 0, 'f', 1), "相机");
    }
    
    // 更新投影矩阵
    if (m_viewer && m_viewer->getCamera())
    {
        updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                              m_viewer->getCamera()->getViewport()->height());
    }
}

// 统一的投影参数设置函数
void CameraController::setFOV(double fov)
{
    if (m_fov != fov) {
        // 设置FOV（移除调试日志）
        m_fov = fov;
        if (m_projectionMode == ProjectionMode::Perspective && m_viewer && m_viewer->getCamera())
        {
            updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                                  m_viewer->getCamera()->getViewport()->height());
        }
    }
}

void CameraController::setNearFar(double near, double far)
{
    if (m_near != near || m_far != far) {
        LOG_DEBUG(QString("设置近远平面: near=%1 -> %2, far=%3 -> %4")
                  .arg(m_near, 0, 'f', 2).arg(near, 0, 'f', 2)
                  .arg(m_far, 0, 'f', 2).arg(far, 0, 'f', 2), "相机");
        m_near = near;
        m_far = far;
        if (m_viewer && m_viewer->getCamera())
        {
            updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                                  m_viewer->getCamera()->getViewport()->height());
        }
    }
}

void CameraController::setViewSize(double left, double right, double bottom, double top)
{
    if (m_left != left || m_right != right || m_bottom != bottom || m_top != top) {
        LOG_DEBUG(QString("设置正交视图大小: [%1,%2]x[%3,%4] -> [%5,%6]x[%7,%8]")
                  .arg(m_left, 0, 'f', 2).arg(m_right, 0, 'f', 2)
                  .arg(m_bottom, 0, 'f', 2).arg(m_top, 0, 'f', 2)
                  .arg(left, 0, 'f', 2).arg(right, 0, 'f', 2)
                  .arg(bottom, 0, 'f', 2).arg(top, 0, 'f', 2), "相机");
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        if (m_projectionMode == ProjectionMode::Orthographic && m_viewer && m_viewer->getCamera())
        {
            updateProjectionMatrix(m_viewer->getCamera()->getViewport()->width(), 
                                  m_viewer->getCamera()->getViewport()->height());
        }
    }
}



void CameraController::updateProjectionMatrix(int viewportWidth, int viewportHeight)
{
    if (!m_viewer || !m_viewer->getCamera()) {
        LOG_WARNING("更新投影矩阵失败：查看器或相机为空", "相机");
        return;
    }
    
    osg::Camera* camera = m_viewer->getCamera();
    
    if (m_projectionMode == ProjectionMode::Perspective)
    {
        // 透视投影：使用FOV、宽高比、近远平面
        double aspectRatio = static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight);
        camera->setProjectionMatrixAsPerspective(m_fov, aspectRatio, m_near, m_far);
        
         LOG_DEBUG(QString("更新透视投影矩阵: FOV=%1°, 宽高比=%2, 近平面=%3, 远平面=%4")
                   .arg(m_fov, 0, 'f', 1)
                   .arg(aspectRatio, 0, 'f', 3)
                   .arg(m_near, 0, 'f', 2)
                   .arg(m_far, 0, 'f', 2), "相机");
    }
    else
    {
        // 正交投影：根据视口宽高比动态调整正交投影边界
        double aspectRatio = static_cast<double>(viewportWidth) / static_cast<double>(viewportHeight);
        double currentWidth = m_right - m_left;
        double currentHeight = m_top - m_bottom;
        
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
        
        double centerX = (m_left + m_right) * 0.5;
        double centerY = (m_bottom + m_top) * 0.5;
        
        double left = centerX - targetWidth * 0.5;
        double right = centerX + targetWidth * 0.5;
        double bottom = centerY - targetHeight * 0.5;
        double top = centerY + targetHeight * 0.5;
        
        camera->setProjectionMatrixAsOrtho(left, right, bottom, top, m_near, m_far);
        
        // 注释掉频繁的DEBUG日志
        // LOG_DEBUG(QString("更新正交投影矩阵: 边界=[%1,%2]x[%3,%4], 宽高比=%5, 近平面=%6, 远平面=%7")
        //           .arg(left, 0, 'f', 2).arg(right, 0, 'f', 2)
        //           .arg(bottom, 0, 'f', 2).arg(top, 0, 'f', 2)
        //           .arg(aspectRatio, 0, 'f', 3)
        //           .arg(m_near, 0, 'f', 2)
        //           .arg(m_far, 0, 'f', 2), "相机");
    }
}

// ========================================= 摄像机移动控制方法（从OSGWidget转移） =========================================

void CameraController::setCameraMoveSpeed(double speed)
{
    if (m_cameraMoveSpeed != speed) {
        LOG_INFO(QString("设置相机移动速度: %1").arg(speed, 0, 'f', 2), "相机");
        m_cameraMoveSpeed = speed;
        emit cameraMoveSpeedChanged(speed);
    }
}

void CameraController::setWheelMoveSensitivity(double sensitivity)
{
    if (m_wheelMoveSensitivity != sensitivity) {
        LOG_INFO(QString("设置滚轮灵敏度: %1").arg(sensitivity, 0, 'f', 2), "相机");
        m_wheelMoveSensitivity = sensitivity;
        emit wheelMoveSensitivityChanged(sensitivity);
    }
}

void CameraController::setAccelerationRate(double rate)
{
    if (m_accelerationRate != rate) {
        // 设置加速度率（移除调试日志）
        m_accelerationRate = rate;
        emit accelerationRateChanged(rate);
    }
}

void CameraController::setMaxAccelerationSpeed(double speed)
{
    if (m_maxAccelerationSpeed != speed) {
        // 设置最大加速度速度（移除调试日志）
        m_maxAccelerationSpeed = speed;
        emit maxAccelerationSpeedChanged(speed);
    }
}

void CameraController::resetAllAcceleration()
{
    // 重置所有加速度（移除调试日志）
    for (int i = 0; i < 6; ++i) {
        m_accelerationSpeeds[i] = 0.0;
    }
}

void CameraController::setKeyPressed(int key, bool pressed)
{
    QString keyName;
    int keyIndex = -1;
    
    switch (key) {
        case Qt::Key_W: // 前
        case Qt::Key_Up: // 前
            keyIndex = 4;
            keyName = "前";
            m_cameraMoveKeys[4] = pressed;
            break;
        case Qt::Key_S: // 后
        case Qt::Key_Down: // 后
            keyIndex = 5;
            keyName = "后";
            m_cameraMoveKeys[5] = pressed;
            break;
        case Qt::Key_A: // 左
        case Qt::Key_Left: // 左
            keyIndex = 2;
            keyName = "左";
            m_cameraMoveKeys[2] = pressed;
            break;
        case Qt::Key_D: // 右
        case Qt::Key_Right: // 右
            keyIndex = 3;
            keyName = "右";
            m_cameraMoveKeys[3] = pressed;
            break;
        case Qt::Key_Q: // 上
        case Qt::Key_PageUp: // 上
            keyIndex = 0;
            keyName = "上";
            m_cameraMoveKeys[0] = pressed;
            break;
        case Qt::Key_E: // 下
        case Qt::Key_PageDown: // 下
            keyIndex = 1;
            keyName = "下";
            m_cameraMoveKeys[1] = pressed;
            break;
    }
    
    if (keyIndex >= 0) {
        // 键盘事件（移除频繁的日志）
    }
    
    // 检查是否有任何按键按下
    bool anyKeyPressed = false;
    for (int i = 0; i < 6; ++i) {
        if (m_cameraMoveKeys[i]) {
            anyKeyPressed = true;
            break;
        }
    }
    
    if (anyKeyPressed && !m_isMoving) {
        // 开始移动
        m_isMoving = true;
        m_lastMoveTime = QDateTime::currentMSecsSinceEpoch();
    } else if (!anyKeyPressed && m_isMoving) {
        // 停止移动
        m_isMoving = false;
        resetAllAcceleration();
    }
    
    // 如果按键按下，立即执行一次移动
    if (pressed && keyIndex >= 0) {
        updateCameraPosition();
    }
}

void CameraController::updateCameraPosition()
{
    if (!m_isMoving) return;
    
    // 计算时间差
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDiff = currentTime - m_lastMoveTime;
    double deltaTime = timeDiff / 1000.0; // 转换为秒
    
    if (deltaTime < 0.001) return; // 时间间隔太小，跳过
    
    double adaptiveSpeed = calculateAdaptiveMoveSpeed(m_cameraMoveSpeed);
    
    // 更新加速度
    for (int i = 0; i < 6; ++i) {
        if (m_cameraMoveKeys[i]) {
            // 增加加速度 - 修复：如果加速度为0，则设置为1.0作为初始值
            if (m_accelerationSpeeds[i] < 0.01) {
                m_accelerationSpeeds[i] = 1.0;
            } else {
                m_accelerationSpeeds[i] = std::min(m_accelerationSpeeds[i] * m_accelerationRate, m_maxAccelerationSpeed);
            }
        } else {
            // 减少加速度
            m_accelerationSpeeds[i] *= 0.8; // 衰减因子
        }
    }
    
    // 计算移动距离（基于时间）
    double moveDistance = adaptiveSpeed * deltaTime;
    
    // 应用移动
    bool moved = false;
    QStringList moveDirections;
    
    if (m_accelerationSpeeds[0] > 0.01) { 
        moveUp(moveDistance * m_accelerationSpeeds[0]); 
        moved = true; 
        moveDirections << QString("上(%1)").arg(m_accelerationSpeeds[0], 0, 'f', 2);
    }
    if (m_accelerationSpeeds[1] > 0.01) { 
        moveDown(moveDistance * m_accelerationSpeeds[1]); 
        moved = true; 
        moveDirections << QString("下(%1)").arg(m_accelerationSpeeds[1], 0, 'f', 2);
    }
    if (m_accelerationSpeeds[2] > 0.01) { 
        moveLeft(moveDistance * m_accelerationSpeeds[2]); 
        moved = true; 
        moveDirections << QString("左(%1)").arg(m_accelerationSpeeds[2], 0, 'f', 2);
    }
    if (m_accelerationSpeeds[3] > 0.01) { 
        moveRight(moveDistance * m_accelerationSpeeds[3]); 
        moved = true; 
        moveDirections << QString("右(%1)").arg(m_accelerationSpeeds[3], 0, 'f', 2);
    }
    if (m_accelerationSpeeds[4] > 0.01) { 
        moveForward(moveDistance * m_accelerationSpeeds[4]); 
        moved = true; 
        moveDirections << QString("前(%1)").arg(m_accelerationSpeeds[4], 0, 'f', 2);
    }
    if (m_accelerationSpeeds[5] > 0.01) { 
        moveBackward(moveDistance * m_accelerationSpeeds[5]); 
        moved = true; 
        moveDirections << QString("后(%1)").arg(m_accelerationSpeeds[5], 0, 'f', 2);
    }
    
    // 相机移动（移除频繁的调试日志）
    if (moved) {
        // 移动详情（移除调试日志）
    }
    
    // 更新上次移动时间
    m_lastMoveTime = currentTime;
}

void CameraController::handleWheelZoom(int delta)
{
    if (!m_currentManipulator) {
        LOG_WARNING("滚轮缩放失败：当前操控器为空", "相机");
        return;
    }
    
    // 滚轮缩放事件（移除调试日志）
    
    double zoomFactor = delta > 0 ? 0.9 : 1.1; // 滚轮向上缩小，向下放大
    zoomFactor *= m_wheelMoveSensitivity;
    
    // 根据当前操控器类型执行不同的缩放操作
    switch (m_currentManipulatorType) {
        case ManipulatorType::Trackball:
            // 轨道球操控器：调整距离
            if (m_trackballManipulator) {
                double distance = m_trackballManipulator->getDistance();
                m_trackballManipulator->setDistance(distance * zoomFactor);
                // 轨道球缩放（移除调试日志）
            } else {
                LOG_WARNING("轨道球操控器为空，无法执行缩放", "相机");
            }
            break;
        case ManipulatorType::FirstPerson:
        case ManipulatorType::Flight:
        case ManipulatorType::Drive:
            // 其他操控器：向前或向后移动
            double moveDistance = delta > 0 ? -m_cameraMoveSpeed : m_cameraMoveSpeed;
            moveDistance *= m_wheelMoveSensitivity;
            moveForward(moveDistance);
            break;
    }
}

void CameraController::onUpdateTimer()
{
    // 定时器更新相机位置（移除调试日志）
    updateCameraPosition();
} 

QString CameraController::getManipulatorTypeString(ManipulatorType type) const
{
    QString result;
    switch (type) {
    case ManipulatorType::Trackball: 
        result = "轨道球";
        break;
    case ManipulatorType::FirstPerson: 
        result = "第一人称";
        break;
    case ManipulatorType::Flight: 
        result = "飞行";
        break;
    case ManipulatorType::Drive: 
        result = "驾驶";
        break;
    default: 
        result = "未知";
        LOG_WARNING(QString("未知的操控器类型: %1").arg(static_cast<int>(type)), "相机");
        break;
    }
    
    // 获取操控器类型字符串（移除调试日志）
    return result;
}

bool CameraController::getViewMatrixAsLookAt(osg::Vec3& eye, osg::Vec3& center, osg::Vec3& up) const
{
    if (!m_currentManipulator) {
        return false;
    }
    
    // 从当前操控器获取视图参数
    osg::Vec3d eyePosition = getEyePosition();
    osg::Vec3d centerPosition = getCenterPosition();
    osg::Vec3d upVector = getCameraUpVector();
    
    // 转换为Vec3类型
    eye.set(eyePosition.x(), eyePosition.y(), eyePosition.z());
    center.set(centerPosition.x(), centerPosition.y(), centerPosition.z());
    up.set(upVector.x(), upVector.y(), upVector.z());
    
    return true;
}


