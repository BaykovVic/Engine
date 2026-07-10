#pragma once

#include <cstdint>
#include <string>

namespace sky::platform {

/// Opaque, platform-owned window identity. The OS handle never leaks above
/// the Platform Layer.
struct WindowHandle {
    std::uint64_t value = 0;

    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
    auto operator<=>(const WindowHandle&) const = default;
};

struct WindowDesc {
    std::string title;
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool resizable = true;
};

/// Platform Layer contract: windowing for the editor shell, render backends
/// and standalone runtime targets.
class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    virtual WindowHandle createWindow(const WindowDesc& desc) = 0;
    virtual void destroyWindow(WindowHandle window) = 0;
    virtual void setTitle(WindowHandle window, const std::string& title) = 0;
    virtual void resize(WindowHandle window, std::uint32_t width, std::uint32_t height) = 0;
    /// Pumps the platform message loop; returns false when the application
    /// has been asked to quit.
    virtual bool pumpEvents() = 0;
};

} // namespace sky::platform
