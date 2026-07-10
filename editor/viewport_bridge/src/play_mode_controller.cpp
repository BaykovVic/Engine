#include <vector>

#include "sky/editor/viewport/play_mode_controller.hpp"

namespace sky::editor {
namespace {

class PlayModeControllerImpl final : public PlayModeController {
public:
    explicit PlayModeControllerImpl(scene::ISceneRuntime& sceneRuntime)
        : sceneRuntime_(sceneRuntime) {}

    // IPlayModeController

    bool play() override {
        if (!scene_.isValid() || state_ == PlayModeState::Playing) {
            return false;
        }
        if (state_ == PlayModeState::Editing) {
            sceneRuntime_.activate(scene_);
        }
        transition(PlayModeState::Playing);
        return true;
    }

    bool pause() override {
        if (state_ != PlayModeState::Playing) {
            return false;
        }
        transition(PlayModeState::Paused);
        return true;
    }

    bool stop() override {
        if (state_ == PlayModeState::Editing) {
            return false;
        }
        sceneRuntime_.deactivate(scene_);
        transition(PlayModeState::Editing);
        return true;
    }

    PlayModeState state() const override { return state_; }

    void onStateChanged(StateChanged callback) override {
        callbacks_.push_back(std::move(callback));
    }

    // PlayModeController

    void setScene(scene::SceneHandle scene) override {
        // Switching scenes mid-play stops the session first.
        if (state_ != PlayModeState::Editing) {
            stop();
        }
        scene_ = scene;
    }

    void tickFrame(double deltaSeconds) override {
        if (state_ == PlayModeState::Playing) {
            sceneRuntime_.tick(deltaSeconds);
        }
    }

private:
    void transition(PlayModeState next) {
        state_ = next;
        for (const auto& callback : callbacks_) {
            callback(state_);
        }
    }

    scene::ISceneRuntime& sceneRuntime_;
    scene::SceneHandle scene_;
    PlayModeState state_ = PlayModeState::Editing;
    std::vector<StateChanged> callbacks_;
};

} // namespace

std::unique_ptr<PlayModeController> createPlayModeController(
    scene::ISceneRuntime& sceneRuntime) {
    return std::make_unique<PlayModeControllerImpl>(sceneRuntime);
}

} // namespace sky::editor
