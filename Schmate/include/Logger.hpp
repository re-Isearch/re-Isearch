#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4,
    PANIC = 5
};

class Logger {
public:
    // Singleton access
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    // Configuration
    void set_level(LogLevel level) { min_level = level; }
    void enable_console(bool enabled) { log_to_console = enabled; }
    void enable_syslog(bool enabled);
    void enable_file(const std::string& filename);
    void close_file();

    // Logging methods
    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg);}
    void info(const std::string& msg)  { log(LogLevel::INFO, msg); }
    void warn(const std::string& msg)  { log(LogLevel::WARN, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, msg);}
    void fatal(const std::string& msg) { log(LogLevel::FATAL, msg);}
    void panic(const std::string& msg) { log(LogLevel::PANIC, msg);}

    // Stream-style logging
    class LogStream {
    public:
        LogStream(Logger& logger, LogLevel level) 
            : logger_(logger), level_(level) {}
        
        ~LogStream() {
            if (!buffer_.str().empty()) {
                logger_.log(level_, buffer_.str());
            }
        }

        template<typename T>
        LogStream& operator<<(const T& value) {
            buffer_ << value;
            return *this;
        }

    private:
        Logger& logger_;
        LogLevel level_;
        std::ostringstream buffer_;
    };

    LogStream debug() { return LogStream(*this, LogLevel::DEBUG); }
    LogStream info() { return LogStream(*this, LogLevel::INFO); }
    LogStream warn() { return LogStream(*this, LogLevel::WARN); }
    LogStream error() { return LogStream(*this, LogLevel::ERROR); }
    LogStream fatal() { return LogStream(*this, LogLevel::FATAL); }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(LogLevel level, const std::string& msg);
    std::string format_timestamp();
    std::string level_to_string(LogLevel level);
    int level_to_syslog(LogLevel level);

    LogLevel min_level = LogLevel::INFO;
    bool log_to_console = true;
    bool log_to_syslog = false;
    bool log_to_file = false;
    std::ofstream file_stream;
    std::mutex mutex_;
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::instance().debug(msg)
#define LOG_INFO(msg) Logger::instance().info(msg)
#define LOG_WARN(msg) Logger::instance().warn(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)
#define LOG_FATAL(msg) Logger::instance().fatal(msg)

// Stream-style macros
#define LOG_DEBUG_S() Logger::instance().debug()
#define LOG_INFO_S() Logger::instance().info()
#define LOG_WARN_S() Logger::instance().warn()
#define LOG_ERROR_S() Logger::instance().error()
#define LOG_FATAL_S() Logger::instance().fatal()

// Map messages from HNSWLIB
#ifndef HNSWLIB_ERR_OVERRIDE
  #define HNSWLIB_ERR_OVERRIDE LOG_ERROR_S()
#endif
#ifndef HNSWLIB_WARN_OVERRIDE
  #define HNSWLIB_WARN_OVERRIDE LOG_WARN_S()
#endif
#ifndef HNSWLIB_INFO_OVERRIDE
  #define HNSWLIB_INFO_OVERRIDE LOG_INFO_S()
#endif
#ifndef HNSWLIB_DEBUG_OVERRIDE
  #define HNSWLIB_DEBUG_OVERRIDE LOG_DEBUG_S()
#endif
#ifndef HNSWLIB_FATAL_OVERRIDE
  #define HNSWLIB_FATAL_OVERRIDE LOG_FATAL_S()
#endif



