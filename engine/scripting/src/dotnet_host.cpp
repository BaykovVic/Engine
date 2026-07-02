// Hosts the .NET runtime via hostfxr. The handful of hostfxr declarations
// used here are written out directly (POSIX char_t variant), so the module
// needs no .NET SDK headers — only a dotnet runtime on the machine.

#include <cstdint>
#include <vector>

#include "sky/scripting/dotnet_host.hpp"

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#define SKY_HAS_DLOPEN 1
#endif

namespace sky::scripting {

#ifdef SKY_HAS_DLOPEN

namespace {

// --- Minimal hostfxr surface (POSIX: char_t == char) -------------------------

using hostfxr_handle = void*;
using hostfxr_initialize_fn = std::int32_t (*)(const char* runtimeConfigPath,
                                               const void* parameters,
                                               hostfxr_handle* hostContext);
using hostfxr_get_delegate_fn = std::int32_t (*)(hostfxr_handle, std::int32_t type,
                                                 void** delegateOut);
using hostfxr_close_fn = std::int32_t (*)(hostfxr_handle);

constexpr std::int32_t kHdtLoadAssemblyAndGetFunctionPointer = 5;
const char* const kUnmanagedCallersOnly = reinterpret_cast<const char*>(-1);

using load_assembly_and_get_function_pointer_fn =
    std::int32_t (*)(const char* assemblyPath, const char* typeName,
                     const char* methodName, const char* delegateTypeName,
                     void* reserved, void** delegateOut);

// Managed entry points in SkyEngine.Bootstrap ([UnmanagedCallersOnly]).
using managed_load_assembly_fn = std::int32_t (*)(const char* pathUtf8);
using managed_create_instance_fn = std::uint64_t (*)(const char* typeNameUtf8);
using managed_destroy_instance_fn = void (*)(std::uint64_t id);
using managed_invoke_lifecycle_fn = std::int32_t (*)(std::uint64_t id,
                                                     std::int32_t lifecycleEvent,
                                                     double deltaSeconds);
using managed_get_probe_fn = std::int64_t (*)(std::uint64_t id);
using managed_initialize_fn = void (*)(void* apiTable);
using managed_set_object_id_fn = void (*)(std::uint64_t id, std::uint64_t objectId);
using managed_tick_frame_fn = void (*)(double totalSeconds, double deltaSeconds);
using managed_get_script_classes_fn = std::int32_t (*)(char* buffer,
                                                       std::int32_t capacity);
using managed_get_script_fields_fn = std::int32_t (*)(const char* className,
                                                      char* buffer,
                                                      std::int32_t capacity);
using managed_set_script_field_fn = std::int32_t (*)(std::uint64_t id,
                                                     const char* name,
                                                     const char* value);
using managed_load_user_assembly_fn = std::int32_t (*)(const char* path);
using managed_unload_user_assembly_fn = void (*)();

/// Non-empty lines of a '\n'-separated managed string payload.
std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < text.size()) {
        auto end = text.find('\n', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        if (end > start) {
            lines.push_back(text.substr(start, end - start));
        }
        start = end + 1;
    }
    return lines;
}

std::filesystem::path discoverHostfxr() {
    std::vector<std::filesystem::path> roots;
    if (const char* dotnetRoot = std::getenv("DOTNET_ROOT")) {
        roots.emplace_back(dotnetRoot);
    }
    roots.emplace_back("/usr/lib/dotnet");
    roots.emplace_back("/usr/share/dotnet");

    for (const auto& root : roots) {
        const auto fxr = root / "host" / "fxr";
        std::error_code ec;
        std::filesystem::path best;
        for (const auto& entry : std::filesystem::directory_iterator(fxr, ec)) {
            const auto candidate = entry.path() / "libhostfxr.so";
            if (std::filesystem::exists(candidate, ec) &&
                (best.empty() || entry.path().filename() > best.parent_path().filename())) {
                best = candidate;
            }
        }
        if (!best.empty()) {
            return best;
        }
    }
    return {};
}

class DotNetScriptHostImpl final : public DotNetScriptHost {
public:
    explicit DotNetScriptHostImpl(const DotNetHostConfig& config) : config_(config) {
        if (config_.hostfxrPath.empty()) {
            config_.hostfxrPath = discoverHostfxr();
        }
    }

    ~DotNetScriptHostImpl() override { shutdown(); }

    [[nodiscard]] bool available() const { return !config_.hostfxrPath.empty(); }

    // IScriptHost

    bool start() override {
        if (started_) {
            return true;
        }
        if (config_.hostfxrPath.empty() || config_.bootstrapAssembly.empty()) {
            return false;
        }
        library_ = dlopen(config_.hostfxrPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (library_ == nullptr) {
            return false;
        }
        const auto initialize = reinterpret_cast<hostfxr_initialize_fn>(
            dlsym(library_, "hostfxr_initialize_for_runtime_config"));
        const auto getDelegate = reinterpret_cast<hostfxr_get_delegate_fn>(
            dlsym(library_, "hostfxr_get_runtime_delegate"));
        close_ = reinterpret_cast<hostfxr_close_fn>(dlsym(library_, "hostfxr_close"));
        if (initialize == nullptr || getDelegate == nullptr || close_ == nullptr) {
            return false;
        }

        auto runtimeConfig = config_.bootstrapAssembly;
        runtimeConfig.replace_extension("");
        runtimeConfig += ".runtimeconfig.json";
        // 0 = success; 1/2 = success against an already-initialized runtime
        // (the CLR is process-wide and initializes only once).
        const auto rc = initialize(runtimeConfig.c_str(), nullptr, &context_);
        if (rc < 0 || rc > 2 || context_ == nullptr) {
            return false;
        }
        void* loader = nullptr;
        if (getDelegate(context_, kHdtLoadAssemblyAndGetFunctionPointer, &loader) != 0 ||
            loader == nullptr) {
            return false;
        }
        loader_ = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(loader);

        const bool ok = resolve(managedLoadAssembly_, "LoadAssembly") &&
                        resolve(managedCreateInstance_, "CreateInstance") &&
                        resolve(managedDestroyInstance_, "DestroyInstance") &&
                        resolve(managedInvokeLifecycle_, "InvokeLifecycle") &&
                        resolve(managedGetProbe_, "GetProbe");
        if (!ok) {
            return false;
        }
        started_ = true;
        // Reverse-boundary entry points (present since the engine API landed).
        resolve(managedInitialize_, "Initialize");
        resolve(managedSetObjectId_, "SetObjectId");
        resolve(managedTickFrame_, "TickFrame");
        resolve(managedGetScriptClasses_, "GetScriptClasses");
        resolve(managedGetScriptFields_, "GetScriptFields");
        resolve(managedSetScriptField_, "SetScriptField");
        resolve(managedLoadUserAssembly_, "LoadUserAssembly");
        resolve(managedUnloadUserAssembly_, "UnloadUserAssembly");
        return true;
    }

    void shutdown() override {
        if (context_ != nullptr && close_ != nullptr) {
            close_(context_);
            context_ = nullptr;
        }
        started_ = false;
        // The CLR cannot be unloaded from the process; the library handle
        // stays valid for the process lifetime by design.
    }

    bool loadAssembly(const AssemblyRef& assembly) override {
        if (!started_ ||
            managedLoadAssembly_(assembly.path.string().c_str()) == 0) {
            return false;
        }
        assemblies_.push_back(assembly);
        return true;
    }

    std::vector<AssemblyRef> loadedAssemblies() const override { return assemblies_; }

    std::uint64_t createInstance(const std::string& managedTypeName) override {
        return started_ ? managedCreateInstance_(managedTypeName.c_str()) : 0;
    }

    void destroyInstance(std::uint64_t managedInstanceId) override {
        if (started_) {
            managedDestroyInstance_(managedInstanceId);
        }
    }

    bool invokeLifecycle(std::uint64_t managedInstanceId, ScriptLifecycleEvent event,
                         double deltaSeconds) override {
        return started_ &&
               managedInvokeLifecycle_(managedInstanceId,
                                       static_cast<std::int32_t>(event),
                                       deltaSeconds) != 0;
    }

    // DotNetScriptHost

    std::int64_t probeValue(std::uint64_t managedInstanceId) override {
        return started_ ? managedGetProbe_(managedInstanceId) : -1;
    }

    void installEngineApi(const void* apiTable) override {
        if (started_ && managedInitialize_ != nullptr) {
            managedInitialize_(const_cast<void*>(apiTable));
        }
    }

    void setInstanceObjectId(std::uint64_t managedInstanceId,
                             std::uint64_t objectId) override {
        if (started_ && managedSetObjectId_ != nullptr) {
            managedSetObjectId_(managedInstanceId, objectId);
        }
    }

    void beginFrame(double totalSeconds, double deltaSeconds) override {
        if (started_ && managedTickFrame_ != nullptr) {
            managedTickFrame_(totalSeconds, deltaSeconds);
        }
    }

    std::vector<std::string> scriptClassNames() override {
        std::vector<std::string> names;
        if (!started_ || managedGetScriptClasses_ == nullptr) {
            return names;
        }
        std::string buffer(8192, '\0');
        const auto length = managedGetScriptClasses_(
            buffer.data(), static_cast<std::int32_t>(buffer.size()));
        buffer.resize(length > 0 ? static_cast<std::size_t>(length) : 0);
        for (const auto& line : splitLines(buffer)) {
            names.push_back(line);
        }
        return names;
    }

    std::vector<ScriptFieldInfo> scriptFields(const std::string& className) override {
        std::vector<ScriptFieldInfo> fields;
        if (!started_ || managedGetScriptFields_ == nullptr) {
            return fields;
        }
        std::string buffer(8192, '\0');
        const auto length = managedGetScriptFields_(
            className.c_str(), buffer.data(),
            static_cast<std::int32_t>(buffer.size()));
        buffer.resize(length > 0 ? static_cast<std::size_t>(length) : 0);
        for (const auto& line : splitLines(buffer)) {
            // "name\ttype\tdefault" (default may be empty).
            const auto tab1 = line.find('\t');
            if (tab1 == std::string::npos) {
                continue;
            }
            const auto tab2 = line.find('\t', tab1 + 1);
            ScriptFieldInfo info;
            info.name = line.substr(0, tab1);
            info.typeName = tab2 == std::string::npos
                                ? line.substr(tab1 + 1)
                                : line.substr(tab1 + 1, tab2 - tab1 - 1);
            if (tab2 != std::string::npos) {
                info.defaultValue = line.substr(tab2 + 1);
            }
            fields.push_back(std::move(info));
        }
        return fields;
    }

    bool setInstanceField(std::uint64_t managedInstanceId, const std::string& name,
                          const std::string& value) override {
        return started_ && managedSetScriptField_ != nullptr &&
               managedSetScriptField_(managedInstanceId, name.c_str(),
                                      value.c_str()) != 0;
    }

    bool loadUserAssembly(const std::filesystem::path& path) override {
        return started_ && managedLoadUserAssembly_ != nullptr &&
               managedLoadUserAssembly_(path.string().c_str()) != 0;
    }

    void unloadUserAssembly() override {
        if (started_ && managedUnloadUserAssembly_ != nullptr) {
            managedUnloadUserAssembly_();
        }
    }

private:
    template <typename Fn>
    bool resolve(Fn& slot, const char* methodName) {
        void* fn = nullptr;
        const auto rc = loader_(config_.bootstrapAssembly.c_str(),
                                "SkyEngine.Bootstrap, SkyEngine.Managed", methodName,
                                kUnmanagedCallersOnly, nullptr, &fn);
        slot = reinterpret_cast<Fn>(fn);
        return rc == 0 && slot != nullptr;
    }

    DotNetHostConfig config_;
    void* library_ = nullptr;
    hostfxr_handle context_ = nullptr;
    hostfxr_close_fn close_ = nullptr;
    load_assembly_and_get_function_pointer_fn loader_ = nullptr;
    managed_load_assembly_fn managedLoadAssembly_ = nullptr;
    managed_create_instance_fn managedCreateInstance_ = nullptr;
    managed_destroy_instance_fn managedDestroyInstance_ = nullptr;
    managed_invoke_lifecycle_fn managedInvokeLifecycle_ = nullptr;
    managed_get_probe_fn managedGetProbe_ = nullptr;
    managed_initialize_fn managedInitialize_ = nullptr;
    managed_set_object_id_fn managedSetObjectId_ = nullptr;
    managed_tick_frame_fn managedTickFrame_ = nullptr;
    managed_get_script_classes_fn managedGetScriptClasses_ = nullptr;
    managed_get_script_fields_fn managedGetScriptFields_ = nullptr;
    managed_set_script_field_fn managedSetScriptField_ = nullptr;
    managed_load_user_assembly_fn managedLoadUserAssembly_ = nullptr;
    managed_unload_user_assembly_fn managedUnloadUserAssembly_ = nullptr;
    bool started_ = false;
    std::vector<AssemblyRef> assemblies_;
};

} // namespace

std::unique_ptr<DotNetScriptHost> createDotNetScriptHost(const DotNetHostConfig& config) {
    auto host = std::make_unique<DotNetScriptHostImpl>(config);
    return host->available() ? std::move(host) : nullptr;
}

#else

std::unique_ptr<DotNetScriptHost> createDotNetScriptHost(const DotNetHostConfig&) {
    return nullptr; // dlopen-based hosting is POSIX-only for now
}

#endif

} // namespace sky::scripting
