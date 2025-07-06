#include "LogOutputWidget.h"
#include <QTextStream>
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

LogOutputWidget::LogOutputWidget(QWidget* parent)
    : QWidget(parent)
    , m_maxDisplayLines(1000)
    , m_autoScroll(true)
    , m_showTimestamp(true)
    , m_showCategory(true)
    , m_currentFilterLevel(LogLevel::Debug)
    , m_refreshTimer(new QTimer(this))
{
    setupUI();
    setupTextFormats();
    
    // 连接日志管理器信号
    LogManager* logManager = LogManager::getInstance();
    connect(logManager, &LogManager::logAdded, this, &LogOutputWidget::addLogEntry);
    connect(logManager, &LogManager::logsCleared, this, &LogOutputWidget::clearLogs);
    
    // 设置定时刷新
    connect(m_refreshTimer, &QTimer::timeout, this, &LogOutputWidget::onRefreshTimer);
    m_refreshTimer->start(100); // 100ms刷新一次
    
    // 加载现有日志
    refreshDisplay();
}

LogOutputWidget::~LogOutputWidget()
{
    if (m_refreshTimer)
    {
        m_refreshTimer->stop();
    }
}

void LogOutputWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 创建工具栏
    setupToolbar();
    mainLayout->addWidget(m_toolBar);
    
    // 创建标签页
    setupTabs();
    mainLayout->addWidget(m_tabWidget);
    
    setLayout(mainLayout);
}

void LogOutputWidget::setupToolbar()
{
    m_toolBar = new QToolBar(this);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolBar->setIconSize(QSize(16, 16));
    
    // 过滤级别
    m_toolBar->addWidget(new QLabel("级别:"));
    m_filterLevelCombo = new QComboBox(this);
    m_filterLevelCombo->addItem("全部", static_cast<int>(LogLevel::Debug));
    m_filterLevelCombo->addItem("调试", static_cast<int>(LogLevel::Debug));
    m_filterLevelCombo->addItem("信息", static_cast<int>(LogLevel::Info));
    m_filterLevelCombo->addItem("警告", static_cast<int>(LogLevel::Warning));
    m_filterLevelCombo->addItem("错误", static_cast<int>(LogLevel::Error));
    m_filterLevelCombo->addItem("成功", static_cast<int>(LogLevel::Success));
    m_toolBar->addWidget(m_filterLevelCombo);
    
    // 过滤分类
    m_toolBar->addWidget(new QLabel("分类:"));
    m_filterCategoryCombo = new QComboBox(this);
    m_filterCategoryCombo->addItem("全部", "");
    m_filterCategoryCombo->setEditable(true);
    m_toolBar->addWidget(m_filterCategoryCombo);
    
    m_toolBar->addSeparator();
    
    // 自动滚动
    m_autoScrollCheck = new QCheckBox("自动滚动", this);
    m_autoScrollCheck->setChecked(m_autoScroll);
    m_toolBar->addWidget(m_autoScrollCheck);
    
    // 显示时间戳
    m_showTimestampCheck = new QCheckBox("时间戳", this);
    m_showTimestampCheck->setChecked(m_showTimestamp);
    m_toolBar->addWidget(m_showTimestampCheck);
    
    // 显示分类
    m_showCategoryCheck = new QCheckBox("分类", this);
    m_showCategoryCheck->setChecked(m_showCategory);
    m_toolBar->addWidget(m_showCategoryCheck);
    
    m_toolBar->addSeparator();
    
    // 清空按钮
    m_clearButton = new QPushButton("清空", this);
    m_toolBar->addWidget(m_clearButton);
    
    // 导出按钮
    m_exportButton = new QPushButton("导出", this);
    m_toolBar->addWidget(m_exportButton);
    
    // 复制按钮
    m_copyButton = new QPushButton("复制", this);
    m_toolBar->addWidget(m_copyButton);
    
    // 连接信号
    connect(m_filterLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogOutputWidget::onFilterLevelChanged);
    connect(m_filterCategoryCombo, &QComboBox::currentTextChanged,
            this, &LogOutputWidget::onFilterCategoryChanged);
    connect(m_autoScrollCheck, &QCheckBox::toggled,
            this, &LogOutputWidget::onAutoScrollToggled);
    connect(m_showTimestampCheck, &QCheckBox::toggled,
            this, &LogOutputWidget::onShowTimestampToggled);
    connect(m_showCategoryCheck, &QCheckBox::toggled,
            this, &LogOutputWidget::onShowCategoryToggled);
    connect(m_clearButton, &QPushButton::clicked,
            this, &LogOutputWidget::clearCurrentTab);
    connect(m_exportButton, &QPushButton::clicked,
            this, &LogOutputWidget::exportCurrentTab);
    connect(m_copyButton, &QPushButton::clicked,
            this, &LogOutputWidget::copySelectedText);
}

void LogOutputWidget::setupTabs()
{
    m_tabWidget = new QTabWidget(this);
    
    // 普通输出标签页
    m_normalTextEdit = new QTextEdit(this);
    m_normalTextEdit->setReadOnly(true);
    m_normalTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_normalTextEdit->setFont(QFont("Consolas", 9));
    m_normalTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_normalTextEdit, &QTextEdit::customContextMenuRequested,
            this, &LogOutputWidget::showContextMenu);
    
    // 调试输出标签页
    m_debugTextEdit = new QTextEdit(this);
    m_debugTextEdit->setReadOnly(true);
    m_debugTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_debugTextEdit->setFont(QFont("Consolas", 9));
    m_debugTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_debugTextEdit, &QTextEdit::customContextMenuRequested,
            this, &LogOutputWidget::showContextMenu);
    
    m_tabWidget->addTab(m_normalTextEdit, "普通输出");
    m_tabWidget->addTab(m_debugTextEdit, "调试输出");
    
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &LogOutputWidget::onTabChanged);
}

void LogOutputWidget::setupTextFormats()
{
    // 设置各种日志级别的颜色
    m_debugFormat.setForeground(QColor(128, 128, 128));    // 灰色
    m_infoFormat.setForeground(QColor(0, 0, 0));           // 黑色
    m_warningFormat.setForeground(QColor(255, 165, 0));    // 橙色
    m_errorFormat.setForeground(QColor(255, 0, 0));        // 红色
    m_successFormat.setForeground(QColor(0, 128, 0));      // 绿色
    
    // 时间戳和分类格式
    m_timestampFormat.setForeground(QColor(100, 100, 100));
    m_categoryFormat.setForeground(QColor(0, 0, 255));
}

void LogOutputWidget::setMaxDisplayLines(int lines)
{
    m_maxDisplayLines = lines;
}

int LogOutputWidget::getMaxDisplayLines() const
{
    return m_maxDisplayLines;
}

void LogOutputWidget::setAutoScroll(bool enabled)
{
    m_autoScroll = enabled;
    m_autoScrollCheck->setChecked(enabled);
}

bool LogOutputWidget::isAutoScrollEnabled() const
{
    return m_autoScroll;
}

void LogOutputWidget::setShowTimestamp(bool enabled)
{
    m_showTimestamp = enabled;
    m_showTimestampCheck->setChecked(enabled);
}

bool LogOutputWidget::isShowTimestampEnabled() const
{
    return m_showTimestamp;
}

void LogOutputWidget::setShowCategory(bool enabled)
{
    m_showCategory = enabled;
    m_showCategoryCheck->setChecked(enabled);
}

bool LogOutputWidget::isShowCategoryEnabled() const
{
    return m_showCategory;
}

void LogOutputWidget::addLogEntry(const LogEntry& entry)
{
    QMutexLocker locker(&m_mutex);
    m_pendingLogs.enqueue(entry);
}

void LogOutputWidget::onRefreshTimer()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_pendingLogs.isEmpty())
        return;
    
    // 处理待处理的日志
    while (!m_pendingLogs.isEmpty())
    {
        LogEntry entry = m_pendingLogs.dequeue();
        
        // 添加到总日志列表
        m_allLogs.append(entry);
        
        // 更新分类集合
        if (!entry.category.isEmpty())
        {
            m_categories.insert(entry.category);
        }
        
        // 根据级别分类
        if (entry.level == LogLevel::Debug)
        {
            m_debugLogs.append(entry);
        }
        else
        {
            m_normalLogs.append(entry);
        }
        
        // 检查是否应该显示
        if (shouldDisplayLog(entry))
        {
            // 添加到对应的文本编辑器
            if (entry.level == LogLevel::Debug)
            {
                addLogToTextEdit(m_debugTextEdit, entry);
            }
            else
            {
                addLogToTextEdit(m_normalTextEdit, entry);
            }
        }
    }
    
    // 更新过滤选项
    updateFilterOptions();
    
    // 限制显示行数
    limitDisplayLines();
}

void LogOutputWidget::addLogToTextEdit(QTextEdit* textEdit, const LogEntry& entry)
{
    if (!textEdit) return;
    
    QTextCursor cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    
    // 格式化日志文本
    QString logText = formatLogText(entry);
    
    // 插入文本
    cursor.insertText(logText + "\n");
    
    // 设置颜色格式
    QTextCharFormat format;
    switch (entry.level)
    {
        case LogLevel::Debug:   format = m_debugFormat; break;
        case LogLevel::Info:    format = m_infoFormat; break;
        case LogLevel::Warning: format = m_warningFormat; break;
        case LogLevel::Error:   format = m_errorFormat; break;
        case LogLevel::Success: format = m_successFormat; break;
    }
    
    // 应用格式到新插入的文本
    cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    cursor.setCharFormat(format);
    
    // 自动滚动
    if (m_autoScroll)
    {
        textEdit->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->maximum());
    }
}

QString LogOutputWidget::formatLogText(const LogEntry& entry)
{
    QString text;
    
    // 时间戳
    if (m_showTimestamp)
    {
        text += entry.timestamp.toString("hh:mm:ss.zzz") + " ";
    }
    
    // 级别
    text += "[" + getLogLevelText(entry.level) + "] ";
    
    // 分类
    if (m_showCategory && !entry.category.isEmpty())
    {
        text += "[" + entry.category + "] ";
    }
    
    // 位置信息（文件名:行号）
    if (!entry.fileName.isEmpty() && entry.lineNumber > 0)
    {
        QString fileName = QFileInfo(entry.fileName).fileName();
        text += QString("(%1:%2) ").arg(fileName).arg(entry.lineNumber);
    }
    
    // 函数名
    if (!entry.functionName.isEmpty())
    {
        text += entry.functionName + " ";
    }
    
    // 消息
    text += entry.message;
    
    return text;
}

QColor LogOutputWidget::getLogLevelColor(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:   return QColor(128, 128, 128);
        case LogLevel::Info:    return QColor(0, 0, 0);
        case LogLevel::Warning: return QColor(255, 165, 0);
        case LogLevel::Error:   return QColor(255, 0, 0);
        case LogLevel::Success: return QColor(0, 128, 0);
        default:                return QColor(0, 0, 0);
    }
}

QString LogOutputWidget::getLogLevelText(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Success: return "SUCCESS";
        default:                return "UNKNOWN";
    }
}

void LogOutputWidget::refreshDisplay()
{
    // 清空现有显示
    m_normalTextEdit->clear();
    m_debugTextEdit->clear();
    
    // 重新加载所有日志
    LogManager* logManager = LogManager::getInstance();
    QList<LogEntry> allLogs = logManager->getLogs();
    
    QMutexLocker locker(&m_mutex);
    m_allLogs = allLogs;
    m_normalLogs.clear();
    m_debugLogs.clear();
    m_categories.clear();
    
    // 重新分类
    for (const LogEntry& entry : allLogs)
    {
        if (!entry.category.isEmpty())
        {
            m_categories.insert(entry.category);
        }
        
        if (entry.level == LogLevel::Debug)
        {
            m_debugLogs.append(entry);
        }
        else
        {
            m_normalLogs.append(entry);
        }
    }
    
    // 重新显示
    applyFilters();
    updateFilterOptions();
}

void LogOutputWidget::applyFilters()
{
    // 清空显示
    m_normalTextEdit->clear();
    m_debugTextEdit->clear();
    
    // 重新显示过滤后的日志
    for (const LogEntry& entry : m_allLogs)
    {
        if (shouldDisplayLog(entry))
        {
            if (entry.level == LogLevel::Debug)
            {
                addLogToTextEdit(m_debugTextEdit, entry);
            }
            else
            {
                addLogToTextEdit(m_normalTextEdit, entry);
            }
        }
    }
}

bool LogOutputWidget::shouldDisplayLog(const LogEntry& entry)
{
    // 级别过滤
    if (m_currentFilterLevel != LogLevel::Debug && entry.level != m_currentFilterLevel)
    {
        return false;
    }
    
    // 分类过滤
    if (!m_currentFilterCategory.isEmpty() && entry.category != m_currentFilterCategory)
    {
        return false;
    }
    
    return true;
}

void LogOutputWidget::updateFilterOptions()
{
    // 更新分类过滤选项
    QString currentCategory = m_filterCategoryCombo->currentText();
    m_filterCategoryCombo->clear();
    m_filterCategoryCombo->addItem("全部", "");
    
    for (const QString& category : m_categories)
    {
        m_filterCategoryCombo->addItem(category, category);
    }
    
    // 恢复之前的选择
    int index = m_filterCategoryCombo->findText(currentCategory);
    if (index >= 0)
    {
        m_filterCategoryCombo->setCurrentIndex(index);
    }
}

void LogOutputWidget::limitDisplayLines()
{
    // 限制普通输出行数
    QTextDocument* normalDoc = m_normalTextEdit->document();
    if (normalDoc->lineCount() > m_maxDisplayLines)
    {
        QTextCursor cursor = m_normalTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 
                          normalDoc->lineCount() - m_maxDisplayLines);
        cursor.removeSelectedText();
    }
    
    // 限制调试输出行数
    QTextDocument* debugDoc = m_debugTextEdit->document();
    if (debugDoc->lineCount() > m_maxDisplayLines)
    {
        QTextCursor cursor = m_debugTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 
                          debugDoc->lineCount() - m_maxDisplayLines);
        cursor.removeSelectedText();
    }
}

void LogOutputWidget::clearLogs()
{
    QMutexLocker locker(&m_mutex);
    m_allLogs.clear();
    m_normalLogs.clear();
    m_debugLogs.clear();
    m_categories.clear();
    m_pendingLogs.clear();
    
    m_normalTextEdit->clear();
    m_debugTextEdit->clear();
}

void LogOutputWidget::exportLogs(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "导出失败", "无法创建文件: " + filename);
        return;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    QMutexLocker locker(&m_mutex);
    for (const LogEntry& entry : m_allLogs)
    {
        stream << formatLogText(entry) << "\n";
    }
    
    file.close();
    QMessageBox::information(this, "导出成功", "日志已导出到: " + filename);
}

void LogOutputWidget::copySelectedText()
{
    QTextEdit* currentEdit = qobject_cast<QTextEdit*>(m_tabWidget->currentWidget());
    if (currentEdit)
    {
        QString selectedText = currentEdit->textCursor().selectedText();
        if (!selectedText.isEmpty())
        {
            QApplication::clipboard()->setText(selectedText);
        }
    }
}

// 槽函数实现
void LogOutputWidget::onTabChanged(int index)
{
    // 标签页切换时的处理
    Q_UNUSED(index)
}

void LogOutputWidget::onFilterLevelChanged(int index)
{
    m_currentFilterLevel = static_cast<LogLevel>(m_filterLevelCombo->itemData(index).toInt());
    applyFilters();
}

void LogOutputWidget::onFilterCategoryChanged(const QString& category)
{
    m_currentFilterCategory = category;
    applyFilters();
}

void LogOutputWidget::onAutoScrollToggled(bool checked)
{
    m_autoScroll = checked;
}

void LogOutputWidget::onShowTimestampToggled(bool checked)
{
    m_showTimestamp = checked;
    refreshDisplay();
}

void LogOutputWidget::onShowCategoryToggled(bool checked)
{
    m_showCategory = checked;
    refreshDisplay();
}

void LogOutputWidget::clearCurrentTab()
{
    QTextEdit* currentEdit = qobject_cast<QTextEdit*>(m_tabWidget->currentWidget());
    if (currentEdit)
    {
        currentEdit->clear();
    }
}

void LogOutputWidget::exportCurrentTab()
{
    QString filename = QFileDialog::getSaveFileName(this, "导出日志", 
                                                   "log_export.txt", "文本文件 (*.txt)");
    if (filename.isEmpty())
        return;
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "导出失败", "无法创建文件: " + filename);
        return;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    QTextEdit* currentEdit = qobject_cast<QTextEdit*>(m_tabWidget->currentWidget());
    if (currentEdit)
    {
        stream << currentEdit->toPlainText();
    }
    
    file.close();
    QMessageBox::information(this, "导出成功", "日志已导出到: " + filename);
}

void LogOutputWidget::showContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    
    QAction* copyAction = menu.addAction("复制");
    QAction* selectAllAction = menu.addAction("全选");
    menu.addSeparator();
    QAction* clearAction = menu.addAction("清空");
    QAction* exportAction = menu.addAction("导出");
    
    connect(copyAction, &QAction::triggered, this, &LogOutputWidget::copySelectedText);
    connect(selectAllAction, &QAction::triggered, [this]() {
        QTextEdit* currentEdit = qobject_cast<QTextEdit*>(m_tabWidget->currentWidget());
        if (currentEdit)
        {
            currentEdit->selectAll();
        }
    });
    connect(clearAction, &QAction::triggered, this, &LogOutputWidget::clearCurrentTab);
    connect(exportAction, &QAction::triggered, this, &LogOutputWidget::exportCurrentTab);
    
    menu.exec(mapToGlobal(pos));
} 