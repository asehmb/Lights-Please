#pragma once
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
    void log(LogLevel level, std::string_view fmt, Args&&... args) {
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        
        const char* prefix = "[INFO]";
        if (level == LogLevel::Warning) prefix = "[WARN]";
        if (level == LogLevel::Error)   prefix = "[ERR ]";

        // Lock only during the actual output
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << prefix << " " << message << std::endl;
    }

    // Convenience overload: allow calling logger::log(level, value) with a
    // non-format value (e.g., an int or a custom type). This is disabled when
    // the single argument is convertible to std::string_view so string literals
    // and format-strings still select the format-based overload above.
    template <typename T>
    std::enable_if_t<!std::is_convertible_v<T, std::string_view>, void>
    log(LogLevel level, T&& value) {
        // Use std::format to convert the value to a string using {}.
        std::string message = std::format("{}", std::forward<T>(value));

        const char* prefix = "[INFO]";
        if (level == LogLevel::Warning) prefix = "[WARN]";
        if (level == LogLevel::Error)   prefix = "[ERR ]";

        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << prefix << " " << message << std::endl;
    }
}

#define LOG_INFO(fmt, ...)  logger::log(LogLevel::Info, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  logger::log(LogLevel::Warning, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)   logger::log(LogLevel::Error, fmt, ##__VA_ARGS__)