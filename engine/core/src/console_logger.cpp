#include <cstdio>
#include <mutex>
#include <string>

#include "sky/core/runtime_services.hpp"

namespace sky::core {
namespace {

const char* levelName(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRIT";
    }
    return "?";
}

class ConsoleLogger final : public ILogger {
public:
    explicit ConsoleLogger(LogLevel minimumLevel) : minimumLevel_(minimumLevel) {}

    void log(LogLevel level, std::string_view category, std::string_view message) override {
        if (level < minimumLevel_) {
            return;
        }
        std::FILE* out = level >= LogLevel::Error ? stderr : stdout;
        const std::scoped_lock lock(mutex_);
        std::fprintf(out, "[%s] [%.*s] %.*s\n", levelName(level),
                     static_cast<int>(category.size()), category.data(),
                     static_cast<int>(message.size()), message.data());
        std::fflush(out);
    }

private:
    LogLevel minimumLevel_;
    std::mutex mutex_;
};

} // namespace

std::unique_ptr<ILogger> createConsoleLogger(LogLevel minimumLevel) {
    return std::make_unique<ConsoleLogger>(minimumLevel);
}

} // namespace sky::core
