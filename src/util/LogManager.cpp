#include "LogManager.h"
#include <QDebug>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>

LogManager* LogManager::s_instance = nullptr;

LogManager* LogManager::getInstance()
{
    if (!s_instance)
    {
        s_instance = new LogManager();
    }
    return s_instance;
}

LogManager::LogManager()
    : m_maxLogCount(1000)
    , m_autoCleanup(true)
    , m_autoCleanupInterval(30) // 30分钟
    , m_cleanupTimer(new QTimer(this))
    , m_consoleOutput(true)
    , m_fileOutput(false)
    , m_logFilePath("")
{
    // 设置默认日志文件路径
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }
    m_logFilePath = dir.filePath("3Drawing.log");
    
    // 设置自动清理定时器
    connect(m_cleanupTimer, &QTimer::timeout, this, &LogManager::cleanupOldLogs);
    if (m_autoCleanup)
    {
        m_cleanupTimer->start(m_autoCleanupInterval * 60 * 1000); // 转换为毫秒
    }
    
    // 输出初始化日志
    info("日志系统初始化完成", "系统");
}

LogManager::~LogManager()
{
    if (m_cleanupTimer)
    {
        m_cleanupTimer->stop();
    }
    
    info("日志系统关闭", "系统");
}

void LogManager::log(LogLevel level, const QString& message, const QString& category,
                     const QString& fileName, int lineNumber, const QString& functionName)
{
    LogEntry entry(level, message, category, fileName, lineNumber, functionName);
    addLog(entry);
}

void LogManager::debug(const QString& message, const QString& category,
                       const QString& fileName, int lineNumber, const QString& functionName)
{
    log(LogLevel::Debug, message, category, fileName, lineNumber, functionName);
}

void LogManager::info(const QString& message, const QString& category,
                      const QString& fileName, int lineNumber, const QString& functionName)
{
    log(LogLevel::Info, message, category, fileName, lineNumber, functionName);
}

void LogManager::warning(const QString& message, const QString& category,
                         const QString& fileName, int lineNumber, const QString& functionName)
{
    log(LogLevel::Warning, message, category, fileName, lineNumber, functionName);
}

void LogManager::error(const QString& message, const QString& category,
                       const QString& fileName, int lineNumber, const QString& functionName)
{
    log(LogLevel::Error, message, category, fileName, lineNumber, functionName);
}

void LogManager::success(const QString& message, const QString& category,
                         const QString& fileName, int lineNumber, const QString& functionName)
{
    log(LogLevel::Success, message, category, fileName, lineNumber, functionName);
}

QList<LogEntry> LogManager::getLogs() const
{
    QMutexLocker locker(&m_mutex);
    return m_logs;
}

QList<LogEntry> LogManager::getLogsByLevel(LogLevel level) const
{
    QMutexLocker locker(&m_mutex);
    QList<LogEntry> filteredLogs;
    
    for (const LogEntry& entry : m_logs)
    {
        if (entry.level == level)
        {
            filteredLogs.append(entry);
        }
    }
    
    return filteredLogs;
}

QList<LogEntry> LogManager::getLogsByCategory(const QString& category) const
{
    QMutexLocker locker(&m_mutex);
    QList<LogEntry> filteredLogs;
    
    for (const LogEntry& entry : m_logs)
    {
        if (entry.category == category)
        {
            filteredLogs.append(entry);
        }
    }
    
    return filteredLogs;
}

void LogManager::clearLogs()
{
    QMutexLocker locker(&m_mutex);
    m_logs.clear();
    emit logsCleared();
}

void LogManager::setMaxLogCount(int count)
{
    m_maxLogCount = count;
    if (m_logs.size() > m_maxLogCount)
    {
        cleanupOldLogs();
    }
}

int LogManager::getMaxLogCount() const
{
    return m_maxLogCount;
}

void LogManager::setAutoCleanup(bool enabled)
{
    m_autoCleanup = enabled;
    if (enabled)
    {
        m_cleanupTimer->start(m_autoCleanupInterval * 60 * 1000);
    }
    else
    {
        m_cleanupTimer->stop();
    }
}

bool LogManager::isAutoCleanupEnabled() const
{
    return m_autoCleanup;
}

void LogManager::setAutoCleanupInterval(int minutes)
{
    m_autoCleanupInterval = minutes;
    if (m_autoCleanup && m_cleanupTimer->isActive())
    {
        m_cleanupTimer->start(m_autoCleanupInterval * 60 * 1000);
    }
}

int LogManager::getAutoCleanupInterval() const
{
    return m_autoCleanupInterval;
}

void LogManager::setConsoleOutput(bool enabled)
{
    m_consoleOutput = enabled;
}

bool LogManager::isConsoleOutputEnabled() const
{
    return m_consoleOutput;
}

void LogManager::setFileOutput(bool enabled)
{
    m_fileOutput = enabled;
}

bool LogManager::isFileOutputEnabled() const
{
    return m_fileOutput;
}

void LogManager::setLogFilePath(const QString& path)
{
    m_logFilePath = path;
}

QString LogManager::getLogFilePath() const
{
    return m_logFilePath;
}

void LogManager::addLog(const LogEntry& entry)
{
    QMutexLocker locker(&m_mutex);
    
    // 添加新日志
    m_logs.append(entry);
    
    // 如果超过最大数量，移除最旧的日志
    if (m_logs.size() > m_maxLogCount)
    {
        m_logs.removeFirst();
    }
    
    // 发送信号（在主线程中）
    QMetaObject::invokeMethod(this, [this, entry]() {
        emit logAdded(entry);
    }, Qt::QueuedConnection);
    
    // 输出到控制台
    if (m_consoleOutput)
    {
        QString formattedMessage = formatLogMessage(entry);
        qDebug().noquote() << formattedMessage;
    }
    
    // 输出到文件
    if (m_fileOutput)
    {
        writeToFile(entry);
    }
}

void LogManager::cleanupOldLogs()
{
    QMutexLocker locker(&m_mutex);
    
    // 保留最近的日志
    while (m_logs.size() > m_maxLogCount)
    {
        m_logs.removeFirst();
    }
}

void LogManager::writeToFile(const LogEntry& entry)
{
    if (m_logFilePath.isEmpty())
        return;
    
    QFile file(m_logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        
        QString formattedMessage = formatLogMessage(entry);
        stream << formattedMessage << "\n";
        
        file.close();
    }
}

QString LogManager::formatLogMessage(const LogEntry& entry) const
{
    QString levelStr;
    switch (entry.level)
    {
        case LogLevel::Debug:   levelStr = "DEBUG"; break;
        case LogLevel::Info:    levelStr = "INFO"; break;
        case LogLevel::Warning: levelStr = "WARN"; break;
        case LogLevel::Error:   levelStr = "ERROR"; break;
        case LogLevel::Success: levelStr = "SUCCESS"; break;
    }
    
    QString timestampStr = entry.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString categoryStr = entry.category.isEmpty() ? "" : QString("[%1]").arg(entry.category);
    
    QString locationStr;
    if (!entry.fileName.isEmpty())
    {
        QString fileName = QFileInfo(entry.fileName).fileName();
        locationStr = QString("(%1:%2)").arg(fileName).arg(entry.lineNumber);
        if (!entry.functionName.isEmpty())
        {
            locationStr += QString(" %1").arg(entry.functionName);
        }
    }
    
    QString threadStr = entry.thread ? QString("[%1]").arg(entry.thread->objectName().isEmpty() ? 
                                                          QString::number(reinterpret_cast<quintptr>(entry.thread)) : 
                                                          entry.thread->objectName()) : "";
    
    return QString("[%1] %2%3 %4%5: %6")
           .arg(timestampStr)
           .arg(levelStr)
           .arg(categoryStr)
           .arg(locationStr)
           .arg(threadStr)
           .arg(entry.message);
} 