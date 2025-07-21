#pragma once
#pragma execution_character_set("utf-8")

#include "../Common3D.h"
#include <QObject>
#include <vector>
#include <string>

// 前向声明
class Geo3D;

/**
 * @brief 阶段描述符结构体
 * 描述每个绘制阶段的规则
 */
struct StageDescriptor 
{
    std::string stageName;      // 阶段名称
    int minControlPoints;       // 该阶段最少控制点数量
    int maxControlPoints;       // 该阶段最多控制点数量 (-1表示无限制)
    
    StageDescriptor() : minControlPoints(1), maxControlPoints(1) {}
    StageDescriptor(const std::string& name, int minPoints, int maxPoints)
        : stageName(name), minControlPoints(minPoints), maxControlPoints(maxPoints) {}
};

/**
 * @brief 控制点管理器
 * 负责管理几何对象的多阶段控制点，支持每个阶段的临时点跟踪
 */
class GeoControlPointManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoControlPointManager(Geo3D* parent);
    ~GeoControlPointManager() = default;

    // ==================== 多阶段管理接口 ====================
    
    // 阶段管理
    void setStageDescriptors(const std::vector<StageDescriptor>& descriptors);
    const std::vector<StageDescriptor>& getStageDescriptors() const;
    
    // 当前阶段控制
    int getCurrentStage() const;
    bool nextStage(); // 进入下一阶段，返回是否成功
    bool canAdvanceToNextStage() const; // 检查是否可以进入下一阶段
    bool isCurrentStageComplete() const; // 检查当前阶段是否完成
    bool isAllStagesComplete() const; // 检查是否所有阶段都完成
    const StageDescriptor* getCurrentStageDescriptor() const;
    
    // ==================== 控制点访问接口 ====================
    
    // 获取指定阶段的控制点（包含临时点用于预览）
    const std::vector<Point3D>& getStageControlPoints(int stage) const;
    
    // 获取当前阶段的控制点（包含临时点用于预览）
    const std::vector<Point3D>& getCurrentStageControlPoints() const;
    
    // 获取所有阶段的控制点（二维数组）
    const std::vector<std::vector<Point3D>>& getAllStageControlPoints() const;
    
    // 获取指定阶段、指定索引的控制点
    Point3D getStageControlPoint(int stage, int index) const;
    
    // 获取当前阶段的控制点数量（不包含临时点）
    int getCurrentStageControlPointCount() const;
    
    // 获取指定阶段的控制点数量（不包含临时点）
    int getStageControlPointCount(int stage) const;
    
    // ==================== 控制点操作接口 ====================
    
    // 向当前阶段添加控制点
    bool addControlPointToCurrentStage(const Point3D& point);
    
    // 设置指定阶段、指定索引的控制点
    bool setStageControlPoint(int stage, int index, const Point3D& point);
    
    // 移除当前阶段的最后一个控制点
    bool removeLastControlPointFromCurrentStage();
    
    // 清除所有控制点
    void clearAllControlPoints();
    
    // 清除当前阶段的控制点
    void clearCurrentStageControlPoints();
    
    // ==================== 临时点管理接口 ====================
    
    // 设置当前阶段的临时点
    void setCurrentStageTempPoint(const Point3D& point);
    
    // 清除当前阶段的临时点
    void clearCurrentStageTempPoint();
    
    // 检查当前阶段是否有临时点
    bool hasCurrentStageTempPoint() const;
    
    // ==================== 兼容性接口（保持原有接口） ====================
    
    // 兼容原有的单一控制点数组接口，返回所有阶段的所有控制点
    const std::vector<Point3D>& getControlPoints() const;
    Point3D getControlPoint(int index) const;
    int getControlPointCountWithoutTempPoint() const;
    bool hasControlPoints() const;
    
    // 兼容原有的操作接口，操作当前阶段
    void addControlPoint(const Point3D& point);
    void setControlPoint(int index, const Point3D& point);
    void removeControlPoint(int index);
    void clearControlPoints();
    
    // 兼容原有的临时点接口，操作当前阶段的临时点
    void setTempPoint(const Point3D& point);
    void clearTempPoint();
    
    // ==================== 查询和验证接口 ====================
    
    // 控制点查询
    int findNearestControlPoint(const Point3D& point, float threshold = 0.1f) const;
    bool isValidStageIndex(int stage) const;
    bool isValidControlPointIndex(int stage, int index) const;
    
    // 绘制相关通知方法
    void notifyGeometryChanged();

signals:
    void stageChanged(int newStage); // 阶段切换信号
    void stageCompleted(int stage);  // 阶段完成信号
    void allStagesCompleted();       // 所有阶段完成信号

private:
    // 验证方法
    void validateStageIndex(int stage) const;
    void validateControlPointIndex(int stage, int index) const;
    
    // 内部状态检查
    bool isDrawingComplete() const; // 检查绘制是否完成状态

private:
    Geo3D* m_parent;
    
    // 多阶段控制点存储（二维数组）
    std::vector<std::vector<Point3D>> m_stageControlPoints;
    
    // 每个阶段的临时点
    std::vector<Point3D> m_stageTempPoints;
    
    // 阶段描述符
    std::vector<StageDescriptor> m_stageDescriptors;
    
    // 当前阶段索引
    int m_currentStage;
    
    // 兼容性支持 - 用于返回扁平化的控制点列表
    mutable std::vector<Point3D> m_flattenedControlPoints;
    
    // 用于返回包含临时点的控制点列表（每个阶段）
    mutable std::vector<std::vector<Point3D>> m_tempStageControlPointsList;
};
