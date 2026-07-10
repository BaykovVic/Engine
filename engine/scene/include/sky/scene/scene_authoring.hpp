#pragma once

#include <string>

#include "sky/component/component_world.hpp"
#include "sky/object/object_model.hpp"

namespace sky::scene {

/// Built-in primitive meshes the renderer can draw without an imported asset.
enum class PrimitiveKind { Cube, Plane, Sphere };

/// The object and component services a scene-authoring operation needs. The
/// caller owns these. Authoring deliberately never touches rendering,
/// physics or scene-root membership — those stay the caller's orchestration,
/// so the same helpers serve the editor today and the C# editor API later.
struct AuthoringServices {
    object::IObjectFactory& factory;
    object::IObjectHierarchyAccess& hierarchy;
    object::IObjectQueryService& queries;
    component::IComponentAttachmentService& attachment;
    component::IComponentQueryService& componentQueries;
    component::ComponentWorld& componentData;
};

/// The mesh-field value naming a built-in primitive (e.g. "cube").
const char* primitiveMeshName(PrimitiveKind kind);

/// Creates an object carrying a Mesh Renderer bound to a built-in primitive
/// mesh, with the default material. Returns the new object (not yet added to
/// any scene root — the caller decides where it lives).
object::ObjectHandle createPrimitive(const AuthoringServices& services,
                                     PrimitiveKind kind, const std::string& name);

/// Deep-copies an object and its descendants — local transform, attached
/// components and every component field value — parenting the copy under the
/// source's parent. Returns the root of the copy. (The caller re-registers a
/// new root with its scene and rebinds any non-component state such as
/// physics bodies.)
object::ObjectHandle duplicateObject(const AuthoringServices& services,
                                     object::ObjectHandle source);

} // namespace sky::scene
