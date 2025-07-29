// #pragma once

// #include <QMouseEvent>
// #include <QKeyEvent>
// #include <QWheelEvent>
// #include <osg/ref_ptr>
// #include "../../core/Common3D.h"

// class OSGWidget;
// class Geo3D;

// /**
//  * 输入交互模式基类
//  * 定义了所有交互模式需要实现的接口
//  */
// class InputInteractionMode {
// public:
//     explicit InputInteractionMode(OSGWidget* widget) : m_widget(widget) {}
//     virtual ~InputInteractionMode() = default;

//     // 鼠标事件
//     virtual void onMousePress(QMouseEvent* event){}
//     virtual void onMouseMove(QMouseEvent* event){}
//     virtual void onMouseRelease(QMouseEvent* event){}
//     virtual void onMouseDoubleClick(QMouseEvent* event){}
    
//     // 滚轮事件
//     virtual void onWheel(QWheelEvent* event){}
    
//     // 键盘事件
//     virtual void onKeyPress(QKeyEvent* event){}
//     virtual void onKeyRelease(QKeyEvent* event){}
    
//     // 模式激活/停用
//     virtual void activate() {}
//     virtual void deactivate() {}
    
//     // 获取模式名称
//     virtual QString getModeName() const = 0;

// protected:
//     OSGWidget* m_widget;
// };

// /**
//  * 相机控制交互模式
//  * 处理相机的旋转、平移、缩放
//  */
// class CameraInteractionMode : public InputInteractionMode {
// public:
//     explicit CameraInteractionMode(OSGWidget* widget);
    
//     void onMousePress(QMouseEvent* event) override;
//     void onMouseMove(QMouseEvent* event) override;
//     void onMouseRelease(QMouseEvent* event) override;
//     void onMouseDoubleClick(QMouseEvent* event) override;
//     void onWheel(QWheelEvent* event) override;
//     void onKeyPress(QKeyEvent* event) override;
//     void onKeyRelease(QKeyEvent* event) override;
    
//     QString getModeName() const override { return "相机控制模式"; }

// private:
//     bool m_isDragging = false;
//     Qt::MouseButton m_dragButton = Qt::NoButton;
// };

// /**
//  * 选择交互模式
//  * 处理对象选择、多选、控制点操作
//  */
// class SelectionInteractionMode : public InputInteractionMode {
// public:
//     explicit SelectionInteractionMode(OSGWidget* widget);
    
//     void onMousePress(QMouseEvent* event) override;
//     void onMouseMove(QMouseEvent* event) override;
//     void onMouseRelease(QMouseEvent* event) override;
//     void onMouseDoubleClick(QMouseEvent* event) override;
//     void onWheel(QWheelEvent* event) override;
//     void onKeyPress(QKeyEvent* event) override;
//     void onKeyRelease(QKeyEvent* event) override;
    
//     void activate() override;
//     void deactivate() override;
    
//     QString getModeName() const override { return "选择模式"; }

// private:
//     bool m_isDraggingControlPoint = false;
//     osg::ref_ptr<Geo3D> m_draggingGeo;
//     int m_draggingControlPointIndex = -1;
//     glm::dvec3 m_dragStartPosition;
    
//     void handleSingleSelection(QMouseEvent* event);
//     void handleMultiSelection(QMouseEvent* event);
//     void startControlPointDrag(osg::ref_ptr<Geo3D> geo, int pointIndex);
//     void stopControlPointDrag();
// };

// /**
//  * 绘制交互模式
//  * 处理几何体的绘制操作
//  */
// class DrawingInteractionMode : public InputInteractionMode {
// public:
//     explicit DrawingInteractionMode(OSGWidget* widget, DrawMode3D drawMode);
    
//     void onMousePress(QMouseEvent* event) override;
//     void onMouseMove(QMouseEvent* event) override;
//     void onMouseRelease(QMouseEvent* event) override;
//     void onMouseDoubleClick(QMouseEvent* event) override;
//     void onWheel(QWheelEvent* event) override;
//     void onKeyPress(QKeyEvent* event) override;
//     void onKeyRelease(QKeyEvent* event) override;
    
//     void activate() override;
//     void deactivate() override;
    
//     QString getModeName() const override;

// private:
//     DrawMode3D m_drawMode;
//     osg::ref_ptr<Geo3D> m_currentDrawingGeo;
//     bool m_isDrawing = false;
    
//     void startDrawing();
//     void continueDrawing(const glm::dvec3& worldPos);
//     void completeDrawing();
//     void cancelDrawing();
//     void updateTemporaryPoint(const glm::dvec3& worldPos);
// };

// /**
//  * 查看交互模式
//  * 只允许相机操作，禁用选择和绘制
//  */
// class ViewOnlyInteractionMode : public InputInteractionMode {
// public:
//     explicit ViewOnlyInteractionMode(OSGWidget* widget);
    
//     void onMousePress(QMouseEvent* event) override;
//     void onMouseMove(QMouseEvent* event) override;
//     void onMouseRelease(QMouseEvent* event) override;
//     void onMouseDoubleClick(QMouseEvent* event) override;
//     void onWheel(QWheelEvent* event) override;
//     void onKeyPress(QKeyEvent* event) override;
//     void onKeyRelease(QKeyEvent* event) override;
    
//     QString getModeName() const override { return "查看模式"; }
// };

// /**
//  * 测量交互模式
//  * 用于距离、面积等测量操作
//  */
// class MeasurementInteractionMode : public InputInteractionMode {
// public:
//     explicit MeasurementInteractionMode(OSGWidget* widget);
    
//     void onMousePress(QMouseEvent* event) override;
//     void onMouseMove(QMouseEvent* event) override;
//     void onMouseRelease(QMouseEvent* event) override;
//     void onMouseDoubleClick(QMouseEvent* event) override;
//     void onWheel(QWheelEvent* event) override;
//     void onKeyPress(QKeyEvent* event) override;
//     void onKeyRelease(QKeyEvent* event) override;
    
//     void activate() override;
//     void deactivate() override;
    
//     QString getModeName() const override { return "测量模式"; }

// private:
//     std::vector<glm::dvec3> m_measurementPoints;
//     bool m_isMeasuring = false;
    
//     void addMeasurementPoint(const glm::dvec3& point);
//     void completeMeasurement();
//     void cancelMeasurement();
//     void updateTemporaryLine(const glm::dvec3& currentPos);
// }; 