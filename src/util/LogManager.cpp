#include "LogManager.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QFileInfo>
#include <algorithm>

// ==================== LogManager 实现 ====================

LogManager* LogManager::s_instance = nullptr;

LogManager::LogManager()
{
    // 初始化默认配置
    m_config.maxLogCount = 1000;
    m_config.enableConsoleOutput = true;
    m_config.enableFileOutput = true;
    m_config.logFilePath = "";
    m_config.minLogLevel = LogLevel::Debug;
    m_config.enableLevelFilter = false;
    
    // 设置默认日志文件路径
    QString appDir = QCoreApplication::applicationDirPath();
    m_config.logFilePath = appDir + "/logs/app.log";
    
    // 确保日志目录存在
    ensureLogDirectory();
}

LogManager::~LogManager()
{
}

LogManager* LogManager::getInstance()
{
    if (!s_instance) {
        s_instance = new LogManager();
    }
    return s_instance;
}

void LogManager::log(LogLevel level, const QString& message, const QString& category,
                    const QString& fileName, int lineNumber, const QString& functionName)
{
    // 创建日志条目
    LogEntry entry(level, message, category, fileName, lineNumber, functionName);
    
    // 直接在主线程处理
    processLog(entry);
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

void LogManager::clearLogs()
{
    m_logs.clear();
    emit logsCleared();
}

void LogManager::setConfig(const LogConfig& config)
{
    m_config = config;
    ensureLogDirectory();
}

LogConfig LogManager::getConfig() const
{
    return m_config;
}

void LogManager::setMaxLogCount(int count)
{
    m_config.maxLogCount = count;
}

int LogManager::getMaxLogCount() const
{
    return m_config.maxLogCount;
}

void LogManager::setConsoleOutput(bool enabled)
{
    m_config.enableConsoleOutput = enabled;
}

bool LogManager::isConsoleOutputEnabled() const
{
    return m_config.enableConsoleOutput;
}

void LogManager::setFileOutput(bool enabled)
{
    m_config.enableFileOutput = enabled;
    if (enabled) {
        ensureLogDirectory();
    }
}

bool LogManager::isFileOutputEnabled() const
{
    return m_config.enableFileOutput;
}

void LogManager::setLogFilePath(const QString& path)
{
    m_config.logFilePath = path;
    ensureLogDirectory();
}

QString LogManager::getLogFilePath() const
{
    return m_config.logFilePath;
}

int LogManager::getPendingLogCount() const
{
    // 主线程实现中没有待处理队列，总是返回0
    return 0;
}

int LogManager::getCurrentLogCount() const
{
    return static_cast<int>(m_logs.size());
}

std::list<LogEntry> LogManager::getLogs() const
{
    return m_logs;
}

void LogManager::processLog(const LogEntry& entry)
{
    // 检查是否应该接受这个日志
    if (!shouldAcceptLog(entry)) {
        return;
    }
    
    // 添加到日志列表
    m_logs.push_back(entry);
    
    // 如果日志数量超过限制，删除最旧的
    if (m_logs.size() > static_cast<size_t>(m_config.maxLogCount)) {
        m_logs.pop_front();
    }
    
    // 控制台输出
    if (m_config.enableConsoleOutput) {
        QString msg = formatLogMessage(entry);
        qDebug().noquote() << msg;
    }
    
    // 文件输出
    if (m_config.enableFileOutput && !m_config.logFilePath.isEmpty()) {
        writeToFile(entry);
    }
    
    // 发送信号
    emit logAdded(entry);
}

void LogManager::writeToFile(const LogEntry& entry)
{
    QFile file(m_config.logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << formatLogMessage(entry) << "\n";
        file.close();
    }
}

QString LogManager::formatLogMessage(const LogEntry& entry) const
{
    QString levelStr;
    switch (entry.level) {
        case LogLevel::Debug:   levelStr = "DEBUG"; break;
        case LogLevel::Info:    levelStr = "INFO"; break;
        case LogLevel::Warning: levelStr = "WARN"; break;
        case LogLevel::Error:   levelStr = "ERROR"; break;
        case LogLevel::Success: levelStr = "SUCCESS"; break;
    }
    
    // 完整格式
    QString timestamp = entry.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString threadId = QString::number((quintptr)entry.thread);
    QString message = QString("[%1] [%2] [Thread:%3]")
                     .arg(timestamp)
                     .arg(levelStr)
                     .arg(threadId);
    
    if (!entry.category.isEmpty()) {
        message += QString(" [%1]").arg(entry.category);
    }
    
    if (!entry.fileName.isEmpty()) {
        QString fileName = QFileInfo(entry.fileName).fileName();
        message += QString(" [%1:%2]").arg(fileName).arg(entry.lineNumber);
    }
    
    if (!entry.functionName.isEmpty()) {
        message += QString(" [%1]").arg(entry.functionName);
    }
    
    message += QString(": %1").arg(entry.message);
    return message;
}

bool LogManager::shouldAcceptLog(const LogEntry& entry) const
{
    // 级别过滤
    if (m_config.enableLevelFilter) {
        if (entry.level < m_config.minLogLevel) {
            return false;
        }
    }
    return true;
}

void LogManager::ensureLogDirectory()
{
    if (!m_config.logFilePath.isEmpty()) {
        QDir logDir(QFileInfo(m_config.logFilePath).absolutePath());
        if (!logDir.exists()) {
            logDir.mkpath(".");
        }
    }
} 

