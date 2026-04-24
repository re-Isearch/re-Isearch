#include "Logger.hpp"
#include <ctime>
#include <unistd.h>

#ifdef __APPLE__
#include <os/log.h>
#else
#include <syslog.h>
#endif

Logger::Logger(std::string_view prefix) : prefix_(prefix) {
    // Check if stdout is a TTY (terminal)
    log_to_console = isatty(fileno(stdout));
}

Logger::~Logger() {
    flush_repeated();
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
    log_to_syslog = enabled;
#else
    if (enabled && !log_to_syslog) {
        openlog(prefix_, LOG_PID | LOG_CONS, LOG_USER);
        log_to_syslog = true;
    } else if (!enabled && log_to_syslog) {
        closelog();
        log_to_syslog = false;
    }
#endif
}

void Logger::enable_file(const std::string_view filename) {
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

// Must be called with mutex_ already held
void Logger::flush_repeated() {
    if (repeat_count_ > 1) {
        std::string repeat_msg = "**** last message repeated " + 
                                 std::to_string(repeat_count_ - 1) + " times";
        emit_raw(last_level_, repeat_msg);
    }
    repeat_count_ = 0;
    last_message_.clear();
}

// Emit a formatted message to all outputs — no dedup logic, no locking
void Logger::emit_raw(LogLevel level, const std::string_view formatted) {
    std::string      timestamp = format_timestamp();
    std::string_view level_str = level_to_string(level);

    std::string out;
    out.reserve(32 + timestamp.size() + level_str.size() + formatted.size());
    if (! log_to_console) {
       out = " ["; out += timestamp; out += "]";
    }
    out += " ["; out += level_str; out += "]: "; out += formatted;

    if (log_to_console) {
        if (level >= LogLevel::ERROR)
            std::cerr << prefix_ << out << std::endl;
        else
            std::cout << prefix_ << out << std::endl;
    }

    if (log_to_file && file_stream.is_open()) {
        file_stream << out << std::endl;
        file_stream.flush();
    }

    if (log_to_syslog) {
       // Safe — construct a temporary string only when needed for the C API
       std::string str(formatted); // Null terminated!
#ifdef __APPLE__
        os_log_type_t os_level;
        switch (level) {
            case LogLevel::DEBUG: os_level = OS_LOG_TYPE_DEBUG;   break;
            case LogLevel::INFO:  os_level = OS_LOG_TYPE_INFO;    break;
            case LogLevel::WARN:  os_level = OS_LOG_TYPE_DEFAULT; break;
            case LogLevel::ERROR: os_level = OS_LOG_TYPE_ERROR;   break;
            case LogLevel::FATAL:
            case LogLevel::PANIC: os_level = OS_LOG_TYPE_FAULT;   break;
        }
        os_log_with_type(OS_LOG_DEFAULT, os_level, "%{public}s", str.c_str());
#else
        int priority = level_to_syslog(level);
        syslog(priority, "%s", str.c_str());
#endif
    }
}

void Logger::log(LogLevel level, const std::string_view message) {
    if (level < min_level || message.empty()) return;

    std::string msg(message);
    if (msg.back() == '\n') msg.pop_back();

    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    if (msg == last_message_ && level == last_level_) {
      if (now - last_message_time_ > repeat_timeout_) {
        flush_repeated();
        emit_raw(level, msg);
        last_message_time_ = now;
        repeat_count_ = 1;
      } else {
        // Same message — just count it
        repeat_count_++;
      }
      if (repeat_count_ < 100) return;
    }
    last_message_time_ = now;

    // Different message — flush any pending repeat count first
    flush_repeated();

    // Emit the new message and record it
    emit_raw(level, msg);
    last_message_ = msg;
    last_level_   = level;
    repeat_count_ = 1;
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

std::string_view Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "Debug";
        case LogLevel::INFO:  return "Info";
        case LogLevel::WARN:  return "Warn";
        case LogLevel::ERROR: return "Error";
        case LogLevel::FATAL: return "Fatal";
        case LogLevel::PANIC: return "PANIC";
        default:              return "UNKNOWN";
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
        case LogLevel::PANIC: return LOG_CRIT;
        default:              return LOG_INFO;
    }
#else
    return 0;
#endif
}
