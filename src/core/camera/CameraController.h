#pragma once
#pragma execution_character_set("utf-8")

#include <osg/Matrix>
#include <osg/Vec3d>
#include <osgGA/CameraManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/FirstPersonManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgViewer/Viewer>
#include <QObject>
#include <QTimer>

// 投影模式枚举
enum class ProjectionMode
{
    Perspective,  // 透视投影
    Orthographic  // 正交投影
};

// 相机操控器类型枚举
enum class ManipulatorType
{
    Trackball,    // 轨道球操控器
    FirstPerson,  // 第一人称操控器
    Flight,       // 飞行操控器
    Drive         // 驾驶操控器
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
    inline osgViewer::Viewer* getViewer() const { return m_viewer; }

    // 相机操控器管理
    void setManipulatorType(ManipulatorType type);
    inline ManipulatorType getManipulatorType() const { return m_currentManipulatorType; }
    void switchToNextManipulator();
    void switchToPreviousManipulator();
    
    // 获取当前操控器
    osgGA::CameraManipulator* getCurrentManipulator() const;
    inline osgGA::TrackballManipulator* getTrackballManipulator() const { return m_trackballManipulator.get(); }
    inline osgGA::FirstPersonManipulator* getFirstPersonManipulator() const { return m_firstPersonManipulator.get(); }
    inline osgGA::FlightManipulator* getFlightManipulator() const { return m_flightManipulator.get(); }
    inline osgGA::DriveManipulator* getDriveManipulator() const { return m_driveManipulator.get(); }

    // 投影模式管理
    void setProjectionMode(ProjectionMode mode);
    void setProjectionModeSmooth(ProjectionMode mode); // 平滑切换投影模式
    inline ProjectionMode getProjectionMode() const { return m_projectionMode; }
    
    // 统一的投影参数
    void setFOV(double fov);
    void setNearFar(double near, double far);
    void setViewSize(double left, double right, double bottom, double top);
    
    // 获取投影参数
    inline double getFOV() const { return m_fov; }
    inline double getNear() const { return m_near; }
    inline double getFar() const { return m_far; }
    inline double getLeft() const { return m_left; }
    inline double getRight() const { return m_right; }
    inline double getBottom() const { return m_bottom; }
    inline double getTop() const { return m_top; }
    
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
    
    // 设置相机旋转中心（仅对轨道球操控器有效）
    void setRotationCenter(const osg::Vec3d& center);
    osg::Vec3d getRotationCenter() const;

    // 获取当前视图矩阵
    inline osg::Matrix getViewMatrix() const 
    { 
        if (!m_currentManipulator) return osg::Matrix::identity();
        return m_currentManipulator->getInverseMatrix(); 
    }
    
    // 获取LookAt形式的视图矩阵参数
    bool getViewMatrixAsLookAt(osg::Vec3& eye, osg::Vec3& center, osg::Vec3& up) const;

    // 计算屏幕到世界的转换
    osg::Vec3d screenToWorld(int screenX, int screenY, double depth, int viewportWidth, int viewportHeight) const;
    osg::Vec2d worldToScreen(const osg::Vec3d& worldPos, int viewportWidth, int viewportHeight) const;

    // 计算自适应移动速度
    double calculateAdaptiveMoveSpeed(double baseSpeed) const;

    // 摄像机移动控制（从OSGWidget转移）
    void setCameraMoveSpeed(double speed);
    inline double getCameraMoveSpeed() const { return m_cameraMoveSpeed; }
    void setWheelMoveSensitivity(double sensitivity);
    inline double getWheelMoveSensitivity() const { return m_wheelMoveSensitivity; }
    
    // 加速度移动控制
    void setAccelerationRate(double rate);
    inline double getAccelerationRate() const { return m_accelerationRate; }
    void setMaxAccelerationSpeed(double speed);
    inline double getMaxAccelerationSpeed() const { return m_maxAccelerationSpeed; }
    void resetAllAcceleration();
    
    // 键盘移动控制
    void setKeyPressed(int key, bool pressed);
    void updateCameraPosition();
    inline bool isMoving() const { return m_isMoving; }
    
    // 滚轮缩放控制
    void handleWheelZoom(int delta);

signals:
    void manipulatorTypeChanged(ManipulatorType type);
    void cameraMoveSpeedChanged(double speed);
    void wheelMoveSensitivityChanged(double sensitivity);
    void accelerationRateChanged(double rate);
    void maxAccelerationSpeedChanged(double speed);

private slots:
    void onUpdateTimer();

private:
    void initializeManipulators();
    void switchManipulator(ManipulatorType type);
    void updateDirectionCache() const;
    void invalidateDirectionCache();
    QString getManipulatorTypeString(ManipulatorType type) const;

private:
    osgViewer::Viewer* m_viewer;
    
    // 多种相机操控器
    osg::ref_ptr<osgGA::TrackballManipulator> m_trackballManipulator;
    osg::ref_ptr<osgGA::FirstPersonManipulator> m_firstPersonManipulator;
    osg::ref_ptr<osgGA::FlightManipulator> m_flightManipulator;
    osg::ref_ptr<osgGA::DriveManipulator> m_driveManipulator;
    osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> m_keySwitchManipulator;
    
    ManipulatorType m_currentManipulatorType;
    osgGA::CameraManipulator* m_currentManipulator;

    // 投影模式相关
    ProjectionMode m_projectionMode;
    double m_fov;
    double m_near;
    double m_far;
    double m_left;
    double m_right;
    double m_bottom;
    double m_top;

    // 缓存的方向向量
    mutable osg::Vec3d m_cachedForward;
    mutable osg::Vec3d m_cachedRight;
    mutable osg::Vec3d m_cachedUp;
    mutable bool m_directionCacheValid;

    // 摄像机移动控制
    double m_cameraMoveSpeed;
    double m_wheelMoveSensitivity;
    bool m_cameraMoveKeys[6]; // up, down, left, right, forward, backward
    
    // 加速度移动相关
    double m_accelerationRate;
    double m_maxAccelerationSpeed;
    double m_accelerationSpeeds[6]; // 各方向的当前加速度
    
    // 移动控制相关
    QTimer* m_updateTimer;
    qint64 m_lastMoveTime;  // 上次移动时间
    bool m_isMoving;        // 是否正在移动
}; 

