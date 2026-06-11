#include <algorithm>
#include <thread>

#include "sky/platform/platform_services.hpp"

namespace sky::platform {
namespace {

class StdThreading final : public IThreadingPrimitives {
public:
    void launchThread(const std::string& /*name*/, ThreadEntry entry) override {
        std::thread(std::move(entry)).detach();
    }

    std::uint32_t hardwareConcurrency() const override {
        return std::max(1u, std::thread::hardware_concurrency());
    }

    std::uint64_t currentThreadId() const override {
        return std::hash<std::thread::id>{}(std::this_thread::get_id());
    }
};

} // namespace

std::unique_ptr<IThreadingPrimitives> createStdThreading() {
    return std::make_unique<StdThreading>();
}

} // namespace sky::platform
