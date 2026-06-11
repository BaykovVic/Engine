#pragma once

#include <cstdint>
#include <string_view>

namespace sky::core {

/// Core Foundation contract: structured diagnostics (counters, timings)
/// emitted by engine modules and consumed by editor tooling.
class IDiagnosticsSink {
public:
    virtual ~IDiagnosticsSink() = default;

    virtual void counter(std::string_view name, std::int64_t value) = 0;
    virtual void timingMicros(std::string_view name, std::uint64_t microseconds) = 0;
};

} // namespace sky::core
