#include "LogOutputWidget.h"
#include <QTextStream>
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QRegularExpression>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QTimer>
#include <QThread>
// 使用Qt信号槽机制确保线程安全
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

// ==================== LogOutputWidget 实现 ====================

LogOutputWidget::LogOutputWidget(QWidget* parent)
    : QWidget(parent)
    , m_maxDisplayLines(1000)
    , m_autoScroll(true)
    , m_showTimestamp(true)
    , m_showCategory(true)
    , m_needsFullRefresh(false)
    , m_isFiltering(false)
{
    // 初始化过滤级别为全部
    m_selectedFilterLevels.insert(LogLevel::Debug);
    m_selectedFilterLevels.insert(LogLevel::Info);
    m_selectedFilterLevels.insert(LogLevel::Warning);
    m_selectedFilterLevels.insert(LogLevel::Error);
    m_selectedFilterLevels.insert(LogLevel::Success);
    
    setupUI();
    setupTextFormats();
    
    // 连接日志管理器信号
    LogManager* logManager = LogManager::getInstance();
    connect(logManager, &LogManager::logAdded, this, &LogOutputWidget::addLogEntry);
    connect(logManager, &LogManager::logsCleared, this, &LogOutputWidget::clearLogs);
}

LogOutputWidget::~LogOutputWidget()
{
}

void LogOutputWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 创建工具栏
    setupToolbar();
    mainLayout->addWidget(m_toolBar);
    
    // 创建文本编辑器 - 使用QPlainTextEdit提升性能
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_textEdit->setFont(QFont("Consolas", 9));
    m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textEdit, &QPlainTextEdit::customContextMenuRequested,
            this, &LogOutputWidget::showContextMenu);
    
    mainLayout->addWidget(m_textEdit);
    
    setLayout(mainLayout);
}

void LogOutputWidget::setupToolbar()
{
    m_toolBar = new QToolBar(this);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolBar->setIconSize(QSize(16, 16));
    
    // 过滤级别（下拉多选）
    m_toolBar->addWidget(new QLabel("级别:"));
    m_filterLevelCombo = new QComboBox(this);
    m_filterLevelCombo->setEditable(false);
    m_filterLevelCombo->setMaxVisibleItems(10);
    
    // 添加级别选项
    m_filterLevelCombo->addItem("全部", -1);
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
    m_filterCategoryCombo->setToolTip("支持多关键词搜索，用空格分隔");
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
    connect(m_filterLevelCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onFilterLevelChanged()));
    connect(m_filterCategoryCombo, SIGNAL(currentTextChanged(QString)),
            this, SLOT(onFilterCategoryChanged(QString)));
    connect(m_autoScrollCheck, SIGNAL(toggled(bool)),
            this, SLOT(onAutoScrollToggled(bool)));
    connect(m_showTimestampCheck, SIGNAL(toggled(bool)),
            this, SLOT(onShowTimestampToggled(bool)));
    connect(m_showCategoryCheck, SIGNAL(toggled(bool)),
            this, SLOT(onShowCategoryToggled(bool)));
    connect(m_clearButton, SIGNAL(clicked()),
            this, SLOT(clearLogs()));
    connect(m_exportButton, SIGNAL(clicked()),
            this, SLOT(exportLogs()));
    connect(m_copyButton, SIGNAL(clicked()),
            this, SLOT(copySelectedText()));
    
    // 设置默认选中"全部"
    m_filterLevelCombo->setCurrentIndex(0);
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
    qDebug() << "LogOutputWidget::addLogEntry - 接收到日志:" << entry.message;
    
    // 添加日志到本地存储
    m_allLogs.append(entry);
    
    // 更新分类集合
    if (!entry.category.isEmpty())
    {
        m_categories.insert(entry.category);
    }
    
    // 检查是否应该显示
    if (shouldDisplayLog(entry))
    {
        m_filteredLogs.append(entry);
        // 直接插入UI
        addLogToTextEdit(entry);
        limitDisplayLines();
    }
    
    // 更新过滤选项（异步，避免阻塞UI）
    QTimer::singleShot(0, this, &LogOutputWidget::updateFilterOptions);
}

bool LogOutputWidget::shouldDisplayLog(const LogEntry& entry) const
{
    // 级别过滤
    if (!m_selectedFilterLevels.isEmpty() && !m_selectedFilterLevels.contains(entry.level))
    {
        return false;
    }
    
    // 分类过滤
    if (!m_currentFilterCategory.isEmpty())
    {
        QStringList keywords = m_currentFilterCategory.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (keywords.isEmpty())
        {
            return true; // 如果没有有效关键词，显示所有
        }
        
        bool matchesAnyKeyword = false;
        
        for (const QString& keyword : keywords)
        {
            if (!keyword.isEmpty() && entry.category.contains(keyword, Qt::CaseInsensitive))
            {
                matchesAnyKeyword = true;
                break;
            }
        }
        
        if (!matchesAnyKeyword)
        {
            return false;
        }
    }
    
    return true;
}

void LogOutputWidget::applyFilters()
{
    // 清空之前的过滤结果
    m_filteredLogs.clear();
    
    // 如果日志数量很大，使用分批处理避免卡死
    const int BATCH_SIZE = 1000;
    int totalLogs = m_allLogs.size();
    
    if (totalLogs > BATCH_SIZE)
    {
        // 分批处理大量日志
        for (int i = 0; i < totalLogs; i += BATCH_SIZE)
        {
            int endIndex = qMin(i + BATCH_SIZE, totalLogs);
            
            for (int j = i; j < endIndex; ++j)
            {
                const LogEntry& entry = m_allLogs[j];
                if (shouldDisplayLog(entry))
                {
                    m_filteredLogs.append(entry);
                }
            }
            
            // 让出CPU时间，避免卡死UI
            if (i + BATCH_SIZE < totalLogs)
            {
                QThread::msleep(1);
                QApplication::processEvents();
            }
        }
    }
    else
    {
        // 小量日志直接处理
        for (const LogEntry& entry : m_allLogs)
        {
            if (shouldDisplayLog(entry))
            {
                m_filteredLogs.append(entry);
            }
        }
    }
}

void LogOutputWidget::addLogsToTextEdit(const QList<LogEntry>& logs)
{
    if (!m_textEdit || logs.isEmpty()) return;
    
    qDebug() << "LogOutputWidget::addLogsToTextEdit - 开始添加日志到UI，数量:" << logs.size();
    
    // 准备批量文本插入 - 优化版本
    QStringList textLines;
    textLines.reserve(logs.size()); // 预分配内存
    
    for (const LogEntry& entry : logs)
    {
        QString logText = formatLogText(entry);
        textLines.append(logText);
    }
    
    // 使用QTextDocument的批量操作提高性能
    QTextDocument* document = m_textEdit->document();
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    
    // 批量插入文本 - 使用单个insertText操作
    QString allText = textLines.join("\n") + "\n";
    cursor.insertText(allText);
    
    // 自动滚动 - 只在需要时执行
    if (m_autoScroll)
    {
        QScrollBar* scrollBar = m_textEdit->verticalScrollBar();
        if (scrollBar && scrollBar->maximum() > scrollBar->value())
        {
            scrollBar->setValue(scrollBar->maximum());
        }
    }
    
    qDebug() << "LogOutputWidget::addLogsToTextEdit - 完成添加日志到UI";
}

void LogOutputWidget::addLogToTextEdit(const LogEntry& entry)
{
    if (!m_textEdit) return;
    
    QString logText = formatLogText(entry);
    
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
    
    // 插入文本
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(logText + "\n", format);
    
    // 自动滚动
    if (m_autoScroll)
    {
        m_textEdit->verticalScrollBar()->setValue(m_textEdit->verticalScrollBar()->maximum());
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
    
    // 位置信息
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

void LogOutputWidget::updateFilterOptions()
{
    // 更新分类下拉框选项
    QString currentText = m_filterCategoryCombo->currentText();
    m_filterCategoryCombo->clear();
    m_filterCategoryCombo->addItem("全部", "");
    
    // 添加所有可用的分类
    for (const QString& category : m_categories)
    {
        if (!category.isEmpty())
        {
            m_filterCategoryCombo->addItem(category, category);
        }
    }
    
    // 恢复当前选中的文本
    int index = m_filterCategoryCombo->findText(currentText);
    if (index >= 0)
    {
        m_filterCategoryCombo->setCurrentIndex(index);
    }
    else
    {
        m_filterCategoryCombo->setCurrentText(currentText);
    }
}

void LogOutputWidget::refreshDisplay()
{
    m_needsFullRefresh = true;
    m_textEdit->clear();
    
    // 应用过滤器并显示过滤后的日志
    applyFilters();
    
    // 批量添加到UI
    if (!m_filteredLogs.isEmpty())
    {
        addLogsToTextEdit(m_filteredLogs);
    }
}

void LogOutputWidget::limitDisplayLines()
{
    QTextDocument* doc = m_textEdit->document();
    if (doc->lineCount() > m_maxDisplayLines)
    {
        QTextCursor cursor = m_textEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 
                          doc->lineCount() - m_maxDisplayLines);
        cursor.removeSelectedText();
    }
}

void LogOutputWidget::clearLogs()
{
    m_textEdit->clear();
    
    // 清空所有日志数据
    m_allLogs.clear();
    m_filteredLogs.clear();
    m_categories.clear();
    
    // 通知LogManager清空全局日志
    LogManager* logManager = LogManager::getInstance();
    if (logManager)
    {
        logManager->clearLogs();
    }
    
    // 重置过滤选项
    m_filterCategoryCombo->clear();
    m_filterCategoryCombo->addItem("全部", "");
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
    
    // 导出过滤后的日志或所有日志
    QList<LogEntry> logsToExport = m_filteredLogs.isEmpty() ? m_allLogs : m_filteredLogs;
    
    for (const LogEntry& entry : logsToExport)
    {
        stream << formatLogText(entry) << "\n";
    }
    
    file.close();
    
    QString message = QString("日志已导出到: %1\n共导出 %2 条日志").arg(filename).arg(logsToExport.size());
    QMessageBox::information(this, "导出成功", message);
}

void LogOutputWidget::exportLogs()
{
    QString filename = QFileDialog::getSaveFileName(this, "导出日志", 
                                                   "log_export.txt", "文本文件 (*.txt)");
    if (!filename.isEmpty())
    {
        // 调用带参数的版本，避免无限递归
        this->exportLogs(filename);
    }
}

void LogOutputWidget::copySelectedText()
{
    if (m_textEdit)
    {
        QString selectedText = m_textEdit->textCursor().selectedText();
        if (!selectedText.isEmpty())
        {
            QApplication::clipboard()->setText(selectedText);
        }
    }
}

void LogOutputWidget::selectAllText()
{
    if (m_textEdit)
    {
        m_textEdit->selectAll();
    }
}

// 槽函数实现
void LogOutputWidget::onFilterLevelChanged()
{
    // 防抖：如果正在处理筛选，跳过
    if (m_isFiltering)
    {
        return;
    }
    
    m_isFiltering = true;
    
    QSet<LogLevel> selectedLevels;
    
    int currentIndex = m_filterLevelCombo->currentIndex();
    if (currentIndex == 0) // "全部"选项
    {
        selectedLevels.insert(LogLevel::Debug);
        selectedLevels.insert(LogLevel::Info);
        selectedLevels.insert(LogLevel::Warning);
        selectedLevels.insert(LogLevel::Error);
        selectedLevels.insert(LogLevel::Success);
    }
    else
    {
        int levelValue = m_filterLevelCombo->itemData(currentIndex).toInt();
        if (levelValue == -1) // "全部"选项
        {
            selectedLevels.insert(LogLevel::Debug);
            selectedLevels.insert(LogLevel::Info);
            selectedLevels.insert(LogLevel::Warning);
            selectedLevels.insert(LogLevel::Error);
            selectedLevels.insert(LogLevel::Success);
        }
        else if (levelValue >= 0)
        {
            selectedLevels.insert(static_cast<LogLevel>(levelValue));
        }
    }
    
    // 更新过滤器
    m_selectedFilterLevels = selectedLevels;
    m_currentFilterCategory = m_filterCategoryCombo->currentText();
    
    // 应用过滤器
    applyFilters();
    
    // 刷新显示
    refreshDisplay();
    
    m_isFiltering = false;
}

void LogOutputWidget::onFilterCategoryChanged(const QString& category)
{
    // 防抖：如果正在处理筛选，跳过
    if (m_isFiltering)
    {
        return;
    }
    
    m_isFiltering = true;
    
    QString filterCategory;
    if (category != "全部" && !category.isEmpty())
    {
        filterCategory = category.trimmed();
    }
    
    // 更新过滤器
    m_currentFilterCategory = filterCategory;
    
    // 应用过滤器
    applyFilters();
    
    // 刷新显示
    refreshDisplay();
    
    m_isFiltering = false;
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

void LogOutputWidget::exportLogsWithOptions(const QString& filename, bool exportFiltered)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "导出失败", "无法创建文件: " + filename);
        return;
    }
    
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    
    // 根据选项导出日志
    QList<LogEntry> logsToExport;
    
    if (exportFiltered && !m_filteredLogs.isEmpty())
    {
        logsToExport = m_filteredLogs;
    }
    else
    {
        logsToExport = m_allLogs;
    }
    
    for (const LogEntry& entry : logsToExport)
    {
        stream << formatLogText(entry) << "\n";
    }
    
    file.close();
    
    QString rangeText = exportFiltered ? "筛选结果" : "所有日志";
    QString message = QString("日志已导出到: %1\n导出范围: %2\n共导出 %3 条日志")
                     .arg(filename).arg(rangeText).arg(logsToExport.size());
    QMessageBox::information(this, "导出成功", message);
}



void LogOutputWidget::showContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    
    QAction* copyAction = menu.addAction("复制");
    QAction* selectAllAction = menu.addAction("全选");
    menu.addSeparator();
    QAction* clearAction = menu.addAction("清空");
    QAction* exportAction = menu.addAction("导出");
    
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copySelectedText()));
    connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllText()));
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clearLogs()));
    connect(exportAction, SIGNAL(triggered()), this, SLOT(exportLogs()));
    
    menu.exec(mapToGlobal(pos));
} 