#include "editor_context.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <variant>

#include "sky/component/data_asset.hpp"
#include "sky/package/package_installer.hpp"
#include "sky/package/package_lock.hpp"
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

void scriptGetWorldPosition(std::uint64_t obj, float* x, float* y, float* z) {
    sky::core::Vec3 p{};
    if (g_scriptObjects != nullptr) {
        p = g_scriptObjects->worldTransform(sky::object::ObjectHandle{obj}).position;
    }
    if (x != nullptr) *x = p.x;
    if (y != nullptr) *y = p.y;
    if (z != nullptr) *z = p.z;
}

std::uint64_t scriptInstantiate(const char* path, float x, float y, float z) {
    if (g_scriptContext == nullptr || path == nullptr) {
        return 0;
    }
    return g_scriptContext->spawnPrefabAt(path, {x, y, z}).value;
}

void scriptDestroyObject(std::uint64_t obj) {
    const sky::object::ObjectHandle handle{obj};
    if (g_scriptContext != nullptr && g_scriptContext->objects->exists(handle)) {
        g_scriptContext->destroyObject(handle);
    }
}

void scriptSetVelocity(std::uint64_t obj, float x, float y, float z) {
    if (g_scriptContext != nullptr) {
        g_scriptContext->setObjectVelocity(sky::object::ObjectHandle{obj},
                                           {x, y, z});
    }
}

void scriptGetVelocity(std::uint64_t obj, float* x, float* y, float* z) {
    sky::core::Vec3 v{};
    if (g_scriptContext != nullptr) {
        v = g_scriptContext->objectVelocity(sky::object::ObjectHandle{obj});
    }
    if (x != nullptr) *x = v.x;
    if (y != nullptr) *y = v.y;
    if (z != nullptr) *z = v.z;
}

std::int32_t scriptRaycast(float ox, float oy, float oz, float dx, float dy,
                           float dz, float maxDistance, float* px, float* py,
                           float* pz, float* nx, float* ny, float* nz,
                           float* distance) {
    if (g_scriptContext == nullptr) {
        return 0;
    }
    const auto hit = g_scriptContext->physics->raycast({ox, oy, oz}, {dx, dy, dz},
                                                       maxDistance);
    if (!hit) {
        return 0;
    }
    if (px != nullptr) *px = hit->point.x;
    if (py != nullptr) *py = hit->point.y;
    if (pz != nullptr) *pz = hit->point.z;
    if (nx != nullptr) *nx = hit->normal.x;
    if (ny != nullptr) *ny = hit->normal.y;
    if (nz != nullptr) *nz = hit->normal.z;
    if (distance != nullptr) *distance = hit->distance;
    return 1;
}

/// Native function table handed to managed SkyEngine.Engine (layout must match
/// the managed Api struct: thirteen cdecl pointers, see the static_assert
/// below the table).
struct SkyScriptApi {
    void* setLocalPosition;
    void* setLocalEuler;
    void* setLocalScale;
    void* log;
    void* getLocalPosition;
    void* isKeyDown;
    void* getWorldPosition;
    void* instantiate;
    void* destroyObject;
    void* setVelocity;
    void* getVelocity;
    void* raycast;
    void* dataAsset;
};

/// Serializes the resolved (inheritance applied) fields of a data asset as
/// records "name US type US value RS" (US = 0x1F, RS = 0x1E). Writes up to
/// capacity-1 bytes plus a terminator and returns the full length, so the
/// managed side can retry with an exact buffer.
std::int32_t scriptDataAsset(const char* ref, char* buffer,
                             std::int32_t capacity) {
    if (g_scriptContext == nullptr || ref == nullptr) {
        return 0;
    }
    std::string text;
    for (const auto& [name, value] :
         g_scriptContext->resolvedDataAssetFields(ref)) {
        text += name;
        text += '\x1F';
        char formatted[64];
        if (const auto* f = std::get_if<float>(&value)) {
            std::snprintf(formatted, sizeof(formatted), "%g", *f);
            text += "float\x1F";
            text += formatted;
        } else if (const auto* i = std::get_if<std::int64_t>(&value)) {
            std::snprintf(formatted, sizeof(formatted), "%lld",
                          static_cast<long long>(*i));
            text += "int\x1F";
            text += formatted;
        } else if (const auto* b = std::get_if<bool>(&value)) {
            text += "bool\x1F";
            text += *b ? "true" : "false";
        } else if (const auto* v = std::get_if<sky::core::Vec3>(&value)) {
            std::snprintf(formatted, sizeof(formatted), "%g, %g, %g", v->x,
                          v->y, v->z);
            text += "Vec3\x1F";
            text += formatted;
        } else if (const auto* s = std::get_if<std::string>(&value)) {
            text += "string\x1F";
            text += *s;
        }
        text += '\x1E';
    }
    if (buffer != nullptr && capacity > 0) {
        const auto copied =
            std::min<std::size_t>(text.size(), std::size_t(capacity) - 1);
        std::memcpy(buffer, text.data(), copied);
        buffer[copied] = '\0';
    }
    return std::int32_t(text.size());
}

// Layout guard: the managed Api struct mirrors this table field for field;
// a one-sided edit must fail the build, not corrupt memory at runtime.
static_assert(sizeof(SkyScriptApi) == 13 * sizeof(void*),
              "SkyScriptApi changed: mirror the managed Engine.Api struct and "
              "update both counts");

SkyScriptApi g_scriptApi{reinterpret_cast<void*>(&scriptSetLocalPosition),
                         reinterpret_cast<void*>(&scriptSetLocalEuler),
                         reinterpret_cast<void*>(&scriptSetLocalScale),
                         reinterpret_cast<void*>(&scriptLogMessage),
                         reinterpret_cast<void*>(&scriptGetLocalPosition),
                         reinterpret_cast<void*>(&scriptIsKeyDown),
                         reinterpret_cast<void*>(&scriptGetWorldPosition),
                         reinterpret_cast<void*>(&scriptInstantiate),
                         reinterpret_cast<void*>(&scriptDestroyObject),
                         reinterpret_cast<void*>(&scriptSetVelocity),
                         reinterpret_cast<void*>(&scriptGetVelocity),
                         reinterpret_cast<void*>(&scriptRaycast),
                         reinterpret_cast<void*>(&scriptDataAsset)};

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

/// Newest last-write time across *.cs in the given directories (epoch when
/// none exist).
std::filesystem::file_time_type newestUserScriptStamp(
    const std::vector<std::filesystem::path>& dirs) {
    std::filesystem::file_time_type newest{};
    std::error_code ec;
    for (const auto& dir : dirs) {
        if (!std::filesystem::exists(dir, ec)) {
            continue;
        }
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (entry.path().extension() == ".cs") {
                const auto stamp = std::filesystem::last_write_time(entry.path(), ec);
                newest = std::max(newest, stamp);
            }
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
    // Note: no Scripts fixture — anything under Assets/Scripts is compiled as
    // real user code at startup, so the demo must not plant placeholder .cs
    // files there.
    std::error_code ec;
    std::filesystem::remove(root / "Scripts" / "TerrainStreamer.cs", ec);
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
    // Asset-reference bridge (SKYB >= 1.2): "assets://…" string fields are
    // persisted with their sidecar GUID and re-resolved on load, so a
    // renamed source keeps working. Deferred through `this`: the asset
    // database is constructed later in this constructor.
    sceneDeps.refToGuid = [this](const std::string& ref) -> std::uint64_t {
        if (assets == nullptr || ref.rfind("assets://", 0) != 0) {
            return 0;
        }
        const auto host = assetsRoot / ref.substr(9);
        const auto id = assets->findBySourcePath(host);
        return id ? id->value : 0;
    };
    sceneDeps.guidToRef = [this](std::uint64_t guid) -> std::string {
        if (assets == nullptr) {
            return {};
        }
        const auto descriptor = assets->resolve(asset::AssetId{guid});
        if (!descriptor) {
            return {};
        }
        const auto relative = descriptor->sourcePath.lexically_relative(assetsRoot);
        if (relative.empty() || *relative.begin() == "..") {
            return {};
        }
        return "assets://" + relative.generic_string();
    };
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
    packageCacheRoot =
        std::filesystem::temp_directory_path() / "sky_editor_pkg_cache";
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
    // Re-apply the persisted activation state (Packages/sky.lock).
    applyPackageLock();

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
    components->registerComponentType(
        {"sky.environment", "Environment", false, "",
         {{"horizon", "Vec3"}, {"zenith", "Vec3"}, {"exposure", "float"}},
         "Rendering"});

    // Asset pipeline: own OBJ and PNG importers plus FBX via OpenFBX.
    // Sidecar GUID identity: renaming a source (with its .skymeta) keeps
    // the asset id stable.
    assets = asset::createAssetDatabase(*fileSystem);
    objImporter = asset::createObjImporter(*fileSystem);
    fbxImporter = asset::createFbxImporter(*fileSystem);
    gltfImporter = asset::createGltfImporter(*fileSystem);
    pngImporter = asset::createPngImporter(*fileSystem);
    assets->registerImporter(*objImporter);
    assets->registerImporter(*fbxImporter);
    assets->registerImporter(*gltfImporter);
    assets->registerImporter(*pngImporter);
    dataImporter = asset::createDataImporter();
    assets->registerImporter(*dataImporter);
    scanProjectAssets();

    // Starter material set; the Inspector edits assignments by name.
    materials = rendering::createMaterialLibrary();
    materials->createMaterial({"Default", {0.72f, 0.72f, 0.74f}, 0.85f, 0.0f, {}});
    materials->createMaterial({"Gold", {1.00f, 0.78f, 0.30f}, 0.25f, 1.0f, {}});
    materials->createMaterial(
        {"Glow", {0.20f, 0.55f, 0.85f}, 0.9f, 0.0f, {0.05f, 0.35f, 0.65f}});

    // Demo-scene look: engine-generated noise textures (real PNGs through
    // the import pipeline, like the crate checker below) and the materials
    // the golden-hour sample scene is dressed with.
    const auto generatedAssets =
        std::filesystem::temp_directory_path() / "sky_editor_assets";
    {
        // Cheap value noise: integer-hash bilinear patches.
        const auto hash01 = [](std::uint32_t x, std::uint32_t y) {
            std::uint32_t h = x * 374761393u + y * 668265263u;
            h = (h ^ (h >> 13)) * 1274126177u;
            return float((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
        };
        const auto noise = [&](float x, float y) {
            const auto xi = std::uint32_t(x), yi = std::uint32_t(y);
            const float fx = x - float(xi), fy = y - float(yi);
            const float a = hash01(xi, yi), b = hash01(xi + 1, yi);
            const float c = hash01(xi, yi + 1), d = hash01(xi + 1, yi + 1);
            return (a * (1 - fx) + b * fx) * (1 - fy) +
                   (c * (1 - fx) + d * fx) * fy;
        };
        const auto makeTexture = [&](const char* file, auto&& shade) {
            asset::ImageData image;
            image.width = image.height = 128;
            image.pixels.resize(std::size_t(128) * 128 * 4);
            for (std::uint32_t y = 0; y < 128; ++y) {
                for (std::uint32_t x = 0; x < 128; ++x) {
                    auto* px = image.pixels.data() + (std::size_t(y) * 128 + x) * 4;
                    shade(x, y, px);
                }
            }
            const auto path = generatedAssets / file;
            fileSystem->writeAll(path, asset::encodePngRgba(image));
            assets->importAsset(path);
            return path.generic_string();
        };
        const auto grassPath = makeTexture(
            "ground_noise.png", [&](std::uint32_t x, std::uint32_t y, std::uint8_t* px) {
                // Meadow: green base, broad tone patches, soft dry blend.
                const float broad = noise(x / 16.0f, y / 16.0f);
                const float fine = noise(x / 2.0f, y / 2.0f);
                const float dryNoise = noise(x / 11.0f + 40.0f, y / 11.0f);
                const float dry =
                    std::clamp((dryNoise - 0.62f) / 0.30f, 0.0f, 1.0f);
                const float tone = 0.82f + broad * 0.24f + (fine - 0.5f) * 0.22f;
                px[0] = std::uint8_t(std::min(255.0f, (88.0f + dry * 34.0f) * tone));
                px[1] = std::uint8_t(std::min(255.0f, (116.0f + dry * 16.0f) * tone));
                px[2] = std::uint8_t(std::min(255.0f, (64.0f + dry * 8.0f) * tone));
                px[3] = 255;
            });
        const auto stonePath = makeTexture(
            "stone_noise.png", [&](std::uint32_t x, std::uint32_t y, std::uint8_t* px) {
                // Mottled warm granite with darker veins.
                const float broad = noise(x / 12.0f + 80.0f, y / 12.0f);
                const float vein = noise(x / 5.0f, y / 5.0f + 80.0f);
                float tone = 0.62f + broad * 0.30f;
                if (vein < 0.26f) {
                    tone *= 0.55f;
                }
                px[0] = std::uint8_t(168.0f * tone);
                px[1] = std::uint8_t(158.0f * tone);
                px[2] = std::uint8_t(146.0f * tone);
                px[3] = 255;
            });

        rendering::MaterialDesc terrainDesc;
        terrainDesc.name = "Terrain";
        terrainDesc.baseColor = {0.58f, 0.66f, 0.46f};
        terrainDesc.roughness = 1.0f;
        terrainDesc.texturePath = grassPath;
        terrainDesc.uvTiling = {10.0f, 10.0f};
        materials->createMaterial(terrainDesc);

        rendering::MaterialDesc stoneDesc;
        stoneDesc.name = "Stone";
        stoneDesc.baseColor = {0.78f, 0.72f, 0.66f};
        stoneDesc.roughness = 0.95f;
        stoneDesc.texturePath = stonePath;
        stoneDesc.uvTiling = {2.0f, 2.0f};
        materials->createMaterial(stoneDesc);
    }
    {
        // Polished dark stone: near-mirror metal picks up the low sun as a
        // sharp warm highlight.
        rendering::MaterialDesc obsidian;
        obsidian.name = "Obsidian";
        obsidian.baseColor = {0.16f, 0.15f, 0.18f};
        obsidian.roughness = 0.12f;
        obsidian.metallic = 0.9f;
        materials->createMaterial(obsidian);

        // Translucent cyan crystal — the sorted blend pass shows the scene
        // through it; a touch of emissive keeps it readable in shade.
        rendering::MaterialDesc crystal;
        crystal.name = "Crystal";
        crystal.baseColor = {0.30f, 0.78f, 0.88f};
        crystal.roughness = 0.15f;
        crystal.emissive = {0.08f, 0.34f, 0.42f};
        crystal.opacity = 0.4f;
        materials->createMaterial(crystal);

        // Hot ember: emissive above 1 relies on the HDR target + ACES.
        rendering::MaterialDesc ember;
        ember.name = "Ember";
        ember.baseColor = {1.0f, 0.55f, 0.18f};
        ember.roughness = 0.6f;
        ember.emissive = {1.9f, 0.85f, 0.25f};
        materials->createMaterial(ember);
    }

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

    // Persisted material edits (Assets/Materials/*.skymat) override the
    // built-in defaults created above.
    loadProjectMaterials();

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
    // Capture the FULL scene state — hierarchy, transforms, component
    // fields — so leaving play can reconcile everything the game mutated,
    // not just transforms.
    playRoots_.clear();
    playExisting_.clear();
    for (const auto root : roots_) {
        playRoots_.push_back(snapshotObject(root));
    }
    std::vector<object::ObjectHandle> stack(roots_.begin(), roots_.end());
    while (!stack.empty()) {
        const auto object = stack.back();
        stack.pop_back();
        playExisting_.insert(object.value);
        for (const auto child : objects->childrenOf(object)) {
            stack.push_back(child);
        }
    }
    // Unity-style edit -> Play flow: recompile user scripts when a source
    // changed since the last successful build.
    if (newestUserScriptStamp(scriptSourceDirs()) != userScriptsStamp_) {
        reloadUserScripts();
    }
    startPlayScripts();
}

void EditorContext::endPlay() {
    stopPlayScripts();
    // 1. Survivors go back under their pre-play parents first, so objects
    //    created during play never hold restored children when destroyed.
    for (const auto& root : playRoots_) {
        reparentToSnapshot(root, object::ObjectHandle::invalid());
    }
    // 2. Everything created during play is removed (top-most subtrees).
    destroyPlayCreated();
    // 3. Survivors get their transforms and component state back in place
    //    (stable handles); play-destroyed objects are recreated.
    for (const auto& root : playRoots_) {
        restorePlayState(root, object::ObjectHandle::invalid());
    }
    // 4. Re-seat the simulation: bodies back to the restored pose, no
    //    residual velocity, so the next play session starts clean.
    for (const auto& [id, body] : bodies_) {
        const object::ObjectHandle object{id};
        if (objects->exists(object)) {
            physics->setBodyTransform(body, objects->worldTransform(object));
            physics->setBodyVelocity(body, {0.0f, 0.0f, 0.0f});
        }
    }
    playRoots_.clear();
    playExisting_.clear();
}

void EditorContext::reparentToSnapshot(const ObjectSnapshot& snapshot,
                                       object::ObjectHandle parent) {
    if (!objects->exists(snapshot.source)) {
        return; // destroyed during play; restorePlayState recreates it
    }
    if (parent.isValid() && objects->parentOf(snapshot.source) != parent) {
        objects->setParent(snapshot.source, parent);
    }
    for (const auto& child : snapshot.children) {
        reparentToSnapshot(child, snapshot.source);
    }
}

void EditorContext::destroyPlayCreated() {
    // Top-most objects that did not exist when play started; their whole
    // subtrees go (depth-first — destroyObject does not cascade).
    std::vector<object::ObjectHandle> created;
    std::vector<object::ObjectHandle> stack(roots_.begin(), roots_.end());
    while (!stack.empty()) {
        const auto object = stack.back();
        stack.pop_back();
        if (!playExisting_.contains(object.value)) {
            created.push_back(object);
            continue; // the whole subtree is play-created
        }
        for (const auto child : objects->childrenOf(object)) {
            stack.push_back(child);
        }
    }
    const std::function<void(object::ObjectHandle)> destroySubtree =
        [&](object::ObjectHandle object) {
            for (const auto child : objects->childrenOf(object)) {
                destroySubtree(child);
            }
            destroyObject(object);
        };
    for (const auto object : created) {
        destroySubtree(object);
    }
}

void EditorContext::restorePlayState(const ObjectSnapshot& snapshot,
                                     object::ObjectHandle parent) {
    if (!objects->exists(snapshot.source)) {
        // Destroyed during play: bring the whole subtree back from the
        // snapshot (new handles — the old ones died with the objects).
        restoreObject(snapshot, parent);
        return;
    }
    objects->setLocalTransform(snapshot.source, snapshot.local);
    // Component state back IN PLACE: existing components keep their handles
    // (undo history and editor caches hold them); only real differences
    // attach or detach. Snapshot components match current ones by typeId.
    auto current = components->componentsOf(snapshot.source);
    std::vector<bool> used(current.size(), false);
    for (const auto& comp : snapshot.components) {
        auto target = component::ComponentHandle::invalid();
        for (std::size_t i = 0; i < current.size(); ++i) {
            if (!used[i] &&
                components->descriptorOf(current[i]).typeId == comp.typeId) {
                used[i] = true;
                target = current[i];
                break;
            }
        }
        if (!target.isValid()) {
            target = components->attach(snapshot.source, comp.typeId);
        }
        for (const auto& [name, value] : comp.fields) {
            components->setField(target, name, value);
        }
    }
    for (std::size_t i = 0; i < current.size(); ++i) {
        if (!used[i]) {
            components->detach(current[i]); // added during play
        }
    }
    for (const auto& child : snapshot.children) {
        restorePlayState(child, snapshot.source);
    }
}

std::vector<std::filesystem::path> EditorContext::scriptSourceDirs() const {
    std::vector<std::filesystem::path> dirs{assetsRoot / "Scripts"};
    // Active code-carrying packages contribute their Runtime folder.
    for (const auto& [id, handle] : packageHandles_) {
        if (activePackages_.contains(id)) {
            dirs.push_back(packages->manifest(handle).rootPath / "Runtime");
        }
    }
    return dirs;
}

bool EditorContext::reloadUserScripts() {
#ifdef SKY_MANAGED_DIR
    if (scriptHost == nullptr) {
        return false;
    }
    const auto dirs = scriptSourceDirs();
    std::error_code ec;
    std::size_t sources = 0;
    std::vector<std::filesystem::path> sourceDirs;
    for (const auto& dir : dirs) {
        std::size_t here = 0;
        if (std::filesystem::exists(dir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
                if (entry.path().extension() == ".cs") {
                    ++here;
                }
            }
        }
        if (here > 0) {
            sources += here;
            sourceDirs.push_back(dir);
        }
    }
    if (sources == 0) {
        // The last source went away (e.g. the only code package deactivated):
        // drop the previously loaded user assembly too.
        scriptHost->unloadUserAssembly();
        userScriptsStamp_ = {};
        return false;
    }

    // Library/ScriptBuild next to Assets, Unity-style: a generated csproj
    // over every source directory, referencing the engine's managed assembly.
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
               << "  <ItemGroup>\n";
        for (const auto& dir : sourceDirs) {
            csproj << "    <Compile Include=\""
                   << (dir / "*.cs").generic_string() << "\"/>\n";
        }
        csproj << "    <Reference Include=\"SkyEngine.Managed\">\n"
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
            scriptLog(4, "Script compilation failed (project + package scripts)");
        }
        return false;
    }
    const bool loaded =
        scriptHost->loadUserAssembly(outDir / "SkyProject.Scripts.dll");
    if (loaded) {
        userScriptsStamp_ = newestUserScriptStamp(dirs);
        if (scriptLog) {
            scriptLog(2, "Compiled " + std::to_string(sources) +
                             " user script(s) from " +
                             std::to_string(sourceDirs.size()) +
                             " source set(s) -> SkyProject.Scripts.dll");
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
    for (const auto root : roots_) {
        startScriptsFor(root);
    }
}

void EditorContext::startScriptsFor(object::ObjectHandle object) {
    if (scriptHost == nullptr) {
        return;
    }
    std::vector<object::ObjectHandle> stack{object};
    while (!stack.empty()) {
        const auto current = stack.back();
        stack.pop_back();
        for (const auto child : objects->childrenOf(current)) {
            stack.push_back(child);
        }
        for (const auto comp : components->componentsOf(current)) {
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
            scriptHost->setInstanceObjectId(mid, current.value);
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
            playScripts_.emplace_back(mid, current.value);
        }
    }
}

void EditorContext::tickScripts(double deltaSeconds) {
    if (scriptHost == nullptr) {
        return;
    }
    playTime_ += deltaSeconds;
    scriptHost->beginFrame(playTime_, deltaSeconds);
    // By index with a size snapshot: a script may Instantiate (appending to
    // playScripts_) or Destroy objects mid-loop. Freshly spawned scripts get
    // their first OnUpdate next frame.
    const std::size_t liveCount = playScripts_.size();
    for (std::size_t i = 0; i < liveCount; ++i) {
        const auto [mid, objectId] = playScripts_[i];
        if (!objects->exists(object::ObjectHandle{objectId})) {
            continue; // destroyed earlier this frame; swept below
        }
        scriptHost->invokeLifecycle(mid, scripting::ScriptLifecycleEvent::OnUpdate,
                                    deltaSeconds);
    }
    // Sweep instances whose object died (script Destroy or an editor delete
    // during play): OnDestroy fires, then the managed peer goes away.
    for (auto it = playScripts_.begin(); it != playScripts_.end();) {
        if (objects->exists(object::ObjectHandle{it->second})) {
            ++it;
            continue;
        }
        scriptHost->invokeLifecycle(it->first,
                                    scripting::ScriptLifecycleEvent::OnDestroy, 0.0);
        scriptHost->destroyInstance(it->first);
        it = playScripts_.erase(it);
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

object::ObjectHandle EditorContext::spawnPrefabAt(const std::string& path,
                                                  core::Vec3 position) {
    const auto object = instantiatePrefab(path);
    if (!object.isValid()) {
        return object;
    }
    auto local = objects->localTransform(object); // spawned at root: local = world
    local.position = position;
    objects->setLocalTransform(object, local);
    // Re-seat the subtree's physics bodies at the new pose, no residual motion.
    std::vector<object::ObjectHandle> stack{object};
    while (!stack.empty()) {
        const auto current = stack.back();
        stack.pop_back();
        for (const auto child : objects->childrenOf(current)) {
            stack.push_back(child);
        }
        if (const auto it = bodies_.find(current.value); it != bodies_.end()) {
            physics->setBodyTransform(it->second, objects->worldTransform(current));
            physics->setBodyVelocity(it->second, {0.0f, 0.0f, 0.0f});
        }
    }
    if (playMode != nullptr && playMode->state() == PlayModeState::Playing) {
        startScriptsFor(object);
    }
    return object;
}

bool EditorContext::installPackage(const std::string& source) {
    package::PackageInstaller installer(*storage, packageCacheRoot);
    if (!installer.install(source, packagesRoot)) {
        return false;
    }
    packages->discoverPackages(packagesRoot);
    writePackageLock(); // the newcomer appears in the lock (inactive)
    return true;
}

bool EditorContext::setPackageActive(const std::string& packageId, bool active) {
    bool changed = false;
    if (active) {
        // The whole dependency graph activates, dependencies first; the
        // resolver enforces version requirements (empty result = conflict,
        // unknown package or cycle).
        const auto order = packages->resolve({packageId});
        if (order.empty()) {
            return false;
        }
        for (const auto& manifest : order) {
            if (activePackages_.contains(manifest.packageId)) {
                continue;
            }
            const auto [it, inserted] =
                packageHandles_.try_emplace(manifest.packageId);
            if (inserted) {
                it->second = packages->registerPackage(manifest);
            }
            if (packages->activate(it->second)) {
                activePackages_.insert(manifest.packageId);
                changed = true;
            }
        }
    } else {
        const auto it = packageHandles_.find(packageId);
        if (it == packageHandles_.end() ||
            !activePackages_.contains(packageId) ||
            !packages->deactivate(it->second)) {
            return false;
        }
        activePackages_.erase(packageId);
        changed = true;
    }
    if (changed) {
        writePackageLock();
        // The active set defines which package Runtime/*.cs compile into the
        // user assembly; rebuild it (no-op before scripting comes up).
        reloadUserScripts();
    }
    return changed;
}

void EditorContext::applyPackageLock() {
    const auto lock = package::loadPackageLock(
        *storage, packagesRoot / package::kPackageLockFileName);
    if (!lock) {
        return; // no lock yet: everything starts inactive
    }
    std::unordered_set<std::string> seen;
    for (const auto& entry : *lock) {
        if (entry.active && seen.insert(entry.packageId).second) {
            setPackageActive(entry.packageId, true);
        }
    }
}

void EditorContext::writePackageLock() {
    std::vector<package::LockedPackage> locked;
    for (const auto& manifest : packages->discoveredPackages()) {
        locked.push_back({manifest.packageId, manifest.version,
                          package::manifestChecksum(manifest),
                          activePackages_.contains(manifest.packageId)});
    }
    package::savePackageLock(
        *storage, packagesRoot / package::kPackageLockFileName, locked);
}

bool EditorContext::setObjectVelocity(object::ObjectHandle object,
                                      core::Vec3 velocity) {
    const auto it = bodies_.find(object.value);
    if (it == bodies_.end()) {
        return false;
    }
    physics->setBodyVelocity(it->second, velocity);
    return true;
}

core::Vec3 EditorContext::objectVelocity(object::ObjectHandle object) const {
    const auto it = bodies_.find(object.value);
    return it != bodies_.end() ? physics->bodyVelocity(it->second)
                               : core::Vec3{0.0f, 0.0f, 0.0f};
}

ObjectSnapshot EditorContext::snapshotObject(object::ObjectHandle object) const {
    ObjectSnapshot snapshot;
    snapshot.source = object;
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

namespace {

// MaterialDesc <-> data-asset fields. uvTiling (Vec2) rides as two floats:
// FieldValue has no Vec2 alternative.
component::DataAssetDesc materialToDataAsset(const rendering::MaterialDesc& desc) {
    component::DataAssetDesc data;
    data.typeId = "sky.material";
    data.fields["name"] = desc.name;
    data.fields["baseColor"] = desc.baseColor;
    data.fields["roughness"] = desc.roughness;
    data.fields["metallic"] = desc.metallic;
    data.fields["emissive"] = desc.emissive;
    data.fields["texturePath"] = desc.texturePath;
    data.fields["normalPath"] = desc.normalPath;
    data.fields["roughnessPath"] = desc.roughnessPath;
    data.fields["metallicPath"] = desc.metallicPath;
    data.fields["occlusionPath"] = desc.occlusionPath;
    data.fields["heightPath"] = desc.heightPath;
    data.fields["uvTilingX"] = desc.uvTiling.x;
    data.fields["uvTilingY"] = desc.uvTiling.y;
    data.fields["parallaxDepth"] = desc.parallaxDepth;
    data.fields["opacity"] = desc.opacity;
    return data;
}

rendering::MaterialDesc materialFromDataAsset(const component::DataAssetDesc& data) {
    rendering::MaterialDesc desc;
    const auto text = [&](const char* key, std::string fallback = {}) {
        const auto it = data.fields.find(key);
        if (it != data.fields.end()) {
            if (const auto* value = std::get_if<std::string>(&it->second)) {
                return *value;
            }
        }
        return fallback;
    };
    const auto number = [&](const char* key, float fallback) {
        const auto it = data.fields.find(key);
        if (it != data.fields.end()) {
            if (const auto* value = std::get_if<float>(&it->second)) {
                return *value;
            }
        }
        return fallback;
    };
    const auto vector = [&](const char* key, core::Vec3 fallback) {
        const auto it = data.fields.find(key);
        if (it != data.fields.end()) {
            if (const auto* value = std::get_if<core::Vec3>(&it->second)) {
                return *value;
            }
        }
        return fallback;
    };
    desc.name = text("name");
    desc.baseColor = vector("baseColor", desc.baseColor);
    desc.roughness = number("roughness", desc.roughness);
    desc.metallic = number("metallic", desc.metallic);
    desc.emissive = vector("emissive", desc.emissive);
    desc.texturePath = text("texturePath");
    desc.normalPath = text("normalPath");
    desc.roughnessPath = text("roughnessPath");
    desc.metallicPath = text("metallicPath");
    desc.occlusionPath = text("occlusionPath");
    desc.heightPath = text("heightPath");
    desc.uvTiling = {number("uvTilingX", 1.0f), number("uvTilingY", 1.0f)};
    desc.parallaxDepth = number("parallaxDepth", 0.0f);
    desc.opacity = number("opacity", 1.0f);
    return desc;
}

} // namespace

void EditorContext::persistMaterial(rendering::MaterialHandle material) {
    const auto& desc = materials->material(material);
    if (desc.name.empty()) {
        return;
    }
    const auto file = assetsRoot / "Materials" / (desc.name + ".skymat");
    if (component::saveDataAsset(*storage, file, materialToDataAsset(desc))) {
        assets->importAsset(file); // GUID sidecar + registry entry
    }
}

void EditorContext::loadProjectMaterials() {
    namespace fs = std::filesystem;
    std::error_code ec;
    for (fs::directory_iterator it(assetsRoot / "Materials", ec), end; it != end;
         it.increment(ec)) {
        if (ec || !it->is_regular_file(ec) || it->path().extension() != ".skymat") {
            continue;
        }
        const auto data = component::loadDataAsset(*storage, it->path());
        if (!data || data->typeId != "sky.material") {
            continue;
        }
        const auto desc = materialFromDataAsset(*data);
        if (desc.name.empty()) {
            continue;
        }
        if (const auto handle = materials->findMaterial(desc.name)) {
            materials->updateMaterial(*handle, desc);
        } else {
            materials->createMaterial(desc);
        }
    }
}

namespace {
/// "assets://<rel>" -> host path under the given root; empty otherwise.
std::filesystem::path hostPathForRef(const std::filesystem::path& assetsRoot,
                                     const std::string& ref) {
    if (ref.rfind("assets://", 0) != 0) {
        return {};
    }
    return assetsRoot / ref.substr(9);
}
} // namespace

bool EditorContext::createDataAsset(const std::string& name,
                                    const std::string& typeId) {
    if (name.empty() || typeId.empty()) {
        return false;
    }
    const auto file = assetsRoot / "Data" / (name + ".skydata");
    std::error_code exists;
    if (std::filesystem::exists(file, exists)) {
        return false; // never silently overwrite an authored asset
    }
    component::DataAssetDesc desc;
    desc.typeId = typeId;
    if (!component::saveDataAsset(*storage, file, desc)) {
        return false;
    }
    return assets->importAsset(file).has_value(); // GUID sidecar + registry
}

std::optional<component::DataAssetDesc> EditorContext::loadDataAssetByRef(
    const std::string& ref) const {
    const auto host = hostPathForRef(assetsRoot, ref);
    if (host.empty()) {
        return std::nullopt;
    }
    return component::loadDataAsset(*storage, host);
}

bool EditorContext::setDataAssetField(const std::string& ref,
                                      const std::string& name,
                                      const component::FieldValue& value) {
    const auto host = hostPathForRef(assetsRoot, ref);
    if (host.empty() || name.empty()) {
        return false;
    }
    auto desc = component::loadDataAsset(*storage, host);
    if (!desc) {
        return false;
    }
    desc->fields[name] = value;
    return component::saveDataAsset(*storage, host, *desc);
}

std::map<std::string, component::FieldValue>
EditorContext::resolvedDataAssetFields(const std::string& ref) const {
    // Walk up the parent chain (bounded against cycles), then overlay from
    // the root ancestor down so nearer overrides win.
    std::vector<component::DataAssetDesc> chain;
    auto current = loadDataAssetByRef(ref);
    for (int depth = 0; current && depth < 8; ++depth) {
        chain.push_back(*current);
        if (current->parentGuid == 0) {
            break;
        }
        const auto parent = assets->resolve(asset::AssetId{current->parentGuid});
        current = parent ? component::loadDataAsset(*storage, parent->sourcePath)
                         : std::nullopt;
    }
    std::map<std::string, component::FieldValue> result;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        result = component::mergedFields(result, it->fields);
    }
    return result;
}

void EditorContext::scanProjectAssets() {
    namespace fs = std::filesystem;
    if (assets == nullptr) {
        return;
    }
    std::error_code ec;
    for (fs::recursive_directory_iterator it(assetsRoot, ec), end; it != end;
         it.increment(ec)) {
        if (ec) {
            break;
        }
        if (!it->is_regular_file(ec)) {
            continue;
        }
        if (it->path().extension() == ".skymeta") {
            continue; // sidecars are identity, not assets
        }
        assets->importAsset(it->path()); // importers filter by supports()
    }
}

bool EditorContext::saveScene(const std::filesystem::path& path) {
    // The editor owns the live root list (deletes/reparents mutate roots_,
    // the scene record only ever grew) — sync it so the save never walks
    // stale roots.
    scenes->setRootObjects(activeScene, roots_);
    // The terrain fixture is excluded — its heightfield is not in the schema.
    return scenes->saveSceneAs(activeScene, path, terrainObject);
}

bool EditorContext::openScene(const std::filesystem::path& path) {
    // Re-scan first: files may have been added or renamed since the last
    // scan, and GUID re-resolution needs their current locations.
    scanProjectAssets();
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

    // Rolling meadow: gentle hills around the rim, the central stage kept
    // flat so the physics crates land cleanly.
    {
        const auto raise = [&](float worldX, float worldZ, float radius,
                               float strength) {
            terrain::TerrainEdit edit;
            edit.center = {worldX - kTerrainOriginX, 0.0f,
                           worldZ - kTerrainOriginZ};
            edit.radius = radius;
            edit.strength = strength;
            edit.operation = "raise";
            terrain->applyEdit(terrainHandle, edit);
        };
        // Kept clear of the flat centre stage AND of (10, 10) — the bridge
        // tests raycast bare terrain (world y = 0) there.
        raise(-15.0f, 14.0f, 12.0f, 2.6f);
        raise(20.0f, -2.0f, 11.0f, 2.2f);
        raise(-12.0f, -10.0f, 10.0f, 1.8f);
        raise(13.0f, -13.0f, 12.0f, 2.4f);
        raise(-2.0f, 21.0f, 9.0f, 1.6f);
    }

    // Golden-hour environment: warm horizon, deep zenith, filmic exposure
    // through the HDR + ACES post pass.
    {
        const auto environment = createEmpty("Environment");
        // A settings holder, not scenery: parked far above the play space so
        // viewport picking never lands on it.
        objects->setLocalTransform(environment, {{0.0f, 80.0f, 0.0f}, {}, {1, 1, 1}});
        const auto env = components->attach(environment, "sky.environment");
        components->setField(env, "horizon", core::Vec3{0.94f, 0.56f, 0.34f});
        components->setField(env, "zenith", core::Vec3{0.16f, 0.26f, 0.48f});
        components->setField(env, "exposure", 1.15f);
    }

    createCrate("Crate A", {-4.6f, 2.0f, 1.0f});
    createCrate("Crate B", {-4.1f, 4.0f, 1.6f});
    const auto crateC = createCrate("Crate C", {-3.5f, 6.0f, 0.7f});
    components->setField(components->componentsOf(crateC).front(), "material",
                         std::string("Gold"));

    // A low golden-hour sun: pitched ~20 degrees below the horizon plane and
    // yawed to rake across the scene, so shadows run long and warm.
    const auto light = createEmpty("Directional Light");
    {
        const core::Quat pitch{-0.1736f, 0.0f, 0.0f, 0.9848f}; // -20 deg X
        const core::Quat yaw{0.0f, 0.342f, 0.0f, 0.9397f};     // +40 deg Y
        objects->setLocalTransform(light,
                                   {{0.0f, 8.0f, -5.0f}, yaw * pitch, {1, 1, 1}});
    }
    const auto sun = components->attach(light, "sky.light");
    components->setField(sun, "type", std::string("directional"));
    components->setField(sun, "color", core::Vec3{1.0f, 0.64f, 0.36f});
    components->setField(sun, "intensity", 2.2f);
    components->setField(sun, "range", 0.0f);

    // Monument stage: a polished obsidian monolith inside a granite ring,
    // floating crystals above, ember orbs glowing at its feet.
    {
        const auto meshOf = [&](object::ObjectHandle object) {
            for (const auto component : components->componentsOf(object)) {
                if (components->descriptorOf(component).typeId == "sky.mesh") {
                    return component;
                }
            }
            return component::ComponentHandle::invalid();
        };
        const auto monolith = createPrimitive(scene::PrimitiveKind::Cube, "Monolith");
        const float ground = terrainHeightAt(0.0f, 7.0f);
        objects->setLocalTransform(monolith, {{0.0f, ground + 2.8f, 7.0f},
                                              {0.0f, 0.1305f, 0.0f, 0.9914f},
                                              {1.2f, 5.6f, 1.2f}});
        components->setField(meshOf(monolith), "material",
                             std::string("Obsidian"));

        const float ringRadius = 6.5f;
        for (int i = 0; i < 6; ++i) {
            const float angle = float(i) * 1.0472f + 0.35f; // 60 deg apart
            const float x = std::cos(angle) * ringRadius;
            const float z = 7.0f + std::sin(angle) * ringRadius;
            const float height = 2.0f + 0.5f * float((i * 3) % 4);
            const auto stone = createPrimitive(scene::PrimitiveKind::Cube,
                                               "Standing Stone " +
                                                   std::to_string(i + 1));
            const float yawHalf = angle * 0.5f;
            objects->setLocalTransform(
                stone, {{x, terrainHeightAt(x, z) + height * 0.5f - 0.15f, z},
                        {0.0f, std::sin(yawHalf), 0.0f, std::cos(yawHalf)},
                        {0.85f, height, 0.7f}});
            components->setField(meshOf(stone), "material",
                                 std::string("Stone"));
        }

        // Low enough that the monolith, stones and ground sit behind them —
        // the translucency has something to read against.
        const core::Vec3 crystalSpots[3] = {
            {-1.9f, 2.4f, 5.6f}, {2.2f, 3.0f, 6.6f}, {1.1f, 1.8f, 4.0f}};
        const float crystalScale[3] = {0.9f, 0.7f, 0.5f};
        for (int i = 0; i < 3; ++i) {
            const auto crystal = createEmpty("Crystal " + std::to_string(i + 1));
            const core::Quat tilt{0.12f + 0.05f * float(i), 0.28f, 0.08f, 0.95f};
            objects->setLocalTransform(crystal,
                                       {crystalSpots[i], tilt,
                                        {crystalScale[i], crystalScale[i],
                                         crystalScale[i]}});
            const auto mesh = components->attach(crystal, "sky.mesh");
            components->setField(mesh, "material", std::string("Crystal"));
            components->setField(mesh, "mesh",
                                 std::string("assets://Models/pyramid.obj"));
        }

        const core::Vec3 emberSpots[2] = {{-2.1f, 0.0f, 4.4f}, {1.8f, 0.0f, 5.6f}};
        for (int i = 0; i < 2; ++i) {
            const auto orb = createPrimitive(scene::PrimitiveKind::Sphere,
                                             "Ember " + std::to_string(i + 1));
            const float x = emberSpots[i].x, z = emberSpots[i].z;
            objects->setLocalTransform(orb, {{x, terrainHeightAt(x, z) + 0.5f, z},
                                             {},
                                             {0.7f, 0.7f, 0.7f}});
            components->setField(meshOf(orb), "material",
                                 std::string("Ember"));
        }
    }

    // A warm ember glow at the monolith's feet…
    const auto pointLight = createEmpty("Point Light");
    objects->setLocalTransform(pointLight, {{0.0f, 1.4f, 5.0f}, {}, {0.4f, 0.4f, 0.4f}});
    const auto lamp = components->attach(pointLight, "sky.light");
    components->setField(lamp, "type", std::string("point"));
    components->setField(lamp, "color", core::Vec3{1.0f, 0.45f, 0.15f});
    components->setField(lamp, "intensity", 6.0f);
    components->setField(lamp, "range", 10.0f);

    // …and a cool counter-glow inside the crystal cluster.
    const auto crystalLight = createEmpty("Crystal Light");
    objects->setLocalTransform(crystalLight, {{2.6f, 3.4f, 5.4f}, {}, {0.3f, 0.3f, 0.3f}});
    const auto glow = components->attach(crystalLight, "sky.light");
    components->setField(glow, "type", std::string("point"));
    components->setField(glow, "color", core::Vec3{0.35f, 0.85f, 1.0f});
    components->setField(glow, "intensity", 4.5f);
    components->setField(glow, "range", 9.0f);

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

    // Demo prefabs under Assets/Prefabs: a physics crate (from Crate A) and
    // a WASD-driven player pawn — the CrateRain demo game spawns both, and
    // the Project panel shows them as draggable assets.
    if (!roots_.empty()) {
        for (const auto root : roots_) {
            if (objects->nameOf(root) == "Crate A") {
                savePrefab(root, "assets://Prefabs/crate.skyprefab");
                break;
            }
        }
        const auto pawn = createPrimitive(scene::PrimitiveKind::Cube, "Player");
        const auto mover = components->attach(pawn, "sky.script");
        components->setField(mover, "class",
                             std::string("SkyEngine.Tests.WasdMover"));
        savePrefab(pawn, "assets://Prefabs/player.skyprefab");
        destroyObject(pawn); // only the prefab file remains
    }
}

} // namespace sky::editor
