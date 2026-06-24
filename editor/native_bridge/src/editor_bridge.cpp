#include "sky/editor/bridge/editor_bridge.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <memory>
#include <string>

#include "editor_camera.hpp"
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
struct BridgeSession {
    EditorContext context;
    sky::editor::EditorCamera camera;
#ifdef SKY_BRIDGE_VULKAN
    // Offscreen path: the editor viewport renders to a texture it blits into a
    // normal UI control (so it receives input and hosts overlays). No window
    // surface — works wherever Vulkan does.
    std::unique_ptr<sky::rendering_vulkan::VulkanRenderer> offscreen;
    std::unique_ptr<sky::editor::FrameBuilder> offscreenFrame;
    std::uint32_t offscreenWidth = 0;
    std::uint32_t offscreenHeight = 0;
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

sky::object::ObjectHandle handle(SkyObjectId id) {
    return sky::object::ObjectHandle{id};
}

/// Picks the nearest object whose scaled unit-cube AABB the camera ray
/// through the viewport pixel intersects (the editor's 3D-view pick).
sky::object::ObjectHandle pickObject(EditorContext& context,
                                     const sky::editor::EditorCamera& camera,
                                     float pixelX, float pixelY, std::uint32_t width,
                                     std::uint32_t height) {
    const auto pose = camera.pose();
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
            const float origins[] = {pose.position.x, pose.position.y, pose.position.z};
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
    session->frame->setCamera(session->camera.pose()); // the editor orbit view
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
    session->offscreenFrame->setCamera(session->camera.pose());
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
