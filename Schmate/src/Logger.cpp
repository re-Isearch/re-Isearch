
#include "Logger.hpp"
#include <ctime>
#include <unistd.h>

#ifdef __APPLE__
#include <os/log.h>
#else
#include <syslog.h>
#endif

Logger::Logger() {
    // Check if stdout is a TTY (terminal)
    log_to_console = isatty(fileno(stdout));
}

Logger::~Logger() {
    close_file();
#ifndef __APPLE__
    if (log_to_syslog) {
        closelog();
    }
#endif
}

void Logger::enable_syslog(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    
#ifdef __APPLE__
    // macOS uses os_log, no need to open/close
    log_to_syslog = enabled;
#else
    if (enabled && !log_to_syslog) {
        openlog("sbert_search", LOG_PID | LOG_CONS, LOG_USER);
        log_to_syslog = true;
    } else if (!enabled && log_to_syslog) {
        closelog();
        log_to_syslog = false;
    }
#endif
}

void Logger::enable_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (log_to_file) {
        file_stream.close();
    }
    
    file_stream.open(filename, std::ios::app);
    if (file_stream.is_open()) {
        log_to_file = true;
    } else {
        std::cerr << "Failed to open log file: " << filename << std::endl;
        log_to_file = false;
    }
}

void Logger::close_file() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (log_to_file && file_stream.is_open()) {
        file_stream.close();
        log_to_file = false;
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < min_level || message.empty()) return;

    // Remove trailing \n from the string.
    std::string msg (message);
    if (msg.back() == '\n') msg.pop_back();

    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string timestamp = format_timestamp();
    std::string level_str = level_to_string(level);
    std::string formatted = "[" + timestamp + "] [" + level_str + "] " + msg;
    
    // Log to console
    if (log_to_console) {
        if (level >= LogLevel::ERROR) {
            std::cerr << formatted << std::endl;
        } else {
            std::cout << formatted << std::endl;
        }
    }
    
    // Log to file
    if (log_to_file && file_stream.is_open()) {
        file_stream << formatted << std::endl;
        file_stream.flush();
    }
    
    // Log to syslog/os_log
    if (log_to_syslog) {
#ifdef __APPLE__
        os_log_type_t os_level;
        switch (level) {
            case LogLevel::DEBUG: os_level = OS_LOG_TYPE_DEBUG; break;
            case LogLevel::INFO:  os_level = OS_LOG_TYPE_INFO; break;
            case LogLevel::WARN:  os_level = OS_LOG_TYPE_DEFAULT; break;
            case LogLevel::ERROR: os_level = OS_LOG_TYPE_ERROR; break;
            case LogLevel::FATAL:
	    case LogLevel::PANIC:
		os_level = OS_LOG_TYPE_FAULT; break;
        }
        os_log_with_type(OS_LOG_DEFAULT, os_level, "%{public}s", msg.c_str());
#else
        int priority = level_to_syslog(level);
        syslog(priority, "%s", msg.c_str());
#endif
    }
}

std::string Logger::format_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_buf;
    localtime_r(&time, &tm_buf);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
	case LogLevel::PANIC: return "PANIC";
        default: return "UNKNOWN";
    }
}

int Logger::level_to_syslog(LogLevel level) {
#ifndef __APPLE__
    switch (level) {
        case LogLevel::DEBUG: return LOG_DEBUG;
        case LogLevel::INFO:  return LOG_INFO;
        case LogLevel::WARN:  return LOG_WARNING;
        case LogLevel::ERROR: return LOG_ERR;
        case LogLevel::FATAL:
	case LogLEvel::PANIC: return LOG_CRIT;
        default: return LOG_INFO;
    }
#else
    return 0; // Not used on macOS
#endif
}
