#pragma once
#include "config.h"
#include <string>
#include <format>
#include <iostream>
#include <mutex>
#include <type_traits>
#include <utility>

enum class LogLevel { Info, Warning, Error };

namespace logger {
    // Internal mutex to prevent interleaving
    static std::mutex log_mutex;

    template <typename... Args>
    void log(LogLevel level, std::string_view tag, std::string_view fmt, Args&&... args) {
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        const char* prefix = "[INFO]";
        if (level == LogLevel::Warning) prefix = "[WARN]";
        if (level == LogLevel::Error)   prefix = "[ERR ]";
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << prefix << " [" << tag << "] " << message << std::endl;
    }

    // Convenience overload: allow calling logger::log(level, value) with a
    // non-format value (e.g., an int or a custom type). This is disabled when
    // the single argument is convertible to std::string_view so string literals
    // and format-strings still select the format-based overload above.
    template <typename T>
    std::enable_if_t<!std::is_convertible_v<T, std::string_view>, void>
    log(LogLevel level, std::string_view tag, T&& value) {
        std::string message = std::format("{}", std::forward<T>(value));
        const char* prefix = "[INFO]";
        if (level == LogLevel::Warning) prefix = "[WARN]";
        if (level == LogLevel::Error)   prefix = "[ERR ]";
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << prefix << " [" << tag << "] " << message << std::endl;
    }
}

// Define logging macros that become no-ops when NDEBUG (or project disable)
// is active. Use LIGHTS_PLEASE_LOG_ENABLED for clarity.
#if LIGHTS_PLEASE_LOG_ENABLED
#  define LOG_INFO(tag, fmt, ...)  logger::log(LogLevel::Info, tag, fmt, ##__VA_ARGS__)
#  define LOG_WARN(tag, fmt, ...)  logger::log(LogLevel::Warning, tag, fmt, ##__VA_ARGS__)
#  define LOG_ERR(tag, fmt, ...)   logger::log(LogLevel::Error, tag, fmt, ##__VA_ARGS__)
#else
// Expand to nothing but keep expression safety by using (void)0
#  define LOG_INFO(tag, fmt, ...)  (void)0
#  define LOG_WARN(tag, fmt, ...)  (void)0
#  define LOG_ERR(tag, fmt, ...)   (void)0
#endif