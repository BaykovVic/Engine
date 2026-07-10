// End-to-end runtime slice: play mode controller -> scene runtime tick ->
// physics fixed step + object sync + ECS systems, per the architecture's
// "один runtime frame" data flow.

#include <cmath>
#include <filesystem>

#include "sky/component/component_world.hpp"
#include "sky/ecs/ecs_world.hpp"
#include "sky/editor/viewport/play_mode_controller.hpp"
#include "sky/object/object_world.hpp"
#include "sky/physics/physics_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/scene/scene_world.hpp"
#include "sky/serialization/backends.hpp"
#include "sky_test.hpp"

namespace {

struct Spin {
    float angle = 0.0f;
};

class SpinSystem final : public sky::ecs::IEcsSystem {
public:
    explicit SpinSystem(sky::ecs::EcsWorld& world) : world_(world) {}

    std::string name() const override { return "spin"; }

    void update(double deltaSeconds) override {
        for (const auto entity : world_.entitiesWith({typeid(Spin)})) {
            world_.storeFor<Spin>().get(entity)->angle +=
                90.0f * static_cast<float>(deltaSeconds);
        }
    }

private:
    sky::ecs::EcsWorld& world_;
};

void testPlayModeDrivesRuntime() {
    const auto objects = sky::object::createObjectWorld();
    const auto components = sky::component::createComponentWorld();
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);
    const auto ecs = sky::ecs::createEcsWorld();
    const auto physics = sky::physics::createPhysicsWorld();
    const auto sync = sky::physics::createObjectPhysicsSync(*physics, *objects);

    const auto scenes = sky::scene::createSceneWorld(
        {*objects, *objects, *objects, *components, *components, *storage,
         ecs.get(), physics.get(), sync.get()});

    // Scene content: a crate above a static floor, plus a spinning entity.
    const auto scene = scenes->createScene({"playtest", {}});
    const auto crate = objects->createObject("crate");
    objects->setLocalTransform(crate, {{0.0f, 3.0f, 0.0f}, {}, {1, 1, 1}});
    scenes->addRootObject(scene, crate);

    const auto crateBody =
        physics->createBody({sky::physics::BodyType::Dynamic, 1.0f, {}});
    physics->attachCollider(crateBody, {sky::physics::ColliderShape::Box,
                                        {0.5f, 0.5f, 0.5f}, 0.0f});
    sync->bind(crateBody, crate);

    const auto floor = physics->createBody(
        {sky::physics::BodyType::Static, 0.0f, {{0.0f, 0.0f, 0.0f}, {}, {1, 1, 1}}});
    physics->attachCollider(floor, {sky::physics::ColliderShape::Box,
                                    {50.0f, 0.5f, 50.0f}, 0.0f});

    const auto spinner = ecs->createEntity();
    ecs->storeFor<Spin>().set(spinner, {0.0f});
    SpinSystem spinSystem(*ecs);
    ecs->registerSystem(spinSystem);

    const auto controller = sky::editor::createPlayModeController(*scenes);

    // Without a scene there is nothing to play.
    CHECK(!controller->play());
    controller->setScene(scene);

    sky::editor::PlayModeState observed = sky::editor::PlayModeState::Editing;
    controller->onStateChanged([&](sky::editor::PlayModeState state) { observed = state; });

    // Ticking while editing must not run the simulation.
    controller->tickFrame(1.0);
    CHECK(objects->localTransform(crate).position.y == 3.0f);

    CHECK(controller->play());
    CHECK(observed == sky::editor::PlayModeState::Playing);
    CHECK(scenes->activeContext().state == sky::scene::SceneState::RuntimeActive);

    // Two simulated seconds: the crate falls and lands on the floor, the
    // spinner accumulates rotation.
    for (int i = 0; i < 120; ++i) {
        controller->tickFrame(1.0 / 60.0);
    }
    CHECK(std::fabs(objects->localTransform(crate).position.y - 1.0f) < 0.05f);
    CHECK(ecs->storeFor<Spin>().get(spinner)->angle > 170.0f);

    // Pause freezes the world.
    CHECK(controller->pause());
    const auto pausedAngle = ecs->storeFor<Spin>().get(spinner)->angle;
    controller->tickFrame(1.0);
    CHECK(ecs->storeFor<Spin>().get(spinner)->angle == pausedAngle);

    // Stop returns to editing and deactivates the scene.
    CHECK(controller->stop());
    CHECK(observed == sky::editor::PlayModeState::Editing);
    CHECK(scenes->activeContext().state == sky::scene::SceneState::Loaded);
    CHECK(!controller->pause());
}

} // namespace

int main() {
    testPlayModeDrivesRuntime();
    return sky::test::summary("runtime_tests");
}
