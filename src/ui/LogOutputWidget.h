#pragma once
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QPlainTextEdit>
#include <QToolBar>
#include <QAction>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
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
#include <QSet>
#include <QThread>
#include <QQueue>
#include <QTimer>
#include <list>
#include "../util/LogManager.h"

// 为LogLevel提供qHash函数
inline uint qHash(LogLevel level, uint seed = 0)
{
    return qHash(static_cast<int>(level), seed);
}

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
    void exportLogsWithOptions(const QString& filename, bool exportFiltered);

    // 复制选中内容
    void copySelectedText();

    // 全选文本
    void selectAllText();

public slots:
    // 添加日志条目
    void addLogEntry(const LogEntry& entry);
    
    // 刷新显示
    void refreshDisplay();

private slots:
    // 过滤级别改变
    void onFilterLevelChanged();
    
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
    
    // 导出日志（无参数版本）
    void exportLogs();
    
    // 已移除定时刷新相关声明

private:
    // 初始化UI
    void setupUI();
    
    // 初始化工具栏
    void setupToolbar();
    
    // 设置文本格式
    void setupTextFormats();
    
    // 添加日志到文本编辑器
    void addLogToTextEdit(const LogEntry& entry);
    
    // 批量添加日志到文本编辑器
    void addLogsToTextEdit(const QList<LogEntry>& logs);
    
    // 格式化日志文本
    QString formatLogText(const LogEntry& entry);
    
    // 获取日志级别颜色
    QColor getLogLevelColor(LogLevel level);
    
    // 获取日志级别文本
    QString getLogLevelText(LogLevel level);
    
    // 更新过滤选项
    void updateFilterOptions();
    
    // 限制显示行数
    void limitDisplayLines();
    
    // 应用过滤条件
    void applyFilters();
    
    // 检查是否应该显示日志
    bool shouldDisplayLog(const LogEntry& entry) const;

private:
    // UI组件
    QPlainTextEdit* m_textEdit;      // 使用QPlainTextEdit提升性能
    
    QToolBar* m_toolBar;
    QComboBox* m_filterLevelCombo;   // 级别过滤下拉框
    QComboBox* m_filterCategoryCombo;
    QCheckBox* m_autoScrollCheck;
    QCheckBox* m_showTimestampCheck;
    QCheckBox* m_showCategoryCheck;
    QPushButton* m_clearButton;
    QPushButton* m_exportButton;
    QPushButton* m_copyButton;
    
    // 日志数据
    QList<LogEntry> m_allLogs;
    QList<LogEntry> m_filteredLogs;
    QSet<QString> m_categories;
    
    // 过滤条件
    QSet<LogLevel> m_selectedFilterLevels;
    QString m_currentFilterCategory;
    
    // 显示设置
    int m_maxDisplayLines;
    bool m_autoScroll;
    bool m_showTimestamp;
    bool m_showCategory;
    
    // 文本格式
    QTextCharFormat m_debugFormat;
    QTextCharFormat m_infoFormat;
    QTextCharFormat m_warningFormat;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_successFormat;
    QTextCharFormat m_timestampFormat;
    QTextCharFormat m_categoryFormat;
    
    // UI更新控制已移除定时刷新相关成员
    bool m_needsFullRefresh;
    
    // 线程安全 - 使用Qt信号槽机制，无需显式锁
    
    // 筛选状态控制
    bool m_isFiltering;
    
    // 更新频率控制 - 简化参数
    // static const int UI_UPDATE_INTERVAL = 100; // 已移除定时刷新
    // static const int MAX_UI_BATCH_SIZE = 50;   // 已移除定时刷新
}; 

