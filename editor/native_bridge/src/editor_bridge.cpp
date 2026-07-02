#include "sky/editor/bridge/editor_bridge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <variant>

#include "editor_camera.hpp"
#include "editor_commands.hpp"
#include "editor_context.hpp"
#include "frame_builder.hpp"

#ifdef SKY_BRIDGE_VULKAN
#include "sky/rendering_vulkan/vulkan_backend.hpp"
#endif
#ifdef SKY_BRIDGE_X11
#include <X11/Xlib.h>
#endif

using sky::editor::EditorContext;

namespace {

/// One editor session: the engine assembly plus the orbit camera and, once a
/// viewport renders, the Vulkan resources. The offscreen path (Vulkan only)
/// drives the editor on every OS, including macOS via MoltenVK; the swapchain
/// path (Vulkan + X11) is the Linux standalone-window route.
/// One captured log line for the editor's Console panel.
struct LogEntry {
    int32_t level = 0; // sky::core::LogLevel
    std::string category;
    std::string message;
};

struct BridgeSession {
    EditorContext context;
    sky::editor::EditorCamera camera;
    // Undo/redo history over the editor context.
    std::unique_ptr<sky::editor::UndoStack> undo =
        std::make_unique<sky::editor::UndoStack>(context);
    // Ring buffer of log lines shown in the Console panel.
    std::vector<LogEntry> logs;
    // Active package ids (managed via the Packages panel).
    std::vector<std::string> activePackageIds;
    // Coalesces a stream of transform edits (e.g. a gizmo drag) into a single
    // undo entry: `pendingBefore` is the local transform captured before the
    // first mutation, committed via sky_editor_commit_edit.
    bool pendingEdit = false;
    sky::object::ObjectHandle pendingObj;
    sky::core::Transform pendingBefore;
#ifdef SKY_BRIDGE_VULKAN
    // Offscreen path: the editor viewport renders to a texture it blits into a
    // normal UI control (so it receives input and hosts overlays). No window
    // surface — works wherever Vulkan does.
    std::unique_ptr<sky::rendering_vulkan::VulkanRenderer> offscreen;
    std::unique_ptr<sky::editor::FrameBuilder> offscreenFrame;
    std::uint32_t offscreenWidth = 0;
    std::uint32_t offscreenHeight = 0;
    // Second offscreen path for the Game view: the scene through its Main
    // Camera (no editor-orbit override), separate so the two panel sizes do
    // not thrash a single swapchain.
    std::unique_ptr<sky::rendering_vulkan::VulkanRenderer> gameOffscreen;
    std::unique_ptr<sky::editor::FrameBuilder> gameFrame;
    std::uint32_t gameWidth = 0;
    std::uint32_t gameHeight = 0;
#endif
#ifdef SKY_BRIDGE_X11
    Display* ownDisplay = nullptr; // opened when the caller provides no display
    std::unique_ptr<sky::rendering_vulkan::VulkanRenderer> renderer;
    std::unique_ptr<sky::editor::FrameBuilder> frame;
#endif
#ifdef SKY_BRIDGE_VULKAN
    ~BridgeSession() {
#ifdef SKY_BRIDGE_X11
        frame.reset();
        renderer.reset(); // releases the Vulkan surface before the display closes
#endif
        offscreenFrame.reset();
        offscreen.reset();
        gameFrame.reset();
        gameOffscreen.reset();
#ifdef SKY_BRIDGE_X11
        if (ownDisplay != nullptr) {
            XCloseDisplay(ownDisplay);
        }
#endif
    }
#endif
};

BridgeSession* self(SkyEditorContext* ctx) {
    return reinterpret_cast<BridgeSession*>(ctx);
}

EditorContext& ec(SkyEditorContext* ctx) { return self(ctx)->context; }

/// Appends a line to the Console log buffer (kept bounded).
void logMsg(BridgeSession* session, sky::core::LogLevel level, const char* category,
            const std::string& message) {
    constexpr std::size_t kMaxLogs = 1000;
    session->logs.push_back({static_cast<int32_t>(level), category, message});
    if (session->logs.size() > kMaxLogs) {
        session->logs.erase(session->logs.begin());
    }
}

/// Commits a coalesced transform edit into the undo stack (no-op if the
/// transform did not actually change).
void commitTransform(BridgeSession* session) {
    if (!session->pendingEdit) {
        return;
    }
    session->pendingEdit = false;
    if (!session->context.objects->exists(session->pendingObj)) {
        return;
    }
    const auto after = session->context.objects->localTransform(session->pendingObj);
    const auto& before = session->pendingBefore;
    const bool changed = before.position.x != after.position.x ||
        before.position.y != after.position.y || before.position.z != after.position.z ||
        before.rotation.x != after.rotation.x || before.rotation.y != after.rotation.y ||
        before.rotation.z != after.rotation.z || before.rotation.w != after.rotation.w ||
        before.scale.x != after.scale.x || before.scale.y != after.scale.y ||
        before.scale.z != after.scale.z;
    if (changed) {
        session->undo->push(sky::editor::makeTransformCommand(session->pendingObj,
                                                              before, after));
    }
}

/// Marks the start of a transform edit on `object`, capturing its local
/// transform so a later commit can record the net change.
void beginTransformEdit(BridgeSession* session, sky::object::ObjectHandle object) {
    if (session->pendingEdit && session->pendingObj.value != object.value) {
        commitTransform(session);
    }
    if (!session->pendingEdit) {
        session->pendingObj = object;
        session->pendingBefore = session->context.objects->localTransform(object);
        session->pendingEdit = true;
    }
}

sky::object::ObjectHandle handle(SkyObjectId id) {
    return sky::object::ObjectHandle{id};
}

sky::component::ComponentHandle componentAt(EditorContext& context, SkyObjectId object,
                                            int32_t index) {
    const auto components = context.components->componentsOf(handle(object));
    if (index < 0 || std::size_t(index) >= components.size()) {
        return {};
    }
    return components[std::size_t(index)];
}

std::string fieldValueString(const sky::component::FieldValue& value) {
    return std::visit(
        [](const auto& x) -> std::string {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, float>) {
                char b[32];
                std::snprintf(b, sizeof(b), "%g", static_cast<double>(x));
                return b;
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                return std::to_string(x);
            } else if constexpr (std::is_same_v<T, bool>) {
                return x ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return x;
            } else {
                char b[64];
                std::snprintf(b, sizeof(b), "%g, %g, %g", static_cast<double>(x.x),
                              static_cast<double>(x.y), static_cast<double>(x.z));
                return b;
            }
        },
        value);
}

/// Picks the nearest object whose scaled unit-cube AABB the camera ray
/// through the viewport pixel intersects (the editor's 3D-view pick).
sky::object::ObjectHandle pickObject(EditorContext& context,
                                     const sky::editor::EditorCamera& camera,
                                     float pixelX, float pixelY, std::uint32_t width,
                                     std::uint32_t height) {
    const auto origin =
        camera.rayOrigin(pixelX, pixelY, float(width), float(height));
    const auto direction =
        camera.rayThrough(pixelX, pixelY, float(width), float(height));
    sky::object::ObjectHandle best;
    float bestDistance = 1e9f;
    const std::function<void(sky::object::ObjectHandle)> test =
        [&](sky::object::ObjectHandle object) {
            if (!context.objects->exists(object)) {
                return;
            }
            const auto world = context.objects->worldTransform(object);
            const float origins[] = {origin.x, origin.y, origin.z};
            const float dirs[] = {direction.x, direction.y, direction.z};
            const float centers[] = {world.position.x, world.position.y, world.position.z};
            const float halves[] = {std::max(0.125f, world.scale.x * 0.5f),
                                    std::max(0.125f, world.scale.y * 0.5f),
                                    std::max(0.125f, world.scale.z * 0.5f)};
            float tMin = 0.0f, tMax = 1e9f;
            bool hit = true;
            for (int axis = 0; axis < 3 && hit; ++axis) {
                if (std::fabs(dirs[axis]) < 1e-7f) {
                    hit = std::fabs(origins[axis] - centers[axis]) <= halves[axis];
                    continue;
                }
                const float inv = 1.0f / dirs[axis];
                float t1 = (centers[axis] - halves[axis] - origins[axis]) * inv;
                float t2 = (centers[axis] + halves[axis] - origins[axis]) * inv;
                if (t1 > t2) {
                    std::swap(t1, t2);
                }
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                hit = tMin <= tMax;
            }
            if (hit && tMax >= 0.0f && tMin < bestDistance) {
                bestDistance = tMin;
                best = object;
            }
            for (const auto child : context.objects->childrenOf(object)) {
                test(child);
            }
        };
    for (const auto root : context.rootObjects()) {
        test(root);
    }
    return best;
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
    auto* session = new BridgeSession();
    // Managed Debug.Log output goes to the Console panel, like everything else.
    session->context.scriptLog = [session](int level, const std::string& message) {
        logMsg(session, static_cast<sky::core::LogLevel>(level), "Script", message);
    };
    logMsg(session, sky::core::LogLevel::Info, "Editor", "Sky Engine editor ready");
    return reinterpret_cast<SkyEditorContext*>(session);
}

void sky_editor_destroy(SkyEditorContext* ctx) { delete self(ctx); }

int32_t sky_editor_root_count(SkyEditorContext* ctx) {
    return static_cast<int32_t>(ec(ctx).rootObjects().size());
}

SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index) {
    const auto roots = ec(ctx).rootObjects();
    if (index < 0 || std::size_t(index) >= roots.size()) {
        return 0;
    }
    return roots[std::size_t(index)].value;
}

int32_t sky_editor_child_count(SkyEditorContext* ctx, SkyObjectId object) {
    return static_cast<int32_t>(ec(ctx).objects->childrenOf(handle(object)).size());
}

SkyObjectId sky_editor_child_at(SkyEditorContext* ctx, SkyObjectId object,
                                int32_t index) {
    const auto children = ec(ctx).objects->childrenOf(handle(object));
    if (index < 0 || std::size_t(index) >= children.size()) {
        return 0;
    }
    return children[std::size_t(index)].value;
}

int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object,
                               char* buffer, int32_t capacity) {
    return copyString(ec(ctx).objects->nameOf(handle(object)), buffer, capacity);
}

void sky_editor_set_object_enabled(SkyEditorContext* ctx, SkyObjectId object,
                                   int32_t enabled) {
    ec(ctx).setObjectEnabled(handle(object), enabled != 0);
    logMsg(self(ctx), sky::core::LogLevel::Info, "Scene",
           (enabled != 0 ? "Enabled " : "Disabled ") +
               ec(ctx).objects->nameOf(handle(object)));
}

int32_t sky_editor_object_enabled(SkyEditorContext* ctx, SkyObjectId object) {
    return ec(ctx).objectEnabled(handle(object)) ? 1 : 0;
}

void sky_editor_rename_object(SkyEditorContext* ctx, SkyObjectId object,
                              const char* name) {
    if (name == nullptr) {
        return;
    }
    commitTransform(self(ctx));
    auto& objects = *ec(ctx).objects;
    std::string before = objects.nameOf(handle(object));
    std::string after = name;
    if (before == after) {
        return;
    }
    objects.renameObject(handle(object), after);
    self(ctx)->undo->push(
        sky::editor::makeRenameCommand(handle(object), std::move(before), std::move(after)));
}

// --- Component types + attach/detach ------------------------------------

int32_t sky_editor_available_type_count(SkyEditorContext* ctx) {
    return static_cast<int32_t>(ec(ctx).components->availableTypes().size());
}

int32_t sky_editor_available_type_id(SkyEditorContext* ctx, int32_t index,
                                     char* buffer, int32_t capacity) {
    const auto types = ec(ctx).components->availableTypes();
    if (index < 0 || std::size_t(index) >= types.size()) {
        return copyString("", buffer, capacity);
    }
    return copyString(types[std::size_t(index)].typeId, buffer, capacity);
}

int32_t sky_editor_available_type_name(SkyEditorContext* ctx, int32_t index,
                                       char* buffer, int32_t capacity) {
    const auto types = ec(ctx).components->availableTypes();
    if (index < 0 || std::size_t(index) >= types.size()) {
        return copyString("", buffer, capacity);
    }
    return copyString(types[std::size_t(index)].displayName, buffer, capacity);
}

int32_t sky_editor_available_type_category(SkyEditorContext* ctx, int32_t index,
                                           char* buffer, int32_t capacity) {
    const auto types = ec(ctx).components->availableTypes();
    if (index < 0 || std::size_t(index) >= types.size()) {
        return copyString("", buffer, capacity);
    }
    return copyString(types[std::size_t(index)].category, buffer, capacity);
}

void sky_editor_add_component(SkyEditorContext* ctx, SkyObjectId object,
                              const char* type_id) {
    if (type_id == nullptr) {
        return;
    }
    commitTransform(self(ctx));
    const auto created = ec(ctx).components->attach(handle(object), type_id);
    self(ctx)->undo->push(
        sky::editor::makeAddComponentCommand(handle(object), type_id, created));
    logMsg(self(ctx), sky::core::LogLevel::Info, "Inspector",
           std::string("Added ") + type_id + " to " +
               ec(ctx).objects->nameOf(handle(object)));
}

void sky_editor_remove_component(SkyEditorContext* ctx, SkyObjectId object,
                                 int32_t component) {
    const auto comp = componentAt(ec(ctx), object, component);
    if (!comp.isValid()) {
        return;
    }
    commitTransform(self(ctx));
    const auto typeId = ec(ctx).components->descriptorOf(comp).typeId;
    auto command = sky::editor::makeRemoveComponentCommand(ec(ctx), comp);
    ec(ctx).components->detach(comp);
    self(ctx)->undo->push(std::move(command));
    logMsg(self(ctx), sky::core::LogLevel::Info, "Inspector",
           "Removed " + typeId + " from " + ec(ctx).objects->nameOf(handle(object)));
}

int32_t sky_editor_object_exists(SkyEditorContext* ctx, SkyObjectId object) {
    return ec(ctx).objects->exists(handle(object)) ? 1 : 0;
}

void sky_editor_get_transform(SkyEditorContext* ctx, SkyObjectId object,
                              float* position, float* rotation, float* scale) {
    const auto t = ec(ctx).objects->localTransform(handle(object));
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
    beginTransformEdit(self(ctx), handle(object));
    auto t = ec(ctx).objects->localTransform(handle(object));
    t.position = {x, y, z};
    ec(ctx).objects->setLocalTransform(handle(object), t);
}

void sky_editor_set_scale(SkyEditorContext* ctx, SkyObjectId object, float x,
                          float y, float z) {
    beginTransformEdit(self(ctx), handle(object));
    auto t = ec(ctx).objects->localTransform(handle(object));
    t.scale = {x, y, z};
    ec(ctx).objects->setLocalTransform(handle(object), t);
}

int32_t sky_editor_component_count(SkyEditorContext* ctx, SkyObjectId object) {
    return static_cast<int32_t>(ec(ctx).components->componentsOf(handle(object)).size());
}

int32_t sky_editor_component_type(SkyEditorContext* ctx, SkyObjectId object,
                                  int32_t index, char* buffer, int32_t capacity) {
    const auto components = ec(ctx).components->componentsOf(handle(object));
    if (index < 0 || std::size_t(index) >= components.size()) {
        return copyString("", buffer, capacity);
    }
    const auto& descriptor =
        ec(ctx).components->descriptorOf(components[std::size_t(index)]);
    return copyString(descriptor.typeId, buffer, capacity);
}

int32_t sky_editor_component_display_name(SkyEditorContext* ctx, SkyObjectId object,
                                          int32_t component, char* buffer,
                                          int32_t capacity) {
    const auto handle = componentAt(ec(ctx), object, component);
    if (!handle.isValid()) {
        return copyString("", buffer, capacity);
    }
    return copyString(ec(ctx).components->descriptorOf(handle).displayName, buffer,
                      capacity);
}

int32_t sky_editor_component_field_count(SkyEditorContext* ctx, SkyObjectId object,
                                         int32_t component) {
    const auto handle = componentAt(ec(ctx), object, component);
    return handle.isValid()
               ? int32_t(ec(ctx).components->descriptorOf(handle).fields.size())
               : 0;
}

int32_t sky_editor_component_field_name(SkyEditorContext* ctx, SkyObjectId object,
                                        int32_t component, int32_t field,
                                        char* buffer, int32_t capacity) {
    const auto handle = componentAt(ec(ctx), object, component);
    if (!handle.isValid()) {
        return copyString("", buffer, capacity);
    }
    const auto& fields = ec(ctx).components->descriptorOf(handle).fields;
    if (field < 0 || std::size_t(field) >= fields.size()) {
        return copyString("", buffer, capacity);
    }
    return copyString(fields[std::size_t(field)].name, buffer, capacity);
}

int32_t sky_editor_component_field_type(SkyEditorContext* ctx, SkyObjectId object,
                                        int32_t component, int32_t field,
                                        char* buffer, int32_t capacity) {
    const auto handle = componentAt(ec(ctx), object, component);
    if (!handle.isValid()) {
        return copyString("", buffer, capacity);
    }
    const auto& fields = ec(ctx).components->descriptorOf(handle).fields;
    if (field < 0 || std::size_t(field) >= fields.size()) {
        return copyString("", buffer, capacity);
    }
    return copyString(fields[std::size_t(field)].typeName, buffer, capacity);
}

int32_t sky_editor_component_field_value(SkyEditorContext* ctx, SkyObjectId object,
                                         int32_t component, int32_t field,
                                         char* buffer, int32_t capacity) {
    const auto handle = componentAt(ec(ctx), object, component);
    if (!handle.isValid()) {
        return copyString("", buffer, capacity);
    }
    const auto& fields = ec(ctx).components->descriptorOf(handle).fields;
    if (field < 0 || std::size_t(field) >= fields.size()) {
        return copyString("", buffer, capacity);
    }
    const auto value = ec(ctx).components->field(handle, fields[std::size_t(field)].name);
    return copyString(value ? fieldValueString(*value) : "", buffer, capacity);
}

void sky_editor_set_component_field(SkyEditorContext* ctx, SkyObjectId object,
                                    int32_t component, int32_t field,
                                    const char* value) {
    const auto handle = componentAt(ec(ctx), object, component);
    if (!handle.isValid() || value == nullptr) {
        return;
    }
    const auto& fields = ec(ctx).components->descriptorOf(handle).fields;
    if (field < 0 || std::size_t(field) >= fields.size()) {
        return;
    }
    const auto& descriptor = fields[std::size_t(field)];
    const std::string text = value;
    sky::component::FieldValue parsed;
    if (descriptor.typeName == "float") {
        parsed = std::strtof(text.c_str(), nullptr);
    } else if (descriptor.typeName == "int") {
        parsed = std::int64_t(std::strtoll(text.c_str(), nullptr, 10));
    } else if (descriptor.typeName == "bool") {
        parsed = text == "true" || text == "1";
    } else if (descriptor.typeName == "Vec3") {
        float x = 0, y = 0, z = 0;
        std::sscanf(text.c_str(), "%g, %g, %g", &x, &y, &z);
        parsed = sky::core::Vec3{x, y, z};
    } else {
        parsed = text;
    }
    commitTransform(self(ctx));
    const auto before = ec(ctx).components->field(handle, descriptor.name);
    ec(ctx).components->setField(handle, descriptor.name, parsed);
    if (before) {
        self(ctx)->undo->push(
            sky::editor::makeFieldCommand(handle, descriptor.name, *before, parsed));
    }
}

namespace {

// The material fields exposed to the Materials panel, in display order.
const char* const kMaterialFields[] = {
    "baseColor", "roughness",  "metallic",    "emissive",  "albedo",  "normal",
    "roughness_map", "metallic_map", "occlusion", "height", "uvTiling", "parallax"};
constexpr int kMaterialFieldCount = 12;

std::string materialFieldValue(const sky::rendering::MaterialDesc& m, int field) {
    char b[64];
    switch (field) {
        case 0: std::snprintf(b, sizeof(b), "%g, %g, %g", m.baseColor.x, m.baseColor.y, m.baseColor.z); return b;
        case 1: std::snprintf(b, sizeof(b), "%g", m.roughness); return b;
        case 2: std::snprintf(b, sizeof(b), "%g", m.metallic); return b;
        case 3: std::snprintf(b, sizeof(b), "%g, %g, %g", m.emissive.x, m.emissive.y, m.emissive.z); return b;
        case 4: return m.texturePath;
        case 5: return m.normalPath;
        case 6: return m.roughnessPath;
        case 7: return m.metallicPath;
        case 8: return m.occlusionPath;
        case 9: return m.heightPath;
        case 10: std::snprintf(b, sizeof(b), "%g, %g", m.uvTiling.x, m.uvTiling.y); return b;
        case 11: std::snprintf(b, sizeof(b), "%g", m.parallaxDepth); return b;
        default: return "";
    }
}

void materialFieldSet(sky::rendering::MaterialDesc& m, int field, const std::string& v) {
    float x = 0, y = 0, z = 0;
    switch (field) {
        case 0: std::sscanf(v.c_str(), "%g, %g, %g", &x, &y, &z); m.baseColor = {x, y, z}; break;
        case 1: m.roughness = std::strtof(v.c_str(), nullptr); break;
        case 2: m.metallic = std::strtof(v.c_str(), nullptr); break;
        case 3: std::sscanf(v.c_str(), "%g, %g, %g", &x, &y, &z); m.emissive = {x, y, z}; break;
        case 4: m.texturePath = v; break;
        case 5: m.normalPath = v; break;
        case 6: m.roughnessPath = v; break;
        case 7: m.metallicPath = v; break;
        case 8: m.occlusionPath = v; break;
        case 9: m.heightPath = v; break;
        case 10: std::sscanf(v.c_str(), "%g, %g", &x, &y); m.uvTiling = {x, y}; break;
        case 11: m.parallaxDepth = std::strtof(v.c_str(), nullptr); break;
        default: break;
    }
}

} // namespace

int32_t sky_editor_terrain_generate(SkyEditorContext* ctx, uint64_t seed) {
    return int32_t(ec(ctx).generateTerrain(seed));
}

int32_t sky_editor_material_count(SkyEditorContext* ctx) {
    return int32_t(ec(ctx).materials->allMaterials().size());
}

int32_t sky_editor_material_name(SkyEditorContext* ctx, int32_t index, char* buffer,
                                 int32_t capacity) {
    const auto materials = ec(ctx).materials->allMaterials();
    if (index < 0 || std::size_t(index) >= materials.size()) {
        return copyString("", buffer, capacity);
    }
    return copyString(materials[std::size_t(index)].name, buffer, capacity);
}

int32_t sky_editor_material_field_count(SkyEditorContext*) { return kMaterialFieldCount; }

int32_t sky_editor_material_field_name(SkyEditorContext*, int32_t field, char* buffer,
                                       int32_t capacity) {
    if (field < 0 || field >= kMaterialFieldCount) {
        return copyString("", buffer, capacity);
    }
    return copyString(kMaterialFields[field], buffer, capacity);
}

int32_t sky_editor_material_field_value(SkyEditorContext* ctx, int32_t index,
                                        int32_t field, char* buffer, int32_t capacity) {
    const auto materials = ec(ctx).materials->allMaterials();
    if (index < 0 || std::size_t(index) >= materials.size() || field < 0 ||
        field >= kMaterialFieldCount) {
        return copyString("", buffer, capacity);
    }
    return copyString(materialFieldValue(materials[std::size_t(index)], field), buffer,
                      capacity);
}

void sky_editor_set_material_field(SkyEditorContext* ctx, int32_t index, int32_t field,
                                   const char* value) {
    const auto materials = ec(ctx).materials->allMaterials();
    if (index < 0 || std::size_t(index) >= materials.size() || field < 0 ||
        field >= kMaterialFieldCount || value == nullptr) {
        return;
    }
    auto desc = materials[std::size_t(index)];
    materialFieldSet(desc, field, value);
    if (const auto handle = ec(ctx).materials->findMaterial(desc.name)) {
        ec(ctx).materials->updateMaterial(*handle, desc);
    }
}

int32_t sky_editor_vfs_count(SkyEditorContext* ctx, const char* dir) {
    return int32_t(ec(ctx).vfs->list(dir != nullptr ? dir : "").size());
}

int32_t sky_editor_vfs_entry(SkyEditorContext* ctx, const char* dir, int32_t index,
                             char* buffer, int32_t capacity) {
    const auto entries = ec(ctx).vfs->list(dir != nullptr ? dir : "");
    if (index < 0 || std::size_t(index) >= entries.size()) {
        return copyString("", buffer, capacity);
    }
    return copyString(entries[std::size_t(index)], buffer, capacity);
}

SkyObjectId sky_editor_create_primitive(SkyEditorContext* ctx,
                                        SkyPrimitiveKind kind, const char* name) {
    commitTransform(self(ctx));
    const auto primitive = static_cast<sky::scene::PrimitiveKind>(kind);
    const auto object =
        ec(ctx).createPrimitive(primitive, name != nullptr ? name : "Object");
    self(ctx)->undo->push(sky::editor::makeCreateSnapshotCommand(ec(ctx), object));
    logMsg(self(ctx), sky::core::LogLevel::Info, "Scene",
           "Created " + ec(ctx).objects->nameOf(object));
    return object.value;
}

SkyObjectId sky_editor_create_mesh_object(SkyEditorContext* ctx, const char* name,
                                          const char* mesh_ref) {
    commitTransform(self(ctx));
    const auto object = ec(ctx).createModelObject(
        name != nullptr ? name : "Model", mesh_ref != nullptr ? mesh_ref : "");
    self(ctx)->undo->push(sky::editor::makeCreateSnapshotCommand(ec(ctx), object));
    logMsg(self(ctx), sky::core::LogLevel::Info, "Scene",
           "Created " + ec(ctx).objects->nameOf(object));
    return object.value;
}

void sky_editor_new_scene(SkyEditorContext* ctx) {
    ec(ctx).newScene();
    self(ctx)->pendingEdit = false;
    self(ctx)->undo->clear();
    logMsg(self(ctx), sky::core::LogLevel::Info, "Scene", "New scene");
}

int32_t sky_editor_save_scene(SkyEditorContext* ctx, const char* path) {
    if (path == nullptr) {
        return 0;
    }
    const bool ok = ec(ctx).saveScene(path);
    logMsg(self(ctx), ok ? sky::core::LogLevel::Info : sky::core::LogLevel::Error,
           "Scene", ok ? std::string("Saved scene to ") + path
                       : std::string("Failed to save scene to ") + path);
    return ok ? 1 : 0;
}

int32_t sky_editor_open_scene(SkyEditorContext* ctx, const char* path) {
    if (path == nullptr) {
        return 0;
    }
    const bool ok = ec(ctx).openScene(path);
    self(ctx)->pendingEdit = false;
    self(ctx)->undo->clear();
    logMsg(self(ctx), ok ? sky::core::LogLevel::Info : sky::core::LogLevel::Error,
           "Scene", ok ? std::string("Opened ") + path
                       : std::string("Failed to open ") + path);
    return ok ? 1 : 0;
}

SkyObjectId sky_editor_duplicate(SkyEditorContext* ctx, SkyObjectId object) {
    commitTransform(self(ctx));
    const auto copy = ec(ctx).duplicateObject(handle(object));
    self(ctx)->undo->push(sky::editor::makeDuplicateCommand(handle(object), copy));
    logMsg(self(ctx), sky::core::LogLevel::Info, "Scene",
           "Duplicated " + ec(ctx).objects->nameOf(copy));
    return copy.value;
}

void sky_editor_delete(SkyEditorContext* ctx, SkyObjectId object) {
    commitTransform(self(ctx));
    const auto name = ec(ctx).objects->nameOf(handle(object));
    auto command = sky::editor::makeDeleteCommand(ec(ctx), handle(object));
    ec(ctx).destroyObject(handle(object));
    self(ctx)->undo->push(std::move(command));
    logMsg(self(ctx), sky::core::LogLevel::Info, "Scene", "Deleted " + name);
}

// --- Undo / Redo --------------------------------------------------------

void sky_editor_commit_edit(SkyEditorContext* ctx) { commitTransform(self(ctx)); }

int32_t sky_editor_undo(SkyEditorContext* ctx) {
    commitTransform(self(ctx));
    return self(ctx)->undo->undo() ? 1 : 0;
}

int32_t sky_editor_redo(SkyEditorContext* ctx) {
    commitTransform(self(ctx));
    return self(ctx)->undo->redo() ? 1 : 0;
}

int32_t sky_editor_can_undo(SkyEditorContext* ctx) {
    return self(ctx)->undo->canUndo() ? 1 : 0;
}

int32_t sky_editor_can_redo(SkyEditorContext* ctx) {
    return self(ctx)->undo->canRedo() ? 1 : 0;
}

int32_t sky_editor_undo_label(SkyEditorContext* ctx, char* buffer, int32_t capacity) {
    return copyString(self(ctx)->undo->undoLabel(), buffer, capacity);
}

int32_t sky_editor_redo_label(SkyEditorContext* ctx, char* buffer, int32_t capacity) {
    return copyString(self(ctx)->undo->redoLabel(), buffer, capacity);
}

// --- Console log --------------------------------------------------------

int32_t sky_editor_log_count(SkyEditorContext* ctx) {
    return static_cast<int32_t>(self(ctx)->logs.size());
}

int32_t sky_editor_log_level(SkyEditorContext* ctx, int32_t index) {
    const auto& logs = self(ctx)->logs;
    if (index < 0 || std::size_t(index) >= logs.size()) {
        return 0;
    }
    return logs[std::size_t(index)].level;
}

int32_t sky_editor_log_text(SkyEditorContext* ctx, int32_t index, char* buffer,
                            int32_t capacity) {
    const auto& logs = self(ctx)->logs;
    if (index < 0 || std::size_t(index) >= logs.size()) {
        return copyString("", buffer, capacity);
    }
    const auto& entry = logs[std::size_t(index)];
    const auto line = entry.category.empty()
                          ? entry.message
                          : entry.category + ": " + entry.message;
    return copyString(line, buffer, capacity);
}

void sky_editor_log_clear(SkyEditorContext* ctx) { self(ctx)->logs.clear(); }

// --- Packages -----------------------------------------------------------

int32_t sky_editor_package_count(SkyEditorContext* ctx) {
    return static_cast<int32_t>(ec(ctx).packages->discoveredPackages().size());
}

int32_t sky_editor_package_info(SkyEditorContext* ctx, int32_t index, int32_t which,
                                char* buffer, int32_t capacity) {
    const auto packages = ec(ctx).packages->discoveredPackages();
    if (index < 0 || std::size_t(index) >= packages.size()) {
        return copyString("", buffer, capacity);
    }
    const auto& p = packages[std::size_t(index)];
    const std::string& value = which == 0 ? p.packageId
                             : which == 1 ? p.displayName
                                          : p.version;
    return copyString(value, buffer, capacity);
}

int32_t sky_editor_package_active(SkyEditorContext* ctx, int32_t index) {
    const auto packages = ec(ctx).packages->discoveredPackages();
    if (index < 0 || std::size_t(index) >= packages.size()) {
        return 0;
    }
    const auto& id = packages[std::size_t(index)].packageId;
    const auto& active = self(ctx)->activePackageIds;
    return std::find(active.begin(), active.end(), id) != active.end() ? 1 : 0;
}

void sky_editor_package_set_active(SkyEditorContext* ctx, int32_t index,
                                   int32_t active) {
    const auto packages = ec(ctx).packages->discoveredPackages();
    if (index < 0 || std::size_t(index) >= packages.size()) {
        return;
    }
    const auto& manifest = packages[std::size_t(index)];
    auto& ids = self(ctx)->activePackageIds;
    const auto handle = ec(ctx).packages->registerPackage(manifest);
    if (active != 0) {
        if (ec(ctx).packages->activate(handle) &&
            std::find(ids.begin(), ids.end(), manifest.packageId) == ids.end()) {
            ids.push_back(manifest.packageId);
            logMsg(self(ctx), sky::core::LogLevel::Info, "Packages",
                   "Activated " + manifest.displayName);
        }
    } else {
        ec(ctx).packages->deactivate(handle);
        std::erase(ids, manifest.packageId);
        logMsg(self(ctx), sky::core::LogLevel::Info, "Packages",
               "Deactivated " + manifest.displayName);
    }
}

int32_t sky_editor_package_refresh(SkyEditorContext* ctx) {
    const auto count = ec(ctx).packages->discoverPackages(ec(ctx).packagesRoot);
    logMsg(self(ctx), sky::core::LogLevel::Info, "Packages",
           "Refreshed: " + std::to_string(count) + " package(s)");
    return static_cast<int32_t>(count);
}

int32_t sky_editor_attach_viewport(SkyEditorContext* ctx, void* x11Display,
                                   uint64_t x11Window, uint32_t width,
                                   uint32_t height) {
#ifdef SKY_BRIDGE_X11
    auto* session = self(ctx);
    // The UI toolkit (Avalonia) hands us the embedded child window's XID but
    // not its X11 display; a fresh connection presents to that server-side
    // window just fine, so open one when none is provided.
    void* display = x11Display;
    if (display == nullptr) {
        session->ownDisplay = XOpenDisplay(nullptr);
        display = session->ownDisplay;
    }
    if (display == nullptr) {
        return 0;
    }
    sky::rendering_vulkan::VulkanPresentTarget target;
    target.x11Display = display;
    target.x11Window = x11Window;
    session->renderer =
        sky::rendering_vulkan::createVulkanRendererForWindow(target, width, height);
    if (session->renderer == nullptr) {
        return 0;
    }
    session->frame = std::make_unique<sky::editor::FrameBuilder>(session->context,
                                                                 *session->renderer);
    return 1;
#else
    (void)ctx; (void)x11Display; (void)x11Window; (void)width; (void)height;
    return 0;
#endif
}

void sky_editor_render_viewport(SkyEditorContext* ctx, uint32_t width,
                                uint32_t height) {
#ifdef SKY_BRIDGE_X11
    auto* session = self(ctx);
    if (session->renderer == nullptr || session->frame == nullptr) {
        return;
    }
    session->frame->setCamera(session->camera.pose(), // the editor orbit view
                              session->camera.orthoHeight());
    session->renderer->submit(session->frame->build(width, height));
    session->renderer->renderFrame();
#else
    (void)ctx; (void)width; (void)height;
#endif
}

int32_t sky_editor_render_offscreen(SkyEditorContext* ctx, uint32_t width,
                                    uint32_t height, uint8_t* out_rgba,
                                    int32_t out_length) {
#ifdef SKY_BRIDGE_VULKAN
    if (width == 0 || height == 0 || out_rgba == nullptr ||
        out_length < int32_t(width * height * 4)) {
        return 0;
    }
    auto* session = self(ctx);
    if (session->offscreen == nullptr || session->offscreenWidth != width ||
        session->offscreenHeight != height) {
        session->offscreenFrame.reset();
        session->offscreen = sky::rendering_vulkan::createVulkanRenderer(width, height);
        if (session->offscreen == nullptr) {
            return 0;
        }
        session->offscreenFrame = std::make_unique<sky::editor::FrameBuilder>(
            session->context, *session->offscreen);
        session->offscreenWidth = width;
        session->offscreenHeight = height;
    }
    // Advance the simulation while playing (physics, scripts, ECS).
    if (session->context.playMode->state() == sky::editor::PlayModeState::Playing) {
        session->context.playMode->tickFrame(1.0 / 60.0);
        session->context.tickScripts(1.0 / 60.0);
    }
    session->offscreenFrame->setCamera(session->camera.pose(),
                                       session->camera.orthoHeight());
    session->offscreen->submit(session->offscreenFrame->build(width, height));
    session->offscreen->renderFrame();
    const auto pixels = session->offscreen->readbackFrame();
    if (pixels.size() != std::size_t(width) * height * 4) {
        return 0;
    }
    std::memcpy(out_rgba, pixels.data(), pixels.size());
    return 1;
#else
    (void)ctx; (void)width; (void)height; (void)out_rgba; (void)out_length;
    return 0;
#endif
}

int32_t sky_editor_render_game_offscreen(SkyEditorContext* ctx, uint32_t width,
                                         uint32_t height, uint8_t* out_rgba,
                                         int32_t out_length) {
#ifdef SKY_BRIDGE_VULKAN
    if (width == 0 || height == 0 || out_rgba == nullptr ||
        out_length < int32_t(width * height * 4)) {
        return 0;
    }
    auto* session = self(ctx);
    if (session->gameOffscreen == nullptr || session->gameWidth != width ||
        session->gameHeight != height) {
        session->gameFrame.reset();
        session->gameOffscreen = sky::rendering_vulkan::createVulkanRenderer(width, height);
        if (session->gameOffscreen == nullptr) {
            return 0;
        }
        session->gameFrame = std::make_unique<sky::editor::FrameBuilder>(
            session->context, *session->gameOffscreen);
        session->gameWidth = width;
        session->gameHeight = height;
    }
    // No camera override: the frame builder renders through the scene's Main
    // Camera — the runtime/game view. Simulation is advanced by the Scene
    // viewport's tick.
    session->gameFrame->setCamera(std::nullopt);
    session->gameOffscreen->submit(session->gameFrame->build(width, height));
    session->gameOffscreen->renderFrame();
    const auto pixels = session->gameOffscreen->readbackFrame();
    if (pixels.size() != std::size_t(width) * height * 4) {
        return 0;
    }
    std::memcpy(out_rgba, pixels.data(), pixels.size());
    return 1;
#else
    (void)ctx; (void)width; (void)height; (void)out_rgba; (void)out_length;
    return 0;
#endif
}

void sky_editor_viewport_orbit(SkyEditorContext* ctx, float delta_yaw_degrees,
                               float delta_pitch_degrees) {
    self(ctx)->camera.orbit(delta_yaw_degrees, delta_pitch_degrees);
}

void sky_editor_viewport_pan(SkyEditorContext* ctx, float delta_right,
                             float delta_up) {
    self(ctx)->camera.pan(delta_right, delta_up);
}

void sky_editor_viewport_zoom(SkyEditorContext* ctx, float factor) {
    self(ctx)->camera.zoom(factor);
}

SkyObjectId sky_editor_pick(SkyEditorContext* ctx, float pixel_x, float pixel_y,
                            uint32_t width, uint32_t height) {
    auto* session = self(ctx);
    return pickObject(session->context, session->camera, pixel_x, pixel_y, width,
                      height)
        .value;
}

void sky_editor_frame_object(SkyEditorContext* ctx, SkyObjectId object) {
    auto* session = self(ctx);
    if (session->context.objects->exists(handle(object))) {
        session->camera.target =
            session->context.objects->worldTransform(handle(object)).position;
    }
}

void sky_editor_camera_position(SkyEditorContext* ctx, float* out_xyz) {
    if (out_xyz == nullptr) {
        return;
    }
    const auto p = self(ctx)->camera.pose().position;
    out_xyz[0] = p.x;
    out_xyz[1] = p.y;
    out_xyz[2] = p.z;
}

void sky_editor_set_view_2d(SkyEditorContext* ctx, int32_t enabled) {
    self(ctx)->camera.orthographic = enabled != 0;
}

int32_t sky_editor_view_2d(SkyEditorContext* ctx) {
    return self(ctx)->camera.orthographic ? 1 : 0;
}

void sky_editor_look_along_axis(SkyEditorContext* ctx, int32_t axis) {
    self(ctx)->camera.lookAlong(axis);
}

void sky_editor_camera_basis(SkyEditorContext* ctx, float* out_right,
                             float* out_up, float* out_forward) {
    const auto rot = self(ctx)->camera.rotation();
    const auto fill = [](float* out, sky::core::Vec3 v) {
        if (out != nullptr) { out[0] = v.x; out[1] = v.y; out[2] = v.z; }
    };
    fill(out_right, sky::core::rotate(rot, {1.0f, 0.0f, 0.0f}));
    fill(out_up, sky::core::rotate(rot, {0.0f, 1.0f, 0.0f}));
    fill(out_forward, sky::core::rotate(rot, {0.0f, 0.0f, -1.0f}));
}

int32_t sky_editor_project(SkyEditorContext* ctx, float world_x, float world_y,
                           float world_z, uint32_t width, uint32_t height,
                           float* out_x, float* out_y) {
    float x = 0.0f, y = 0.0f;
    if (!self(ctx)->camera.project({world_x, world_y, world_z}, float(width),
                                   float(height), x, y)) {
        return 0;
    }
    if (out_x != nullptr) *out_x = x;
    if (out_y != nullptr) *out_y = y;
    return 1;
}

void sky_editor_world_position(SkyEditorContext* ctx, SkyObjectId object,
                               float* out_xyz) {
    if (out_xyz == nullptr) {
        return;
    }
    const auto transform = ec(ctx).objects->worldTransform(handle(object));
    out_xyz[0] = transform.position.x;
    out_xyz[1] = transform.position.y;
    out_xyz[2] = transform.position.z;
}

// --- Transforms across spaces: world, relative-to-self, relative-to-parent --

void sky_editor_get_world_transform(SkyEditorContext* ctx, SkyObjectId object,
                                    float* out_position, float* out_rotation,
                                    float* out_scale) {
    const auto t = ec(ctx).objects->worldTransform(handle(object));
    if (out_position != nullptr) {
        out_position[0] = t.position.x;
        out_position[1] = t.position.y;
        out_position[2] = t.position.z;
    }
    if (out_rotation != nullptr) {
        out_rotation[0] = t.rotation.x;
        out_rotation[1] = t.rotation.y;
        out_rotation[2] = t.rotation.z;
        out_rotation[3] = t.rotation.w;
    }
    if (out_scale != nullptr) {
        out_scale[0] = t.scale.x;
        out_scale[1] = t.scale.y;
        out_scale[2] = t.scale.z;
    }
}

/// World space: place the object at an absolute world position (written back
/// through the local-only model, correct for nested objects).
void sky_editor_set_world_position(SkyEditorContext* ctx, SkyObjectId object,
                                   float x, float y, float z) {
    beginTransformEdit(self(ctx), handle(object));
    auto& objects = *ec(ctx).objects;
    auto world = objects.worldTransform(handle(object));
    world.position = {x, y, z};
    sky::object::setWorldTransform(objects, handle(object), world);
}

/// Relative to self: translate along the object's own (rotated) axes.
void sky_editor_translate_self(SkyEditorContext* ctx, SkyObjectId object, float dx,
                               float dy, float dz) {
    beginTransformEdit(self(ctx), handle(object));
    auto& objects = *ec(ctx).objects;
    auto world = objects.worldTransform(handle(object));
    world.position = world.position + sky::core::rotate(world.rotation, {dx, dy, dz});
    sky::object::setWorldTransform(objects, handle(object), world);
}

/// World space: rotate about a world axis through the object's origin.
void sky_editor_rotate_world_axis(SkyEditorContext* ctx, SkyObjectId object,
                                  float axis_x, float axis_y, float axis_z,
                                  float radians) {
    const float length =
        std::sqrt(axis_x * axis_x + axis_y * axis_y + axis_z * axis_z);
    if (length < 1e-6f) {
        return;
    }
    const float s = std::sin(radians / 2.0f) / length;
    const float c = std::cos(radians / 2.0f);
    const sky::core::Quat delta{axis_x * s, axis_y * s, axis_z * s, c};
    beginTransformEdit(self(ctx), handle(object));
    auto& objects = *ec(ctx).objects;
    auto world = objects.worldTransform(handle(object));
    world.rotation = delta * world.rotation;
    sky::object::setWorldTransform(objects, handle(object), world);
}

/// Relative to parent: set the local rotation from Euler angles (degrees,
/// applied yaw(Y) then pitch(X) then roll(Z)).
void sky_editor_set_local_euler(SkyEditorContext* ctx, SkyObjectId object,
                                float x_degrees, float y_degrees, float z_degrees) {
    const auto axisAngle = [](float degrees, float ax, float ay, float az) {
        const float radians = degrees * 3.14159265358979f / 180.0f;
        const float s = std::sin(radians / 2.0f);
        const float c = std::cos(radians / 2.0f);
        return sky::core::Quat{ax * s, ay * s, az * s, c};
    };
    beginTransformEdit(self(ctx), handle(object));
    auto local = ec(ctx).objects->localTransform(handle(object));
    local.rotation = axisAngle(y_degrees, 0, 1, 0) * axisAngle(x_degrees, 1, 0, 0) *
                     axisAngle(z_degrees, 0, 0, 1);
    ec(ctx).objects->setLocalTransform(handle(object), local);
}

// --- Play mode -----------------------------------------------------------

int32_t sky_editor_play(SkyEditorContext* ctx) {
    auto& context = ec(ctx);
    // Snapshot the scene only when entering play from editing (not on resume
    // from pause), so leaving play can restore the original state.
    const bool entering =
        context.playMode->state() == sky::editor::PlayModeState::Editing;
    context.playMode->setScene(context.activeScene);
    const bool ok = context.playMode->play();
    if (ok && entering) {
        context.beginPlay();
        logMsg(self(ctx), sky::core::LogLevel::Info, "Play", "Entered play mode");
    }
    return ok ? 1 : 0;
}

void sky_editor_pause(SkyEditorContext* ctx) { ec(ctx).playMode->pause(); }
void sky_editor_stop(SkyEditorContext* ctx) {
    auto& context = ec(ctx);
    const bool wasRunning =
        context.playMode->state() != sky::editor::PlayModeState::Editing;
    context.playMode->stop();
    if (wasRunning) {
        context.endPlay();
        logMsg(self(ctx), sky::core::LogLevel::Info, "Play", "Exited play mode");
    }
}

/// 0 = editing, 1 = playing, 2 = paused.
int32_t sky_editor_play_state(SkyEditorContext* ctx) {
    return static_cast<int32_t>(ec(ctx).playMode->state());
}

void sky_editor_detach_viewport(SkyEditorContext* ctx) {
#ifdef SKY_BRIDGE_X11
    auto* session = self(ctx);
    session->frame.reset();
    session->renderer.reset();
    if (session->ownDisplay != nullptr) {
        XCloseDisplay(session->ownDisplay);
        session->ownDisplay = nullptr;
    }
#else
    (void)ctx;
#endif
}

} // extern "C"
