#pragma once

#include <memory>

#include "sky/editor/viewport/viewport_bridge.hpp"
#include "sky/scene/scene_system.hpp"

namespace sky::editor {

/// Concrete play mode controller: explicit Editing -> Playing -> Paused
/// state machine driving the scene runtime. Qt-free by design — the dock
/// widget hosting the viewport consumes this, it does not implement it.
class PlayModeController : public IPlayModeController {
public:
    ~PlayModeController() override = default;

    /// Selects the scene the next play session will run.
    virtual void setScene(scene::SceneHandle scene) = 0;

    /// Drives one frame; forwards to the scene runtime only while playing.
    virtual void tickFrame(double deltaSeconds) = 0;
};

std::unique_ptr<PlayModeController> createPlayModeController(
    scene::ISceneRuntime& sceneRuntime);

} // namespace sky::editor
