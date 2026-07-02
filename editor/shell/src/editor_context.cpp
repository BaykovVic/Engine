#include "editor_context.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "sky/rendering_opengl/opengl_backend.hpp"
#include "sky/serialization/byte_stream.hpp"
#include "sky/terrain/terrain_integration.hpp"

namespace {

// --- Reverse scripting boundary ------------------------------------------
// Managed scripts call these to move the object they are attached to. A single
// active EditorContext owns scripting at a time, so its object hierarchy is
// exposed through this file-scope pointer, set in initScripting().
sky::object::IObjectHierarchyAccess* g_scriptObjects = nullptr;
sky::editor::EditorContext* g_scriptContext = nullptr;

void scriptSetLocalPosition(std::uint64_t obj, float x, float y, float z) {
    if (g_scriptObjects == nullptr) return;
    const sky::object::ObjectHandle handle{obj};
    auto t = g_scriptObjects->localTransform(handle);
    t.position = {x, y, z};
    g_scriptObjects->setLocalTransform(handle, t);
}

void scriptSetLocalEuler(std::uint64_t obj, float xDeg, float yDeg, float zDeg) {
    if (g_scriptObjects == nullptr) return;
    const auto axisAngle = [](float degrees, float ax, float ay, float az) {
        const float r = degrees * 3.14159265358979f / 180.0f;
        const float s = std::sin(r / 2.0f);
        return sky::core::Quat{ax * s, ay * s, az * s, std::cos(r / 2.0f)};
    };
    const sky::object::ObjectHandle handle{obj};
    auto t = g_scriptObjects->localTransform(handle);
    t.rotation = axisAngle(yDeg, 0, 1, 0) * axisAngle(xDeg, 1, 0, 0) *
                 axisAngle(zDeg, 0, 0, 1);
    g_scriptObjects->setLocalTransform(handle, t);
}

void scriptSetLocalScale(std::uint64_t obj, float x, float y, float z) {
    if (g_scriptObjects == nullptr) return;
    const sky::object::ObjectHandle handle{obj};
    auto t = g_scriptObjects->localTransform(handle);
    t.scale = {x, y, z};
    g_scriptObjects->setLocalTransform(handle, t);
}

/// Managed Debug.Log lands here; `level` carries a sky::core::LogLevel value.
void scriptLogMessage(std::int32_t level, const char* message) {
    if (g_scriptContext != nullptr && g_scriptContext->scriptLog) {
        g_scriptContext->scriptLog(level, message != nullptr ? message : "");
    }
}

void scriptGetLocalPosition(std::uint64_t obj, float* x, float* y, float* z) {
    sky::core::Vec3 p{};
    if (g_scriptObjects != nullptr) {
        p = g_scriptObjects->localTransform(sky::object::ObjectHandle{obj}).position;
    }
    if (x != nullptr) *x = p.x;
    if (y != nullptr) *y = p.y;
    if (z != nullptr) *z = p.z;
}

std::int32_t scriptIsKeyDown(std::int32_t key) {
    return g_scriptContext != nullptr && g_scriptContext->keyDown(key) ? 1 : 0;
}

/// Native function table handed to managed SkyEngine.Engine (layout must match
/// the managed Api struct: six cdecl pointers).
struct SkyScriptApi {
    void* setLocalPosition;
    void* setLocalEuler;
    void* setLocalScale;
    void* log;
    void* getLocalPosition;
    void* isKeyDown;
};
SkyScriptApi g_scriptApi{reinterpret_cast<void*>(&scriptSetLocalPosition),
                         reinterpret_cast<void*>(&scriptSetLocalEuler),
                         reinterpret_cast<void*>(&scriptSetLocalScale),
                         reinterpret_cast<void*>(&scriptLogMessage),
                         reinterpret_cast<void*>(&scriptGetLocalPosition),
                         reinterpret_cast<void*>(&scriptIsKeyDown)};

/// Resolves an "assets://" VFS reference against the Assets root; plain
/// filesystem paths pass through unchanged.
std::filesystem::path resolveAssetPath(const std::string& path,
                                       const std::filesystem::path& assetsRoot) {
    constexpr const char* kPrefix = "assets://";
    if (path.rfind(kPrefix, 0) == 0) {
        return assetsRoot / path.substr(std::string(kPrefix).size());
    }
    return std::filesystem::path(path);
}

/// Newest last-write time across Assets/Scripts/*.cs (epoch when none).
std::filesystem::file_time_type newestUserScriptStamp(
    const std::filesystem::path& assetsRoot) {
    std::filesystem::file_time_type newest{};
    std::error_code ec;
    const auto dir = assetsRoot / "Scripts";
    if (!std::filesystem::exists(dir, ec)) {
        return newest;
    }
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.path().extension() == ".cs") {
            const auto stamp = std::filesystem::last_write_time(entry.path(), ec);
            newest = std::max(newest, stamp);
        }
    }
    return newest;
}

/// Populates a Unity-like demo Assets folder so the Project browser has
/// realistic content (textures, materials, scenes) under a stable root.
void populateDemoAssets(const std::filesystem::path& root) {
    const auto touch = [](const std::filesystem::path& path) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream(path) << "";
    };
    touch(root / "Scenes" / "SampleScene.skybox");
    touch(root / "Textures" / "ground_albedo.png");
    touch(root / "Textures" / "grass_n.png");
    touch(root / "Textures" / "cliff_mask.png");
    touch(root / "Textures" / "heightmap.raw");
    touch(root / "Materials" / "Grassland.mat");
    touch(root / "Materials" / "Cliff Rock.mat");
    touch(root / "Scripts" / "TerrainStreamer.cs");
}

// The demo terrain: a 48x48 heightfield centred on the world origin.
constexpr std::uint32_t kTerrainResolution = 48;
constexpr float kTerrainOriginX = -24.0f;
constexpr float kTerrainOriginZ = -24.0f;
} // namespace

namespace sky::editor {

EditorContext::EditorContext() {
    fileSystem = platform::createStdFileSystem();
    vfs = platform::createVirtualFileSystem();
    config = core::createInMemoryConfigService();
    config->set("engine.renderer", "opengl");
    renderers = rendering::createRendererRegistry();
    rendering_opengl::registerOpenGlBackend(*renderers);
    storage = serialization::createFileSerializationBackend(*fileSystem);
    objects = object::createObjectWorld();
    components = component::createComponentWorld();
    ecs = ecs::createEcsWorld();
    ecsSync = ecs::createEcsObjectSync(*ecs, *objects);
    physics = physics::createPhysicsWorld();
    physicsSync = physics::createObjectPhysicsSync(*physics, *objects);
    migrations = serialization::createSchemaMigrationService();
    scene::SceneWorldDeps sceneDeps{*objects,    *objects, *objects,
                                    *components, *components, *storage};
    sceneDeps.ecsScheduler = ecs.get();
    sceneDeps.physicsWorld = physics.get();
    sceneDeps.physicsSync = physicsSync.get();
    sceneDeps.ecsSync = ecsSync.get();
    sceneDeps.componentData = components.get();
    sceneDeps.migrations = migrations.get();
    scenes = scene::createSceneWorld(sceneDeps);
    playMode = createPlayModeController(*scenes);

    // Standard mounts of an opened project: writable project root plus the
    // engine's shipped content.
    const auto projectRoot = std::filesystem::current_path();
    vfs->mount("project", platform::createDirectoryMount(*fileSystem, projectRoot), 10);
    // Assets root: a stable, writable demo Assets folder (Unity-like).
    assetsRoot = std::filesystem::temp_directory_path() / "sky_editor_assets";
    populateDemoAssets(assetsRoot);
    vfs->mount("assets", platform::createDirectoryMount(*fileSystem, assetsRoot, true), 0);

    // Local packages: demo manifests under a writable packages directory,
    // discovered exactly like user packages and mounted into the VFS.
    packages = package::createPackageWorld(*fileSystem, *storage);
    packagesRoot = std::filesystem::temp_directory_path() / "sky_editor_packages";
    {
        package::PackageManifest noise;
        noise.packageId = "sky.noise-lib";
        noise.version = "1.0.0";
        noise.displayName = "Noise Library";
        noise.rootPath = packagesRoot / "sky.noise-lib";
        package::savePackageManifest(*storage, noise);

        package::PackageManifest terrainTools;
        terrainTools.packageId = "sky.terrain-tools";
        terrainTools.version = "1.2.0";
        terrainTools.displayName = "Terrain Tools";
        terrainTools.rootPath = packagesRoot / "sky.terrain-tools";
        terrainTools.dependencies = {{"sky.noise-lib", ">=1.0"}};
        terrainTools.extensionPoints = {"tool:terrain-brush", "importer:heightmap"};
        package::savePackageManifest(*storage, terrainTools);
    }
    packages->discoverPackages(packagesRoot);
    vfs->mount("packages",
               platform::createDirectoryMount(*fileSystem, packagesRoot, true), 0);

    components->registerComponentType(
        {"sky.mesh", "Mesh Renderer", false, "",
         {{"material", "string"}, {"mesh", "string"}}, "Rendering"});
    components->registerComponentType(
        {"sky.camera", "Camera", false, "",
         {{"projection", "string"},
          {"fieldOfView", "float"},
          {"nearPlane", "float"},
          {"farPlane", "float"}},
         "Rendering"});
    components->registerComponentType(
        {"sky.collider.box", "Box Collider", false, "",
         {{"isTrigger", "bool"}, {"center", "Vec3"}, {"size", "Vec3"}}, "Physics"});
    components->registerComponentType(
        {"sky.rigidbody", "Rigidbody", false, "", {{"mass", "float"}}, "Physics"});
    components->registerComponentType(
        {"sky.script", "Script", true, "", {{"class", "string"}}, "Scripting"});
    components->registerComponentType(
        {"sky.light", "Light", false, "",
         {{"type", "string"},
          {"color", "Vec3"},
          {"intensity", "float"},
          {"range", "float"}},
         "Rendering"});

    // Asset pipeline: own OBJ and PNG importers plus FBX via OpenFBX.
    assets = asset::createAssetDatabase();
    objImporter = asset::createObjImporter(*fileSystem);
    fbxImporter = asset::createFbxImporter(*fileSystem);
    gltfImporter = asset::createGltfImporter(*fileSystem);
    pngImporter = asset::createPngImporter(*fileSystem);
    assets->registerImporter(*objImporter);
    assets->registerImporter(*fbxImporter);
    assets->registerImporter(*gltfImporter);
    assets->registerImporter(*pngImporter);

    // Starter material set; the Inspector edits assignments by name.
    materials = rendering::createMaterialLibrary();
    materials->createMaterial({"Default", {0.72f, 0.72f, 0.74f}, 0.85f, 0.0f, {}});
    materials->createMaterial({"Gold", {1.00f, 0.78f, 0.30f}, 0.25f, 1.0f, {}});
    materials->createMaterial({"Terrain", {0.35f, 0.47f, 0.31f}, 1.0f, 0.0f, {}});
    materials->createMaterial(
        {"Glow", {0.20f, 0.55f, 0.85f}, 0.9f, 0.0f, {0.05f, 0.35f, 0.65f}});

    // The crate material uses an engine-generated checkerboard texture,
    // written as a real PNG and run through the import pipeline.
    {
        asset::ImageData checker;
        checker.width = checker.height = 64;
        checker.pixels.resize(64 * 64 * 4);
        for (std::uint32_t y = 0; y < 64; ++y) {
            for (std::uint32_t x = 0; x < 64; ++x) {
                const bool dark = ((x / 8) + (y / 8)) % 2 == 0;
                auto* px = checker.pixels.data() + (y * 64 + x) * 4;
                px[0] = dark ? 150 : 235;
                px[1] = dark ? 100 : 190;
                px[2] = dark ? 55 : 120;
                px[3] = 255;
            }
        }
        const auto texturePath = std::filesystem::temp_directory_path() /
                                 "sky_editor_assets" / "crate_checker.png";
        fileSystem->writeAll(texturePath, asset::encodePngRgba(checker));
        assets->importAsset(texturePath);
        materials->createMaterial({"Crate", {1.0f, 1.0f, 1.0f}, 0.75f, 0.0f, {},
                                   texturePath.generic_string()});
    }

    initScripting();
    buildDemoScene();
}

object::ObjectHandle EditorContext::createEmpty(const std::string& name) {
    const auto object = objects->createObject(name);
    scenes->addRootObject(activeScene, object);
    roots_.push_back(object);
    return object;
}

object::ObjectHandle EditorContext::createPrimitive(scene::PrimitiveKind kind,
                                                    const std::string& name) {
    const scene::AuthoringServices services{*objects,    *objects,
                                            *objects,     *components,
                                            *components,  *components};
    const auto object = scene::createPrimitive(services, kind, name);
    scenes->addRootObject(activeScene, object);
    roots_.push_back(object);
    return object;
}

object::ObjectHandle EditorContext::createCrate(const std::string& name,
                                                core::Vec3 position) {
    const auto crate = createEmpty(name);
    objects->setLocalTransform(crate, {position, {}, {1.0f, 1.0f, 1.0f}});
    const auto mesh = components->attach(crate, "sky.mesh");
    components->setField(mesh, "material", std::string("Crate"));
    components->setField(mesh, "mesh", std::string("cube"));
    const auto collider = components->attach(crate, "sky.collider.box");
    components->setField(collider, "isTrigger", false);
    components->setField(collider, "center", core::Vec3{0.0f, 0.0f, 0.0f});
    components->setField(collider, "size", core::Vec3{1.0f, 1.0f, 1.0f});
    const auto rigidbody = components->attach(crate, "sky.rigidbody");
    components->setField(rigidbody, "mass", 1.0f);
    attachCrateBody(crate);
    return crate;
}

void EditorContext::attachCrateBody(object::ObjectHandle object) {
    const auto body = physics->createBody(
        {physics::BodyType::Dynamic, 1.0f, objects->worldTransform(object)});
    physics->attachCollider(body,
                            {physics::ColliderShape::Box, {0.5f, 0.5f, 0.5f}, 0.0f});
    physicsSync->bind(body, object);
    bodies_.emplace(object.value, body);
}

void EditorContext::destroyObject(object::ObjectHandle object) {
    if (const auto it = bodies_.find(object.value); it != bodies_.end()) {
        physicsSync->unbind(it->second);
        physics->destroyBody(it->second);
        bodies_.erase(it);
    }
    components->detachAllFrom(object);
    objects->destroyObject(object);
    std::erase(roots_, object);
}

object::ObjectHandle EditorContext::createModelObject(const std::string& name,
                                                      const std::string& meshRef) {
    const auto object = createEmpty(name);
    const auto mesh = components->attach(object, "sky.mesh");
    components->setField(mesh, "material", std::string("Default"));
    components->setField(mesh, "mesh", meshRef);
    return object;
}

void EditorContext::beginPlay() {
    // Capture every object's local transform so leaving play can restore it.
    playSnapshot_.clear();
    std::vector<object::ObjectHandle> stack(roots_.begin(), roots_.end());
    while (!stack.empty()) {
        const auto object = stack.back();
        stack.pop_back();
        playSnapshot_[object.value] = objects->localTransform(object);
        for (const auto child : objects->childrenOf(object)) {
            stack.push_back(child);
        }
    }
    // Unity-style edit -> Play flow: recompile user scripts when a source
    // changed since the last successful build.
    if (newestUserScriptStamp(assetsRoot) != userScriptsStamp_) {
        reloadUserScripts();
    }
    startPlayScripts();
}

void EditorContext::endPlay() {
    stopPlayScripts();
    for (const auto& [id, transform] : playSnapshot_) {
        const object::ObjectHandle object{id};
        if (objects->exists(object)) {
            objects->setLocalTransform(object, transform);
        }
    }
    // Re-seat the simulation: bodies back to the restored pose, no residual
    // velocity, so the next play session starts from the original state.
    for (const auto& [id, body] : bodies_) {
        const object::ObjectHandle object{id};
        if (objects->exists(object)) {
            physics->setBodyTransform(body, objects->worldTransform(object));
            physics->setBodyVelocity(body, {0.0f, 0.0f, 0.0f});
        }
    }
    playSnapshot_.clear();
}

bool EditorContext::reloadUserScripts() {
#ifdef SKY_MANAGED_DIR
    if (scriptHost == nullptr) {
        return false;
    }
    const auto scriptsDir = assetsRoot / "Scripts";
    std::error_code ec;
    std::size_t sources = 0;
    if (std::filesystem::exists(scriptsDir, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(scriptsDir, ec)) {
            if (entry.path().extension() == ".cs") {
                ++sources;
            }
        }
    }
    if (sources == 0) {
        return false;
    }

    // Library/ScriptBuild next to Assets, Unity-style: a generated csproj
    // over Assets/Scripts/*.cs referencing the engine's managed assembly.
    const auto buildDir = assetsRoot.parent_path() / "Library" / "ScriptBuild";
    std::filesystem::create_directories(buildDir, ec);
    const std::filesystem::path managedDir = SKY_MANAGED_DIR;
    const auto csprojPath = buildDir / "SkyProject.Scripts.csproj";
    {
        std::ofstream csproj(csprojPath);
        csproj << "<Project Sdk=\"Microsoft.NET.Sdk\">\n"
               << "  <PropertyGroup>\n"
               << "    <TargetFramework>net8.0</TargetFramework>\n"
               << "    <AssemblyName>SkyProject.Scripts</AssemblyName>\n"
               << "    <Nullable>enable</Nullable>\n"
               << "    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>\n"
               << "  </PropertyGroup>\n"
               << "  <ItemGroup>\n"
               << "    <Compile Include=\""
               << (scriptsDir / "*.cs").generic_string() << "\"/>\n"
               << "    <Reference Include=\"SkyEngine.Managed\">\n"
               << "      <HintPath>"
               << (managedDir / "SkyEngine.Managed.dll").generic_string()
               << "</HintPath>\n"
               << "    </Reference>\n"
               << "  </ItemGroup>\n"
               << "</Project>\n";
    }

    const auto outDir = buildDir / "out";
    const std::string command = "dotnet build \"" + csprojPath.string() +
                                "\" -c Release -o \"" + outDir.string() +
                                "\" --nologo -v q > /dev/null 2>&1";
    if (std::system(command.c_str()) != 0) {
        if (scriptLog) {
            scriptLog(4, "Script compilation failed (Assets/Scripts)");
        }
        return false;
    }
    const bool loaded =
        scriptHost->loadUserAssembly(outDir / "SkyProject.Scripts.dll");
    if (loaded) {
        userScriptsStamp_ = newestUserScriptStamp(assetsRoot);
        if (scriptLog) {
            scriptLog(2, "Compiled " + std::to_string(sources) +
                             " user script(s) -> SkyProject.Scripts.dll");
        }
    } else if (scriptLog) {
        scriptLog(4, "Failed to load compiled user scripts");
    }
    return loaded;
#else
    return false;
#endif
}

void EditorContext::initScripting() {
    // Default sink: plain process output. The editor bridge replaces this to
    // route script logs into the Console panel.
    scriptLog = [](int level, const std::string& message) {
        std::fprintf(level >= 4 ? stderr : stdout, "[script] %s\n",
                     message.c_str());
    };
#ifdef SKY_MANAGED_DIR
    const std::filesystem::path managedDir = SKY_MANAGED_DIR;
    scripting::DotNetHostConfig config;
    config.bootstrapAssembly = managedDir / "SkyEngine.Managed.dll";
    scriptHost = scripting::createDotNetScriptHost(config);
    if (scriptHost == nullptr || !scriptHost->start()) {
        scriptHost.reset(); // no .NET runtime here: scripting is simply off
        return;
    }
    scriptHost->loadAssembly(
        {"SkyEngine.TestScripts", managedDir / "SkyEngine.TestScripts.dll"});
    g_scriptObjects = objects.get();
    g_scriptContext = this;
    scriptHost->installEngineApi(&g_scriptApi);
    // Project scripts, when the project has any (no-op otherwise).
    reloadUserScripts();
#endif
}

void EditorContext::startPlayScripts() {
    playScripts_.clear();
    playTime_ = 0.0;
    if (scriptHost == nullptr) {
        return;
    }
    g_scriptObjects = objects.get(); // this context owns scripting while playing
    g_scriptContext = this;
    std::vector<object::ObjectHandle> stack(roots_.begin(), roots_.end());
    while (!stack.empty()) {
        const auto object = stack.back();
        stack.pop_back();
        for (const auto child : objects->childrenOf(object)) {
            stack.push_back(child);
        }
        for (const auto comp : components->componentsOf(object)) {
            if (components->descriptorOf(comp).typeId != "sky.script") {
                continue;
            }
            std::string className;
            if (const auto field = components->field(comp, "class")) {
                if (const auto* s = std::get_if<std::string>(&*field)) {
                    className = *s;
                }
            }
            if (className.empty()) {
                continue;
            }
            const auto mid = scriptHost->createInstance(className);
            if (mid == 0) {
                continue;
            }
            scriptHost->setInstanceObjectId(mid, object.value);
            // Authored field values (Inspector edits stored on the component)
            // reach the instance before any lifecycle runs, Unity-style.
            for (const auto& [name, value] : components->fields(comp)) {
                if (name == "class") {
                    continue;
                }
                scriptHost->setInstanceField(
                    mid, name,
                    std::visit(
                        [](const auto& x) -> std::string {
                            using T = std::decay_t<decltype(x)>;
                            if constexpr (std::is_same_v<T, float>) {
                                char b[32];
                                std::snprintf(b, sizeof(b), "%g",
                                              static_cast<double>(x));
                                return b;
                            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                                return std::to_string(x);
                            } else if constexpr (std::is_same_v<T, bool>) {
                                return x ? "true" : "false";
                            } else if constexpr (std::is_same_v<T, std::string>) {
                                return x;
                            } else {
                                return {}; // Vec3 params are not supported yet
                            }
                        },
                        value));
            }
            scriptHost->invokeLifecycle(mid, scripting::ScriptLifecycleEvent::OnCreate, 0.0);
            scriptHost->invokeLifecycle(mid, scripting::ScriptLifecycleEvent::OnStart, 0.0);
            playScripts_.emplace_back(mid, object.value);
        }
    }
}

void EditorContext::tickScripts(double deltaSeconds) {
    if (scriptHost == nullptr) {
        return;
    }
    playTime_ += deltaSeconds;
    scriptHost->beginFrame(playTime_, deltaSeconds);
    for (const auto& [mid, objectId] : playScripts_) {
        scriptHost->invokeLifecycle(mid, scripting::ScriptLifecycleEvent::OnUpdate,
                                    deltaSeconds);
    }
}

void EditorContext::stopPlayScripts() {
    if (scriptHost != nullptr) {
        for (const auto& [mid, objectId] : playScripts_) {
            scriptHost->invokeLifecycle(mid, scripting::ScriptLifecycleEvent::OnDestroy, 0.0);
            scriptHost->destroyInstance(mid);
        }
    }
    playScripts_.clear();
}

object::ObjectHandle EditorContext::duplicateObject(object::ObjectHandle object) {
    if (!objects->exists(object)) {
        return object::ObjectHandle::invalid();
    }
    const auto parent = objects->parentOf(object);
    const auto copy = cloneSubtree(object, parent);
    if (!parent.isValid()) {
        scenes->addRootObject(activeScene, copy);
        roots_.push_back(copy);
    }
    return copy;
}

object::ObjectHandle EditorContext::cloneSubtree(object::ObjectHandle source,
                                                 object::ObjectHandle parent) {
    const auto copy = objects->createObject(objects->nameOf(source) + " Copy");
    if (parent.isValid()) {
        objects->setParent(copy, parent);
    }
    objects->setLocalTransform(copy, objects->localTransform(source));

    for (const auto component : components->componentsOf(source)) {
        const auto& descriptor = components->descriptorOf(component);
        const auto cloned = components->attach(copy, descriptor.typeId);
        for (const auto& [field, value] : components->fields(component)) {
            components->setField(cloned, field, value);
        }
    }
    if (bodies_.contains(source.value)) {
        attachCrateBody(copy);
    }
    for (const auto child : objects->childrenOf(source)) {
        cloneSubtree(child, copy);
    }
    return copy;
}

void EditorContext::reparent(object::ObjectHandle child, object::ObjectHandle newParent) {
    if (!objects->exists(child) || child == newParent) {
        return;
    }
    // Keep the object where it is in the world: recompute the local
    // transform against the new parent.
    const auto world = objects->worldTransform(child);
    objects->setParent(child, newParent);
    if (newParent.isValid()) {
        std::erase(roots_, child);
        // Local = inverse(parentWorld) * world; with uniform editor usage we
        // derive it through the hierarchy by assigning world and letting the
        // viewport math stay consistent for unrotated parents.
        const auto parentWorld = objects->worldTransform(newParent);
        const auto invRotation =
            core::Quat{-parentWorld.rotation.x, -parentWorld.rotation.y,
                       -parentWorld.rotation.z, parentWorld.rotation.w};
        const core::Vec3 offset{world.position.x - parentWorld.position.x,
                                world.position.y - parentWorld.position.y,
                                world.position.z - parentWorld.position.z};
        auto local = world;
        local.position = core::rotate(invRotation, offset);
        local.position = {local.position.x / parentWorld.scale.x,
                          local.position.y / parentWorld.scale.y,
                          local.position.z / parentWorld.scale.z};
        local.rotation = invRotation * world.rotation;
        local.scale = {world.scale.x / parentWorld.scale.x,
                       world.scale.y / parentWorld.scale.y,
                       world.scale.z / parentWorld.scale.z};
        objects->setLocalTransform(child, local);
    } else {
        if (std::ranges::find(roots_, child) == roots_.end()) {
            scenes->addRootObject(activeScene, child);
            roots_.push_back(child);
        }
        objects->setLocalTransform(child, world);
    }
}

void EditorContext::applyTerrainBrush(core::Vec3 worldPoint) {
    if (!brush.enabled || !terrainHandle.isValid()) {
        return;
    }
    terrain::TerrainEdit edit;
    edit.center = {worldPoint.x - kTerrainOriginX, 0.0f,
                   worldPoint.z - kTerrainOriginZ};
    edit.radius = brush.radius;
    edit.strength = brush.strength;
    edit.operation = brush.operation;
    terrain->applyEdit(terrainHandle, edit);
}

float EditorContext::terrainHeightAt(float worldX, float worldZ) const {
    if (!terrainHandle.isValid()) {
        return 0.0f;
    }
    return terrain->heightAt(terrainHandle, worldX - kTerrainOriginX,
                             worldZ - kTerrainOriginZ);
}

std::size_t EditorContext::generateTerrain(std::uint64_t seed) {
    if (!terrainHandle.isValid()) {
        return 0;
    }
    for (const auto object : generatedObjects_) {
        destroyObject(object);
    }
    generatedObjects_.clear();

    mapgen::GenerationProfile profile;
    profile.profileId = "editor";
    profile.seed = seed;
    profile.mapSize = kTerrainResolution;
    profile.enabledStages = {mapgen::kStageHeightfield, mapgen::kStagePlacement};

    const auto result = mapgenPipeline->generate({profile, terrainHandle});
    generatedObjects_ = mapgen::materializeGenerationResult(
        *result, *terrain, terrainHandle, *scenes, activeScene, *objects, *objects);
    // Placements are in terrain-local space; shift them to world space and
    // give them a visible mesh.
    for (const auto object : generatedObjects_) {
        auto transform = objects->localTransform(object);
        transform.position.x += kTerrainOriginX;
        transform.position.z += kTerrainOriginZ;
        transform.position.y += 0.5f; // rest on the surface
        transform.scale = {0.8f, 0.8f, 0.8f};
        objects->setLocalTransform(object, transform);
        components->attach(object, "sky.mesh");
        roots_.push_back(object);
    }
    return generatedObjects_.size();
}

namespace {

constexpr std::uint32_t kPrefabMagic = 0x50594B53;   // "SKYP"
constexpr std::uint32_t kPrefabVersion = 1;

void writeSnapshot(serialization::ByteWriter& writer, const ObjectSnapshot& s) {
    writer.writeString(s.name);
    for (const float v : {s.local.position.x, s.local.position.y, s.local.position.z,
                          s.local.rotation.x, s.local.rotation.y, s.local.rotation.z,
                          s.local.rotation.w, s.local.scale.x, s.local.scale.y,
                          s.local.scale.z}) {
        writer.writeF32(v);
    }
    writer.writeU32(s.hasPhysicsBody ? 1 : 0);
    writer.writeU32(static_cast<std::uint32_t>(s.components.size()));
    for (const auto& comp : s.components) {
        writer.writeString(comp.typeId);
        writer.writeU32(static_cast<std::uint32_t>(comp.fields.size()));
        for (const auto& [name, value] : comp.fields) {
            writer.writeString(name);
            writer.writeU32(static_cast<std::uint32_t>(value.index()));
            std::visit(
                [&writer](const auto& x) {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T, float>) {
                        writer.writeF32(x);
                    } else if constexpr (std::is_same_v<T, std::int64_t>) {
                        writer.writeU64(static_cast<std::uint64_t>(x));
                    } else if constexpr (std::is_same_v<T, bool>) {
                        writer.writeU32(x ? 1 : 0);
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        writer.writeString(x);
                    } else {
                        writer.writeF32(x.x);
                        writer.writeF32(x.y);
                        writer.writeF32(x.z);
                    }
                },
                value);
        }
    }
    writer.writeU32(static_cast<std::uint32_t>(s.children.size()));
    for (const auto& child : s.children) {
        writeSnapshot(writer, child);
    }
}

bool readSnapshot(serialization::ByteReader& reader, ObjectSnapshot& out) {
    const auto name = reader.readString();
    if (!name) {
        return false;
    }
    out.name = *name;
    float values[10] = {};
    for (float& v : values) {
        const auto f = reader.readF32();
        if (!f) {
            return false;
        }
        v = *f;
    }
    out.local.position = {values[0], values[1], values[2]};
    out.local.rotation = {values[3], values[4], values[5], values[6]};
    out.local.scale = {values[7], values[8], values[9]};
    const auto hasBody = reader.readU32();
    const auto componentCount = reader.readU32();
    if (!hasBody || !componentCount) {
        return false;
    }
    out.hasPhysicsBody = *hasBody != 0;
    for (std::uint32_t c = 0; c < *componentCount; ++c) {
        ComponentSnapshot comp;
        const auto typeId = reader.readString();
        const auto fieldCount = reader.readU32();
        if (!typeId || !fieldCount) {
            return false;
        }
        comp.typeId = *typeId;
        for (std::uint32_t f = 0; f < *fieldCount; ++f) {
            const auto fieldName = reader.readString();
            const auto kind = reader.readU32();
            if (!fieldName || !kind) {
                return false;
            }
            component::FieldValue value;
            switch (*kind) {
                case 0: {
                    const auto v = reader.readF32();
                    if (!v) return false;
                    value = *v;
                    break;
                }
                case 1: {
                    const auto v = reader.readU64();
                    if (!v) return false;
                    value = static_cast<std::int64_t>(*v);
                    break;
                }
                case 2: {
                    const auto v = reader.readU32();
                    if (!v) return false;
                    value = *v != 0;
                    break;
                }
                case 3: {
                    const auto v = reader.readString();
                    if (!v) return false;
                    value = *v;
                    break;
                }
                case 4: {
                    const auto x = reader.readF32();
                    const auto y = reader.readF32();
                    const auto z = reader.readF32();
                    if (!x || !y || !z) return false;
                    value = core::Vec3{*x, *y, *z};
                    break;
                }
                default:
                    return false;
            }
            comp.fields.emplace(*fieldName, std::move(value));
        }
        out.components.push_back(std::move(comp));
    }
    const auto childCount = reader.readU32();
    if (!childCount) {
        return false;
    }
    for (std::uint32_t i = 0; i < *childCount; ++i) {
        ObjectSnapshot child;
        if (!readSnapshot(reader, child)) {
            return false;
        }
        out.children.push_back(std::move(child));
    }
    return true;
}

} // namespace

bool EditorContext::savePrefab(object::ObjectHandle object,
                               const std::string& path) {
    if (!objects->exists(object)) {
        return false;
    }
    serialization::ByteWriter writer;
    writer.writeU32(kPrefabMagic);
    writer.writeU32(kPrefabVersion);
    writeSnapshot(writer, snapshotObject(object));

    const auto resolved = resolveAssetPath(path, assetsRoot);
    fileSystem->createDirectories(resolved.parent_path());
    return fileSystem->writeAll(resolved, writer.buffer());
}

object::ObjectHandle EditorContext::instantiatePrefab(const std::string& path) {
    const auto bytes = fileSystem->readAll(resolveAssetPath(path, assetsRoot));
    if (!bytes) {
        return object::ObjectHandle::invalid();
    }
    serialization::ByteReader reader(*bytes);
    const auto magic = reader.readU32();
    const auto version = reader.readU32();
    if (!magic || *magic != kPrefabMagic || !version || *version > kPrefabVersion) {
        return object::ObjectHandle::invalid();
    }
    ObjectSnapshot snapshot;
    if (!readSnapshot(reader, snapshot)) {
        return object::ObjectHandle::invalid();
    }
    return restoreObject(snapshot, object::ObjectHandle::invalid());
}

ObjectSnapshot EditorContext::snapshotObject(object::ObjectHandle object) const {
    ObjectSnapshot snapshot;
    snapshot.name = objects->nameOf(object);
    snapshot.local = objects->localTransform(object);
    snapshot.hasPhysicsBody = hasPhysicsBody(object);
    for (const auto component : components->componentsOf(object)) {
        snapshot.components.push_back(
            {components->descriptorOf(component).typeId, components->fields(component)});
    }
    for (const auto child : objects->childrenOf(object)) {
        snapshot.children.push_back(snapshotObject(child));
    }
    return snapshot;
}

object::ObjectHandle EditorContext::restoreObject(const ObjectSnapshot& snapshot,
                                                  object::ObjectHandle parent) {
    const auto object = objects->createObject(snapshot.name);
    if (parent.isValid()) {
        objects->setParent(object, parent);
    } else {
        scenes->addRootObject(activeScene, object);
        roots_.push_back(object);
    }
    objects->setLocalTransform(object, snapshot.local);
    for (const auto& comp : snapshot.components) {
        const auto handle = components->attach(object, comp.typeId);
        for (const auto& [name, value] : comp.fields) {
            components->setField(handle, name, value);
        }
    }
    if (snapshot.hasPhysicsBody) {
        attachCrateBody(object);
    }
    for (const auto& child : snapshot.children) {
        restoreObject(child, object);
    }
    return object;
}

void EditorContext::initTerrain() {
    // The ground is a real terrain: heightfield data, physics collider and
    // a scene object carrying its world placement. Its heightfield is not part
    // of the scene schema, so scene save/open treat it as a fixture.
    terrain = terrain::createTerrainWorld(*storage);
    mapgenPipeline = mapgen::createGenerationPipeline();

    terrain::TerrainDataset flat;
    flat.resolution = kTerrainResolution;
    flat.chunkSize = 16;
    flat.worldScale = {1.0f, 1.0f, 1.0f};
    flat.heights.assign(static_cast<std::size_t>(kTerrainResolution) *
                            kTerrainResolution,
                        0.0f);
    terrainHandle = terrain->createTerrain(flat);

    terrainObject = createEmpty("Terrain");
    objects->setLocalTransform(
        terrainObject, {{kTerrainOriginX, 0.0f, kTerrainOriginZ}, {}, {1, 1, 1}});

    terrainBody_ = physics->createBody(
        {physics::BodyType::Static, 0.0f,
         {{kTerrainOriginX, 0.0f, kTerrainOriginZ}, {}, {1, 1, 1}}});
    terrainCollider_ = physics->attachCollider(
        terrainBody_, terrain::makeTerrainCollider(terrain->dataset(terrainHandle)));

    // Terrain edits invalidate the render mesh and rebuild the collider.
    terrain->onTerrainChanged([this](terrain::TerrainHandle changed) {
        if (changed != terrainHandle) {
            return;
        }
        ++terrainVersion_;
        if (terrainCollider_.isValid()) {
            physics->detachCollider(terrainCollider_);
        }
        terrainCollider_ = physics->attachCollider(
            terrainBody_, terrain::makeTerrainCollider(terrain->dataset(terrainHandle)));
    });
    ++terrainVersion_;
}

/// Tears the whole scene down: physics bodies, terrain fixture and every root
/// object, leaving no active scene. Callers rebuild afterwards.
void EditorContext::resetScene() {
    for (auto& [id, body] : bodies_) {
        physicsSync->unbind(body);
        physics->destroyBody(body);
    }
    bodies_.clear();
    if (terrainCollider_.isValid()) {
        physics->detachCollider(terrainCollider_);
        terrainCollider_ = {};
    }
    if (terrainBody_.isValid()) {
        physics->destroyBody(terrainBody_);
        terrainBody_ = {};
    }
    terrain.reset();
    mapgenPipeline.reset();

    // Detach components across every root subtree, then let the scene destroy
    // the objects.
    std::vector<object::ObjectHandle> stack(roots_.begin(), roots_.end());
    while (!stack.empty()) {
        const auto object = stack.back();
        stack.pop_back();
        for (const auto child : objects->childrenOf(object)) {
            stack.push_back(child);
        }
        components->detachAllFrom(object);
    }
    if (activeScene.isValid()) {
        scenes->unloadScene(activeScene);
    }
    activeScene = scene::SceneHandle::invalid();
    roots_.clear();
    generatedObjects_.clear();
    disabled_.clear();
    terrainObject = object::ObjectHandle::invalid();
}

/// Rebuilds physics bodies for loaded objects that carry a rigidbody component
/// (the live simulation state is not part of the scene schema).
void EditorContext::reattachPhysics() {
    std::vector<object::ObjectHandle> stack(roots_.begin(), roots_.end());
    while (!stack.empty()) {
        const auto object = stack.back();
        stack.pop_back();
        for (const auto child : objects->childrenOf(object)) {
            stack.push_back(child);
        }
        if (object == terrainObject || bodies_.contains(object.value)) {
            continue;
        }
        for (const auto component : components->componentsOf(object)) {
            if (components->descriptorOf(component).typeId == "sky.rigidbody") {
                attachCrateBody(object);
                break;
            }
        }
    }
}

void EditorContext::newScene() {
    resetScene();
    activeScene = scenes->createScene({"Untitled", {}});
    initTerrain();
}

bool EditorContext::saveScene(const std::filesystem::path& path) {
    // The terrain fixture is excluded — its heightfield is not in the schema.
    return scenes->saveSceneAs(activeScene, path, terrainObject);
}

bool EditorContext::openScene(const std::filesystem::path& path) {
    resetScene();
    activeScene = scenes->loadScene(path);
    if (!activeScene.isValid()) {
        // Bad or missing file: leave the editor on a fresh empty scene.
        activeScene = scenes->createScene({"Untitled", {}});
        initTerrain();
        return false;
    }
    roots_ = scenes->rootObjectsOf(activeScene);
    initTerrain();      // appends the Terrain fixture to the loaded scene
    reattachPhysics();  // recreate rigidbody bodies from components
    return true;
}

void EditorContext::buildDemoScene() {
    activeScene = scenes->createScene({"SampleScene", {}});
    initTerrain();

    createCrate("Crate A", {-1.5f, 2.0f, 0.0f});
    createCrate("Crate B", {0.0f, 4.0f, 0.0f});
    const auto crateC = createCrate("Crate C", {1.5f, 6.0f, 0.0f});
    components->setField(components->componentsOf(crateC).front(), "material",
                         std::string("Gold"));

    // A real directional sun: pitched ~50 degrees down (rotation around X)
    // so its -Z forward axis shines down onto the scene.
    const auto light = createEmpty("Directional Light");
    objects->setLocalTransform(
        light, {{0.0f, 8.0f, -5.0f}, {-0.42f, 0.0f, 0.0f, 0.907f}, {1, 1, 1}});
    const auto sun = components->attach(light, "sky.light");
    components->setField(sun, "type", std::string("directional"));
    components->setField(sun, "color", core::Vec3{1.0f, 0.96f, 0.86f});
    components->setField(sun, "intensity", 1.1f);
    components->setField(sun, "range", 0.0f);

    // A warm point light hovering over the crates.
    const auto pointLight = createEmpty("Point Light");
    objects->setLocalTransform(pointLight, {{3.0f, 3.5f, 1.5f}, {}, {0.4f, 0.4f, 0.4f}});
    const auto lamp = components->attach(pointLight, "sky.light");
    components->setField(lamp, "type", std::string("point"));
    components->setField(lamp, "color", core::Vec3{1.0f, 0.45f, 0.15f});
    components->setField(lamp, "intensity", 5.0f);
    components->setField(lamp, "range", 9.0f);

    // An imported OBJ model: lives under Assets/Models, runs through the asset
    // pipeline and is referenced by a clean VFS path (resolved at render time),
    // not a raw filesystem path — Unity-style.
    const auto objPath = assetsRoot / "Models" / "pyramid.obj";
    const std::string objText =
        "v -1 0 -1\nv 1 0 -1\nv 1 0 1\nv -1 0 1\nv 0 1.8 0\n"
        "f 1 2 5\nf 2 3 5\nf 3 4 5\nf 4 1 5\nf 4 3 2 1\n";
    std::vector<std::byte> objBytes(objText.size());
    for (std::size_t i = 0; i < objText.size(); ++i) {
        objBytes[i] = static_cast<std::byte>(objText[i]);
    }
    fileSystem->writeAll(objPath, objBytes);
    if (assets->importAsset(objPath)) {
        const auto pyramid = createEmpty("Pyramid (obj)");
        objects->setLocalTransform(pyramid,
                                   {{4.5f, 0.0f, 2.5f}, {}, {1.4f, 1.4f, 1.4f}});
        const auto mesh = components->attach(pyramid, "sky.mesh");
        components->setField(mesh, "material", std::string("Gold"));
        components->setField(mesh, "mesh", std::string("assets://Models/pyramid.obj"));
        // A demo gameplay script: the pyramid spins in play mode.
        const auto script = components->attach(pyramid, "sky.script");
        components->setField(script, "class", std::string("SkyEngine.Tests.Rotator"));
    }

    const auto camera = createEmpty("Main Camera");
    // WASD moves the camera in play mode — the input demo script.
    {
        const auto script = components->attach(camera, "sky.script");
        components->setField(script, "class",
                             std::string("SkyEngine.Tests.WasdMover"));
    }
    // Positioned behind the scene, yawed 180 degrees to face it (cameras
    // look along their local -Z).
    objects->setLocalTransform(camera,
                               {{0.0f, 2.5f, -12.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                                {1, 1, 1}});
    const auto cameraComponent = components->attach(camera, "sky.camera");
    components->setField(cameraComponent, "projection", std::string("perspective"));
    components->setField(cameraComponent, "fieldOfView", 60.0f);
    components->setField(cameraComponent, "nearPlane", 0.1f);
    components->setField(cameraComponent, "farPlane", 1000.0f);
}

} // namespace sky::editor
