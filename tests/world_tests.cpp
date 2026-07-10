#include <cmath>
#include <typeindex>

#include "sky/component/component_world.hpp"
#include "sky/ecs/ecs_world.hpp"
#include "sky/object/object_world.hpp"
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

} // namespace

int main() {
    testObjectHierarchy();
    testWorldTransformWriteback();
    testComponentWorld();
    testEcsWorld();
    return sky::test::summary("world_tests");
}
