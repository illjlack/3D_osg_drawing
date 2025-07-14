#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QTimer>
#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QVector>
#include <memory>
#include <sstream>
#include <filesystem>
#include <chrono>

// 日志级别枚举
enum class LogLevel
{
    Debug,      // 调试信息
    Info,       // 一般信息
    Warning,    // 警告信息
    Error,      // 错误信息
    Success     // 成功信息
};

// 日志条目结构
struct LogEntry
{
    QDateTime timestamp;
    LogLevel level;
    QString message;
    QString category;  // 可选分类，如"绘制"、"坐标"等
    QString fileName;  // 文件名
    int lineNumber;    // 行号
    QString functionName; // 函数名
    QThread* thread;   // 线程信息
    
    LogEntry() = default;
    LogEntry(LogLevel l, const QString& msg, const QString& cat = "",
             const QString& file = "", int line = 0, const QString& func = "")
        : timestamp(QDateTime::currentDateTime())
        , level(l)
        , message(msg)
        , category(cat)
        , fileName(file)
        , lineNumber(line)
        , functionName(func)
        , thread(QThread::currentThread())
    {}
};

// 日志配置结构
struct LogConfig
{
    int maxLogCount = 1000;              // 最大日志条数
    int highWaterMark = 800;             // 高水位标记（达到时开始清理）
    int lowWaterMark = 600;              // 低水位标记（清理到此数量）
    int batchCleanupSize = 200;          // 批量清理大小
    int autoCleanupInterval = 30;        // 自动清理间隔（秒）
    bool enableCircularBuffer = true;     // 启用循环缓冲区
    bool enableLevelFilter = false;      // 启用日志级别过滤
    LogLevel minLogLevel = LogLevel::Debug; // 最小日志级别
    bool enablePerformanceMode = false;  // 启用性能模式（减少格式化）
    bool enableAsyncWrite = true;        // 启用异步写入
    int writeBufferSize = 50;            // 写入缓冲区大小
};

// 高性能循环缓冲区
template<typename T>
class CircularBuffer
{
public:
    explicit CircularBuffer(int capacity = 1000)
        : m_capacity(capacity)
        , m_buffer(capacity)
        , m_head(0)
        , m_size(0)
    {}

    void push(const T& item)
    {
        m_buffer[m_head] = item;
        m_head = (m_head + 1) % m_capacity;
        if (m_size < m_capacity) {
            ++m_size;
        }
    }

    void clear()
    {
        m_head = 0;
        m_size = 0;
    }

    int size() const { return m_size; }
    int capacity() const { return m_capacity; }
    bool empty() const { return m_size == 0; }
    bool full() const { return m_size == m_capacity; }

    QList<T> toList() const
    {
        QList<T> result;
        result.reserve(m_size);
        int start = (m_head - m_size + m_capacity) % m_capacity;
        for (int i = 0; i < m_size; ++i) {
            result.append(m_buffer[(start + i) % m_capacity]);
        }
        return result;
    }

    void resize(int newCapacity)
    {
        if (newCapacity == m_capacity) return;
        
        QList<T> currentData = toList();
        m_capacity = newCapacity;
        m_buffer.resize(newCapacity);
        m_head = 0;
        m_size = 0;
        
        int keepSize = qMin(currentData.size(), newCapacity);
        for (int i = currentData.size() - keepSize; i < currentData.size(); ++i) {
            push(currentData[i]);
        }
    }

private:
    int m_capacity;
    QVector<T> m_buffer;
    int m_head;
    int m_size;
};

// 日志工作线程类
class LogWorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit LogWorkerThread(QObject* parent = nullptr);
    ~LogWorkerThread();

    // 添加日志到队列
    void addLog(const LogEntry& entry);
    
    // 配置管理
    void setConfig(const LogConfig& config);
    LogConfig getConfig() const;
    
    // 设置输出选项
    void setConsoleOutput(bool enabled);
    void setFileOutput(bool enabled);
    void setLogFilePath(const QString& path);
    
    // 兼容旧接口
    void setMaxLogCount(int count);
    void setAutoCleanup(bool enabled, int interval = 60);

    // 获取日志（线程安全）
    QList<LogEntry> getLogs() const;
    QList<LogEntry> getLogsByLevel(LogLevel level) const;
    QList<LogEntry> getLogsByCategory(const QString& category) const;
    
    // 清理日志
    void clearLogs();
    
    // 性能统计
    int getPendingLogCount() const;
    int getCurrentLogCount() const;
    double getAverageProcessingTime() const;

signals:
    void logAdded(const LogEntry& entry);
    void logsCleared();
    void logLevelChanged(LogLevel level);
    void performanceWarning(const QString& message);

protected:
    void run() override;

private slots:
    void performCleanup();
    void checkPerformance();

private:
    void writeToFile(const LogEntry& entry);
    void batchWriteToFile(const QList<LogEntry>& entries);
    QString formatLogMessage(const LogEntry& entry) const;
    void emitLogAdded(const LogEntry& entry);
    void intelligentCleanup();
    bool shouldAcceptLog(const LogEntry& entry) const;
    void updatePerformanceStats(double processTime);

    // 日志队列
    QQueue<LogEntry> m_logQueue;
    mutable QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    
    // 日志存储（支持循环缓冲区）
    CircularBuffer<LogEntry> m_circularBuffer;
    QList<LogEntry> m_linearBuffer;
    mutable QMutex m_bufferMutex;
    
    // 写入缓冲区
    QList<LogEntry> m_writeBuffer;
    QMutex m_writeBufferMutex;
    
    // 配置
    LogConfig m_config;
    mutable QMutex m_configMutex;
    
    // 输出设置
    bool m_consoleOutput;
    bool m_fileOutput;
    QString m_logFilePath;
    
    // 控制标志
    bool m_running;
    QTimer* m_cleanupTimer;
    QTimer* m_performanceTimer;
    
    // 性能统计
    QAtomicInt m_totalProcessedLogs;
    QAtomicInt m_droppedLogs;
    std::chrono::steady_clock::time_point m_lastStatsReset;
    double m_averageProcessingTime;
    mutable QMutex m_statsMutex;
};

// 日志管理器类（主线程接口）
class LogManager : public QObject
{
    Q_OBJECT

public:
    static LogManager* getInstance();
    
    // 日志输出接口
    void log(LogLevel level, const QString& message, const QString& category = "",
             const QString& fileName = "", int lineNumber = 0, const QString& functionName = "");
    void debug(const QString& message, const QString& category = "",
               const QString& fileName = "", int lineNumber = 0, const QString& functionName = "");
    void info(const QString& message, const QString& category = "",
              const QString& fileName = "", int lineNumber = 0, const QString& functionName = "");
    void warning(const QString& message, const QString& category = "",
                 const QString& fileName = "", int lineNumber = 0, const QString& functionName = "");
    void error(const QString& message, const QString& category = "",
               const QString& fileName = "", int lineNumber = 0, const QString& functionName = "");
    void success(const QString& message, const QString& category = "",
                 const QString& fileName = "", int lineNumber = 0, const QString& functionName = "");
    
    // 获取日志（异步，通过信号返回）
    void requestLogs();
    void requestLogsByLevel(LogLevel level);
    void requestLogsByCategory(const QString& category);
    
    // 日志管理
    void clearLogs();
    
    // 配置管理
    void setConfig(const LogConfig& config);
    LogConfig getConfig() const;
    
    // 兼容旧接口
    void setMaxLogCount(int count);
    int getMaxLogCount() const;
    void setAutoCleanup(bool enabled);
    bool isAutoCleanupEnabled() const;
    void setAutoCleanupInterval(int minutes);
    int getAutoCleanupInterval() const;
    
    // 日志输出设置
    void setConsoleOutput(bool enabled);
    bool isConsoleOutputEnabled() const;
    void setFileOutput(bool enabled);
    bool isFileOutputEnabled() const;
    void setLogFilePath(const QString& path);
    QString getLogFilePath() const;
    
    // 性能相关
    void enablePerformanceMode(bool enabled);
    bool isPerformanceModeEnabled() const;
    void setCircularBufferEnabled(bool enabled);
    bool isCircularBufferEnabled() const;
    
    // 统计信息
    int getPendingLogCount() const;
    int getCurrentLogCount() const;
    double getAverageProcessingTime() const;
    
    // 预设配置
    void applyHighVolumeConfig();  // 高并发配置
    void applyLowMemoryConfig();   // 低内存配置
    void applyDefaultConfig();     // 默认配置

signals:
    void logAdded(const LogEntry& entry);
    void logsCleared();
    void logLevelChanged(LogLevel level);
    void logsReceived(const QList<LogEntry>& logs);
    void logsByLevelReceived(LogLevel level, const QList<LogEntry>& logs);
    void logsByCategoryReceived(const QString& category, const QList<LogEntry>& logs);
    void performanceWarning(const QString& message);

private:
    LogManager();
    ~LogManager();
    
    static LogManager* s_instance;
    LogWorkerThread* m_workerThread;
    LogConfig m_config;
    mutable QMutex m_configMutex;
};

// 日志辅助类（支持流式输出）
class LogStream
{
public:
    LogStream(LogLevel level, const QString& category = "",
              const QString& fileName = "", int lineNumber = 0, const QString& functionName = "")
        : m_level(level)
        , m_category(category)
        , m_fileName(fileName)
        , m_lineNumber(lineNumber)
        , m_functionName(functionName)
    {}
    
    template<typename T>
    LogStream& operator<<(const T& value)
    {
        m_stream << value;
        return *this;
    }
    
    LogStream& operator<<(const QString& value)
    {
        m_stream << value.toStdString();
        return *this;
    }
    
    ~LogStream()
    {
        QString message = QString::fromStdString(m_stream.str());
        LogManager::getInstance()->log(m_level, message, m_category, m_fileName, m_lineNumber, m_functionName);
    }

private:
    LogLevel m_level;
    QString m_category;
    QString m_fileName;
    int m_lineNumber;
    QString m_functionName;
    std::ostringstream m_stream;
};

// 便捷宏定义
#define LOG_DEBUG(msg, cat) LogManager::getInstance()->debug(msg, cat, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_INFO(msg, cat) LogManager::getInstance()->info(msg, cat, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_WARNING(msg, cat) LogManager::getInstance()->warning(msg, cat, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_ERROR(msg, cat) LogManager::getInstance()->error(msg, cat, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_SUCCESS(msg, cat) LogManager::getInstance()->success(msg, cat, __FILE__, __LINE__, Q_FUNC_INFO)

// 流式日志宏
#define LOG_DEBUG_STREAM(cat) LogStream(LogLevel::Debug, cat, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_INFO_STREAM(cat) LogStream(LogLevel::Info, cat, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_WARNING_STREAM(cat) LogStream(LogLevel::Warning, cat, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_ERROR_STREAM(cat) LogStream(LogLevel::Error, cat, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_SUCCESS_STREAM(cat) LogStream(LogLevel::Success, cat, __FILE__, __LINE__, Q_FUNC_INFO)

// 简化的流式日志宏（无分类）
#define LOG_DEBUG_S LOG_DEBUG_STREAM("")
#define LOG_INFO_S LOG_INFO_STREAM("")
#define LOG_WARNING_S LOG_WARNING_STREAM("")
#define LOG_ERROR_S LOG_ERROR_STREAM("")
#define LOG_SUCCESS_S LOG_SUCCESS_STREAM("") 