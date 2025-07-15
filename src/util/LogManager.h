#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QThread>
#include <QQueue>
#include <sstream>

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

// 简化的日志配置结构
struct LogConfig
{
    int maxLogCount = 1000;              // 最大日志条数
    bool enableConsoleOutput = true;      // 启用控制台输出
    bool enableFileOutput = false;        // 启用文件输出
    QString logFilePath = "";             // 日志文件路径
    LogLevel minLogLevel = LogLevel::Debug; // 最小日志级别
    bool enableLevelFilter = false;       // 启用日志级别过滤
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
    void setMaxLogCount(int count);

    // 获取日志（线程安全）
    QList<LogEntry> getLogs() const;
    QList<LogEntry> getLogsByLevel(LogLevel level) const;
    QList<LogEntry> getLogsByCategory(const QString& category) const;
    
    // 清理日志
    void clearLogs();
    
    // 统计信息
    int getPendingLogCount() const;
    int getCurrentLogCount() const;
    
    // 线程控制
    void requestExit();

signals:
    void logAdded(const LogEntry& entry);
    void logsCleared();

protected:
    void run() override;

private:
    void writeToFile(const LogEntry& entry);
    QString formatLogMessage(const LogEntry& entry) const;
    void emitLogAdded(const LogEntry& entry);
    bool shouldAcceptLog(const LogEntry& entry) const;

    // 日志队列
    QQueue<LogEntry> m_logQueue;
    
    // 日志存储
    QList<LogEntry> m_logs;
    
    // 配置
    LogConfig m_config;
    
    // 控制标志
    bool m_running;
    bool m_shouldExit;
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
    
    // 日志查询
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
    
    // 日志输出设置
    void setConsoleOutput(bool enabled);
    bool isConsoleOutputEnabled() const;
    void setFileOutput(bool enabled);
    bool isFileOutputEnabled() const;
    void setLogFilePath(const QString& path);
    QString getLogFilePath() const;
    
    // 统计信息
    int getPendingLogCount() const;
    int getCurrentLogCount() const;

signals:
    void logAdded(const LogEntry& entry);
    void logsCleared();
    void logsReceived(const QList<LogEntry>& logs);
    void logsByLevelReceived(LogLevel level, const QList<LogEntry>& logs);
    void logsByCategoryReceived(const QString& category, const QList<LogEntry>& logs);

private:
    LogManager();
    ~LogManager();
    
    static LogManager* s_instance;
    LogWorkerThread* m_workerThread;
    LogConfig m_config;
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
    
    ~LogStream()
    {
        QString message = m_stream.str().c_str();
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
#define LOG_DEBUG(msg, category) \
    LogManager::getInstance()->debug(msg, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_INFO(msg, category) \
    LogManager::getInstance()->info(msg, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_WARNING(msg, category) \
    LogManager::getInstance()->warning(msg, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_ERROR(msg, category) \
    LogManager::getInstance()->error(msg, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_SUCCESS(msg, category) \
    LogManager::getInstance()->success(msg, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_STREAM(level, category) \
    LogStream(level, category, __FILE__, __LINE__, __FUNCTION__) 