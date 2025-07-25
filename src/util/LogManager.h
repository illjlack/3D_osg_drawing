#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QThread>
#include <list>
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
    
    // 默认构造函数
    LogEntry() 
        : timestamp(QDateTime::currentDateTime())
        , level(LogLevel::Info)
        , message("")
        , category("")
        , fileName("")
        , lineNumber(0)
        , functionName("")
        , thread(QThread::currentThread())
    {}
    
    // 参数构造函数
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
    
    // 拷贝构造函数
    LogEntry(const LogEntry& other)
        : timestamp(other.timestamp)
        , level(other.level)
        , message(other.message)
        , category(other.category)
        , fileName(other.fileName)
        , lineNumber(other.lineNumber)
        , functionName(other.functionName)
        , thread(other.thread)
    {}
    
    // 移动构造函数
    LogEntry(LogEntry&& other) noexcept
        : timestamp(std::move(other.timestamp))
        , level(other.level)
        , message(std::move(other.message))
        , category(std::move(other.category))
        , fileName(std::move(other.fileName))
        , lineNumber(other.lineNumber)
        , functionName(std::move(other.functionName))
        , thread(other.thread)
    {
        // 清空源对象
        other.level = LogLevel::Info;
        other.lineNumber = 0;
        other.thread = nullptr;
    }
    
    // 拷贝赋值运算符
    LogEntry& operator=(const LogEntry& other)
    {
        if (this != &other)
        {
            timestamp = other.timestamp;
            level = other.level;
            message = other.message;
            category = other.category;
            fileName = other.fileName;
            lineNumber = other.lineNumber;
            functionName = other.functionName;
            thread = other.thread;
        }
        return *this;
    }
    
    // 移动赋值运算符
    LogEntry& operator=(LogEntry&& other) noexcept
    {
        if (this != &other)
        {
            timestamp = std::move(other.timestamp);
            level = other.level;
            message = std::move(other.message);
            category = std::move(other.category);
            fileName = std::move(other.fileName);
            lineNumber = other.lineNumber;
            functionName = std::move(other.functionName);
            thread = other.thread;
            
            // 清空源对象
            other.level = LogLevel::Info;
            other.lineNumber = 0;
            other.thread = nullptr;
        }
        return *this;
    }
    
    // 析构函数
    ~LogEntry()
    {
        // 清理资源（QString等会自动清理）
        thread = nullptr;
    }
    
    // 比较运算符
    bool operator==(const LogEntry& other) const
    {
        return timestamp == other.timestamp &&
               level == other.level &&
               message == other.message &&
               category == other.category &&
               fileName == other.fileName &&
               lineNumber == other.lineNumber &&
               functionName == other.functionName &&
               thread == other.thread;
    }
    
    bool operator!=(const LogEntry& other) const
    {
        return !(*this == other);
    }
    
    // 小于比较运算符（用于排序）
    bool operator<(const LogEntry& other) const
    {
        if (timestamp != other.timestamp)
            return timestamp < other.timestamp;
        if (level != other.level)
            return static_cast<int>(level) < static_cast<int>(other.level);
        if (category != other.category)
            return category < other.category;
        return message < other.message;
    }
    
    // 大于比较运算符
    bool operator>(const LogEntry& other) const
    {
        return other < *this;
    }
    
    // 小于等于比较运算符
    bool operator<=(const LogEntry& other) const
    {
        return !(other < *this);
    }
    
    // 大于等于比较运算符
    bool operator>=(const LogEntry& other) const
    {
        return !(*this < other);
    }
};

// 简化的日志配置结构
struct LogConfig
{
    int maxLogCount = 1000;              // 最大日志条数
    bool enableConsoleOutput = true;      // 启用控制台输出
    bool enableFileOutput = true;        // 启用文件输出
    QString logFilePath = "";             // 日志文件路径
    LogLevel minLogLevel = LogLevel::Debug; // 最小日志级别
    bool enableLevelFilter = false;       // 启用日志级别过滤
};

// 日志管理器类（主线程使用，其他线程不应该调用，不处理其他线程竞争问题）
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
    
    // 清空日志
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
    int getPendingLogCount() const;  // 主线程实现中总是返回0
    int getCurrentLogCount() const;
    
    // 获取日志列表
    std::list<LogEntry> getLogs() const;

signals:
    // 日志添加信号
    void logAdded(const LogEntry& entry);
    
    // 日志清空信号
    void logsCleared();

private:
    LogManager();
    ~LogManager();
    
    // 处理日志
    void processLog(const LogEntry& entry);
    
    // 日志格式化和输出
    void writeToFile(const LogEntry& entry);
    QString formatLogMessage(const LogEntry& entry) const;
    bool shouldAcceptLog(const LogEntry& entry) const;
    
    // 确保日志目录存在
    void ensureLogDirectory();

    static LogManager* s_instance;
    LogConfig m_config;
    std::list<LogEntry> m_logs;  // 日志存储
};

// 日志辅助类
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

