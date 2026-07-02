#pragma once

#include <filesystem>
#include <memory>

#include "sky/scripting/script_host.hpp"

namespace sky::scripting {

/// Configuration of the .NET-backed Managed Runtime Host.
struct DotNetHostConfig {
    /// Path to libhostfxr; empty = discover under standard dotnet roots
    /// (DOTNET_ROOT, /usr/lib/dotnet, /usr/share/dotnet).
    std::filesystem::path hostfxrPath;
    /// The bootstrap assembly (SkyEngine.Managed.dll) and its
    /// runtimeconfig.json next to it.
    std::filesystem::path bootstrapAssembly;
};

/// The real Managed Runtime Host: hosts the .NET runtime through hostfxr
/// and dispatches into SkyEngine.Bootstrap. Implements the same IScriptHost
/// contract the test double does — the Scripting Boundary cannot tell them
/// apart, and the host never owns engine state.
class DotNetScriptHost : public IScriptHost {
public:
    ~DotNetScriptHost() override = default;

    /// Test/diagnostic readback: the IProbe value of a managed instance
    /// (-1 when the instance does not expose one).
    [[nodiscard]] virtual std::int64_t probeValue(std::uint64_t managedInstanceId) = 0;

    /// Installs the reverse-boundary engine API (a table of native function
    /// pointers scripts call to affect their object). Passed once at startup.
    virtual void installEngineApi(const void* apiTable) = 0;

    /// Binds a live managed instance to the native object id it drives, so the
    /// script's transform helpers act on that object.
    virtual void setInstanceObjectId(std::uint64_t managedInstanceId,
                                     std::uint64_t objectId) = 0;

    /// Publishes frame timing to the managed side (SkyEngine.Time). Call once
    /// per play frame, before dispatching the frame's lifecycle events.
    virtual void beginFrame(double totalSeconds, double deltaSeconds) = 0;
};

/// Returns nullptr when hostfxr cannot be located on this machine.
std::unique_ptr<DotNetScriptHost> createDotNetScriptHost(const DotNetHostConfig& config);

} // namespace sky::scripting
