#include <map>

#include "sky/rendering/null_renderer.hpp"
#include "sky/rendering/renderer_registry.hpp"

namespace sky::rendering {
namespace {

class RendererRegistryImpl final : public IRendererRegistry {
public:
    RendererRegistryImpl() {
        registerBackend("null",
                        [](const BackendInit&) -> std::unique_ptr<IRenderer> {
                            return createNullRenderer();
                        });
    }

    bool registerBackend(const std::string& name, RendererFactory factory) override {
        if (name.empty() || factory == nullptr) {
            return false;
        }
        return factories_.emplace(name, std::move(factory)).second;
    }

    std::vector<std::string> availableBackends() const override {
        std::vector<std::string> names;
        names.reserve(factories_.size());
        for (const auto& [name, factory] : factories_) {
            names.push_back(name);
        }
        return names;
    }

    bool hasBackend(const std::string& name) const override {
        return factories_.contains(name);
    }

    std::unique_ptr<IRenderer> create(const std::string& name,
                                      const BackendInit& init) override {
        const auto it = factories_.find(name);
        return it != factories_.end() ? it->second(init) : nullptr;
    }

private:
    std::map<std::string, RendererFactory> factories_;
};

} // namespace

std::unique_ptr<IRendererRegistry> createRendererRegistry() {
    return std::make_unique<RendererRegistryImpl>();
}

} // namespace sky::rendering
