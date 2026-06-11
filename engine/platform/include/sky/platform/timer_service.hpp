#pragma once

#include <chrono>
#include <cstdint>

namespace sky::platform {

/// Platform Layer contract: monotonic time for frame pacing, simulation
/// stepping and diagnostics.
class ITimerService {
public:
    virtual ~ITimerService() = default;

    [[nodiscard]] virtual std::chrono::nanoseconds monotonicNow() const = 0;
    /// Ticks elapsed since engine start, in nanoseconds.
    [[nodiscard]] virtual std::uint64_t frameTimestamp() const = 0;
};

} // namespace sky::platform
