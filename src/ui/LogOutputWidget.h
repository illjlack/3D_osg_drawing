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
#include <QSpinBox>
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
#include <QMutex>
#include <QSet>
#include <QListWidget>
#include <QThread>
#include <QQueue>
#include <QTimer>
#include <QWaitCondition>
#include <QAtomicInt>
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
    
    // 批量更新UI
    void processUIUpdate();

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
    
    // 调度UI更新
    void scheduleUIUpdate();
    
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
    QQueue<LogEntry> m_pendingLogs;
    
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
    
    // UI更新控制 - 简化版本
    QTimer* m_uiUpdateTimer;
    QList<LogEntry> m_pendingUILogs;
    bool m_needsFullRefresh;
    
    // 线程安全
    mutable QMutex m_logsMutex;
    
    // 更新频率控制 - 简化参数
    static const int UI_UPDATE_INTERVAL = 100; // 100ms UI更新间隔
    static const int MAX_UI_BATCH_SIZE = 50;   // UI线程最多处理50条日志
}; 