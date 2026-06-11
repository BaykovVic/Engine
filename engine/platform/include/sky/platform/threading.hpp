#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace sky::platform {

/// Platform Layer contract: thread creation and identification primitives
/// for the Core Foundation job system.
class IThreadingPrimitives {
public:
    virtual ~IThreadingPrimitives() = default;

    using ThreadEntry = std::function<void()>;

    virtual void launchThread(const std::string& name, ThreadEntry entry) = 0;
    [[nodiscard]] virtual std::uint32_t hardwareConcurrency() const = 0;
    [[nodiscard]] virtual std::uint64_t currentThreadId() const = 0;
};

} // namespace sky::platform
