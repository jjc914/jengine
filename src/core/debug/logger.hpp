#ifndef CORE_DEBUG_LOGGER_HPP
#define CORE_DEBUG_LOGGER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <mutex>
#include <format>

namespace core::debug {

enum class LogLevel {
    INFO,
    WARNING,
    ERROR
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

        std::string formatted = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
        std::ostringstream output;
        output << "[" << level_to_string(level) << "] " << formatted << "\n";

        std::cout << output.str();
    }

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args)  { log(LogLevel::INFO, fmt, std::forward<Args>(args)...); }
    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args)  { log(LogLevel::WARNING, fmt, std::forward<Args>(args)...); }
    template <typename... Args>
    void error(std::string_view fmt, Args&&... args) { log(LogLevel::ERROR, fmt, std::forward<Args>(args)...); }

private:
    Logger() = default;
    ~Logger() = default;

    std::string level_to_string(LogLevel level) const {
        switch (level) {
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    std::mutex _mutex;
};

} // namespace core::debug

#endif // CORE_DEBUG_LOGGER_HPP
