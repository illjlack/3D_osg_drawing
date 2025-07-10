#include "LogManager.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QMetaObject>
#include <QTimer>
#include <QFileInfo>

// ==================== LogWorkerThread 实现 ====================

LogWorkerThread::LogWorkerThread(QObject* parent)
    : QThread(parent)
    , m_maxLogCount(1000)
    , m_autoCleanup(true)
    , m_autoCleanupInterval(60)
    , m_consoleOutput(true)
    , m_fileOutput(false)
    , m_logFilePath("")
    , m_running(false)
    , m_cleanupTimer(nullptr)
{
    QString appDir = QCoreApplication::applicationDirPath();
    m_logFilePath = appDir + "/logs/app.log";
    QDir logDir(QFileInfo(m_logFilePath).absolutePath());
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
}

LogWorkerThread::~LogWorkerThread()
{
    m_running = false;
    m_queueCondition.wakeAll();
    if (isRunning()) {
        quit();
        wait(3000);
        if (isRunning()) {
            terminate();
            wait(1000);
        }
    }
    delete m_cleanupTimer;
}

void LogWorkerThread::addLog(const LogEntry& entry)
{
    QMutexLocker locker(&m_queueMutex);
    m_logQueue.enqueue(entry);
    m_queueCondition.wakeOne();
}

void LogWorkerThread::setConsoleOutput(bool enabled)
{
    QMutexLocker locker(&m_logsMutex);
    m_consoleOutput = enabled;
}

void LogWorkerThread::setFileOutput(bool enabled)
{
    QMutexLocker locker(&m_logsMutex);
    m_fileOutput = enabled;
}

void LogWorkerThread::setLogFilePath(const QString& path)
{
    QMutexLocker locker(&m_logsMutex);
    m_logFilePath = path;
    if (!path.isEmpty()) {
        QDir logDir(QFileInfo(path).absolutePath());
        if (!logDir.exists()) {
            logDir.mkpath(".");
        }
    }
}

void LogWorkerThread::setMaxLogCount(int count)
{
    QMutexLocker locker(&m_logsMutex);
    m_maxLogCount = count;
}

void LogWorkerThread::setAutoCleanup(bool enabled, int interval)
{
    QMutexLocker locker(&m_logsMutex);
    m_autoCleanup = enabled;
    m_autoCleanupInterval = interval;
    if (m_cleanupTimer) {
        if (enabled) {
            m_cleanupTimer->start(interval * 60 * 1000);
        } else {
            m_cleanupTimer->stop();
        }
    }
}

QList<LogEntry> LogWorkerThread::getLogs() const
{
    QMutexLocker locker(&m_logsMutex);
    return m_logs;
}

QList<LogEntry> LogWorkerThread::getLogsByLevel(LogLevel level) const
{
    QMutexLocker locker(&m_logsMutex);
    QList<LogEntry> result;
    for (const auto& entry : m_logs) {
        if (entry.level == level) {
            result.append(entry);
        }
    }
    return result;
}

QList<LogEntry> LogWorkerThread::getLogsByCategory(const QString& category) const
{
    QMutexLocker locker(&m_logsMutex);
    QList<LogEntry> result;
    for (const auto& entry : m_logs) {
        if (entry.category == category) {
            result.append(entry);
        }
    }
    return result;
}

void LogWorkerThread::clearLogs()
{
    QMutexLocker locker(&m_logsMutex);
    m_logs.clear();
    emit logsCleared();
}

void LogWorkerThread::run()
{
    m_running = true;
    m_cleanupTimer = new QTimer();
    m_cleanupTimer->moveToThread(this);
    connect(m_cleanupTimer, &QTimer::timeout, this, &LogWorkerThread::cleanupOldLogs);
    if (m_autoCleanup) {
        m_cleanupTimer->start(m_autoCleanupInterval * 60 * 1000);
    }
    while (m_running) {
        LogEntry entry;
        bool hasEntry = false;
        {
            QMutexLocker locker(&m_queueMutex);
            if (!m_logQueue.isEmpty()) {
                entry = m_logQueue.dequeue();
                hasEntry = true;
            } else {
                m_queueCondition.wait(&m_queueMutex, 100);
            }
        }
        if (hasEntry) {
            // 处理日志
            QMutexLocker locker(&m_logsMutex);
            m_logs.append(entry);
            while (m_logs.size() > m_maxLogCount) {
                m_logs.removeFirst();
            }
            if (m_consoleOutput) {
                QString msg = formatLogMessage(entry);
                qDebug().noquote() << msg;
            }
            if (m_fileOutput && !m_logFilePath.isEmpty()) {
                writeToFile(entry);
            }
            emitLogAdded(entry);
        }
    }
}

void LogWorkerThread::cleanupOldLogs()
{
    QMutexLocker locker(&m_logsMutex);
    if (m_logs.size() > m_maxLogCount) {
        int removeCount = m_logs.size() - m_maxLogCount;
        for (int i = 0; i < removeCount; ++i) {
            m_logs.removeFirst();
        }
    }
}

void LogWorkerThread::writeToFile(const LogEntry& entry)
{
    QFile file(m_logFilePath);
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
    QString timestamp = entry.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString threadId = QString::number((quintptr)entry.thread);
    QString message = QString("[%1] [%2] [Thread:%3] %4")
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

// ==================== LogManager 实现 ====================

LogManager* LogManager::s_instance = nullptr;

LogManager::LogManager()
    : m_workerThread(nullptr)
    , m_maxLogCount(1000)
    , m_autoCleanup(true)
    , m_autoCleanupInterval(60)
    , m_consoleOutput(true)
    , m_fileOutput(false)
    , m_logFilePath("")
{
    m_workerThread = new LogWorkerThread(this);
    connect(m_workerThread, &LogWorkerThread::logAdded, this, &LogManager::logAdded);
    connect(m_workerThread, &LogWorkerThread::logsCleared, this, &LogManager::logsCleared);
    connect(m_workerThread, &LogWorkerThread::logLevelChanged, this, &LogManager::logLevelChanged);
    m_workerThread->start();
    m_workerThread->setConsoleOutput(m_consoleOutput);
    m_workerThread->setFileOutput(m_fileOutput);
    m_workerThread->setMaxLogCount(m_maxLogCount);
    m_workerThread->setAutoCleanup(m_autoCleanup, m_autoCleanupInterval);
}

LogManager::~LogManager()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(3000);
        delete m_workerThread;
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
        LogEntry entry(level, message, category, fileName, lineNumber, functionName);
        m_workerThread->addLog(entry);
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

void LogManager::requestLogs()
{
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, [this]() {
            QList<LogEntry> logs = m_workerThread->getLogs();
            emit logsReceived(logs);
        }, Qt::QueuedConnection);
    }
}

void LogManager::requestLogsByLevel(LogLevel level)
{
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, [this, level]() {
            QList<LogEntry> logs = m_workerThread->getLogsByLevel(level);
            emit logsByLevelReceived(level, logs);
        }, Qt::QueuedConnection);
    }
}

void LogManager::requestLogsByCategory(const QString& category)
{
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, [this, category]() {
            QList<LogEntry> logs = m_workerThread->getLogsByCategory(category);
            emit logsByCategoryReceived(category, logs);
        }, Qt::QueuedConnection);
    }
}

void LogManager::clearLogs()
{
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_workerThread, &LogWorkerThread::clearLogs, Qt::QueuedConnection);
    }
}

void LogManager::setMaxLogCount(int count)
{
    m_maxLogCount = count;
    if (m_workerThread) {
        m_workerThread->setMaxLogCount(count);
    }
}

int LogManager::getMaxLogCount() const
{
    return m_maxLogCount;
}

void LogManager::setAutoCleanup(bool enabled)
{
    m_autoCleanup = enabled;
    if (m_workerThread) {
        m_workerThread->setAutoCleanup(enabled, m_autoCleanupInterval);
    }
}

bool LogManager::isAutoCleanupEnabled() const
{
    return m_autoCleanup;
}

void LogManager::setAutoCleanupInterval(int minutes)
{
    m_autoCleanupInterval = minutes;
    if (m_workerThread) {
        m_workerThread->setAutoCleanup(m_autoCleanup, minutes);
    }
}

int LogManager::getAutoCleanupInterval() const
{
    return m_autoCleanupInterval;
}

void LogManager::setConsoleOutput(bool enabled)
{
    m_consoleOutput = enabled;
    if (m_workerThread) {
        m_workerThread->setConsoleOutput(enabled);
    }
}

bool LogManager::isConsoleOutputEnabled() const
{
    return m_consoleOutput;
}

void LogManager::setFileOutput(bool enabled)
{
    m_fileOutput = enabled;
    if (m_workerThread) {
        m_workerThread->setFileOutput(enabled);
    }
}

bool LogManager::isFileOutputEnabled() const
{
    return m_fileOutput;
}

void LogManager::setLogFilePath(const QString& path)
{
    m_logFilePath = path;
    if (m_workerThread) {
        m_workerThread->setLogFilePath(path);
    }
}

QString LogManager::getLogFilePath() const
{
    return m_logFilePath;
} 