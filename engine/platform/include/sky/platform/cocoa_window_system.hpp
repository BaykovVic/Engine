#pragma once

#include <memory>

#include "sky/platform/input_source.hpp"
#include "sky/platform/window_system.hpp"

namespace sky::platform {

/// The Cocoa window system for macOS standalone-runtime targets: an NSWindow
/// whose content view is backed by a CAMetalLayer, the unified input stream
/// and WM close handling — the macOS sibling of X11WindowSystem behind the
/// same contracts. The Vulkan backend presents into the CAMetalLayer via
/// VK_EXT_metal_surface (MoltenVK).
///
/// NOTE: implemented in cocoa_window_system.mm and compiled only on Apple
/// platforms; it has not yet been built or run on a Mac (the project is
/// developed on Linux). Treat it as a validated-by-contract starting point.
class CocoaWindowSystem : public IWindowSystem, public IInputSource {
public:
    ~CocoaWindowSystem() override = default;

    /// Cocoa is available and the application object is initialised.
    [[nodiscard]] virtual bool connected() const = 0;
    /// Current drawable size of a window in pixels (backing scale applied).
    virtual bool windowSize(WindowHandle window, std::uint32_t& width,
                            std::uint32_t& height) = 0;
    /// The CAMetalLayer* backing the window, passed opaquely to the renderer
    /// for VK_EXT_metal_surface creation. Null for an unknown window.
    virtual void* metalLayer(WindowHandle window) = 0;
};

/// Returns nullptr when Cocoa is unavailable (non-Apple builds compile this
/// to the null factory).
std::unique_ptr<CocoaWindowSystem> createCocoaWindowSystem();

} // namespace sky::platform
