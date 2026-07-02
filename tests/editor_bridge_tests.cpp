// Drives the engine purely through the C ABI the Avalonia (.NET) editor will
// P/Invoke — no C++ engine types — proving the marshalling surface round-trips.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "sky/editor/bridge/editor_bridge.h"
#include "sky/package/package_world.hpp"
#include "sky/platform/platform_services.hpp"
#include "sky/platform/x11_window_system.hpp"
#include "sky/serialization/backends.hpp"
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

    // project is the inverse of the pick ray: the framed cube's world origin
    // lands near the centre of the viewport (the gizmo overlay relies on this).
    float worldPosition[3] = {0.0f, 0.0f, 0.0f};
    sky_editor_world_position(ctx, cube, worldPosition);
    float screenX = 0.0f, screenY = 0.0f;
    const int visible = sky_editor_project(ctx, worldPosition[0], worldPosition[1],
                                           worldPosition[2], width, height, &screenX,
                                           &screenY);
    CHECK(visible == 1);
    CHECK(std::fabs(screenX - width / 2.0f) < width * 0.25f);
    CHECK(std::fabs(screenY - height / 2.0f) < height * 0.25f);

    sky_editor_destroy(ctx);
}

// Data-driven component fields: read the field names/values and write one back.
void testBridgeComponentFields()
{
    SkyEditorContext* ctx = sky_editor_create();
    const SkyObjectId cube =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "Fields");

    // The cube carries a Mesh Renderer (sky.mesh) with material + mesh fields.
    CHECK(sky_editor_component_field_count(ctx, cube, 0) >= 2);

    char name[64] = {0};
    sky_editor_component_field_name(ctx, cube, 0, 0, name, sizeof(name));
    CHECK(std::string(name) == "material");

    sky_editor_set_component_field(ctx, cube, 0, 0, "Stone");
    char value[64] = {0};
    sky_editor_component_field_value(ctx, cube, 0, 0, value, sizeof(value));
    CHECK(std::string(value) == "Stone");

    sky_editor_destroy(ctx);
}

// Transforms across spaces: world placement, and a self-relative translate
// that, after a 90-degree yaw, moves the object along a different world axis.
void testBridgeTransformSpaces()
{
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);
    const SkyObjectId object =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "Spaces");

    // World space: absolute placement reads back through world_position.
    sky_editor_set_world_position(ctx, object, 4.0f, 5.0f, 6.0f);
    float position[3] = {0, 0, 0};
    sky_editor_world_position(ctx, object, position);
    CHECK(std::fabs(position[0] - 4.0f) < 1e-3f);
    CHECK(std::fabs(position[1] - 5.0f) < 1e-3f);
    CHECK(std::fabs(position[2] - 6.0f) < 1e-3f);

    // Yaw 90 about Y, then translate along the object's own +Z. A +90 yaw maps
    // local +Z to world +X, so the world position gains +2 on X only.
    sky_editor_set_local_euler(ctx, object, 0.0f, 90.0f, 0.0f);
    sky_editor_translate_self(ctx, object, 0.0f, 0.0f, 2.0f);
    sky_editor_world_position(ctx, object, position);
    CHECK(std::fabs(position[0] - 6.0f) < 1e-2f);
    CHECK(std::fabs(position[1] - 5.0f) < 1e-2f);
    CHECK(std::fabs(position[2] - 6.0f) < 1e-2f);

    // The world rotation reads back as the 90-degree yaw (w = cos 45).
    float rotation[4] = {0, 0, 0, 0};
    sky_editor_get_world_transform(ctx, object, nullptr, rotation, nullptr);
    CHECK(std::fabs(rotation[3] - 0.7071f) < 1e-2f);

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

// Managed Debug.Log routes into the Console log buffer: entering play starts
// the demo Rotator script, whose OnStart logs through the reverse boundary.
// Compiled only when the build carries the managed assemblies.
void testBridgeScriptLog() {
#ifdef SKY_TEST_MANAGED
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);
    CHECK(sky_editor_play(ctx) == 1);

    bool found = false;
    const int32_t count = sky_editor_log_count(ctx);
    for (int32_t i = 0; i < count && !found; ++i) {
        char buffer[256] = {0};
        sky_editor_log_text(ctx, i, buffer, sizeof(buffer));
        found = std::strstr(buffer, "Script: Rotator started") != nullptr;
    }
    CHECK(found);

    sky_editor_stop(ctx);
    sky_editor_destroy(ctx);
#endif
}

// Keyboard input reaches gameplay scripts: the demo Main Camera carries a
// WasdMover (3 units/s). Holding W for one simulated second moves it +3 on Z;
// releasing stops it. Also proves Time.DeltaTime scaling and headless play
// stepping through sky_editor_tick_play.
void testBridgeScriptInput() {
#ifdef SKY_TEST_MANAGED
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    SkyObjectId camera = 0;
    const int32_t roots = sky_editor_root_count(ctx);
    for (int32_t i = 0; i < roots && camera == 0; ++i) {
        const SkyObjectId root = sky_editor_root_at(ctx, i);
        if (nameOf(ctx, root) == "Main Camera") {
            camera = root;
        }
    }
    CHECK(camera != 0);

    float before[3] = {0};
    sky_editor_get_transform(ctx, camera, before, nullptr, nullptr);

    sky_editor_set_key_state(ctx, 'W', 1);
    CHECK(sky_editor_play(ctx) == 1);
    for (int i = 0; i < 60; ++i) {
        sky_editor_tick_play(ctx, 1.0 / 60.0);
    }
    float held[3] = {0};
    sky_editor_get_transform(ctx, camera, held, nullptr, nullptr);
    CHECK(std::fabs((held[2] - before[2]) - 3.0f) < 1e-3f);

    sky_editor_set_key_state(ctx, 'W', 0);
    for (int i = 0; i < 30; ++i) {
        sky_editor_tick_play(ctx, 1.0 / 60.0);
    }
    float released[3] = {0};
    sky_editor_get_transform(ctx, camera, released, nullptr, nullptr);
    CHECK(std::fabs(released[2] - held[2]) < 1e-4f);

    sky_editor_stop(ctx);
    sky_editor_destroy(ctx);
#endif
}

// The script-class list (reflection over the loaded managed assemblies)
// surfaces every ScriptComponent subclass for the Inspector's picker.
void testBridgeScriptClasses() {
#ifdef SKY_TEST_MANAGED
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    const int32_t count = sky_editor_script_class_count(ctx);
    CHECK(count >= 3); // Rotator, Spinner, WasdMover at minimum
    bool foundRotator = false;
    for (int32_t i = 0; i < count; ++i) {
        char buffer[128] = {0};
        sky_editor_script_class_name(ctx, i, buffer, sizeof(buffer));
        if (std::strcmp(buffer, "SkyEngine.Tests.Rotator") == 0) {
            foundRotator = true;
        }
    }
    CHECK(foundRotator);
    // Out of range reads back as empty.
    char overflow[8] = {0};
    sky_editor_script_class_name(ctx, count, overflow, sizeof(overflow));
    CHECK(overflow[0] == '\0');

    sky_editor_destroy(ctx);
#endif
}

// Serializable script fields: the managed Rotator declares `public float
// Speed = 90`. The Inspector ABI surfaces it with its default; authoring it
// to 180 stores on the component and reaches the instance at play start —
// one simulated second then yaws the pyramid 180° instead of 90°.
void testBridgeScriptFields() {
#ifdef SKY_TEST_MANAGED
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    SkyObjectId pyramid = 0;
    const int32_t roots = sky_editor_root_count(ctx);
    for (int32_t i = 0; i < roots && pyramid == 0; ++i) {
        const SkyObjectId root = sky_editor_root_at(ctx, i);
        if (nameOf(ctx, root) == "Pyramid (obj)") {
            pyramid = root;
        }
    }
    CHECK(pyramid != 0);
    int32_t script = -1;
    const int32_t count = sky_editor_component_count(ctx, pyramid);
    for (int32_t i = 0; i < count; ++i) {
        char type[64] = {0};
        sky_editor_component_type(ctx, pyramid, i, type, sizeof(type));
        if (std::strcmp(type, "sky.script") == 0) {
            script = i;
        }
    }
    CHECK(script >= 0);

    // The managed class's field list surfaces Speed with its declared default.
    CHECK(sky_editor_script_field_count(ctx, pyramid, script) >= 1);
    int32_t speed = -1;
    for (int32_t i = 0; i < sky_editor_script_field_count(ctx, pyramid, script); ++i) {
        char name[64] = {0};
        sky_editor_script_field_name(ctx, pyramid, script, i, name, sizeof(name));
        if (std::strcmp(name, "Speed") == 0) {
            speed = i;
        }
    }
    CHECK(speed >= 0);
    char text[64] = {0};
    sky_editor_script_field_type(ctx, pyramid, script, speed, text, sizeof(text));
    CHECK(std::strcmp(text, "float") == 0);
    sky_editor_script_field_value(ctx, pyramid, script, speed, text, sizeof(text));
    CHECK(std::strcmp(text, "90") == 0);

    // Author 180 and verify the read-back and the play-time effect.
    sky_editor_set_script_field(ctx, pyramid, script, speed, "180");
    sky_editor_script_field_value(ctx, pyramid, script, speed, text, sizeof(text));
    CHECK(std::strcmp(text, "180") == 0);

    CHECK(sky_editor_play(ctx) == 1);
    for (int i = 0; i < 60; ++i) {
        sky_editor_tick_play(ctx, 1.0 / 60.0);
    }
    float rotation[4] = {0};
    sky_editor_get_transform(ctx, pyramid, nullptr, rotation, nullptr);
    // 180° yaw: quaternion (0, ±1, 0, ~0).
    CHECK(std::fabs(rotation[1]) > 0.999f);
    CHECK(std::fabs(rotation[3]) < 0.01f);
    sky_editor_stop(ctx);

    // The authored value is undoable back to the declared default.
    CHECK(sky_editor_undo(ctx) == 1);
    sky_editor_script_field_value(ctx, pyramid, script, speed, text, sizeof(text));
    CHECK(std::strcmp(text, "90") == 0);

    sky_editor_destroy(ctx);
#endif
}

// User scripts, the full project workflow: author a .cs under Assets/Scripts,
// compile + load it through the ABI (dotnet build behind the scenes), pick the
// class on an object, play — the user's code drives the object.
void testBridgeUserScripts() {
#ifdef SKY_TEST_MANAGED
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    char root[512] = {0};
    sky_editor_assets_root(ctx, root, sizeof(root));
    CHECK(root[0] != '\0');
    const std::filesystem::path scriptsDir = std::filesystem::path(root) / "Scripts";
    // The demo assets root is stable across runs: clear anything an earlier
    // interrupted run left behind before authoring this test's script.
    std::error_code stale;
    std::filesystem::remove_all(scriptsDir, stale);
    std::filesystem::create_directories(scriptsDir);
    {
        std::ofstream source(scriptsDir / "Lifter.cs");
        source << "namespace SkyProject;\n"
               << "public class Lifter : SkyEngine.ScriptComponent\n"
               << "{\n"
               << "    public float Height = 2.0f;\n"
               << "    public override void OnUpdate(double dt) "
               << "{ SetLocalPosition(0.0f, Height, 0.0f); }\n"
               << "}\n";
    }

    // Compile + load; the class list now carries the project class.
    CHECK(sky_editor_reload_scripts(ctx) == 1);
    bool found = false;
    for (int32_t i = 0; i < sky_editor_script_class_count(ctx) && !found; ++i) {
        char name[128] = {0};
        sky_editor_script_class_name(ctx, i, name, sizeof(name));
        found = std::strcmp(name, "SkyProject.Lifter") == 0;
    }
    CHECK(found);

    // Attach it and play: the user's OnUpdate lifts the object to Height.
    const SkyObjectId object =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "Lifted");
    CHECK(object != 0);
    sky_editor_add_component(ctx, object, "sky.script");
    int32_t script = -1;
    for (int32_t i = 0; i < sky_editor_component_count(ctx, object); ++i) {
        char type[64] = {0};
        sky_editor_component_type(ctx, object, i, type, sizeof(type));
        if (std::strcmp(type, "sky.script") == 0) {
            script = i;
        }
    }
    CHECK(script >= 0);
    sky_editor_set_component_field(ctx, object, script, 0, "SkyProject.Lifter");
    // Its serializable field surfaces like any script's.
    CHECK(sky_editor_script_field_count(ctx, object, script) == 1);

    CHECK(sky_editor_play(ctx) == 1);
    sky_editor_tick_play(ctx, 1.0 / 60.0);
    float position[3] = {0};
    sky_editor_get_transform(ctx, object, position, nullptr, nullptr);
    CHECK(std::fabs(position[1] - 2.0f) < 1e-4f);
    sky_editor_stop(ctx);

    sky_editor_destroy(ctx);
    // The demo assets root is shared and stable: leave no user scripts behind,
    // or every future context would recompile them at startup.
    std::error_code cleanup;
    std::filesystem::remove_all(scriptsDir, cleanup);
#endif
}

std::string componentField(SkyEditorContext* ctx, SkyObjectId object,
                           const char* typeId, int32_t fieldIndex) {
    for (int32_t i = 0; i < sky_editor_component_count(ctx, object); ++i) {
        char type[64] = {0};
        sky_editor_component_type(ctx, object, i, type, sizeof(type));
        if (std::strcmp(type, typeId) == 0) {
            char value[256] = {0};
            sky_editor_component_field_value(ctx, object, i, fieldIndex, value,
                                             sizeof(value));
            return value;
        }
    }
    return {};
}

// Prefabs: saving the demo pyramid captures its components, fields and
// transform; instantiating rebuilds an identical object (undoable).
void testBridgePrefabs() {
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    SkyObjectId pyramid = 0;
    for (int32_t i = 0; i < sky_editor_root_count(ctx) && pyramid == 0; ++i) {
        const SkyObjectId root = sky_editor_root_at(ctx, i);
        if (nameOf(ctx, root) == "Pyramid (obj)") {
            pyramid = root;
        }
    }
    CHECK(pyramid != 0);

    const auto path = std::filesystem::temp_directory_path() /
                      "sky_engine_tests" / "bridge_prefab" / "pyramid.skyprefab";
    std::filesystem::create_directories(path.parent_path());
    CHECK(sky_editor_save_prefab(ctx, pyramid, path.string().c_str()) == 1);

    const int32_t rootsBefore = sky_editor_root_count(ctx);
    const SkyObjectId copy =
        sky_editor_instantiate_prefab(ctx, path.string().c_str());
    CHECK(copy != 0);
    CHECK(copy != pyramid);
    CHECK(sky_editor_root_count(ctx) == rootsBefore + 1);
    CHECK(nameOf(ctx, copy) == "Pyramid (obj)");

    // Components and authored fields round-trip: mesh ref, material, script.
    CHECK(hasComponent(ctx, copy, "sky.mesh"));
    CHECK(hasComponent(ctx, copy, "sky.script"));
    CHECK(componentField(ctx, copy, "sky.mesh", 1) ==
          "assets://Models/pyramid.obj");
    CHECK(componentField(ctx, copy, "sky.script", 0) == "SkyEngine.Tests.Rotator");
    float scale[3] = {0};
    sky_editor_get_transform(ctx, copy, nullptr, nullptr, scale);
    CHECK(std::fabs(scale[0] - 1.4f) < 1e-5f);

    // Instantiation is one undo step.
    CHECK(sky_editor_undo(ctx) == 1);
    CHECK(sky_editor_object_exists(ctx, copy) == 0);
    CHECK(sky_editor_root_count(ctx) == rootsBefore);

    // A bad file fails cleanly.
    CHECK(sky_editor_instantiate_prefab(ctx, "/nonexistent.skyprefab") == 0);

    sky_editor_destroy(ctx);
    std::error_code cleanup;
    std::filesystem::remove_all(path.parent_path(), cleanup);
}

#ifdef SKY_TEST_MANAGED
// Attaches a sky.script with the given class to an object and returns the
// component index (asserts on failure).
int32_t attachScript(SkyEditorContext* ctx, SkyObjectId object,
                     const char* className) {
    sky_editor_add_component(ctx, object, "sky.script");
    int32_t script = -1;
    for (int32_t i = 0; i < sky_editor_component_count(ctx, object); ++i) {
        char type[64] = {0};
        sky_editor_component_type(ctx, object, i, type, sizeof(type));
        if (std::strcmp(type, "sky.script") == 0) {
            script = i;
        }
    }
    CHECK(script >= 0);
    sky_editor_set_component_field(ctx, object, script, 0, className);
    return script;
}

// Index of a serializable script field by name (asserts when absent) —
// reflection order is an implementation detail, so tests look fields up.
int32_t scriptFieldIndex(SkyEditorContext* ctx, SkyObjectId object,
                         int32_t component, const char* fieldName) {
    for (int32_t i = 0; i < sky_editor_script_field_count(ctx, object, component);
         ++i) {
        char name[64] = {0};
        sky_editor_script_field_name(ctx, object, component, i, name, sizeof(name));
        if (std::strcmp(name, fieldName) == 0) {
            return i;
        }
    }
    CHECK(false);
    return -1;
}
#endif

// Gameplay engine API from scripts: Instantiate spawns prefabs during play
// (their own scripts start too), Destroy removes the script's object, and
// Physics.Raycast sees the world. All driven end-to-end through managed code.
void testBridgeScriptEngineApi() {
#ifdef SKY_TEST_MANAGED
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    // A prefab to spawn: a plain crate saved from the demo scene.
    SkyObjectId crate = 0;
    for (int32_t i = 0; i < sky_editor_root_count(ctx) && crate == 0; ++i) {
        const SkyObjectId root = sky_editor_root_at(ctx, i);
        if (nameOf(ctx, root) == "Crate A") {
            crate = root;
        }
    }
    CHECK(crate != 0);
    const auto prefab = std::filesystem::temp_directory_path() /
                        "sky_engine_tests" / "bridge_script_api" /
                        "crate.skyprefab";
    std::filesystem::create_directories(prefab.parent_path());
    CHECK(sky_editor_save_prefab(ctx, crate, prefab.string().c_str()) == 1);

    // Spawner: 4 spawns over a simulated second (t=0, .3, .6, .9).
    const SkyObjectId spawner =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "SpawnerRig");
    const int32_t spawnerScript =
        attachScript(ctx, spawner, "SkyEngine.Tests.Spawner");
    sky_editor_set_script_field(
        ctx, spawner, spawnerScript,
        scriptFieldIndex(ctx, spawner, spawnerScript, "PrefabPath"),
        prefab.string().c_str());
    sky_editor_set_script_field(
        ctx, spawner, spawnerScript,
        scriptFieldIndex(ctx, spawner, spawnerScript, "Interval"), "0.3");

    // SelfDestruct: gone after 0.5 simulated seconds.
    const SkyObjectId doomed =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "Doomed");
    attachScript(ctx, doomed, "SkyEngine.Tests.SelfDestruct");

    // GroundProbe: raycasts down onto the terrain and logs the hit. Placed
    // away from the demo crates so the ray sees bare terrain (world y = 0).
    const SkyObjectId probe =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "Probe");
    sky_editor_set_position(ctx, probe, 10.0f, 0.0f, 10.0f);
    attachScript(ctx, probe, "SkyEngine.Tests.GroundProbe");

    const int32_t rootsBefore = sky_editor_root_count(ctx);
    CHECK(sky_editor_play(ctx) == 1);
    for (int i = 0; i < 60; ++i) {
        sky_editor_tick_play(ctx, 1.0 / 60.0);
    }

    // 4 crates spawned, one object self-destructed.
    CHECK(sky_editor_root_count(ctx) == rootsBefore + 4 - 1);
    CHECK(sky_editor_object_exists(ctx, doomed) == 0);

    // The probe hit the terrain (world y = 0) via the physics raycast.
    bool probeHit = false;
    for (int32_t i = 0; i < sky_editor_log_count(ctx) && !probeHit; ++i) {
        char buffer[256] = {0};
        sky_editor_log_text(ctx, i, buffer, sizeof(buffer));
        probeHit = std::strstr(buffer, "GroundProbe hit y=0") != nullptr;
    }
    if (!probeHit) { // dump the console on failure — the probe's log says why
        for (int32_t i = 0; i < sky_editor_log_count(ctx); ++i) {
            char buffer[256] = {0};
            sky_editor_log_text(ctx, i, buffer, sizeof(buffer));
            std::printf("LOG[%d]: %s\n", i, buffer);
        }
    }
    CHECK(probeHit);

    sky_editor_stop(ctx);
    sky_editor_destroy(ctx);
    std::error_code cleanup;
    std::filesystem::remove_all(prefab.parent_path(), cleanup);
#endif
}

// Package activation persists (Packages/sky.lock) and pulls dependencies:
// activating terrain-tools also activates noise-lib, and a fresh context
// (a new editor session) comes up with both still active.
void testBridgePackagePersistence() {
    const auto lockPath = std::filesystem::temp_directory_path() /
                          "sky_editor_packages" / "sky.lock";
    std::error_code stale;
    std::filesystem::remove(lockPath, stale); // clean slate

    int32_t terrainIndex = -1;
    int32_t noiseIndex = -1;
    const auto findPackages = [&](SkyEditorContext* ctx) {
        terrainIndex = noiseIndex = -1;
        for (int32_t i = 0; i < sky_editor_package_count(ctx); ++i) {
            char id[64] = {0};
            sky_editor_package_info(ctx, i, 0, id, sizeof(id));
            if (std::strcmp(id, "sky.terrain-tools") == 0) {
                terrainIndex = i;
            } else if (std::strcmp(id, "sky.noise-lib") == 0) {
                noiseIndex = i;
            }
        }
        CHECK(terrainIndex >= 0);
        CHECK(noiseIndex >= 0);
    };

    {
        SkyEditorContext* ctx = sky_editor_create();
        findPackages(ctx);
        CHECK(sky_editor_package_active(ctx, terrainIndex) == 0);
        // Activating the dependent activates its dependency too.
        sky_editor_package_set_active(ctx, terrainIndex, 1);
        CHECK(sky_editor_package_active(ctx, terrainIndex) == 1);
        CHECK(sky_editor_package_active(ctx, noiseIndex) == 1);
        sky_editor_destroy(ctx);
    }
    {
        // A brand-new session restores the activation state from the lock.
        SkyEditorContext* ctx = sky_editor_create();
        findPackages(ctx);
        CHECK(sky_editor_package_active(ctx, terrainIndex) == 1);
        CHECK(sky_editor_package_active(ctx, noiseIndex) == 1);
        // Deactivation persists as well.
        sky_editor_package_set_active(ctx, terrainIndex, 0);
        sky_editor_package_set_active(ctx, noiseIndex, 0);
        CHECK(sky_editor_package_active(ctx, terrainIndex) == 0);
        sky_editor_destroy(ctx);
    }
    {
        SkyEditorContext* ctx = sky_editor_create();
        findPackages(ctx);
        CHECK(sky_editor_package_active(ctx, terrainIndex) == 0);
        CHECK(sky_editor_package_active(ctx, noiseIndex) == 0);
        sky_editor_destroy(ctx);
    }
    // The demo packages root is shared and stable: leave no lock behind.
    std::filesystem::remove(lockPath, stale);
}

// Installing a package through the ABI: a local source directory lands in
// the project's Packages via the global cache and shows up in the list.
void testBridgePackageInstall() {
    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);

    // Author a source package outside the project.
    const auto base = std::filesystem::temp_directory_path() /
                      "sky_engine_tests" / "bridge_pkg_install";
    {
        // Reuse the engine's own manifest writer through a tiny local blob:
        // the bridge has no manifest-authoring ABI (by design), so the test
        // shells through the C++ helper linked into this binary.
        std::filesystem::create_directories(base / "pkg.extra");
    }
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage =
        sky::serialization::createFileSerializationBackend(*fileSystem);
    sky::package::PackageManifest manifest;
    manifest.packageId = "pkg.extra";
    manifest.version = "0.9.0";
    manifest.displayName = "Extra";
    manifest.rootPath = base / "pkg.extra";
    CHECK(sky::package::savePackageManifest(*storage, manifest));

    const int32_t before = sky_editor_package_count(ctx);
    CHECK(sky_editor_package_install(ctx, manifest.rootPath.string().c_str()) == 1);
    CHECK(sky_editor_package_count(ctx) == before + 1);
    CHECK(sky_editor_package_install(ctx, "/nonexistent") == 0);

    sky_editor_destroy(ctx);
    // Shared demo roots: leave no installed copy, cache or lock behind.
    std::error_code cleanup;
    std::filesystem::remove_all(base, cleanup);
    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                    "sky_editor_packages" / "pkg.extra-0.9.0",
                                cleanup);
    std::filesystem::remove(std::filesystem::temp_directory_path() /
                                "sky_editor_packages" / "sky.lock",
                            cleanup);
    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                    "sky_editor_pkg_cache",
                                cleanup);
}

// A package that ships code: its Runtime/*.cs compile into the user assembly
// on activation, the class appears in the picker and drives objects in play;
// deactivation recompiles without it and the class disappears.
void testBridgePackageCode() {
#ifdef SKY_TEST_MANAGED
    // Author a code-carrying package.
    const auto base = std::filesystem::temp_directory_path() /
                      "sky_engine_tests" / "bridge_pkg_code";
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto storage =
        sky::serialization::createFileSerializationBackend(*fileSystem);
    sky::package::PackageManifest manifest;
    manifest.packageId = "pkg.gameplay";
    manifest.version = "1.0.0";
    manifest.displayName = "Gameplay Pack";
    manifest.rootPath = base / "pkg.gameplay";
    CHECK(sky::package::savePackageManifest(*storage, manifest));
    std::filesystem::create_directories(manifest.rootPath / "Runtime");
    {
        std::ofstream source(manifest.rootPath / "Runtime" / "Riser.cs");
        source << "namespace SkyPackages;\n"
               << "public class Riser : SkyEngine.ScriptComponent\n"
               << "{\n"
               << "    public float Height = 3.0f;\n"
               << "    public override void OnUpdate(double dt) "
               << "{ SetLocalPosition(0.0f, Height, 0.0f); }\n"
               << "}\n";
    }

    SkyEditorContext* ctx = sky_editor_create();
    CHECK(ctx != nullptr);
    CHECK(sky_editor_package_install(ctx, manifest.rootPath.string().c_str()) == 1);

    const auto hasClass = [&](const char* name) {
        for (int32_t i = 0; i < sky_editor_script_class_count(ctx); ++i) {
            char buffer[128] = {0};
            sky_editor_script_class_name(ctx, i, buffer, sizeof(buffer));
            if (std::strcmp(buffer, name) == 0) {
                return true;
            }
        }
        return false;
    };
    const auto setActive = [&](int32_t active) {
        for (int32_t i = 0; i < sky_editor_package_count(ctx); ++i) {
            char id[64] = {0};
            sky_editor_package_info(ctx, i, 0, id, sizeof(id));
            if (std::strcmp(id, "pkg.gameplay") == 0) {
                sky_editor_package_set_active(ctx, i, active);
                return;
            }
        }
        CHECK(false);
    };

    CHECK(!hasClass("SkyPackages.Riser")); // inactive: not compiled in
    setActive(1);
    CHECK(hasClass("SkyPackages.Riser"));

    // The package class works like any script: attach, play, it moves things.
    const SkyObjectId cube =
        sky_editor_create_primitive(ctx, SKY_PRIMITIVE_CUBE, "Risen");
    attachScript(ctx, cube, "SkyPackages.Riser");
    CHECK(sky_editor_play(ctx) == 1);
    sky_editor_tick_play(ctx, 1.0 / 60.0);
    float position[3] = {0};
    sky_editor_get_transform(ctx, cube, position, nullptr, nullptr);
    CHECK(std::fabs(position[1] - 3.0f) < 1e-4f);
    sky_editor_stop(ctx);

    // Deactivation recompiles without the package: the class is gone.
    setActive(0);
    CHECK(!hasClass("SkyPackages.Riser"));

    sky_editor_destroy(ctx);
    // Shared demo roots: leave nothing behind.
    std::error_code cleanup;
    std::filesystem::remove_all(base, cleanup);
    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                    "sky_editor_packages" / "pkg.gameplay-1.0.0",
                                cleanup);
    std::filesystem::remove(std::filesystem::temp_directory_path() /
                                "sky_editor_packages" / "sky.lock",
                            cleanup);
    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                    "sky_editor_pkg_cache",
                                cleanup);
#endif
}

int main() {
    testBridgeLifecycleAndHierarchy();
    testBridgeAuthoring();
    testBridgePicking();
    testBridgeTransformSpaces();
    testBridgeComponentFields();
    testBridgeViewport();
    testBridgeScriptLog();
    testBridgeScriptInput();
    testBridgeScriptClasses();
    testBridgeScriptFields();
    testBridgeUserScripts();
    testBridgePrefabs();
    testBridgeScriptEngineApi();
    testBridgePackagePersistence();
    testBridgePackageInstall();
    testBridgePackageCode();
    return sky::test::summary("editor_bridge_tests");
}
