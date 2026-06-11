#include <unordered_map>

#include "sky/scripting/script_runtime.hpp"

namespace sky::scripting {
namespace {

class ScriptRuntimeImpl final : public ScriptRuntime {
public:
    explicit ScriptRuntimeImpl(IScriptHost& host) : host_(host) {}

    // IScriptBindingService

    void registerBinding(const ManagedTypeBinding& binding) override {
        bindings_[binding.nativeTypeId] = binding;
    }

    std::optional<ManagedTypeBinding> bindingFor(
        const std::string& nativeTypeId) const override {
        const auto it = bindings_.find(nativeTypeId);
        if (it == bindings_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    NativeHandle bindInstance(const std::string& nativeTypeId,
                              std::uint64_t nativeObjectId) override {
        const auto binding = bindingFor(nativeTypeId);
        if (!binding) {
            return NativeHandle::invalid();
        }
        const auto managedInstanceId = host_.createInstance(binding->managedTypeName);
        if (managedInstanceId == 0) {
            return NativeHandle::invalid();
        }
        const auto handle = allocate(nativeObjectId);
        managedPeers_[handle.value] = managedInstanceId;
        return handle;
    }

    void unbindInstance(NativeHandle handle) override {
        const auto it = managedPeers_.find(handle.value);
        if (it == managedPeers_.end()) {
            return;
        }
        host_.destroyInstance(it->second);
        managedPeers_.erase(it);
        release(handle);
    }

    // IScriptLifecycleBridge

    void dispatch(NativeHandle instance, ScriptLifecycleEvent event,
                  double deltaSeconds) override {
        const auto it = managedPeers_.find(instance.value);
        if (it == managedPeers_.end()) {
            return;
        }
        // The host reports managed failures via the return value; the
        // boundary isolates them instead of propagating into native code.
        host_.invokeLifecycle(it->second, event, deltaSeconds);
    }

    void dispatchAll(ScriptLifecycleEvent event, double deltaSeconds) override {
        for (const auto& [handle, managedInstanceId] : managedPeers_) {
            host_.invokeLifecycle(managedInstanceId, event, deltaSeconds);
        }
    }

    // INativeHandleRegistry

    NativeHandle allocate(std::uint64_t nativeObjectId) override {
        const NativeHandle handle{nextId_++};
        nativeObjects_[handle.value] = nativeObjectId;
        return handle;
    }

    void release(NativeHandle handle) override { nativeObjects_.erase(handle.value); }

    std::optional<std::uint64_t> resolve(NativeHandle handle) const override {
        const auto it = nativeObjects_.find(handle.value);
        if (it == nativeObjects_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

private:
    IScriptHost& host_;
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::string, ManagedTypeBinding> bindings_;
    std::unordered_map<std::uint64_t, std::uint64_t> nativeObjects_;
    std::unordered_map<std::uint64_t, std::uint64_t> managedPeers_;
};

} // namespace

std::unique_ptr<ScriptRuntime> createScriptRuntime(IScriptHost& host) {
    return std::make_unique<ScriptRuntimeImpl>(host);
}

} // namespace sky::scripting
