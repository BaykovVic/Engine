#pragma once

#include <memory>

#include "sky/platform/input_source.hpp"
#include "sky/platform/window_system.hpp"

namespace sky::platform {

/// The real X11 window system for Linux standalone runtime targets: native
/// windows, the unified input stream and WM close handling, all behind the
/// same contracts the headless implementation serves. Win32 and Cocoa are
/// sibling implementations of the same pair.
class X11WindowSystem : public IWindowSystem, public IInputSource {
public:
    ~X11WindowSystem() override = default;

    /// The native Display connection is alive and usable.
    [[nodiscard]] virtual bool connected() const = 0;
    /// Current size of a window as the server reports it.
    virtual bool windowSize(WindowHandle window, std::uint32_t& width,
                            std::uint32_t& height) = 0;
    /// Native handles for graphics-backend surface creation (Display* as
    /// an opaque pointer and the X11 Window id). The types stay opaque so
    /// the platform boundary holds.
    virtual bool nativeHandles(WindowHandle window, void** nativeDisplay,
                               std::uint64_t* nativeWindow) = 0;
};

/// Returns nullptr when no X server is reachable (headless boxes fall back
/// to createHeadlessWindowSystem()).
std::unique_ptr<X11WindowSystem> createX11WindowSystem();

} // namespace sky::platform
