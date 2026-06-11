#pragma once

#include <cstdint>
#include <string_view>

namespace sky::core {

enum class LogLevel : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

/// Core Foundation contract: logging available to every native module.
class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0;

    void info(std::string_view category, std::string_view message) {
        log(LogLevel::Info, category, message);
    }
    void warning(std::string_view category, std::string_view message) {
        log(LogLevel::Warning, category, message);
    }
    void error(std::string_view category, std::string_view message) {
        log(LogLevel::Error, category, message);
    }
};

} // namespace sky::core
