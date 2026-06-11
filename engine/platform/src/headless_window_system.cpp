#include <unordered_map>

#include "sky/platform/platform_services.hpp"

namespace sky::platform {
namespace {

class HeadlessWindowSystem final : public IWindowSystem {
public:
    WindowHandle createWindow(const WindowDesc& desc) override {
        const WindowHandle handle{nextId_++};
        windows_.emplace(handle.value, desc);
        return handle;
    }

    void destroyWindow(WindowHandle window) override { windows_.erase(window.value); }

    void setTitle(WindowHandle window, const std::string& title) override {
        if (const auto it = windows_.find(window.value); it != windows_.end()) {
            it->second.title = title;
        }
    }

    void resize(WindowHandle window, std::uint32_t width, std::uint32_t height) override {
        if (const auto it = windows_.find(window.value); it != windows_.end()) {
            it->second.width = width;
            it->second.height = height;
        }
    }

    bool pumpEvents() override { return true; }

private:
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::uint64_t, WindowDesc> windows_;
};

} // namespace

std::unique_ptr<IWindowSystem> createHeadlessWindowSystem() {
    return std::make_unique<HeadlessWindowSystem>();
}

} // namespace sky::platform
