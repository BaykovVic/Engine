// The real Managed Runtime Host: .NET hosted via hostfxr behind the same
// IScriptHost contract as the test double. The final case closes the whole
// loop from the architecture docs: a scene tick on the native side drives
// OnUpdate of a real C# ScriptComponent.

#include <filesystem>

#include "sky/component/component_world.hpp"
#include "sky/object/object_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/scene/scene_world.hpp"
#include "sky/scripting/dotnet_host.hpp"
#include "sky/scripting/script_runtime.hpp"
#include "sky/serialization/backends.hpp"
#include "sky_test.hpp"

namespace {

const std::filesystem::path kManagedDir = SKY_MANAGED_DIR;

std::unique_ptr<sky::scripting::DotNetScriptHost> startHost() {
    sky::scripting::DotNetHostConfig config;
    config.bootstrapAssembly = kManagedDir / "SkyEngine.Managed.dll";
    auto host = sky::scripting::createDotNetScriptHost(config);
    CHECK(host != nullptr);
    if (host != nullptr) {
        CHECK(host->start());
        CHECK(host->loadAssembly(
            {"SkyEngine.TestScripts", kManagedDir / "SkyEngine.TestScripts.dll"}));
    }
    return host;
}

void testLifecycleThroughRealDotNet() {
    const auto host = startHost();
    if (host == nullptr) {
        return;
    }

    // Instance creation by managed type name; unknown types fail cleanly.
    const auto spinner = host->createInstance("SkyEngine.Tests.Spinner");
    CHECK(spinner != 0);
    CHECK(host->createInstance("SkyEngine.Tests.DoesNotExist") == 0);
    CHECK(host->loadedAssemblies().size() == 1);

    // Lifecycle callbacks execute real C# (Probe = updates + 1000*created
    // + 10000*started).
    CHECK(host->invokeLifecycle(spinner, sky::scripting::ScriptLifecycleEvent::OnCreate,
                                0.0));
    CHECK(host->invokeLifecycle(spinner, sky::scripting::ScriptLifecycleEvent::OnStart,
                                0.0));
    CHECK(host->invokeLifecycle(spinner, sky::scripting::ScriptLifecycleEvent::OnUpdate,
                                1.0 / 60.0));
    CHECK(host->invokeLifecycle(spinner, sky::scripting::ScriptLifecycleEvent::OnUpdate,
                                1.0 / 60.0));
    CHECK(host->probeValue(spinner) == 11002);

    // Managed exceptions are reported, isolated and non-fatal.
    const auto faulty = host->createInstance("SkyEngine.Tests.Faulty");
    CHECK(faulty != 0);
    CHECK(!host->invokeLifecycle(faulty, sky::scripting::ScriptLifecycleEvent::OnUpdate,
                                 0.0));
    // Native side is still alive and other instances unaffected.
    CHECK(host->probeValue(spinner) == 11002);

    // Destroyed instances stop responding.
    host->destroyInstance(spinner);
    CHECK(!host->invokeLifecycle(spinner, sky::scripting::ScriptLifecycleEvent::OnUpdate,
                                 0.0));
    CHECK(host->probeValue(spinner) == -1);
}

void testSceneTickDrivesCSharp() {
    const auto host = startHost();
    if (host == nullptr) {
        return;
    }

    // Assemble the full native stack: scripting boundary over the real
    // host, scene world dispatching script callbacks per frame.
    const auto runtime = sky::scripting::createScriptRuntime(*host);
    runtime->registerBinding(
        {"sky.script.spinner", "SkyEngine.Tests.Spinner", "SkyEngine.TestScripts"});

    const auto objects = sky::object::createObjectWorld();
    const auto components = sky::component::createComponentWorld();
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage = sky::serialization::createFileSerializationBackend(*fileSystem);

    sky::scene::SceneWorldDeps deps{*objects,    *objects,   *objects,
                                    *components, *components, *storage};
    deps.scriptBridge = runtime.get();
    const auto scenes = sky::scene::createSceneWorld(deps);

    const auto scene = scenes->createScene({"scripted", {}});
    const auto hero = objects->createObject("hero");
    scenes->addRootObject(scene, hero);

    // The CLR is process-wide, so managed instance ids continue across
    // hosts; learn the next id with a sacrificial instance.
    const auto probeId = host->createInstance("SkyEngine.Tests.Spinner");
    host->destroyInstance(probeId);
    const auto expectedId = probeId + 1;

    // Bind the script component: the boundary creates the managed peer.
    const auto handle = runtime->bindInstance("sky.script.spinner", hero.value);
    CHECK(handle.isValid());
    CHECK(*runtime->resolve(handle) == hero.value);
    runtime->dispatch(handle, sky::scripting::ScriptLifecycleEvent::OnCreate, 0.0);
    CHECK(host->probeValue(expectedId) == 1000); // created, no updates yet

    // Three engine frames -> three C# OnUpdate calls.
    scenes->activate(scene);
    scenes->tick(1.0 / 60.0);
    scenes->tick(1.0 / 60.0);
    scenes->tick(1.0 / 60.0);
    CHECK(host->probeValue(expectedId) == 1003);

    // Unbinding tears the managed peer down through the boundary.
    runtime->unbindInstance(handle);
    CHECK(host->probeValue(expectedId) == -1);
}

} // namespace

int main() {
    testLifecycleThroughRealDotNet();
    testSceneTickDrivesCSharp();
    return sky::test::summary("dotnet_host_tests");
}
