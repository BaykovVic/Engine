// Drives the engine purely through the C ABI the Avalonia (.NET) editor will
// P/Invoke — no C++ engine types — proving the marshalling surface round-trips.

#include <cstring>
#include <string>

#include "sky/editor/bridge/editor_bridge.h"
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

} // namespace

int main() {
    testBridgeLifecycleAndHierarchy();
    testBridgeAuthoring();
    return sky::test::summary("editor_bridge_tests");
}
