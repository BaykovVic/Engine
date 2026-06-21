#include "sky/editor/bridge/editor_bridge.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

#include "editor_context.hpp"
#include "frame_builder.hpp"

#ifdef SKY_BRIDGE_VULKAN
#include "sky/rendering_vulkan/vulkan_backend.hpp"
#include <X11/Xlib.h>
#endif

using sky::editor::EditorContext;

namespace {

/// One editor session: the engine assembly plus, once a viewport is attached,
/// a window-bound renderer and the shared frame builder.
struct BridgeSession {
    EditorContext context;
#ifdef SKY_BRIDGE_VULKAN
    Display* ownDisplay = nullptr; // opened when the caller provides no display
    std::unique_ptr<sky::rendering_vulkan::VulkanRenderer> renderer;
    std::unique_ptr<sky::editor::FrameBuilder> frame;

    ~BridgeSession() {
        frame.reset();
        renderer.reset(); // releases the Vulkan surface before the display closes
        if (ownDisplay != nullptr) {
            XCloseDisplay(ownDisplay);
        }
    }
#endif
};

BridgeSession* self(SkyEditorContext* ctx) {
    return reinterpret_cast<BridgeSession*>(ctx);
}

EditorContext& ec(SkyEditorContext* ctx) { return self(ctx)->context; }

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
    return reinterpret_cast<SkyEditorContext*>(new BridgeSession());
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
    auto t = ec(ctx).objects->localTransform(handle(object));
    t.position = {x, y, z};
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

SkyObjectId sky_editor_create_primitive(SkyEditorContext* ctx,
                                        SkyPrimitiveKind kind, const char* name) {
    const auto primitive = static_cast<sky::scene::PrimitiveKind>(kind);
    return ec(ctx).createPrimitive(primitive, name != nullptr ? name : "Object").value;
}

SkyObjectId sky_editor_duplicate(SkyEditorContext* ctx, SkyObjectId object) {
    return ec(ctx).duplicateObject(handle(object)).value;
}

void sky_editor_delete(SkyEditorContext* ctx, SkyObjectId object) {
    ec(ctx).destroyObject(handle(object));
}

int32_t sky_editor_attach_viewport(SkyEditorContext* ctx, void* x11Display,
                                   uint64_t x11Window, uint32_t width,
                                   uint32_t height) {
#ifdef SKY_BRIDGE_VULKAN
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
#ifdef SKY_BRIDGE_VULKAN
    auto* session = self(ctx);
    if (session->renderer == nullptr || session->frame == nullptr) {
        return;
    }
    session->renderer->submit(session->frame->build(width, height));
    session->renderer->renderFrame();
#else
    (void)ctx; (void)width; (void)height;
#endif
}

void sky_editor_detach_viewport(SkyEditorContext* ctx) {
#ifdef SKY_BRIDGE_VULKAN
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
