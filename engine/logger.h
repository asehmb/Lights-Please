#pragma once
#include <string>
#include <format>
#include <iostream>
#include <mutex>

enum class LogLevel { Info, Warning, Error };

namespace logger {
    // Internal mutex to prevent interleaving
    static std::mutex log_mutex;

    template <typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args) {
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        
        const char* prefix = "[INFO]";
        if (level == LogLevel::Warning) prefix = "[WARN]";
        if (level == LogLevel::Error)   prefix = "[ERR ]";

        // Lock only during the actual output
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << prefix << " " << message << std::endl;
    }
}

#define LOG_INFO(fmt, ...)  logger::log(LogLevel::Info, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  logger::log(LogLevel::Warning, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)   logger::log(LogLevel::Error, fmt, ##__VA_ARGS__)