#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QAction>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QMenu>
#include <QTimer>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QColor>
#include <QFont>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMutex>
#include <QQueue>
#include <QSet>
#include "../util/LogManager.h"

// 日志输出栏组件
class LogOutputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogOutputWidget(QWidget* parent = nullptr);
    ~LogOutputWidget();

    // 设置最大显示行数
    void setMaxDisplayLines(int lines);
    int getMaxDisplayLines() const;

    // 设置自动滚动
    void setAutoScroll(bool enabled);
    bool isAutoScrollEnabled() const;

    // 设置时间戳显示
    void setShowTimestamp(bool enabled);
    bool isShowTimestampEnabled() const;

    // 设置分类显示
    void setShowCategory(bool enabled);
    bool isShowCategoryEnabled() const;

    // 清空日志
    void clearLogs();

    // 导出日志
    void exportLogs(const QString& filename);

    // 复制选中内容
    void copySelectedText();

public slots:
    // 添加日志条目
    void addLogEntry(const LogEntry& entry);
    
    // 刷新显示
    void refreshDisplay();
    
    // 清空当前标签页
    void clearCurrentTab();
    
    // 导出当前标签页
    void exportCurrentTab();

private slots:
    // 标签页切换
    void onTabChanged(int index);
    
    // 过滤级别改变
    void onFilterLevelChanged(int index);
    
    // 过滤分类改变
    void onFilterCategoryChanged(const QString& category);
    
    // 自动滚动切换
    void onAutoScrollToggled(bool checked);
    
    // 时间戳显示切换
    void onShowTimestampToggled(bool checked);
    
    // 分类显示切换
    void onShowCategoryToggled(bool checked);
    
    // 右键菜单
    void showContextMenu(const QPoint& pos);
    
    // 定时刷新
    void onRefreshTimer();

private:
    // 初始化UI
    void setupUI();
    
    // 初始化工具栏
    void setupToolbar();
    
    // 初始化标签页
    void setupTabs();
    
    // 设置文本格式
    void setupTextFormats();
    
    // 添加日志到指定文本编辑器
    void addLogToTextEdit(QTextEdit* textEdit, const LogEntry& entry);
    
    // 格式化日志文本
    QString formatLogText(const LogEntry& entry);
    
    // 获取日志级别颜色
    QColor getLogLevelColor(LogLevel level);
    
    // 获取日志级别文本
    QString getLogLevelText(LogLevel level);
    
    // 应用过滤
    void applyFilters();
    
    // 更新过滤选项
    void updateFilterOptions();
    
    // 限制显示行数
    void limitDisplayLines();
    
    // 检查是否应该显示日志
    bool shouldDisplayLog(const LogEntry& entry);

private:
    // UI组件
    QTabWidget* m_tabWidget;
    QTextEdit* m_normalTextEdit;    // 普通输出
    QTextEdit* m_debugTextEdit;     // 调试输出
    
    QToolBar* m_toolBar;
    QComboBox* m_filterLevelCombo;
    QComboBox* m_filterCategoryCombo;
    QCheckBox* m_autoScrollCheck;
    QCheckBox* m_showTimestampCheck;
    QCheckBox* m_showCategoryCheck;
    QPushButton* m_clearButton;
    QPushButton* m_exportButton;
    QPushButton* m_copyButton;
    
    // 设置
    int m_maxDisplayLines;
    bool m_autoScroll;
    bool m_showTimestamp;
    bool m_showCategory;
    LogLevel m_currentFilterLevel;
    QString m_currentFilterCategory;
    
    // 文本格式
    QTextCharFormat m_debugFormat;
    QTextCharFormat m_infoFormat;
    QTextCharFormat m_warningFormat;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_successFormat;
    QTextCharFormat m_timestampFormat;
    QTextCharFormat m_categoryFormat;
    
    // 数据
    QList<LogEntry> m_allLogs;
    QList<LogEntry> m_normalLogs;
    QList<LogEntry> m_debugLogs;
    QSet<QString> m_categories;
    
    // 定时器
    QTimer* m_refreshTimer;
    
    // 互斥锁
    mutable QMutex m_mutex;
    
    // 待处理日志队列
    QQueue<LogEntry> m_pendingLogs;
}; 