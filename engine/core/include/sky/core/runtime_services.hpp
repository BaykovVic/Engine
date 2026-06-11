#pragma once

#include <memory>

#include "sky/core/config_service.hpp"
#include "sky/core/diagnostics.hpp"
#include "sky/core/event_bus.hpp"
#include "sky/core/job_scheduler.hpp"
#include "sky/core/logger.hpp"

namespace sky::core {

/// Logger writing to stdout/stderr with level and category prefixes.
std::unique_ptr<ILogger> createConsoleLogger(LogLevel minimumLevel = LogLevel::Info);

/// In-memory configuration store (string-typed with parsed accessors).
std::unique_ptr<IConfigService> createInMemoryConfigService();

/// Single-process synchronous event bus.
std::unique_ptr<IEventBus> createEventBus();

/// Thread-pool backed job scheduler. threadCount == 0 uses hardware
/// concurrency.
std::unique_ptr<IJobScheduler> createThreadPoolScheduler(unsigned threadCount = 0);

/// Diagnostics sink that aggregates counters/timings in memory; readable
/// via the returned concrete type for editor tooling and tests.
class InMemoryDiagnostics : public IDiagnosticsSink {
public:
    [[nodiscard]] virtual std::int64_t counterValue(std::string_view name) const = 0;
    [[nodiscard]] virtual std::uint64_t lastTimingMicros(std::string_view name) const = 0;
};

std::unique_ptr<InMemoryDiagnostics> createInMemoryDiagnostics();

} // namespace sky::core
