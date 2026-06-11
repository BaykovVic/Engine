#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sky::scripting {

/// Identifies a loaded managed assembly inside the host.
struct AssemblyRef {
    std::string assemblyName;
    std::filesystem::path path;
};

enum class ReloadPolicy {
    /// Reload only outside play mode.
    EditTimeOnly,
    /// Tear down and recreate the managed domain on reload.
    FullDomainReload,
};

/// Native-side contract of the Managed Runtime Host. The host loads
/// assemblies and executes managed code; it never owns engine state.
class IScriptHost {
public:
    virtual ~IScriptHost() = default;

    virtual bool start() = 0;
    virtual void shutdown() = 0;

    virtual bool loadAssembly(const AssemblyRef& assembly) = 0;
    [[nodiscard]] virtual std::vector<AssemblyRef> loadedAssemblies() const = 0;

    /// Creates a managed instance of the given type; returns an opaque
    /// managed instance id used by the lifecycle bridge.
    virtual std::uint64_t createInstance(const std::string& managedTypeName) = 0;
    virtual void destroyInstance(std::uint64_t managedInstanceId) = 0;
};

/// Native-side contract: when and how the managed domain may be reloaded.
class IDomainReloadPolicy {
public:
    virtual ~IDomainReloadPolicy() = default;

    [[nodiscard]] virtual ReloadPolicy policy() const = 0;
    [[nodiscard]] virtual bool canReloadNow() const = 0;
    virtual void requestReload() = 0;
};

} // namespace sky::scripting
