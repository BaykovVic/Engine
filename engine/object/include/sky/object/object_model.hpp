#pragma once

#include <string>
#include <vector>

#include "sky/core/handle.hpp"
#include "sky/core/math.hpp"

namespace sky::object {

struct ObjectTag {};
/// Identity of a scene object. The Object Model owns the hierarchy and
/// transforms; everyone else holds handles.
using ObjectHandle = core::Handle<ObjectTag>;

/// A node of the transform tree: local transform plus hierarchy links.
struct TransformNode {
    core::Transform local;
    ObjectHandle parent;
    std::vector<ObjectHandle> children;
};

/// Object Model contract: creation and destruction of scene objects.
class IObjectFactory {
public:
    virtual ~IObjectFactory() = default;

    virtual ObjectHandle createObject(const std::string& name) = 0;
    virtual void destroyObject(ObjectHandle object) = 0;
};

/// Object Model contract: hierarchy and transform access. The transform has
/// a single owner — this module.
class IObjectHierarchyAccess {
public:
    virtual ~IObjectHierarchyAccess() = default;

    virtual void setParent(ObjectHandle child, ObjectHandle parent) = 0;
    [[nodiscard]] virtual ObjectHandle parentOf(ObjectHandle object) const = 0;
    [[nodiscard]] virtual std::vector<ObjectHandle> childrenOf(ObjectHandle object) const = 0;

    virtual void setLocalTransform(ObjectHandle object, const core::Transform& transform) = 0;
    [[nodiscard]] virtual core::Transform localTransform(ObjectHandle object) const = 0;
    [[nodiscard]] virtual core::Transform worldTransform(ObjectHandle object) const = 0;
};

/// Object Model contract: read-only queries (name, existence, lookup).
class IObjectQueryService {
public:
    virtual ~IObjectQueryService() = default;

    [[nodiscard]] virtual bool exists(ObjectHandle object) const = 0;
    [[nodiscard]] virtual std::string nameOf(ObjectHandle object) const = 0;
    [[nodiscard]] virtual std::vector<ObjectHandle> findByName(const std::string& name) const = 0;
};

/// Sets an object's transform in world space, written back through the
/// local-only hierarchy: a root takes the world transform as its local, a
/// child takes invCompose(parentWorld, world). Composed from the existing
/// contract so any IObjectHierarchyAccess gains world-space placement.
inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object,
                              const core::Transform& world) {
    const auto parent = access.parentOf(object);
    if (!parent.isValid()) {
        access.setLocalTransform(object, world);
    } else {
        access.setLocalTransform(
            object, core::invCompose(access.worldTransform(parent), world));
    }
}

} // namespace sky::object
