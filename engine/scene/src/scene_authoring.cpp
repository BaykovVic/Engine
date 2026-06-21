#include "sky/scene/scene_authoring.hpp"

namespace sky::scene {
namespace {

object::ObjectHandle cloneRecursive(const AuthoringServices& s,
                                    object::ObjectHandle source,
                                    object::ObjectHandle parent, bool isRoot) {
    const std::string name =
        s.queries.nameOf(source) + (isRoot ? std::string(" Copy") : std::string());
    const auto copy = s.factory.createObject(name);
    if (parent.isValid()) {
        s.hierarchy.setParent(copy, parent);
    }
    s.hierarchy.setLocalTransform(copy, s.hierarchy.localTransform(source));

    // Re-attach every component and carry its field values across, so a
    // duplicate is a faithful copy rather than a bag of empty components.
    for (const auto component : s.componentQueries.componentsOf(source)) {
        const auto& descriptor = s.componentQueries.descriptorOf(component);
        const auto cloned = s.attachment.attach(copy, descriptor.typeId);
        for (const auto& [field, value] : s.componentData.fields(component)) {
            s.componentData.setField(cloned, field, value);
        }
    }
    for (const auto child : s.hierarchy.childrenOf(source)) {
        cloneRecursive(s, child, copy, false);
    }
    return copy;
}

} // namespace

const char* primitiveMeshName(PrimitiveKind kind) {
    switch (kind) {
        case PrimitiveKind::Cube:
            return "cube";
        case PrimitiveKind::Plane:
            return "plane";
        case PrimitiveKind::Sphere:
            return "sphere";
    }
    return "cube";
}

object::ObjectHandle createPrimitive(const AuthoringServices& services,
                                     PrimitiveKind kind, const std::string& name) {
    const auto object = services.factory.createObject(name);
    const auto mesh = services.attachment.attach(object, "sky.mesh");
    services.componentData.setField(mesh, "material", std::string("Default"));
    services.componentData.setField(mesh, "mesh",
                                    std::string(primitiveMeshName(kind)));
    return object;
}

object::ObjectHandle duplicateObject(const AuthoringServices& services,
                                     object::ObjectHandle source) {
    if (!services.queries.exists(source)) {
        return object::ObjectHandle::invalid();
    }
    return cloneRecursive(services, source, services.hierarchy.parentOf(source),
                          true);
}

} // namespace sky::scene
