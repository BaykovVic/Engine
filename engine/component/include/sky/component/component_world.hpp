#pragma once

#include <map>
#include <memory>
#include <optional>
#include <variant>

#include "sky/component/component_model.hpp"
#include "sky/core/math.hpp"

namespace sky::component {

/// Value of one inspectable field of a component instance.
using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>;

/// In-memory implementation of the Component Model: owner of component
/// instances, their descriptors, inspectable metadata and per-instance
/// field values.
class ComponentWorld : public IComponentRegistry,
                       public IComponentAttachmentService,
                       public IComponentQueryService {
public:
    ~ComponentWorld() override = default;

    /// Detaches every component attached to the object (used by scene/object
    /// teardown).
    virtual void detachAllFrom(object::ObjectHandle object) = 0;

    /// Per-instance field data shown in the Inspector and persisted with
    /// the scene.
    virtual void setField(ComponentHandle component, const std::string& name,
                          FieldValue value) = 0;
    [[nodiscard]] virtual std::optional<FieldValue> field(
        ComponentHandle component, const std::string& name) const = 0;
    [[nodiscard]] virtual std::map<std::string, FieldValue> fields(
        ComponentHandle component) const = 0;
};

std::unique_ptr<ComponentWorld> createComponentWorld();

} // namespace sky::component
