#pragma once

#include <string>
#include <vector>

#include "sky/core/handle.hpp"
#include "sky/object/object_model.hpp"

namespace sky::component {

struct ComponentTag {};
using ComponentHandle = core::Handle<ComponentTag>;

/// A single field exposed to the Inspector and to serialization.
struct InspectableField {
    std::string name;
    std::string typeName;
};

/// Describes a component type: native or script-backed, with its
/// inspectable surface.
struct ComponentDescriptor {
    std::string typeId;
    std::string displayName;
    bool isScriptComponent = false;
    /// For script components: the managed type bound via Scripting Boundary.
    std::string managedTypeName;
    std::vector<InspectableField> fields;
};

/// Component Model contract: registry of available component types,
/// including types contributed by packages and managed scripts.
class IComponentRegistry {
public:
    virtual ~IComponentRegistry() = default;

    virtual void registerComponentType(const ComponentDescriptor& descriptor) = 0;
    [[nodiscard]] virtual std::vector<ComponentDescriptor> availableTypes() const = 0;
};

/// Component Model contract: attaching/detaching component instances to
/// objects. Owns component instances and their lifecycle events.
class IComponentAttachmentService {
public:
    virtual ~IComponentAttachmentService() = default;

    virtual ComponentHandle attach(object::ObjectHandle object, const std::string& typeId) = 0;
    virtual void detach(ComponentHandle component) = 0;
};

/// Component Model contract: read-only queries over attached components.
class IComponentQueryService {
public:
    virtual ~IComponentQueryService() = default;

    [[nodiscard]] virtual std::vector<ComponentHandle> componentsOf(
        object::ObjectHandle object) const = 0;
    [[nodiscard]] virtual const ComponentDescriptor& descriptorOf(
        ComponentHandle component) const = 0;
    [[nodiscard]] virtual object::ObjectHandle ownerOf(ComponentHandle component) const = 0;
};

} // namespace sky::component
