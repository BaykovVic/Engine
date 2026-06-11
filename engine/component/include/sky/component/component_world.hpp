#pragma once

#include <memory>

#include "sky/component/component_model.hpp"

namespace sky::component {

/// In-memory implementation of the Component Model: owner of component
/// instances, their descriptors and inspectable metadata.
class ComponentWorld : public IComponentRegistry,
                       public IComponentAttachmentService,
                       public IComponentQueryService {
public:
    ~ComponentWorld() override = default;

    /// Detaches every component attached to the object (used by scene/object
    /// teardown).
    virtual void detachAllFrom(object::ObjectHandle object) = 0;
};

std::unique_ptr<ComponentWorld> createComponentWorld();

} // namespace sky::component
