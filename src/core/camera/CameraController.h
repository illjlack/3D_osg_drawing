#pragma once

#include <osg/Matrix>
#include <osg/Vec3d>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <QObject>

// 投影模式枚举
enum class ProjectionMode
{
    Perspective,  // 透视投影
    Orthographic  // 正交投影
};

// 摄像机控制器类
class CameraController : public QObject
{
    Q_OBJECT

public:
    CameraController();
    virtual ~CameraController();

    // 设置和获取OSG查看器
    void setViewer(osgViewer::Viewer* viewer);
    osgViewer::Viewer* getViewer() const { return m_viewer; }

    // 投影模式管理
    void setProjectionMode(ProjectionMode mode);
    ProjectionMode getProjectionMode() const { return m_projectionMode; }
    
    // 透视投影参数
    void setPerspectiveFOV(double fov);
    void setPerspectiveNearFar(double near, double far);
    double getPerspectiveFOV() const { return m_perspectiveFOV; }
    double getPerspectiveNear() const { return m_perspectiveNear; }
    double getPerspectiveFar() const { return m_perspectiveFar; }
    
    // 正交投影参数
    void setOrthographicSize(double left, double right, double bottom, double top);
    void setOrthographicNearFar(double near, double far);
    double getOrthographicLeft() const { return m_orthographicLeft; }
    double getOrthographicRight() const { return m_orthographicRight; }
    double getOrthographicBottom() const { return m_orthographicBottom; }
    double getOrthographicTop() const { return m_orthographicTop; }
    double getOrthographicNear() const { return m_orthographicNear; }
    double getOrthographicFar() const { return m_orthographicFar; }
    
    // 更新投影矩阵
    void updateProjectionMatrix(int viewportWidth, int viewportHeight);

    // 摄像机状态获取
    osg::Vec3d getEyePosition() const;
    osg::Vec3d getCenterPosition() const;
    osg::Vec3d getCameraUpVector() const;
    
    // 摄像机方向向量
    osg::Vec3d getForwardVector() const;  // 前方向
    osg::Vec3d getRightVector() const;    // 右方向
    osg::Vec3d getUpVector() const;       // 上方向（缓存的）

    // 摄像机移动控制
    void moveForward(double distance);
    void moveBackward(double distance);
    void moveLeft(double distance);
    void moveRight(double distance);
    void moveUp(double distance);
    void moveDown(double distance);

    // 摄像机旋转控制
    void rotateHorizontal(double angle);
    void rotateVertical(double angle);

    // 设置摄像机位置和朝向
    void setPosition(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up);
    void setHomePosition(const osg::Vec3d& eye, const osg::Vec3d& center, const osg::Vec3d& up);
    void home();

    // 获取当前视图矩阵
    osg::Matrix getViewMatrix() const;
    osg::Matrix getInverseViewMatrix() const;

    // 计算屏幕到世界的转换
    osg::Vec3d screenToWorld(int screenX, int screenY, double depth, int viewportWidth, int viewportHeight) const;
    osg::Vec2d worldToScreen(const osg::Vec3d& worldPos, int viewportWidth, int viewportHeight) const;

    // 计算自适应移动速度
    double calculateAdaptiveMoveSpeed(double baseSpeed) const;

private:
    osgViewer::Viewer* m_viewer;
    osg::ref_ptr<osgGA::TrackballManipulator> m_manipulator;

    // 投影模式相关
    ProjectionMode m_projectionMode;
    double m_perspectiveFOV;
    double m_perspectiveNear;
    double m_perspectiveFar;
    double m_orthographicLeft;
    double m_orthographicRight;
    double m_orthographicBottom;
    double m_orthographicTop;
    double m_orthographicNear;
    double m_orthographicFar;

    // 缓存的方向向量
    mutable osg::Vec3d m_cachedForward;
    mutable osg::Vec3d m_cachedRight;
    mutable osg::Vec3d m_cachedUp;
    mutable bool m_directionCacheValid;

    // 更新方向向量缓存
    void updateDirectionCache() const;
}; 