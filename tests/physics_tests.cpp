#include <cmath>

#include "sky/object/object_world.hpp"
#include "sky/physics/physics_world.hpp"
#include "sky_test.hpp"

namespace {

bool nearly(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

void testGravityIntegration() {
    const auto physics = sky::physics::createPhysicsWorld();
    const auto body = physics->createBody(
        {sky::physics::BodyType::Dynamic, 1.0f, {{0.0f, 100.0f, 0.0f}, {}, {1, 1, 1}}});

    for (int i = 0; i < 60; ++i) {
        physics->step(1.0 / 60.0);
    }

    // After one second of free fall: v = g*t ~ -9.81, y dropped by ~g/2*t^2
    // (slightly more with explicit Euler).
    CHECK(nearly(physics->bodyVelocity(body).y, -9.81f, 0.05f));
    const auto y = physics->bodyTransform(body).position.y;
    CHECK(y < 96.0f && y > 94.0f);

    // Static bodies do not integrate.
    const auto floor = physics->createBody(
        {sky::physics::BodyType::Static, 0.0f, {{0.0f, 0.0f, 0.0f}, {}, {1, 1, 1}}});
    physics->step(1.0 / 60.0);
    CHECK(physics->bodyTransform(floor).position.y == 0.0f);
}

void testCollisionAndResolution() {
    const auto physics = sky::physics::createPhysicsWorld();

    const auto floor = physics->createBody(
        {sky::physics::BodyType::Static, 0.0f, {{0.0f, 0.0f, 0.0f}, {}, {1, 1, 1}}});
    physics->attachCollider(floor, {sky::physics::ColliderShape::Box,
                                    {50.0f, 0.5f, 50.0f}, 0.0f});

    const auto crate = physics->createBody(
        {sky::physics::BodyType::Dynamic, 1.0f, {{0.0f, 3.0f, 0.0f}, {}, {1, 1, 1}}});
    physics->attachCollider(crate, {sky::physics::ColliderShape::Box,
                                    {0.5f, 0.5f, 0.5f}, 0.0f});

    bool collided = false;
    for (int i = 0; i < 240; ++i) {
        physics->step(1.0 / 60.0);
        if (!physics->drainCollisionEvents().empty()) {
            collided = true;
        }
    }

    CHECK(collided);
    // The crate must come to rest on top of the floor: 0.5 (floor top) + 0.5
    // (crate half height) = 1.0.
    CHECK(nearly(physics->bodyTransform(crate).position.y, 1.0f, 0.05f));
    CHECK(nearly(physics->bodyVelocity(crate).y, 0.0f, 0.5f));
}

void testRaycast() {
    const auto physics = sky::physics::createPhysicsWorld();
    const auto body = physics->createBody(
        {sky::physics::BodyType::Static, 0.0f, {{0.0f, 0.0f, 10.0f}, {}, {1, 1, 1}}});
    const auto collider = physics->attachCollider(
        body, {sky::physics::ColliderShape::Box, {1.0f, 1.0f, 1.0f}, 0.0f});

    const auto hit = physics->raycast({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 100.0f);
    CHECK(hit.has_value());
    CHECK(hit->collider == collider);
    CHECK(nearly(hit->distance, 9.0f));
    CHECK(nearly(hit->normal.z, -1.0f));

    CHECK(!physics->raycast({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 100.0f).has_value());
    CHECK(!physics->raycast({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 5.0f).has_value());
}

void testObjectSync() {
    const auto objects = sky::object::createObjectWorld();
    const auto physics = sky::physics::createPhysicsWorld();
    const auto sync = sky::physics::createObjectPhysicsSync(*physics, *objects);

    const auto crate = objects->createObject("crate");
    objects->setLocalTransform(crate, {{0.0f, 10.0f, 0.0f}, {}, {1, 1, 1}});
    const auto body = physics->createBody({sky::physics::BodyType::Dynamic, 1.0f, {}});
    sync->bind(body, crate);

    // Object transform flows into physics before the step…
    sync->pushKinematicState();
    CHECK(nearly(physics->bodyTransform(body).position.y, 10.0f));

    // …and simulated motion flows back to the object after it.
    physics->step(1.0 / 60.0);
    sync->pullSimulationResults();
    CHECK(objects->localTransform(crate).position.y < 10.0f);

    sync->unbind(body);
}

} // namespace

int main() {
    testGravityIntegration();
    testCollisionAndResolution();
    testRaycast();
    testObjectSync();
    return sky::test::summary("physics_tests");
}
