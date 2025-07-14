#include "LogManager.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QMetaObject>
#include <QTimer>
#include <QFileInfo>
#include <QElapsedTimer>
#include <algorithm>

// ==================== LogWorkerThread 实现 ====================

LogWorkerThread::LogWorkerThread(QObject* parent)
    : QThread(parent)
    , m_circularBuffer(1000)
    , m_consoleOutput(true)
    , m_fileOutput(false)
    , m_logFilePath("")
    , m_running(false)
    , m_cleanupTimer(nullptr)
    , m_performanceTimer(nullptr)
    , m_totalProcessedLogs(0)
    , m_droppedLogs(0)
    , m_averageProcessingTime(0.0)
{
    // 设置默认配置
    m_config.maxLogCount = 1000;
    m_config.highWaterMark = 800;
    m_config.lowWaterMark = 600;
    m_config.batchCleanupSize = 200;
    m_config.autoCleanupInterval = 30;
    m_config.enableCircularBuffer = true;
    m_config.enableLevelFilter = false;
    m_config.minLogLevel = LogLevel::Debug;
    m_config.enablePerformanceMode = false;
    m_config.enableAsyncWrite = true;
    m_config.writeBufferSize = 50;
    
    // 初始化日志目录
    QString appDir = QCoreApplication::applicationDirPath();
    m_logFilePath = appDir + "/logs/app.log";
    QDir logDir(QFileInfo(m_logFilePath).absolutePath());
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    
    // 初始化缓冲区
    m_circularBuffer.resize(m_config.maxLogCount);
    m_writeBuffer.reserve(m_config.writeBufferSize);
    
    // 重置性能统计
    m_lastStatsReset = std::chrono::steady_clock::now();
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
    delete m_performanceTimer;
}

void LogWorkerThread::addLog(const LogEntry& entry)
{
    // 快速过滤
    if (!shouldAcceptLog(entry)) {
        m_droppedLogs.fetchAndAddRelaxed(1);
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);
    
    // 如果队列过满，丢弃旧的日志
    if (m_logQueue.size() > m_config.maxLogCount * 2) {
        while (m_logQueue.size() > m_config.maxLogCount) {
            m_logQueue.dequeue();
            m_droppedLogs.fetchAndAddRelaxed(1);
        }
    }
    
    m_logQueue.enqueue(entry);
    m_queueCondition.wakeOne();
}

void LogWorkerThread::setConfig(const LogConfig& config)
{
    QMutexLocker locker(&m_configMutex);
    m_config = config;
    
    // 更新缓冲区大小
    if (m_config.enableCircularBuffer) {
        m_circularBuffer.resize(m_config.maxLogCount);
    } else {
        m_linearBuffer.reserve(m_config.maxLogCount);
    }
    
    // 更新写入缓冲区
    m_writeBuffer.reserve(m_config.writeBufferSize);
    
    // 重置定时器
    if (m_cleanupTimer) {
        m_cleanupTimer->start(m_config.autoCleanupInterval * 1000);
    }
}

LogConfig LogWorkerThread::getConfig() const
{
    QMutexLocker locker(&m_configMutex);
    return m_config;
}

void LogWorkerThread::setConsoleOutput(bool enabled)
{
    QMutexLocker locker(&m_configMutex);
    m_consoleOutput = enabled;
}

void LogWorkerThread::setFileOutput(bool enabled)
{
    QMutexLocker locker(&m_configMutex);
    m_fileOutput = enabled;
}

void LogWorkerThread::setLogFilePath(const QString& path)
{
    QMutexLocker locker(&m_configMutex);
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
    QMutexLocker locker(&m_configMutex);
    m_config.maxLogCount = count;
    m_config.highWaterMark = static_cast<int>(count * 0.8);
    m_config.lowWaterMark = static_cast<int>(count * 0.6);
    
    if (m_config.enableCircularBuffer) {
        m_circularBuffer.resize(count);
    }
}

void LogWorkerThread::setAutoCleanup(bool enabled, int interval)
{
    QMutexLocker locker(&m_configMutex);
    m_config.autoCleanupInterval = interval;
    
    if (m_cleanupTimer) {
        if (enabled) {
            m_cleanupTimer->start(interval * 1000);
        } else {
            m_cleanupTimer->stop();
        }
    }
}

QList<LogEntry> LogWorkerThread::getLogs() const
{
    QMutexLocker locker(&m_bufferMutex);
    if (m_config.enableCircularBuffer) {
        return m_circularBuffer.toList();
    } else {
        return m_linearBuffer;
    }
}

QList<LogEntry> LogWorkerThread::getLogsByLevel(LogLevel level) const
{
    QMutexLocker locker(&m_bufferMutex);
    QList<LogEntry> result;
    
    QList<LogEntry> allLogs = m_config.enableCircularBuffer ? 
        m_circularBuffer.toList() : m_linearBuffer;
    
    for (const auto& entry : allLogs) {
        if (entry.level == level) {
            result.append(entry);
        }
    }
    return result;
}

QList<LogEntry> LogWorkerThread::getLogsByCategory(const QString& category) const
{
    QMutexLocker locker(&m_bufferMutex);
    QList<LogEntry> result;
    
    QList<LogEntry> allLogs = m_config.enableCircularBuffer ? 
        m_circularBuffer.toList() : m_linearBuffer;
    
    for (const auto& entry : allLogs) {
        if (entry.category == category) {
            result.append(entry);
        }
    }
    return result;
}

void LogWorkerThread::clearLogs()
{
    QMutexLocker locker(&m_bufferMutex);
    if (m_config.enableCircularBuffer) {
        m_circularBuffer.clear();
    } else {
        m_linearBuffer.clear();
    }
    
    // 清理写入缓冲区
    QMutexLocker writeLocker(&m_writeBufferMutex);
    m_writeBuffer.clear();
    
    emit logsCleared();
}

int LogWorkerThread::getPendingLogCount() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_logQueue.size();
}

int LogWorkerThread::getCurrentLogCount() const
{
    QMutexLocker locker(&m_bufferMutex);
    return m_config.enableCircularBuffer ? 
        m_circularBuffer.size() : m_linearBuffer.size();
}

double LogWorkerThread::getAverageProcessingTime() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_averageProcessingTime;
}

void LogWorkerThread::run()
{
    m_running = true;
    m_totalProcessedLogs = 0;
    m_droppedLogs = 0;
    
    // 初始化定时器
    m_cleanupTimer = new QTimer();
    m_performanceTimer = new QTimer();
    
    m_cleanupTimer->moveToThread(this);
    m_performanceTimer->moveToThread(this);
    
    connect(m_cleanupTimer, &QTimer::timeout, this, &LogWorkerThread::performCleanup);
    connect(m_performanceTimer, &QTimer::timeout, this, &LogWorkerThread::checkPerformance);
    
    // 启动定时器
    m_cleanupTimer->start(m_config.autoCleanupInterval * 1000);
    m_performanceTimer->start(10000); // 10秒检查一次性能
    
    while (m_running) {
        QElapsedTimer timer;
        timer.start();
        
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
            {
                QMutexLocker locker(&m_bufferMutex);
                if (m_config.enableCircularBuffer) {
                    m_circularBuffer.push(entry);
                } else {
                    m_linearBuffer.append(entry);
                    
                    // 检查是否需要清理
                    if (m_linearBuffer.size() > m_config.highWaterMark) {
                        intelligentCleanup();
                    }
                }
            }
            
            // 控制台输出
            if (m_consoleOutput) {
                QString msg = formatLogMessage(entry);
                qDebug().noquote() << msg;
            }
            
            // 文件输出
            if (m_fileOutput && !m_logFilePath.isEmpty()) {
                if (m_config.enableAsyncWrite) {
                    // 异步批量写入
                    QMutexLocker writeLocker(&m_writeBufferMutex);
                    m_writeBuffer.append(entry);
                    
                    if (m_writeBuffer.size() >= m_config.writeBufferSize) {
                        QList<LogEntry> batch = m_writeBuffer;
                        m_writeBuffer.clear();
                        writeLocker.unlock();
                        
                        batchWriteToFile(batch);
                    }
                } else {
                    // 同步写入
                    writeToFile(entry);
                }
            }
            
            // 发送信号
            emitLogAdded(entry);
            
            // 更新统计
            double processTime = timer.elapsed();
            updatePerformanceStats(processTime);
            m_totalProcessedLogs.fetchAndAddRelaxed(1);
        }
        
        // 批量处理多个日志以提高效率
        if (hasEntry && m_logQueue.size() > 10) {
            QList<LogEntry> batch;
            {
                QMutexLocker locker(&m_queueMutex);
                int batchSize = qMin(20, m_logQueue.size());
                for (int i = 0; i < batchSize && !m_logQueue.isEmpty(); ++i) {
                    batch.append(m_logQueue.dequeue());
                }
            }
            
            // 批量处理
            for (const auto& batchEntry : batch) {
                {
                    QMutexLocker locker(&m_bufferMutex);
                    if (m_config.enableCircularBuffer) {
                        m_circularBuffer.push(batchEntry);
                    } else {
                        m_linearBuffer.append(batchEntry);
                    }
                }
                
                if (m_consoleOutput) {
                    QString msg = formatLogMessage(batchEntry);
                    qDebug().noquote() << msg;
                }
                
                emitLogAdded(batchEntry);
                m_totalProcessedLogs.fetchAndAddRelaxed(1);
            }
            
            // 批量写入文件
            if (m_fileOutput && !m_logFilePath.isEmpty() && m_config.enableAsyncWrite) {
                QMutexLocker writeLocker(&m_writeBufferMutex);
                m_writeBuffer.append(batch);
                
                if (m_writeBuffer.size() >= m_config.writeBufferSize) {
                    QList<LogEntry> writeBatch = m_writeBuffer;
                    m_writeBuffer.clear();
                    writeLocker.unlock();
                    
                    batchWriteToFile(writeBatch);
                }
            }
        }
    }
    
    // 清理剩余的写入缓冲区
    if (m_fileOutput && !m_logFilePath.isEmpty() && m_config.enableAsyncWrite) {
        QMutexLocker writeLocker(&m_writeBufferMutex);
        if (!m_writeBuffer.isEmpty()) {
            batchWriteToFile(m_writeBuffer);
            m_writeBuffer.clear();
        }
    }
    
    delete m_cleanupTimer;
    delete m_performanceTimer;
    m_cleanupTimer = nullptr;
    m_performanceTimer = nullptr;
}

void LogWorkerThread::performCleanup()
{
    if (!m_config.enableCircularBuffer) {
        QMutexLocker locker(&m_bufferMutex);
        intelligentCleanup();
    }
}

void LogWorkerThread::checkPerformance()
{
    QMutexLocker locker(&m_statsMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastStatsReset);
    
    if (duration.count() >= 60) { // 每分钟检查一次
        int processed = m_totalProcessedLogs.loadRelaxed();
        int dropped = m_droppedLogs.loadRelaxed();
        
        if (dropped > 0) {
            double dropRate = static_cast<double>(dropped) / (processed + dropped) * 100;
            if (dropRate > 5.0) { // 丢弃率超过5%
                emit performanceWarning(QString("日志丢弃率过高: %1% (%2/%3)")
                    .arg(dropRate, 0, 'f', 1).arg(dropped).arg(processed + dropped));
            }
        }
        
        if (m_averageProcessingTime > 10.0) { // 平均处理时间超过10ms
            emit performanceWarning(QString("日志处理时间过长: %1ms")
                .arg(m_averageProcessingTime, 0, 'f', 2));
        }
        
        // 重置统计
        m_totalProcessedLogs = 0;
        m_droppedLogs = 0;
        m_averageProcessingTime = 0.0;
        m_lastStatsReset = now;
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

void LogWorkerThread::batchWriteToFile(const QList<LogEntry>& entries)
{
    if (entries.isEmpty()) return;
    
    QFile file(m_logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        
        for (const auto& entry : entries) {
            stream << formatLogMessage(entry) << "\n";
        }
        
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
    
    if (m_config.enablePerformanceMode) {
        // 性能模式：简化格式
        return QString("[%1] %2: %3")
            .arg(levelStr)
            .arg(entry.category.isEmpty() ? "App" : entry.category)
            .arg(entry.message);
    } else {
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
}

void LogWorkerThread::emitLogAdded(const LogEntry& entry)
{
    QMetaObject::invokeMethod(this, [this, entry]() {
        emit logAdded(entry);
    }, Qt::QueuedConnection);
}

void LogWorkerThread::intelligentCleanup()
{
    // 只在线性缓冲区模式下使用
    if (m_config.enableCircularBuffer) return;
    
    if (m_linearBuffer.size() > m_config.highWaterMark) {
        // 计算要删除的数量
        int targetSize = m_config.lowWaterMark;
        int removeCount = m_linearBuffer.size() - targetSize;
        
        if (removeCount > 0) {
            // 优先删除Debug级别的日志
            auto it = m_linearBuffer.begin();
            int removed = 0;
            
            while (it != m_linearBuffer.end() && removed < removeCount) {
                if (it->level == LogLevel::Debug) {
                    it = m_linearBuffer.erase(it);
                    removed++;
                } else {
                    ++it;
                }
            }
            
            // 如果还需要删除，按时间删除最旧的
            if (removed < removeCount) {
                int remaining = removeCount - removed;
                m_linearBuffer.erase(m_linearBuffer.begin(), m_linearBuffer.begin() + remaining);
            }
        }
    }
}

bool LogWorkerThread::shouldAcceptLog(const LogEntry& entry) const
{
    QMutexLocker locker(&m_configMutex);
    
    // 级别过滤
    if (m_config.enableLevelFilter) {
        if (entry.level < m_config.minLogLevel) {
            return false;
        }
    }
    
    return true;
}

void LogWorkerThread::updatePerformanceStats(double processTime)
{
    QMutexLocker locker(&m_statsMutex);
    
    // 计算移动平均值
    static const double alpha = 0.1; // 平滑系数
    if (m_averageProcessingTime == 0.0) {
        m_averageProcessingTime = processTime;
    } else {
        m_averageProcessingTime = alpha * processTime + (1 - alpha) * m_averageProcessingTime;
    }
}

// ==================== LogManager 实现 ====================

LogManager* LogManager::s_instance = nullptr;

LogManager::LogManager()
    : m_workerThread(nullptr)
{
    // 初始化默认配置
    m_config.maxLogCount = 1000;
    m_config.highWaterMark = 800;
    m_config.lowWaterMark = 600;
    m_config.batchCleanupSize = 200;
    m_config.autoCleanupInterval = 30;
    m_config.enableCircularBuffer = true;
    m_config.enableLevelFilter = false;
    m_config.minLogLevel = LogLevel::Debug;
    m_config.enablePerformanceMode = false;
    m_config.enableAsyncWrite = true;
    m_config.writeBufferSize = 50;
    
    m_workerThread = new LogWorkerThread(this);
    connect(m_workerThread, &LogWorkerThread::logAdded, this, &LogManager::logAdded);
    connect(m_workerThread, &LogWorkerThread::logsCleared, this, &LogManager::logsCleared);
    connect(m_workerThread, &LogWorkerThread::logLevelChanged, this, &LogManager::logLevelChanged);
    connect(m_workerThread, &LogWorkerThread::performanceWarning, this, &LogManager::performanceWarning);
    
    m_workerThread->setConfig(m_config);
    m_workerThread->start();
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

void LogManager::setConfig(const LogConfig& config)
{
    QMutexLocker locker(&m_configMutex);
    m_config = config;
    if (m_workerThread) {
        m_workerThread->setConfig(config);
    }
}

LogConfig LogManager::getConfig() const
{
    QMutexLocker locker(&m_configMutex);
    return m_config;
}

void LogManager::setMaxLogCount(int count)
{
    QMutexLocker locker(&m_configMutex);
    m_config.maxLogCount = count;
    m_config.highWaterMark = static_cast<int>(count * 0.8);
    m_config.lowWaterMark = static_cast<int>(count * 0.6);
    
    if (m_workerThread) {
        m_workerThread->setMaxLogCount(count);
    }
}

int LogManager::getMaxLogCount() const
{
    QMutexLocker locker(&m_configMutex);
    return m_config.maxLogCount;
}

void LogManager::setAutoCleanup(bool enabled)
{
    QMutexLocker locker(&m_configMutex);
    if (m_workerThread) {
        m_workerThread->setAutoCleanup(enabled, m_config.autoCleanupInterval);
    }
}

bool LogManager::isAutoCleanupEnabled() const
{
    QMutexLocker locker(&m_configMutex);
    return m_config.autoCleanupInterval > 0;
}

void LogManager::setAutoCleanupInterval(int minutes)
{
    QMutexLocker locker(&m_configMutex);
    m_config.autoCleanupInterval = minutes;
    if (m_workerThread) {
        m_workerThread->setAutoCleanup(true, minutes);
    }
}

int LogManager::getAutoCleanupInterval() const
{
    QMutexLocker locker(&m_configMutex);
    return m_config.autoCleanupInterval;
}

void LogManager::setConsoleOutput(bool enabled)
{
    if (m_workerThread) {
        m_workerThread->setConsoleOutput(enabled);
    }
}

bool LogManager::isConsoleOutputEnabled() const
{
    return m_workerThread ? true : false; // 简化实现
}

void LogManager::setFileOutput(bool enabled)
{
    if (m_workerThread) {
        m_workerThread->setFileOutput(enabled);
    }
}

bool LogManager::isFileOutputEnabled() const
{
    return m_workerThread ? true : false; // 简化实现
}

void LogManager::setLogFilePath(const QString& path)
{
    if (m_workerThread) {
        m_workerThread->setLogFilePath(path);
    }
}

QString LogManager::getLogFilePath() const
{
    return m_workerThread ? m_workerThread->getConfig().writeBufferSize > 0 ? "enabled" : "disabled" : "";
}

void LogManager::enablePerformanceMode(bool enabled)
{
    QMutexLocker locker(&m_configMutex);
    m_config.enablePerformanceMode = enabled;
    if (m_workerThread) {
        m_workerThread->setConfig(m_config);
    }
}

bool LogManager::isPerformanceModeEnabled() const
{
    QMutexLocker locker(&m_configMutex);
    return m_config.enablePerformanceMode;
}

void LogManager::setCircularBufferEnabled(bool enabled)
{
    QMutexLocker locker(&m_configMutex);
    m_config.enableCircularBuffer = enabled;
    if (m_workerThread) {
        m_workerThread->setConfig(m_config);
    }
}

bool LogManager::isCircularBufferEnabled() const
{
    QMutexLocker locker(&m_configMutex);
    return m_config.enableCircularBuffer;
}

int LogManager::getPendingLogCount() const
{
    return m_workerThread ? m_workerThread->getPendingLogCount() : 0;
}

int LogManager::getCurrentLogCount() const
{
    return m_workerThread ? m_workerThread->getCurrentLogCount() : 0;
}

double LogManager::getAverageProcessingTime() const
{
    return m_workerThread ? m_workerThread->getAverageProcessingTime() : 0.0;
}

void LogManager::applyHighVolumeConfig()
{
    LogConfig config;
    config.maxLogCount = 5000;
    config.highWaterMark = 4000;
    config.lowWaterMark = 3000;
    config.batchCleanupSize = 1000;
    config.autoCleanupInterval = 15;
    config.enableCircularBuffer = true;
    config.enableLevelFilter = true;
    config.minLogLevel = LogLevel::Info;
    config.enablePerformanceMode = true;
    config.enableAsyncWrite = true;
    config.writeBufferSize = 100;
    
    setConfig(config);
}

void LogManager::applyLowMemoryConfig()
{
    LogConfig config;
    config.maxLogCount = 200;
    config.highWaterMark = 150;
    config.lowWaterMark = 100;
    config.batchCleanupSize = 50;
    config.autoCleanupInterval = 60;
    config.enableCircularBuffer = true;
    config.enableLevelFilter = true;
    config.minLogLevel = LogLevel::Warning;
    config.enablePerformanceMode = true;
    config.enableAsyncWrite = false;
    config.writeBufferSize = 10;
    
    setConfig(config);
}

void LogManager::applyDefaultConfig()
{
    LogConfig config;
    config.maxLogCount = 1000;
    config.highWaterMark = 800;
    config.lowWaterMark = 600;
    config.batchCleanupSize = 200;
    config.autoCleanupInterval = 30;
    config.enableCircularBuffer = true;
    config.enableLevelFilter = false;
    config.minLogLevel = LogLevel::Debug;
    config.enablePerformanceMode = false;
    config.enableAsyncWrite = true;
    config.writeBufferSize = 50;
    
    setConfig(config);
} 