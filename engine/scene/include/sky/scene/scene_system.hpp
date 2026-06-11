#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "sky/core/handle.hpp"
#include "sky/object/object_model.hpp"

namespace sky::scene {

struct SceneTag {};
using SceneHandle = core::Handle<SceneTag>;

struct SceneDescriptor {
    std::string name;
    std::filesystem::path path;
};

enum class SceneState {
    Unloaded,
    Loaded,
    /// Activated for play mode: runtime systems, physics and scripts run.
    RuntimeActive,
};

/// The active scene context: which scene is current and which objects
/// belong to it. Scene membership is owned here, not by the Object Model.
struct SceneRuntimeContext {
    SceneHandle scene;
    SceneState state = SceneState::Unloaded;
    std::vector<object::ObjectHandle> rootObjects;
};

/// Scene System contract: load/save/unload of scenes.
class ISceneRepository {
public:
    virtual ~ISceneRepository() = default;

    virtual SceneHandle createScene(const SceneDescriptor& descriptor) = 0;
    virtual SceneHandle loadScene(const std::filesystem::path& path) = 0;
    virtual bool saveScene(SceneHandle scene) = 0;
    virtual void unloadScene(SceneHandle scene) = 0;
};

/// Scene System contract: runtime lifecycle of the active scene, including
/// the transition into and out of play mode.
class ISceneRuntime {
public:
    virtual ~ISceneRuntime() = default;

    virtual void activate(SceneHandle scene) = 0;
    virtual void deactivate(SceneHandle scene) = 0;
    [[nodiscard]] virtual const SceneRuntimeContext& activeContext() const = 0;
    /// Propagates the frame tick through the runtime stack.
    virtual void tick(double deltaSeconds) = 0;
};

/// Scene System contract: read-only queries over loaded scenes.
class ISceneQueryService {
public:
    virtual ~ISceneQueryService() = default;

    [[nodiscard]] virtual std::vector<SceneHandle> loadedScenes() const = 0;
    [[nodiscard]] virtual const SceneDescriptor& descriptor(SceneHandle scene) const = 0;
    [[nodiscard]] virtual SceneHandle sceneOf(object::ObjectHandle object) const = 0;
};

} // namespace sky::scene
