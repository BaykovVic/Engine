#include <cmath>
#include <filesystem>
#include <typeindex>

#include "sky/component/component_world.hpp"
#include "sky/component/data_asset.hpp"
#include "sky/ecs/ecs_world.hpp"
#include "sky/object/object_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/serialization/backends.hpp"
#include "sky_test.hpp"

namespace {

void testWorldTransformWriteback() {
    const auto world = sky::object::createObjectWorld();
    const auto parent = world->createObject("parent");
    const auto child = world->createObject("child");
    world->setParent(child, parent);

    // Parent is translated, rotated 90 deg about Y and scaled 2x — exercises
    // every term of the world->local conversion.
    const float s = std::sin(3.14159265f / 4.0f);
    world->setLocalTransform(
        parent, {{5.0f, 1.0f, -2.0f}, {0.0f, s, 0.0f, s}, {2.0f, 2.0f, 2.0f}});

    // Place the child at a known world transform and read it back.
    const sky::core::Transform desired{{1.0f, 2.0f, 3.0f},
                                       {0.0f, 0.0f, 0.0f, 1.0f},
                                       {1.0f, 1.0f, 1.0f}};
    sky::object::setWorldTransform(*world, child, desired);

    const auto got = world->worldTransform(child);
    CHECK(std::fabs(got.position.x - 1.0f) < 1e-3f);
    CHECK(std::fabs(got.position.y - 2.0f) < 1e-3f);
    CHECK(std::fabs(got.position.z - 3.0f) < 1e-3f);
    CHECK(std::fabs(got.scale.x - 1.0f) < 1e-3f);

    // A root takes the world transform directly as its local.
    const auto root = world->createObject("root");
    sky::object::setWorldTransform(*world, root,
                                   {{7.0f, 8.0f, 9.0f}, {}, {1.0f, 1.0f, 1.0f}});
    CHECK(world->worldTransform(root).position.x == 7.0f);
}

void testObjectHierarchy() {
    const auto world = sky::object::createObjectWorld();

    const auto root = world->createObject("root");
    const auto child = world->createObject("child");
    const auto grandchild = world->createObject("grandchild");
    world->setParent(child, root);
    world->setParent(grandchild, child);

    CHECK(world->parentOf(child) == root);
    CHECK(world->childrenOf(root).size() == 1);
    CHECK(world->findByName("child").size() == 1);

    // Reparenting to a descendant would create a cycle and must be refused.
    world->setParent(root, grandchild);
    CHECK(!world->parentOf(root).isValid());

    // World transform composes down the chain.
    world->setLocalTransform(root, {{1.0f, 0.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}});
    world->setLocalTransform(child, {{2.0f, 0.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}});
    CHECK(world->worldTransform(child).position.x == 3.0f);

    // Destroying a parent destroys the subtree.
    world->destroyObject(root);
    CHECK(!world->exists(root));
    CHECK(!world->exists(child));
    CHECK(!world->exists(grandchild));
}

void testComponentWorld() {
    const auto objects = sky::object::createObjectWorld();
    const auto components = sky::component::createComponentWorld();

    components->registerComponentType({"sky.mesh", "Mesh", false, "", {}});
    components->registerComponentType(
        {"sky.script", "Script", true, "Game.Player", {{"speed", "float"}}});
    CHECK(components->availableTypes().size() == 2);

    const auto object = objects->createObject("player");
    const auto mesh = components->attach(object, "sky.mesh");
    const auto script = components->attach(object, "sky.script");
    CHECK(mesh.isValid());
    CHECK(script.isValid());
    // Unregistered types cannot be attached.
    CHECK(!components->attach(object, "sky.unknown").isValid());

    CHECK(components->componentsOf(object).size() == 2);
    CHECK(components->ownerOf(mesh) == object);
    CHECK(components->descriptorOf(script).isScriptComponent);
    CHECK(components->descriptorOf(script).managedTypeName == "Game.Player");

    components->detach(mesh);
    CHECK(components->componentsOf(object).size() == 1);
    components->detachAllFrom(object);
    CHECK(components->componentsOf(object).empty());
}

struct Velocity {
    float x = 0.0f;
};

struct Position {
    float x = 0.0f;
};

class MoveSystem final : public sky::ecs::IEcsSystem {
public:
    explicit MoveSystem(sky::ecs::EcsWorld& world) : world_(world) {}

    std::string name() const override { return "move"; }

    void update(double deltaSeconds) override {
        auto& positions = world_.storeFor<Position>();
        auto& velocities = world_.storeFor<Velocity>();
        for (const auto entity :
             world_.entitiesWith({typeid(Position), typeid(Velocity)})) {
            positions.get(entity)->x +=
                velocities.get(entity)->x * static_cast<float>(deltaSeconds);
        }
    }

private:
    sky::ecs::EcsWorld& world_;
};

void testEcsWorld() {
    const auto world = sky::ecs::createEcsWorld();

    const auto mover = world->createEntity();
    const auto bystander = world->createEntity();
    world->storeFor<Position>().set(mover, {0.0f});
    world->storeFor<Velocity>().set(mover, {10.0f});
    world->storeFor<Position>().set(bystander, {5.0f});

    CHECK(world->isAlive(mover));
    CHECK(world->entitiesWith({typeid(Position)}).size() == 2);
    CHECK(world->entitiesWith({typeid(Position), typeid(Velocity)}).size() == 1);

    MoveSystem system(*world);
    world->registerSystem(system);
    world->tick(0.5);
    CHECK(world->storeFor<Position>().get(mover)->x == 5.0f);
    CHECK(world->storeFor<Position>().get(bystander)->x == 5.0f);

    world->destroyEntity(mover);
    CHECK(!world->isAlive(mover));
    CHECK(!world->storeFor<Position>().has(mover));
    CHECK(world->entitiesWith({typeid(Position)}).size() == 1);

    world->unregisterSystem(system);
}

class OrderProbeSystem final : public sky::ecs::IEcsSystem {
public:
    OrderProbeSystem(std::string name, std::vector<std::string>& log)
        : name_(std::move(name)), log_(log) {}

    std::string name() const override { return name_; }
    void update(double) override { log_.push_back(name_); }

private:
    std::string name_;
    std::vector<std::string>& log_;
};

void testEcsSystemOrderIsRegistrationOrder() {
    // Determinism contract: systems tick in registration order, every frame,
    // regardless of names or registration count. Simulation code may rely on
    // "A registered before B" as an ordering guarantee.
    const auto world = sky::ecs::createEcsWorld();
    std::vector<std::string> log;
    OrderProbeSystem zulu("zulu", log);
    OrderProbeSystem alpha("alpha", log);
    OrderProbeSystem mike("mike", log);
    world->registerSystem(zulu);
    world->registerSystem(alpha);
    world->registerSystem(mike);

    world->tick(0.016);
    world->tick(0.016);
    const std::vector<std::string> expected{"zulu", "alpha", "mike",
                                            "zulu", "alpha", "mike"};
    CHECK(log == expected);

    // Unregistering keeps the relative order of the survivors.
    world->unregisterSystem(alpha);
    log.clear();
    world->tick(0.016);
    CHECK((log == std::vector<std::string>{"zulu", "mike"}));

    world->unregisterSystem(zulu);
    world->unregisterSystem(mike);
}

} // namespace

void testDataAsset() {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "sky_engine_tests" / "data_asset";
    fs::remove_all(root);
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage =
        sky::serialization::createFileSerializationBackend(*fileSystem);

    // Round trip: every field type plus the inheritance pointer.
    sky::component::DataAssetDesc enemy;
    enemy.typeId = "game.enemy";
    enemy.parentGuid = 0xABCDEFu;
    enemy.fields["health"] = 150.0f;
    enemy.fields["lives"] = std::int64_t{3};
    enemy.fields["boss"] = true;
    enemy.fields["model"] = std::string("assets://Models/grunt.obj");
    enemy.fields["tint"] = sky::core::Vec3{1.0f, 0.4f, 0.2f};
    const auto path = root / "grunt.skydata";
    CHECK(sky::component::saveDataAsset(*storage, path, enemy));

    const auto loaded = sky::component::loadDataAsset(*storage, path);
    CHECK(loaded.has_value());
    if (loaded) {
        CHECK(loaded->typeId == "game.enemy");
        CHECK(loaded->parentGuid == 0xABCDEFu);
        CHECK(loaded->fields.size() == 5);
        CHECK(std::get<float>(loaded->fields.at("health")) == 150.0f);
        CHECK(std::get<bool>(loaded->fields.at("boss")));
        CHECK(std::get<std::string>(loaded->fields.at("model")) ==
              "assets://Models/grunt.obj");
        CHECK(std::get<sky::core::Vec3>(loaded->fields.at("tint")).y == 0.4f);
    }

    // A foreign/corrupt file is rejected, not misread.
    fileSystem->writeAll(root / "junk.skydata", {std::byte{0x42}});
    CHECK(!sky::component::loadDataAsset(*storage, root / "junk.skydata")
               .has_value());

    // Inheritance merge: overrides win, base-only fields shine through.
    std::map<std::string, sky::component::FieldValue> base{
        {"health", 100.0f}, {"speed", 5.0f}};
    std::map<std::string, sky::component::FieldValue> overrides{
        {"health", 150.0f}};
    const auto merged = sky::component::mergedFields(base, overrides);
    CHECK(std::get<float>(merged.at("health")) == 150.0f);
    CHECK(std::get<float>(merged.at("speed")) == 5.0f);

    fs::remove_all(root);
}

int main() {
    testObjectHierarchy();
    testWorldTransformWriteback();
    testComponentWorld();
    testEcsWorld();
    testEcsSystemOrderIsRegistrationOrder();
    testDataAsset();
    return sky::test::summary("world_tests");
}
