#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "sky/core/handle.hpp"

namespace sky::scripting {

struct NativeTag {};
/// The only currency that crosses the C++ <-> C# boundary. The managed side
/// holds NativeHandles; authoritative state lives exclusively on the native
/// side and is mutated only through engine API calls.
using NativeHandle = core::Handle<NativeTag>;

/// Binding between a native component type and its managed peer type.
struct ManagedTypeBinding {
    std::string nativeTypeId;
    std::string managedTypeName;
    std::string assemblyName;
};

/// Lifecycle callbacks bridged to managed script instances.
enum class ScriptLifecycleEvent {
    OnCreate,
    OnStart,
    OnUpdate,
    OnFixedUpdate,
    OnDestroy,
};

/// Scripting Boundary contract: registry of native <-> managed type
/// bindings (the BindingRegistry).
class IScriptBindingService {
public:
    virtual ~IScriptBindingService() = default;

    virtual void registerBinding(const ManagedTypeBinding& binding) = 0;
    [[nodiscard]] virtual std::optional<ManagedTypeBinding> bindingFor(
        const std::string& nativeTypeId) const = 0;
    /// Creates the managed peer for a native object/component and returns
    /// the handle the managed side will use.
    virtual NativeHandle bindInstance(const std::string& nativeTypeId,
                                      std::uint64_t nativeObjectId) = 0;
    virtual void unbindInstance(NativeHandle handle) = 0;
};

/// Scripting Boundary contract: dispatch of lifecycle callbacks into the
/// managed runtime. Managed failures are isolated and never corrupt native
/// state.
class IScriptLifecycleBridge {
public:
    virtual ~IScriptLifecycleBridge() = default;

    virtual void dispatch(NativeHandle instance, ScriptLifecycleEvent event,
                          double deltaSeconds) = 0;
    /// Dispatches the event to every bound script instance (the per-frame
    /// Update/FixedUpdate fan-out of the runtime loop).
    virtual void dispatchAll(ScriptLifecycleEvent event, double deltaSeconds) = 0;
};

/// Scripting Boundary contract: the native handle table mapping handles to
/// native object identity.
class INativeHandleRegistry {
public:
    virtual ~INativeHandleRegistry() = default;

    virtual NativeHandle allocate(std::uint64_t nativeObjectId) = 0;
    virtual void release(NativeHandle handle) = 0;
    [[nodiscard]] virtual std::optional<std::uint64_t> resolve(NativeHandle handle) const = 0;
};

} // namespace sky::scripting
