#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "sky/rendering/rendering.hpp"

namespace sky::rendering {

/// Host-provided services a backend may need at creation time. GL backends
/// use the proc resolver; headless backends ignore it.
struct BackendInit {
    std::function<void*(const char*)> resolveGlProc;
};

using RendererFactory =
    std::function<std::unique_ptr<IRenderer>(const BackendInit& init)>;

/// Rendering Abstraction contract: backends register themselves by name and
/// the host picks one via configuration ("engine.renderer") — switching
/// backends never touches code above the IRenderer boundary.
class IRendererRegistry {
public:
    virtual ~IRendererRegistry() = default;

    virtual bool registerBackend(const std::string& name, RendererFactory factory) = 0;
    [[nodiscard]] virtual std::vector<std::string> availableBackends() const = 0;
    [[nodiscard]] virtual bool hasBackend(const std::string& name) const = 0;
    virtual std::unique_ptr<IRenderer> create(const std::string& name,
                                              const BackendInit& init) = 0;
};

/// Creates a registry with the built-in "null" backend pre-registered.
std::unique_ptr<IRendererRegistry> createRendererRegistry();

} // namespace sky::rendering
