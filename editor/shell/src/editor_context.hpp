#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "sky/asset/asset_database.hpp"
#include "sky/asset/fbx_importer.hpp"
#include "sky/asset/gltf_importer.hpp"
#include "sky/asset/obj_importer.hpp"
#include "sky/asset/png_decoder.hpp"
#include "sky/component/component_world.hpp"
#include "sky/core/runtime_services.hpp"
#include "sky/rendering/material.hpp"
#include "sky/mapgen/generation_pipeline.hpp"
#include "sky/mapgen/materialize.hpp"
#include "sky/terrain/terrain_world.hpp"
#include "sky/ecs/ecs_world.hpp"
#include "sky/ecs/object_sync.hpp"
#include "sky/editor/viewport/play_mode_controller.hpp"
#include "sky/object/object_world.hpp"
#include "sky/package/package_world.hpp"
#include "sky/physics/physics_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/platform/virtual_file_system.hpp"
#include "sky/rendering/renderer_registry.hpp"
#include "sky/scene/scene_authoring.hpp"
#include "sky/scene/scene_world.hpp"
#include "sky/scripting/dotnet_host.hpp"
#include "sky/serialization/backends.hpp"

namespace sky::editor {

/// A serializable description of an object subtree: enough to delete an
/// object and bring it back identically (undo of delete).
/// A component and its authored field values, for a lossless object snapshot.
struct ComponentSnapshot {
    std::string typeId;
    std::map<std::string, component::FieldValue> fields;
};

struct ObjectSnapshot {
    std::string name;
    core::Transform local;
    bool hasPhysicsBody = false;
    std::vector<ComponentSnapshot> components;
    std::vector<ObjectSnapshot> children;
};

/// Active terrain brush, set by the Terrain panel and applied by the scene
/// view on click/drag.
struct TerrainBrush {
    bool enabled = false;
    std::string operation = "raise";
    float radius = 4.0f;
    float strength = 1.0f;
};

/// Everything the editor session works with: the assembled engine core plus
/// the demo scene. Qt-free; the UI layer consumes it through references.
class EditorContext {
public:
    EditorContext();

    /// Snapshots every object's local transform so play mode can be entered
    /// non-destructively; endPlay restores them and re-seats the physics
    /// bodies (zero velocity), so leaving play returns the scene to how it was.
    void beginPlay();
    void endPlay();

    /// Scene document lifecycle. New clears to an empty scene (with the terrain
    /// fixture); Save writes the object graph to a .skybox (SKYB, terrain
    /// excluded); Open replaces the scene from a file. Open returns false on a
    /// bad file (the editor is left on a fresh empty scene).
    void newScene();
    bool saveScene(const std::filesystem::path& path);
    bool openScene(const std::filesystem::path& path);

    /// Drives the managed gameplay scripts one frame (call each frame while
    /// playing). No-op when scripting is unavailable or nothing is scripted.
    void tickScripts(double deltaSeconds);

    /// Compiles the project's user scripts (Assets/Scripts/*.cs) into
    /// SkyProject.Scripts.dll and (re)loads it into the script host. False
    /// when there are no scripts, no .NET, or the build failed (the failure
    /// is reported through scriptLog). beginPlay recompiles automatically
    /// when a source changed since the last successful build.
    bool reloadUserScripts();

    /// Keyboard state for gameplay scripts (portable key codes: ASCII
    /// uppercase for letters/digits, named keys from 256 — mirrored by the
    /// managed SkyEngine.KeyCode enum). Fed by the editor's Game view or the
    /// player's window; read by scripts through Input.GetKey.
    void setKeyDown(int key, bool down) {
        if (down) {
            keysDown_.insert(key);
        } else {
            keysDown_.erase(key);
        }
    }
    [[nodiscard]] bool keyDown(int key) const { return keysDown_.contains(key); }

    /// Applies the active brush at a world-space point on the terrain.
    void applyTerrainBrush(core::Vec3 worldPoint);
    /// Terrain height (world Y) under world-space (x, z).
    [[nodiscard]] float terrainHeightAt(float worldX, float worldZ) const;
    /// Regenerates the terrain procedurally and scatters objects; previous
    /// generated objects are removed first.
    std::size_t generateTerrain(std::uint64_t seed);
    /// Incremented on every terrain change; views rebuild meshes when it moves.
    [[nodiscard]] std::uint64_t terrainVersion() const { return terrainVersion_; }

    object::ObjectHandle createEmpty(const std::string& name);
    /// A scene-root object carrying a Mesh Renderer bound to a built-in
    /// primitive (no physics) — the GameObject > 3D Object menu entries.
    object::ObjectHandle createPrimitive(scene::PrimitiveKind kind,
                                         const std::string& name);
    /// A cube with a dynamic rigid body and box collider, Unity-style.
    object::ObjectHandle createCrate(const std::string& name, core::Vec3 position);
    /// A scene-root object carrying a Mesh Renderer that references an existing
    /// mesh asset — used when a model is dragged from Project into the scene.
    object::ObjectHandle createModelObject(const std::string& name,
                                           const std::string& meshRef);
    void destroyObject(object::ObjectHandle object);

    /// Deep-copies an object with its components, physics binding and
    /// children; the copy becomes a sibling of the original.
    object::ObjectHandle duplicateObject(object::ObjectHandle object);

    /// Moves an object under a new parent (invalid parent = scene root),
    /// keeping the scene root list consistent.
    void reparent(object::ObjectHandle child, object::ObjectHandle newParent);

    [[nodiscard]] ObjectSnapshot snapshotObject(object::ObjectHandle object) const;
    /// Rebuilds an object subtree from a snapshot (invalid parent = root).
    object::ObjectHandle restoreObject(const ObjectSnapshot& snapshot,
                                       object::ObjectHandle parent);
    [[nodiscard]] bool hasPhysicsBody(object::ObjectHandle object) const {
        return bodies_.contains(object.value);
    }

    /// Enabled/disabled (like Unity's active checkbox): a disabled object is
    /// not rendered and its lights do not contribute. Physics still runs.
    void setObjectEnabled(object::ObjectHandle object, bool enabled) {
        if (enabled) {
            disabled_.erase(object.value);
        } else {
            disabled_.insert(object.value);
        }
    }
    [[nodiscard]] bool objectEnabled(object::ObjectHandle object) const {
        return !disabled_.contains(object.value);
    }

    [[nodiscard]] std::vector<object::ObjectHandle> rootObjects() const {
        return roots_;
    }

    std::unique_ptr<platform::IFileSystem> fileSystem;
    std::unique_ptr<platform::IVirtualFileSystem> vfs;
    std::unique_ptr<core::IConfigService> config;
    std::unique_ptr<rendering::IRendererRegistry> renderers;
    std::unique_ptr<package::PackageWorld> packages;
    std::filesystem::path packagesRoot;
    std::filesystem::path assetsRoot;
    std::unique_ptr<serialization::ISerializationBackend> storage;
    std::unique_ptr<object::ObjectWorld> objects;
    std::unique_ptr<component::ComponentWorld> components;
    std::unique_ptr<ecs::EcsWorld> ecs;
    std::unique_ptr<ecs::IEcsObjectSync> ecsSync;
    std::unique_ptr<physics::PhysicsWorld> physics;
    std::unique_ptr<physics::ObjectPhysicsSync> physicsSync;
    std::unique_ptr<serialization::SchemaMigrationService> migrations;
    std::unique_ptr<scene::SceneWorld> scenes;
    std::unique_ptr<scripting::DotNetScriptHost> scriptHost;
    std::unique_ptr<PlayModeController> playMode;
    std::unique_ptr<terrain::TerrainWorld> terrain;
    std::unique_ptr<mapgen::IGenerationPipeline> mapgenPipeline;
    std::unique_ptr<rendering::IMaterialLibrary> materials;
    std::unique_ptr<asset::AssetDatabase> assets;
    std::unique_ptr<asset::IAssetImporter> objImporter;
    std::unique_ptr<asset::IAssetImporter> fbxImporter;
    std::unique_ptr<asset::IAssetImporter> gltfImporter;
    std::unique_ptr<asset::IAssetImporter> pngImporter;
    scene::SceneHandle activeScene;
    terrain::TerrainHandle terrainHandle;
    object::ObjectHandle terrainObject;
    TerrainBrush brush;

    /// Sink for managed Debug.Log output (level is a sky::core::LogLevel
    /// value). Defaults to stdout/stderr; the editor bridge redirects it into
    /// the Console panel's log buffer.
    std::function<void(int level, const std::string& message)> scriptLog;

private:
    void buildDemoScene();
    void initTerrain();
    void resetScene();
    void reattachPhysics();
    void initScripting();
    void startPlayScripts();
    void stopPlayScripts();
    object::ObjectHandle cloneSubtree(object::ObjectHandle source,
                                      object::ObjectHandle parent);
    void attachCrateBody(object::ObjectHandle object);

    std::vector<object::ObjectHandle> roots_;
    std::unordered_map<std::uint64_t, physics::RigidBodyHandle> bodies_;
    std::unordered_map<std::uint64_t, core::Transform> playSnapshot_;
    std::unordered_set<std::uint64_t> disabled_;
    // Live managed script instances during play: (managedInstanceId, objectId).
    std::vector<std::pair<std::uint64_t, std::uint64_t>> playScripts_;
    std::unordered_set<int> keysDown_;
    double playTime_ = 0.0; // seconds since play started (drives Time.TotalTime)
    // Newest Assets/Scripts source mtime at the last successful compile;
    // beginPlay recompiles when the sources moved past it.
    std::filesystem::file_time_type userScriptsStamp_{};
    physics::RigidBodyHandle terrainBody_;
    physics::ColliderHandle terrainCollider_;
    std::uint64_t terrainVersion_ = 0;
    std::vector<object::ObjectHandle> generatedObjects_;
};

} // namespace sky::editor
