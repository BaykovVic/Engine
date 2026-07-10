#pragma once

#include <cstdint>
#include <functional>

#include "sky/rendering/rendering.hpp"
#include "sky/scene/scene_system.hpp"

namespace sky::editor {

/// Explicit play mode state machine. Transitions between editor state and
/// runtime state are never hidden.
enum class PlayModeState {
    Editing,
    Playing,
    Paused,
};

/// Placement and identity of the embedded runtime viewport inside the
/// editor shell.
struct ViewportContext {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    PlayModeState state = PlayModeState::Editing;
};

/// Viewport Runtime Bridge contract: play / pause / stop orchestration and
/// editor/runtime state synchronization.
class IPlayModeController {
public:
    virtual ~IPlayModeController() = default;

    using StateChanged = std::function<void(PlayModeState)>;

    virtual bool play() = 0;
    virtual bool pause() = 0;
    virtual bool stop() = 0;
    [[nodiscard]] virtual PlayModeState state() const = 0;
    virtual void onStateChanged(StateChanged callback) = 0;
};

/// Viewport Runtime Bridge contract: hosting the runtime preview inside an
/// editor dock — binds a render surface and keeps the preview in sync.
class IRuntimePreviewHost {
public:
    virtual ~IRuntimePreviewHost() = default;

    virtual void attachSurface(rendering::IRenderSurface& surface) = 0;
    virtual void detachSurface() = 0;
    [[nodiscard]] virtual const ViewportContext& context() const = 0;
};

} // namespace sky::editor
