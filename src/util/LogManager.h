#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QTimer>
#include <QThread>
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

// 日志管理器类
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
    
    // 获取日志
    QList<LogEntry> getLogs() const;
    QList<LogEntry> getLogsByLevel(LogLevel level) const;
    QList<LogEntry> getLogsByCategory(const QString& category) const;
    
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

private:
    LogManager();
    ~LogManager();
    
    void addLog(const LogEntry& entry);
    void cleanupOldLogs();
    void writeToFile(const LogEntry& entry);
    QString formatLogMessage(const LogEntry& entry) const;
    
    static LogManager* s_instance;
    
    QList<LogEntry> m_logs;
    mutable QMutex m_mutex;
    int m_maxLogCount;
    bool m_autoCleanup;
    int m_autoCleanupInterval;
    QTimer* m_cleanupTimer;
    
    // 输出设置
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
#define LOG_DEBUG(msg, cat) LogManager::getInstance()->debug(msg, cat, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg, cat) LogManager::getInstance()->info(msg, cat, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(msg, cat) LogManager::getInstance()->warning(msg, cat, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg, cat) LogManager::getInstance()->error(msg, cat, __FILE__, __LINE__, __FUNCTION__)
#define LOG_SUCCESS(msg, cat) LogManager::getInstance()->success(msg, cat, __FILE__, __LINE__, __FUNCTION__)

// 流式日志宏
#define LOG_DEBUG_STREAM(cat) LogStream(LogLevel::Debug, cat, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO_STREAM(cat) LogStream(LogLevel::Info, cat, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING_STREAM(cat) LogStream(LogLevel::Warning, cat, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR_STREAM(cat) LogStream(LogLevel::Error, cat, __FILE__, __LINE__, __FUNCTION__)
#define LOG_SUCCESS_STREAM(cat) LogStream(LogLevel::Success, cat, __FILE__, __LINE__, __FUNCTION__)

// 简化的流式日志宏（无分类）
#define LOG_DEBUG_S LOG_DEBUG_STREAM("")
#define LOG_INFO_S LOG_INFO_STREAM("")
#define LOG_WARNING_S LOG_WARNING_STREAM("")
#define LOG_ERROR_S LOG_ERROR_STREAM("")
#define LOG_SUCCESS_S LOG_SUCCESS_STREAM("") 