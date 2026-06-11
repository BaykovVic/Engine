#pragma once

#include <memory>

#include "sky/scripting/script_host.hpp"
#include "sky/scripting/scripting_boundary.hpp"

namespace sky::scripting {

/// Native side of the scripting boundary in one unit: binding registry,
/// handle table and lifecycle bridge. Managed peers are created and invoked
/// through the IScriptHost contract; the host never owns engine state.
class ScriptRuntime : public IScriptBindingService,
                      public IScriptLifecycleBridge,
                      public INativeHandleRegistry {
public:
    ~ScriptRuntime() override = default;
};

std::unique_ptr<ScriptRuntime> createScriptRuntime(IScriptHost& host);

} // namespace sky::scripting
