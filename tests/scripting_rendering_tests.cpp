#include <string>
#include <vector>

#include "sky/asset/asset_database.hpp"
#include "sky/rendering/null_renderer.hpp"
#include "sky/scripting/script_runtime.hpp"
#include "sky_test.hpp"

namespace {

/// Script host test double standing in for the .NET-backed Managed Runtime
/// Host. It records every call so the boundary's behaviour is observable.
class RecordingScriptHost final : public sky::scripting::IScriptHost {
public:
    struct Invocation {
        std::uint64_t instanceId;
        sky::scripting::ScriptLifecycleEvent event;
        double deltaSeconds;
    };

    bool start() override { return started_ = true; }
    void shutdown() override { started_ = false; }

    bool loadAssembly(const sky::scripting::AssemblyRef& assembly) override {
        assemblies_.push_back(assembly);
        return true;
    }

    std::vector<sky::scripting::AssemblyRef> loadedAssemblies() const override {
        return assemblies_;
    }

    std::uint64_t createInstance(const std::string& managedTypeName) override {
        if (managedTypeName.empty()) {
            return 0;
        }
        createdTypes_.push_back(managedTypeName);
        return nextInstanceId_++;
    }

    void destroyInstance(std::uint64_t managedInstanceId) override {
        destroyed_.push_back(managedInstanceId);
    }

    bool invokeLifecycle(std::uint64_t managedInstanceId,
                         sky::scripting::ScriptLifecycleEvent event,
                         double deltaSeconds) override {
        invocations_.push_back({managedInstanceId, event, deltaSeconds});
        return true;
    }

    bool started_ = false;
    std::uint64_t nextInstanceId_ = 100;
    std::vector<sky::scripting::AssemblyRef> assemblies_;
    std::vector<std::string> createdTypes_;
    std::vector<std::uint64_t> destroyed_;
    std::vector<Invocation> invocations_;
};

void testScriptingBoundary() {
    RecordingScriptHost host;
    const auto runtime = sky::scripting::createScriptRuntime(host);

    runtime->registerBinding({"sky.script.player", "Game.Player", "Game.dll"});
    CHECK(runtime->bindingFor("sky.script.player").has_value());
    CHECK(runtime->bindingFor("sky.script.player")->managedTypeName == "Game.Player");
    CHECK(!runtime->bindingFor("sky.script.enemy").has_value());

    // Binding creates the managed peer and hands out a native handle.
    constexpr std::uint64_t kNativeObjectId = 4242;
    const auto handle = runtime->bindInstance("sky.script.player", kNativeObjectId);
    CHECK(handle.isValid());
    CHECK(host.createdTypes_ == std::vector<std::string>{"Game.Player"});
    // The handle resolves back to the native object — the only identity the
    // managed side ever sees.
    CHECK(runtime->resolve(handle) == kNativeObjectId);

    // Binding an unregistered type fails without touching the host.
    CHECK(!runtime->bindInstance("sky.script.enemy", 1).isValid());
    CHECK(host.createdTypes_.size() == 1);

    // Lifecycle callbacks reach the managed peer through the bridge.
    runtime->dispatch(handle, sky::scripting::ScriptLifecycleEvent::OnCreate, 0.0);
    runtime->dispatch(handle, sky::scripting::ScriptLifecycleEvent::OnUpdate, 1.0 / 60.0);
    CHECK(host.invocations_.size() == 2);
    CHECK(host.invocations_[0].event == sky::scripting::ScriptLifecycleEvent::OnCreate);
    CHECK(host.invocations_[1].event == sky::scripting::ScriptLifecycleEvent::OnUpdate);
    CHECK(host.invocations_[0].instanceId == 100);

    // Unbinding destroys the managed peer and invalidates the handle.
    runtime->unbindInstance(handle);
    CHECK(host.destroyed_ == std::vector<std::uint64_t>{100});
    CHECK(!runtime->resolve(handle).has_value());
    runtime->dispatch(handle, sky::scripting::ScriptLifecycleEvent::OnDestroy, 0.0);
    CHECK(host.invocations_.size() == 2); // no call for a dead handle
}

void testNullRenderer() {
    const auto renderer = sky::rendering::createNullRenderer();
    CHECK(renderer->backendName() == "null");

    const auto surface = sky::rendering::createOffscreenSurface(640, 360);
    CHECK(surface->width() == 640);
    renderer->attachSurface(*surface);

    // Resources come from assets; invalid assets are rejected.
    const auto mesh = renderer->createFromAsset(
        sky::asset::assetIdFromPath("models/crate.mesh"),
        sky::rendering::RenderResourceType::Mesh);
    CHECK(mesh.isValid());
    CHECK(!renderer->createFromAsset({}, sky::rendering::RenderResourceType::Mesh)
               .isValid());
    CHECK(renderer->liveResourceCount() == 1);

    // A frame consumes exactly the commands submitted for it.
    const sky::rendering::RenderCommand commands[] = {
        {sky::rendering::RenderCommandType::BeginFrame, {}, {}, 640, 360},
        {sky::rendering::RenderCommandType::DrawMesh, mesh, {}, 0, 0},
        {sky::rendering::RenderCommandType::EndFrame, {}, {}, 0, 0},
    };
    renderer->submit(commands);
    renderer->renderFrame();
    CHECK(renderer->frameCount() == 1);
    CHECK(renderer->commandsInLastFrame() == 3);

    // The queue is drained between frames.
    renderer->renderFrame();
    CHECK(renderer->frameCount() == 2);
    CHECK(renderer->commandsInLastFrame() == 0);

    renderer->destroy(mesh);
    CHECK(renderer->liveResourceCount() == 0);
}

} // namespace

int main() {
    testScriptingBoundary();
    testNullRenderer();
    return sky::test::summary("scripting_rendering_tests");
}
