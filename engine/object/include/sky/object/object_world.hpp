#pragma once

#include <memory>

#include "sky/object/object_model.hpp"

namespace sky::object {

/// In-memory implementation of the Object Model: single owner of the object
/// hierarchy, transforms and object identity.
class ObjectWorld : public IObjectFactory,
                    public IObjectHierarchyAccess,
                    public IObjectQueryService {
public:
    ~ObjectWorld() override = default;

    virtual void renameObject(ObjectHandle object, const std::string& name) = 0;
};

std::unique_ptr<ObjectWorld> createObjectWorld();

} // namespace sky::object
