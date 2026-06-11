#include <unordered_map>

#include "sky/component/component_world.hpp"

namespace sky::component {
namespace {

struct ComponentInstance {
    std::string typeId;
    object::ObjectHandle owner;
    std::map<std::string, FieldValue> fields;
};

class ComponentWorldImpl final : public ComponentWorld {
public:
    // IComponentRegistry

    void registerComponentType(const ComponentDescriptor& descriptor) override {
        types_[descriptor.typeId] = descriptor;
    }

    std::vector<ComponentDescriptor> availableTypes() const override {
        std::vector<ComponentDescriptor> result;
        result.reserve(types_.size());
        for (const auto& [typeId, descriptor] : types_) {
            result.push_back(descriptor);
        }
        return result;
    }

    // IComponentAttachmentService

    ComponentHandle attach(object::ObjectHandle object, const std::string& typeId) override {
        if (!types_.contains(typeId) || !object.isValid()) {
            return ComponentHandle::invalid();
        }
        const ComponentHandle handle{nextId_++};
        instances_.emplace(handle.value, ComponentInstance{typeId, object});
        byObject_[object.value].push_back(handle);
        return handle;
    }

    void detach(ComponentHandle component) override {
        const auto it = instances_.find(component.value);
        if (it == instances_.end()) {
            return;
        }
        auto& attached = byObject_[it->second.owner.value];
        std::erase(attached, component);
        instances_.erase(it);
    }

    void detachAllFrom(object::ObjectHandle object) override {
        const auto it = byObject_.find(object.value);
        if (it == byObject_.end()) {
            return;
        }
        for (const auto component : it->second) {
            instances_.erase(component.value);
        }
        byObject_.erase(it);
    }

    void setField(ComponentHandle component, const std::string& name,
                  FieldValue value) override {
        if (const auto it = instances_.find(component.value); it != instances_.end()) {
            it->second.fields[name] = std::move(value);
        }
    }

    std::optional<FieldValue> field(ComponentHandle component,
                                    const std::string& name) const override {
        const auto it = instances_.find(component.value);
        if (it == instances_.end()) {
            return std::nullopt;
        }
        const auto fieldIt = it->second.fields.find(name);
        if (fieldIt == it->second.fields.end()) {
            return std::nullopt;
        }
        return fieldIt->second;
    }

    std::map<std::string, FieldValue> fields(ComponentHandle component) const override {
        const auto it = instances_.find(component.value);
        return it != instances_.end() ? it->second.fields
                                      : std::map<std::string, FieldValue>{};
    }

    // IComponentQueryService

    std::vector<ComponentHandle> componentsOf(object::ObjectHandle object) const override {
        const auto it = byObject_.find(object.value);
        return it != byObject_.end() ? it->second : std::vector<ComponentHandle>{};
    }

    const ComponentDescriptor& descriptorOf(ComponentHandle component) const override {
        static const ComponentDescriptor kEmpty{};
        const auto it = instances_.find(component.value);
        if (it == instances_.end()) {
            return kEmpty;
        }
        const auto typeIt = types_.find(it->second.typeId);
        return typeIt != types_.end() ? typeIt->second : kEmpty;
    }

    object::ObjectHandle ownerOf(ComponentHandle component) const override {
        const auto it = instances_.find(component.value);
        return it != instances_.end() ? it->second.owner
                                      : object::ObjectHandle::invalid();
    }

private:
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::string, ComponentDescriptor> types_;
    std::unordered_map<std::uint64_t, ComponentInstance> instances_;
    std::unordered_map<std::uint64_t, std::vector<ComponentHandle>> byObject_;
};

} // namespace

std::unique_ptr<ComponentWorld> createComponentWorld() {
    return std::make_unique<ComponentWorldImpl>();
}

} // namespace sky::component
