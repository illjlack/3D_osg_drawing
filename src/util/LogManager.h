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
#include <memory>
#include <sstream>
#include <filesystem>

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

// 日志工作线程类
class LogWorkerThread : public QThread
{
    Q_OBJECT

public:
    explicit LogWorkerThread(QObject* parent = nullptr);
    ~LogWorkerThread();

    // 添加日志到队列
    void addLog(const LogEntry& entry);
    
    // 设置输出选项
    void setConsoleOutput(bool enabled);
    void setFileOutput(bool enabled);
    void setLogFilePath(const QString& path);
    void setMaxLogCount(int count);
    void setAutoCleanup(bool enabled, int interval = 60);

    // 获取日志（线程安全）
    QList<LogEntry> getLogs() const;
    QList<LogEntry> getLogsByLevel(LogLevel level) const;
    QList<LogEntry> getLogsByCategory(const QString& category) const;
    
    // 清理日志
    void clearLogs();

signals:
    void logAdded(const LogEntry& entry);
    void logsCleared();
    void logLevelChanged(LogLevel level);

protected:
    void run() override;

private slots:
    void cleanupOldLogs();

private:
    void writeToFile(const LogEntry& entry);
    QString formatLogMessage(const LogEntry& entry) const;
    void emitLogAdded(const LogEntry& entry);

    // 日志队列
    QQueue<LogEntry> m_logQueue;
    mutable QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    
    // 日志存储
    QList<LogEntry> m_logs;
    mutable QMutex m_logsMutex;
    
    // 配置
    int m_maxLogCount;
    bool m_autoCleanup;
    int m_autoCleanupInterval;
    bool m_consoleOutput;
    bool m_fileOutput;
    QString m_logFilePath;
    
    // 控制标志
    bool m_running;
    QTimer* m_cleanupTimer;
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
    void setMaxLogCount(int count);
    int getMaxLogCount() const;
    
    // 自动清理设置
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

signals:
    void logAdded(const LogEntry& entry);
    void logsCleared();
    void logLevelChanged(LogLevel level);
    void logsReceived(const QList<LogEntry>& logs);
    void logsByLevelReceived(LogLevel level, const QList<LogEntry>& logs);
    void logsByCategoryReceived(const QString& category, const QList<LogEntry>& logs);

private:
    LogManager();
    ~LogManager();
    
    static LogManager* s_instance;
    LogWorkerThread* m_workerThread;
    
    // 配置缓存（避免频繁跨线程访问）
    int m_maxLogCount;
    bool m_autoCleanup;
    int m_autoCleanupInterval;
    bool m_consoleOutput;
    bool m_fileOutput;
    QString m_logFilePath;
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