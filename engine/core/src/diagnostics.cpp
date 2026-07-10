#include <mutex>
#include <string>
#include <unordered_map>

#include "sky/core/runtime_services.hpp"

namespace sky::core {
namespace {

class InMemoryDiagnosticsImpl final : public InMemoryDiagnostics {
public:
    void counter(std::string_view name, std::int64_t value) override {
        const std::scoped_lock lock(mutex_);
        counters_[std::string(name)] += value;
    }

    void timingMicros(std::string_view name, std::uint64_t microseconds) override {
        const std::scoped_lock lock(mutex_);
        timings_[std::string(name)] = microseconds;
    }

    std::int64_t counterValue(std::string_view name) const override {
        const std::scoped_lock lock(mutex_);
        const auto it = counters_.find(std::string(name));
        return it == counters_.end() ? 0 : it->second;
    }

    std::uint64_t lastTimingMicros(std::string_view name) const override {
        const std::scoped_lock lock(mutex_);
        const auto it = timings_.find(std::string(name));
        return it == timings_.end() ? 0 : it->second;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::int64_t> counters_;
    std::unordered_map<std::string, std::uint64_t> timings_;
};

} // namespace

std::unique_ptr<InMemoryDiagnostics> createInMemoryDiagnostics() {
    return std::make_unique<InMemoryDiagnosticsImpl>();
}

} // namespace sky::core
