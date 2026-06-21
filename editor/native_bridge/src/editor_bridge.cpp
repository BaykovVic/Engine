#include "sky/editor/bridge/editor_bridge.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "editor_context.hpp"

using sky::editor::EditorContext;

namespace {

EditorContext* self(SkyEditorContext* ctx) {
    return reinterpret_cast<EditorContext*>(ctx);
}

sky::object::ObjectHandle handle(SkyObjectId id) {
    return sky::object::ObjectHandle{id};
}

/// Copies `value` into a caller-owned buffer (NUL-terminated, truncated to
/// `capacity`) and returns the full length, the standard query-buffer idiom.
int32_t copyString(const std::string& value, char* buffer, int32_t capacity) {
    if (buffer != nullptr && capacity > 0) {
        const auto count =
            std::min<std::size_t>(value.size(), std::size_t(capacity - 1));
        std::memcpy(buffer, value.data(), count);
        buffer[count] = '\0';
    }
    return static_cast<int32_t>(value.size());
}

} // namespace

extern "C" {

SkyEditorContext* sky_editor_create(void) {
    return reinterpret_cast<SkyEditorContext*>(new EditorContext());
}

void sky_editor_destroy(SkyEditorContext* ctx) { delete self(ctx); }

int32_t sky_editor_root_count(SkyEditorContext* ctx) {
    return static_cast<int32_t>(self(ctx)->rootObjects().size());
}

SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index) {
    const auto roots = self(ctx)->rootObjects();
    if (index < 0 || std::size_t(index) >= roots.size()) {
        return 0;
    }
    return roots[std::size_t(index)].value;
}

int32_t sky_editor_child_count(SkyEditorContext* ctx, SkyObjectId object) {
    return static_cast<int32_t>(self(ctx)->objects->childrenOf(handle(object)).size());
}

SkyObjectId sky_editor_child_at(SkyEditorContext* ctx, SkyObjectId object,
                                int32_t index) {
    const auto children = self(ctx)->objects->childrenOf(handle(object));
    if (index < 0 || std::size_t(index) >= children.size()) {
        return 0;
    }
    return children[std::size_t(index)].value;
}

int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object,
                               char* buffer, int32_t capacity) {
    return copyString(self(ctx)->objects->nameOf(handle(object)), buffer, capacity);
}

int32_t sky_editor_object_exists(SkyEditorContext* ctx, SkyObjectId object) {
    return self(ctx)->objects->exists(handle(object)) ? 1 : 0;
}

void sky_editor_get_transform(SkyEditorContext* ctx, SkyObjectId object,
                              float* position, float* rotation, float* scale) {
    const auto t = self(ctx)->objects->localTransform(handle(object));
    if (position != nullptr) {
        position[0] = t.position.x;
        position[1] = t.position.y;
        position[2] = t.position.z;
    }
    if (rotation != nullptr) {
        rotation[0] = t.rotation.x;
        rotation[1] = t.rotation.y;
        rotation[2] = t.rotation.z;
        rotation[3] = t.rotation.w;
    }
    if (scale != nullptr) {
        scale[0] = t.scale.x;
        scale[1] = t.scale.y;
        scale[2] = t.scale.z;
    }
}

void sky_editor_set_position(SkyEditorContext* ctx, SkyObjectId object, float x,
                             float y, float z) {
    auto t = self(ctx)->objects->localTransform(handle(object));
    t.position = {x, y, z};
    self(ctx)->objects->setLocalTransform(handle(object), t);
}

int32_t sky_editor_component_count(SkyEditorContext* ctx, SkyObjectId object) {
    return static_cast<int32_t>(self(ctx)->components->componentsOf(handle(object)).size());
}

int32_t sky_editor_component_type(SkyEditorContext* ctx, SkyObjectId object,
                                  int32_t index, char* buffer, int32_t capacity) {
    const auto components = self(ctx)->components->componentsOf(handle(object));
    if (index < 0 || std::size_t(index) >= components.size()) {
        return copyString("", buffer, capacity);
    }
    const auto& descriptor =
        self(ctx)->components->descriptorOf(components[std::size_t(index)]);
    return copyString(descriptor.typeId, buffer, capacity);
}

SkyObjectId sky_editor_create_primitive(SkyEditorContext* ctx,
                                        SkyPrimitiveKind kind, const char* name) {
    const auto primitive = static_cast<sky::scene::PrimitiveKind>(kind);
    return self(ctx)
        ->createPrimitive(primitive, name != nullptr ? name : "Object")
        .value;
}

SkyObjectId sky_editor_duplicate(SkyEditorContext* ctx, SkyObjectId object) {
    return self(ctx)->duplicateObject(handle(object)).value;
}

void sky_editor_delete(SkyEditorContext* ctx, SkyObjectId object) {
    self(ctx)->destroyObject(handle(object));
}

} // extern "C"
