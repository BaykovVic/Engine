#include "editor_context.hpp"

#include <algorithm>
#include <filesystem>

#include "sky/rendering_opengl/opengl_backend.hpp"
#include "sky/terrain/terrain_integration.hpp"

namespace {
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
    vfs->mount("assets",
               platform::createDirectoryMount(*fileSystem, projectRoot / "docs", true),
               0);

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
         {{"material", "string"}, {"mesh", "string"}}});
    components->registerComponentType(
        {"sky.collider.box", "Box Collider", false, "", {{"halfExtents", "Vec3"}}});
    components->registerComponentType(
        {"sky.rigidbody", "Rigidbody", false, "", {{"mass", "float"}}});
    components->registerComponentType(
        {"sky.script", "Script", true, "Game.Behaviour", {}});
    components->registerComponentType(
        {"sky.light", "Light", false, "",
         {{"type", "string"},
          {"color", "Vec3"},
          {"intensity", "float"},
          {"range", "float"}}});

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

    buildDemoScene();
}

object::ObjectHandle EditorContext::createEmpty(const std::string& name) {
    const auto object = objects->createObject(name);
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
    const auto collider = components->attach(crate, "sky.collider.box");
    components->setField(collider, "halfExtents", core::Vec3{0.5f, 0.5f, 0.5f});
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
        components->attach(copy, components->descriptorOf(component).typeId);
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

ObjectSnapshot EditorContext::snapshotObject(object::ObjectHandle object) const {
    ObjectSnapshot snapshot;
    snapshot.name = objects->nameOf(object);
    snapshot.local = objects->localTransform(object);
    snapshot.hasPhysicsBody = hasPhysicsBody(object);
    for (const auto component : components->componentsOf(object)) {
        snapshot.componentTypes.push_back(components->descriptorOf(component).typeId);
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
    for (const auto& typeId : snapshot.componentTypes) {
        components->attach(object, typeId);
    }
    if (snapshot.hasPhysicsBody) {
        attachCrateBody(object);
    }
    for (const auto& child : snapshot.children) {
        restoreObject(child, object);
    }
    return object;
}

void EditorContext::buildDemoScene() {
    activeScene = scenes->createScene({"SampleScene", {}});

    // The ground is a real terrain: heightfield data, physics collider and
    // a scene object carrying its world placement.
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

    // An imported OBJ model: written to disk, run through the asset
    // pipeline and referenced by the mesh component.
    const auto objPath =
        std::filesystem::temp_directory_path() / "sky_editor_assets" / "pyramid.obj";
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
        components->setField(mesh, "mesh", objPath.generic_string());
    }

    const auto camera = createEmpty("Main Camera");
    // Positioned behind the scene, yawed 180 degrees to face it (cameras
    // look along their local -Z).
    objects->setLocalTransform(camera,
                               {{0.0f, 2.5f, -12.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                                {1, 1, 1}});
}

} // namespace sky::editor
