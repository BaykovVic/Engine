#pragma once

#include <cstdint>
#include <functional>

namespace sky::core {

struct JobHandle {
    std::uint64_t value = 0;

    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
};

/// Core Foundation contract: background job scheduling for asset import,
/// terrain rebuilds and other off-frame work.
class IJobScheduler {
public:
    virtual ~IJobScheduler() = default;

    using Job = std::function<void()>;

    virtual JobHandle schedule(Job job) = 0;
    virtual JobHandle scheduleAfter(JobHandle dependency, Job job) = 0;
    virtual void wait(JobHandle job) = 0;
};

} // namespace sky::core
