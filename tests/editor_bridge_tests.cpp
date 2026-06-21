// Drives the engine purely through the C ABI the Avalonia (.NET) editor will
// P/Invoke — no C++ engine types — proving the marshalling surface round-trips.

#include <cstdio>
#include <cstring>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "sky/editor/bridge/editor_bridge.h"
#include "sky/platform/x11_window_system.hpp"
#include "sky_test.hpp"

namespace {

std::string nameOf(SkyEditorContext* ctx, SkyObjectId object) {
    char buffer[128] = {0};
    sky_editor_object_name(ctx, object, buffer, sizeof(buffer));
    return std::string(buffer);
}

bool hasComponent(SkyEditorContext* ctx, SkyObjectId object,
                  const std::string& typeId) {
    const int32_t count = sky_editor_component_count(ctx, object);
    for (int32_t i = 0; i < count; ++i) {
        char buffer[64] = {0};
        sky_editor_component_type(ctx, object, i, buffer, sizeof(buffer));
        if (typeId == buffer) {
            return true;
        }
    }
    return false;
}

void testBridgeLifecycleAndHierarchy() {
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    // The demo scene populated named roots.
    const int32_t roots = sky_editor_root_count(ctx);
    CHECK(roots > 0);
    for (int32_t i = 0; i < roots; ++i) {
        const SkyObjectId root = sky_editor_root_at(ctx, i);
        CHECK(sky_editor_object_exists(ctx, root) == 1);
        CHECK(!nameOf(ctx, root).empty());
        // Child enumeration is callable and well-bounded for every root.
        const int32_t children = sky_editor_child_count(ctx, root);
        CHECK(children >= 0);
        CHECK(sky_editor_child_at(ctx, root, children) == 0); // out of range
    }
    // Out-of-range root access returns the invalid handle.
    CHECK(sky_editor_root_at(ctx, roots) == 0);

    sky_editor_destroy(ctx);
}

void testBridgeAuthoring() {
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);
    const int32_t before = sky_editor_root_count(ctx);

    // Create a cube primitive: a new root carrying a Mesh Renderer.
    const SkyObjectId cube =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "Bridge Cube");
    CHECK(cube != 0);
    CHECK(sky_editor_root_count(ctx) == before + 1);
    CHECK(nameOf(ctx, cube) == "Bridge Cube");
    CHECK(hasComponent(ctx, cube, "sky.mesh"));

    // Move it; the transform reads back through the C ABI.
    sky_editor_set_position(ctx, cube, 3.0f, 4.0f, 5.0f);
    float position[3] = {0, 0, 0};
    sky_editor_get_transform(ctx, cube, position, nullptr, nullptr);
    CHECK(position[0] == 3.0f);
    CHECK(position[1] == 4.0f);
    CHECK(position[2] == 5.0f);

    // Duplicate carries the name (+ " Copy") and the component across.
    const SkyObjectId copy = sky_editor_duplicate(ctx, cube);
    CHECK(copy != 0);
    CHECK(copy != cube);
    CHECK(nameOf(ctx, copy) == "Bridge Cube Copy");
    CHECK(hasComponent(ctx, copy, "sky.mesh"));

    // Delete removes it.
    sky_editor_delete(ctx, copy);
    CHECK(sky_editor_object_exists(ctx, copy) == 0);

    // Name query reports the full length even when the buffer truncates.
    char tiny[4] = {0};
    const int32_t full = sky_editor_object_name(ctx, cube, tiny, sizeof(tiny));
    CHECK(full == static_cast<int32_t>(std::strlen("Bridge Cube")));
    CHECK(std::strlen(tiny) == 3); // truncated to capacity - 1

    sky_editor_destroy(ctx);
}

// Editor orbit camera + click-to-pick. Pure scene math, so it needs no
// window: frame the camera on a known cube and confirm the centre ray hits it,
// before and after an orbit (which moves the camera but keeps the target).
void testBridgePicking()
{
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    const SkyObjectId cube =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "Pick Target");
    CHECK(cube != 0);

    sky_editor_frame_object(ctx, cube); // orbit target = cube (origin)
    sky_editor_viewport_zoom(ctx, 0.2f);
    sky_editor_viewport_zoom(ctx, 0.2f); // pull in close

    const std::uint32_t width = 400;
    const std::uint32_t height = 300;
    const SkyObjectId hit = sky_editor_pick(ctx, width / 2.0f, height / 2.0f, width, height);
    CHECK(hit == cube);

    // After orbiting, the camera has moved but still frames the cube, so the
    // centre ray keeps hitting it.
    sky_editor_viewport_orbit(ctx, 40.0f, 12.0f);
    const SkyObjectId hit2 = sky_editor_pick(ctx, width / 2.0f, height / 2.0f, width, height);
    CHECK(hit2 == cube);

    sky_editor_destroy(ctx);
}

// Drives the viewport ABI exactly as the Avalonia editor does: hand the
// bridge a native window XID (and let it open its own X11 display, as it does
// for Avalonia's embedded surface), render the scene, and confirm the frame
// reached the window.
void testBridgeViewport()
{
    const std::uint32_t width = 200;
    const std::uint32_t height = 150;

    auto windows = sky::platform::createX11WindowSystem();
    if (windows == nullptr)
    {
        std::puts("editor_bridge_tests: no X display, skipping viewport case");
        return;
    }
    const auto window = windows->createWindow({"Sky Bridge Viewport", width, height, false});
    CHECK(window.isValid());
    CHECK(windows->pumpEvents());

    void* display = nullptr;
    std::uint64_t xid = 0;
    CHECK(windows->nativeHandles(window, &display, &xid));

    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    // Pass a null display so the bridge opens its own connection — the path
    // the Avalonia NativeControlHost takes.
    const int attached = sky_editor_attach_viewport(ctx, nullptr, xid, width, height);
    CHECK(attached == 1);
    if (attached == 1)
    {
        for (int frame = 0; frame < 3; ++frame)
        {
            sky_editor_render_viewport(ctx, width, height);
            windows->pumpEvents();
        }

        // The scene's sky fills the top of the frame: a bright, non-black,
        // blue-leaning pixel proves real content reached the window.
        auto* xdisplay = static_cast<Display*>(display);
        XSync(xdisplay, False);
        XImage* image = XGetImage(xdisplay, static_cast<Window>(xid),
                                  int(width / 2), 8, 1, 1, AllPlanes, ZPixmap);
        CHECK(image != nullptr);
        if (image != nullptr)
        {
            const unsigned long pixel = XGetPixel(image, 0, 0);
            const auto red = (pixel & image->red_mask) >> 16;
            const auto green = (pixel & image->green_mask) >> 8;
            const auto blue = pixel & image->blue_mask;
            CHECK(red + green + blue > 120); // not the black clear colour
            CHECK(blue >= red);              // sky leans blue
            XDestroyImage(image);
        }
    }

    sky_editor_detach_viewport(ctx);
    sky_editor_destroy(ctx);
    windows->destroyWindow(window);
}

} // namespace

int main() {
    testBridgeLifecycleAndHierarchy();
    testBridgeAuthoring();
    testBridgePicking();
    testBridgeViewport();
    return sky::test::summary("editor_bridge_tests");
}
