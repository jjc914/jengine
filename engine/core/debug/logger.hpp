#ifndef engine_core_DEBUG_LOGGER_HPP
#define engine_core_DEBUG_LOGGER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <mutex>
#include <format>

namespace engine::core::debug {

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& get_singleton() {
        static Logger logger;
        return logger;
    }

    template <typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args) {
        std::lock_guard<std::mutex> lock(_mutex);

        std::string formatted = std::vformat(fmt, std::make_format_args((args)...));
        std::ostringstream output;
        output << "[" << level_to_string(level) << "] " << formatted << "\n";

        if (level == LogLevel::FATAL) {
            throw std::runtime_error(output.str());
        }

        switch (level) {
            case LogLevel::INFO:    std::cout << "\033[32m"; break; // green
            case LogLevel::WARNING: std::cout << "\033[33m"; break; // yellow
            case LogLevel::ERROR:   std::cout << "\033[31m"; break; // red
            case LogLevel::FATAL:   std::cout << "\033[41m"; break; // red background
        }
        std::cout << output.str() << "\033[0m" << std::flush;
    }

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args)  { log(LogLevel::INFO, fmt, std::forward<Args>(args)...); }
    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args)  { log(LogLevel::WARNING, fmt, std::forward<Args>(args)...); }
    template <typename... Args>
    void error(std::string_view fmt, Args&&... args) { log(LogLevel::ERROR, fmt, std::forward<Args>(args)...); }
    template <typename... Args>
    void fatal(std::string_view fmt, Args&&... args) { log(LogLevel::FATAL, fmt, std::forward<Args>(args)...); }

private:
    Logger() = default;
    ~Logger() = default;

    std::string level_to_string(LogLevel level) const {
        switch (level) {
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL:   return "FATAL";
            default: return "UNKNOWN";
        }
    }

    std::mutex _mutex;
};

} // namespace engine::core::debug

#endif // ENGINE_CORE_DEBUG_LOGGER_HPP
