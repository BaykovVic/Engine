#include "sky/platform/platform_services.hpp"

namespace sky::platform {
namespace {

class ChronoTimerService final : public ITimerService {
public:
    ChronoTimerService() : start_(std::chrono::steady_clock::now()) {}

    std::chrono::nanoseconds monotonicNow() const override {
        return std::chrono::steady_clock::now().time_since_epoch();
    }

    std::uint64_t frameTimestamp() const override {
        const auto elapsed = std::chrono::steady_clock::now() - start_;
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
    }

private:
    std::chrono::steady_clock::time_point start_;
};

} // namespace

std::unique_ptr<ITimerService> createChronoTimerService() {
    return std::make_unique<ChronoTimerService>();
}

} // namespace sky::platform
