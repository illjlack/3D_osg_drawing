#include "LogManager.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QMetaObject>
#include <QFileInfo>
#include <algorithm>

// ==================== LogWorkerThread 实现 ====================

LogWorkerThread::LogWorkerThread(QObject* parent)
    : QThread(parent)
    , m_running(false)
    , m_shouldExit(false)
{
    // 设置默认配置
    m_config.maxLogCount = 1000;
    m_config.enableConsoleOutput = true;
    m_config.enableFileOutput = false;
    m_config.logFilePath = "";
    m_config.minLogLevel = LogLevel::Debug;
    m_config.enableLevelFilter = false;
    
    // 初始化日志目录
    QString appDir = QCoreApplication::applicationDirPath();
    m_config.logFilePath = appDir + "/logs/app.log";
    QDir logDir(QFileInfo(m_config.logFilePath).absolutePath());
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
}

LogWorkerThread::~LogWorkerThread()
{
    // 设置退出标志
    m_shouldExit = true;
    m_running = false;
    
    // 等待线程退出
    if (isRunning()) {
        quit();
        if (!wait(3000)) {  // 等待3秒
            terminate();
            wait(1000);  // 再等待1秒
        }
    }
}

void LogWorkerThread::addLog(const LogEntry& entry)
{
    // 快速过滤
    if (!shouldAcceptLog(entry)) {
        return;
    }
    
    // 如果队列过满，丢弃旧的日志
    if (m_logQueue.size() > m_config.maxLogCount * 2) {
        while (m_logQueue.size() > m_config.maxLogCount) {
            m_logQueue.pop();
        }
    }
    
    m_logQueue.push(entry);
}

void LogWorkerThread::setConfig(const LogConfig& config)
{
    m_config = config;
    
    // 更新日志目录
    if (!m_config.logFilePath.isEmpty()) {
        QDir logDir(QFileInfo(m_config.logFilePath).absolutePath());
        if (!logDir.exists()) {
            logDir.mkpath(".");
        }
    }
}

LogConfig LogWorkerThread::getConfig() const
{
    return m_config;
}

void LogWorkerThread::setConsoleOutput(bool enabled)
{
    m_config.enableConsoleOutput = enabled;
}

void LogWorkerThread::setFileOutput(bool enabled)
{
    m_config.enableFileOutput = enabled;
}

void LogWorkerThread::setLogFilePath(const QString& path)
{
    m_config.logFilePath = path;
    if (!path.isEmpty()) {
        QDir logDir(QFileInfo(path).absolutePath());
        if (!logDir.exists()) {
            logDir.mkpath(".");
        }
    }
}

void LogWorkerThread::setMaxLogCount(int count)
{
    m_config.maxLogCount = count;
}

std::list<LogEntry> LogWorkerThread::getLogs() const
{
    return m_logs;
}

void LogWorkerThread::clearLogs()
{
    m_logs.clear();
    emit logsCleared();
}

int LogWorkerThread::getPendingLogCount() const
{
    return static_cast<int>(m_logQueue.size());
}

int LogWorkerThread::getCurrentLogCount() const
{
    return static_cast<int>(m_logs.size());
}

void LogWorkerThread::requestExit()
{
    m_shouldExit = true;
}

void LogWorkerThread::run()
{
    m_running = true;
    
    while (m_running && !m_shouldExit) {
        LogEntry entry;
        bool hasEntry = false;
        
        if (!m_logQueue.empty()) {
            entry = m_logQueue.front();
            m_logQueue.pop();
            hasEntry = true;
        } else {
            // 检查线程是否应该退出
            if (m_shouldExit) {
                break;
            }
            // 直接继续循环，Qt事件循环会自动处理CPU占用
            continue;
        }
        
        if (hasEntry) {
            // 处理日志
            m_logs.push_back(entry);
            
            // 如果日志数量超过限制，删除最旧的
            if (m_logs.size() > m_config.maxLogCount) {
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
            emitLogAdded(entry);
        }
    }
}

void LogWorkerThread::writeToFile(const LogEntry& entry)
{
    QFile file(m_config.logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << formatLogMessage(entry) << "\n";
        file.close();
    }
}

QString LogWorkerThread::formatLogMessage(const LogEntry& entry) const
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

void LogWorkerThread::emitLogAdded(const LogEntry& entry)
{
    QMetaObject::invokeMethod(this, [this, entry]() {
        emit logAdded(entry);
    }, Qt::QueuedConnection);
}

bool LogWorkerThread::shouldAcceptLog(const LogEntry& entry) const
{
    // 级别过滤
    if (m_config.enableLevelFilter) {
        if (entry.level < m_config.minLogLevel) {
            return false;
        }
    }
    
    return true;
}

// ==================== LogManager 实现 ====================

LogManager* LogManager::s_instance = nullptr;

LogManager::LogManager()
    : m_workerThread(nullptr)
{
    // 初始化默认配置
    m_config.maxLogCount = 1000;
    m_config.enableConsoleOutput = true;
    m_config.enableFileOutput = false;
    m_config.logFilePath = "";
    m_config.minLogLevel = LogLevel::Debug;
    m_config.enableLevelFilter = false;
    
    m_workerThread = new LogWorkerThread(this);
    connect(m_workerThread, &LogWorkerThread::logAdded, this, &LogManager::logAdded);
    connect(m_workerThread, &LogWorkerThread::logsCleared, this, &LogManager::logsCleared);
    
    m_workerThread->setConfig(m_config);
    m_workerThread->start();
}

LogManager::~LogManager()
{
    if (m_workerThread) {
        qDebug() << "LogManager destructor: stopping worker thread";
        
        // 请求线程退出
        m_workerThread->requestExit();
        
        // 停止线程
        m_workerThread->quit();
        
        // 等待线程退出
        if (!m_workerThread->wait(3000)) {
            qDebug() << "LogManager destructor: force terminating worker thread";
            m_workerThread->terminate();
            m_workerThread->wait(1000);
        }
        
        qDebug() << "LogManager destructor: deleting worker thread";
        delete m_workerThread;
        m_workerThread = nullptr;
    }
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
    if (m_workerThread) {
        // 当前调用线程构造对象
        LogEntry entry(level, message, category, fileName, lineNumber, functionName);
        
        // 捕获 entry，并将其推入日志线程中执行（拷贝entry）
        QMetaObject::invokeMethod(m_workerThread, [this, entry]() {
            m_workerThread->addLog(entry);
            }, Qt::QueuedConnection);
    }
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
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, &LogWorkerThread::clearLogs, Qt::QueuedConnection);
    }
}

void LogManager::setConfig(const LogConfig& config)
{
    m_config = config;
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, [this, config]() {
            m_workerThread->setConfig(config);
        }, Qt::QueuedConnection);
    }
}

LogConfig LogManager::getConfig() const
{
    return m_config;
}

void LogManager::setMaxLogCount(int count)
{
    m_config.maxLogCount = count;
    
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, [this, count]() {
            m_workerThread->setMaxLogCount(count);
        }, Qt::QueuedConnection);
    }
}

int LogManager::getMaxLogCount() const
{
    return m_config.maxLogCount;
}

void LogManager::setConsoleOutput(bool enabled)
{
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, [this, enabled]() {
            m_workerThread->setConsoleOutput(enabled);
        }, Qt::QueuedConnection);
    }
}

bool LogManager::isConsoleOutputEnabled() const
{
    return m_workerThread ? true : false;
}

void LogManager::setFileOutput(bool enabled)
{
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, [this, enabled]() {
            m_workerThread->setFileOutput(enabled);
        }, Qt::QueuedConnection);
    }
}

bool LogManager::isFileOutputEnabled() const
{
    return m_workerThread ? true : false;
}

void LogManager::setLogFilePath(const QString& path)
{
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, [this, path]() {
            m_workerThread->setLogFilePath(path);
        }, Qt::QueuedConnection);
    }
}

QString LogManager::getLogFilePath() const
{
    return m_workerThread ? m_workerThread->getConfig().logFilePath : "";
}

int LogManager::getPendingLogCount() const
{
    return m_workerThread ? m_workerThread->getPendingLogCount() : 0;
}

int LogManager::getCurrentLogCount() const
{
    return m_workerThread ? m_workerThread->getCurrentLogCount() : 0;
} 