#include <atomic>
#include <cmath>
#include <string>
#include <thread>

#include "sky/core/math.hpp"
#include "sky/core/runtime_services.hpp"
#include "sky_test.hpp"

namespace {

void testConfigService() {
    const auto config = sky::core::createInMemoryConfigService();
    config->set("engine.threads", "8");
    config->set("engine.vsync", "true");
    config->set("engine.name", "sky");

    CHECK(config->getInt("engine.threads") == 8);
    CHECK(config->getBool("engine.vsync") == true);
    CHECK(config->getString("engine.name") == "sky");
    CHECK(!config->getInt("engine.name").has_value());
    CHECK(!config->getString("missing").has_value());
}

void testEventBus() {
    const auto bus = sky::core::createEventBus();
    int received = 0;
    const auto subscription = bus->subscribe(
        "asset.imported", [&](const sky::core::EventEnvelope&) { ++received; });

    bus->publish({"asset.imported", nullptr});
    bus->publish({"asset.imported", nullptr});
    bus->publish({"other.topic", nullptr});
    CHECK(received == 2);

    bus->unsubscribe(subscription);
    bus->publish({"asset.imported", nullptr});
    CHECK(received == 2);
}

void testJobScheduler() {
    const auto scheduler = sky::core::createThreadPoolScheduler(2);

    std::atomic<int> value{0};
    const auto first = scheduler->schedule([&] { value = 1; });
    // The dependent job must observe the dependency's effect.
    std::atomic<bool> orderedCorrectly{false};
    const auto second =
        scheduler->scheduleAfter(first, [&] { orderedCorrectly = (value == 1); });

    scheduler->wait(second);
    CHECK(orderedCorrectly);

    std::atomic<int> counter{0};
    sky::core::JobHandle last;
    for (int i = 0; i < 50; ++i) {
        last = scheduler->schedule([&] { ++counter; });
    }
    scheduler->wait(last);
    // wait(last) only guarantees that specific job; drain the rest.
    for (int spin = 0; spin < 10000 && counter != 50; ++spin) {
        std::this_thread::yield();
    }
    CHECK(counter == 50);
}

void testDiagnostics() {
    const auto diagnostics = sky::core::createInMemoryDiagnostics();
    diagnostics->counter("frames", 1);
    diagnostics->counter("frames", 2);
    diagnostics->timingMicros("frame.time", 16000);

    CHECK(diagnostics->counterValue("frames") == 3);
    CHECK(diagnostics->lastTimingMicros("frame.time") == 16000);
    CHECK(diagnostics->counterValue("missing") == 0);
}

bool nearlyEqual(float a, float b) { return std::fabs(a - b) < 1e-4f; }

void testMath() {
    // 90 degrees around Y: (1,0,0) -> (0,0,-1).
    const float s = std::sqrt(0.5f);
    const sky::core::Quat yaw90{0.0f, s, 0.0f, s};
    const auto rotated = sky::core::rotate(yaw90, {1.0f, 0.0f, 0.0f});
    CHECK(nearlyEqual(rotated.x, 0.0f));
    CHECK(nearlyEqual(rotated.y, 0.0f));
    CHECK(nearlyEqual(rotated.z, -1.0f));

    // Child at parent origin offset; parent translated and scaled.
    const sky::core::Transform parent{{10.0f, 0.0f, 0.0f}, {}, {2.0f, 2.0f, 2.0f}};
    const sky::core::Transform child{{1.0f, 0.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}};
    const auto world = sky::core::compose(parent, child);
    CHECK(nearlyEqual(world.position.x, 12.0f));
    CHECK(nearlyEqual(world.scale.x, 2.0f));
}

} // namespace

int main() {
    testConfigService();
    testEventBus();
    testJobScheduler();
    testDiagnostics();
    testMath();
    return sky::test::summary("core_tests");
}
