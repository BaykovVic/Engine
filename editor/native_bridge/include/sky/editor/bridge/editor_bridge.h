/* C ABI over the native engine for the Avalonia (.NET) editor.
 *
 * The Avalonia editor owns the window and the process, so it drives the C++
 * engine through P/Invoke against this flat, handle-based surface — the seed
 * of the C# editor API (the UnityEditor analogue). Everything is plain C:
 * opaque pointers, uint64 object ids, out-parameters and caller-owned
 * buffers, so it marshals cleanly from .NET without C++ name mangling.
 */
#ifndef SKY_EDITOR_BRIDGE_H
#define SKY_EDITOR_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define SKY_BRIDGE_API __declspec(dllexport)
#else
#define SKY_BRIDGE_API __attribute__((visibility("default")))
#endif

/* Opaque editor session. */
typedef struct SkyEditorContext SkyEditorContext;

/* An object id; 0 is the invalid handle. */
typedef uint64_t SkyObjectId;

/* Built-in primitive kinds (mirror sky::scene::PrimitiveKind). */
typedef enum {
    SKY_PRIMITIVE_CUBE = 0,
    SKY_PRIMITIVE_PLANE = 1,
    SKY_PRIMITIVE_SPHERE = 2
} SkyPrimitiveKind;

/* Lifecycle: assembles the whole engine plus the demo scene. */
SKY_BRIDGE_API SkyEditorContext* sky_editor_create(void);
SKY_BRIDGE_API void sky_editor_destroy(SkyEditorContext* ctx);

/* Hierarchy enumeration. */
SKY_BRIDGE_API int32_t sky_editor_root_count(SkyEditorContext* ctx);
SKY_BRIDGE_API SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index);
SKY_BRIDGE_API int32_t sky_editor_child_count(SkyEditorContext* ctx, SkyObjectId object);
SKY_BRIDGE_API SkyObjectId sky_editor_child_at(SkyEditorContext* ctx,
                                               SkyObjectId object, int32_t index);

/* Copies the object's name into `buffer` (NUL-terminated, truncated to
 * `capacity`). Returns the full name length regardless of truncation. */
SKY_BRIDGE_API int32_t sky_editor_object_name(SkyEditorContext* ctx,
                                              SkyObjectId object, char* buffer,
                                              int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_object_exists(SkyEditorContext* ctx,
                                                SkyObjectId object);

/* Local transform. `position`/`scale` are float[3], `rotation` is float[4]
 * (quaternion x,y,z,w). Any out-pointer may be null to skip it. */
SKY_BRIDGE_API void sky_editor_get_transform(SkyEditorContext* ctx,
                                             SkyObjectId object, float* position,
                                             float* rotation, float* scale);
SKY_BRIDGE_API void sky_editor_set_position(SkyEditorContext* ctx,
                                            SkyObjectId object, float x, float y,
                                            float z);

/* Components. */
SKY_BRIDGE_API int32_t sky_editor_component_count(SkyEditorContext* ctx,
                                                  SkyObjectId object);
/* Copies the component type id at `index` into `buffer`; returns its length. */
SKY_BRIDGE_API int32_t sky_editor_component_type(SkyEditorContext* ctx,
                                                 SkyObjectId object, int32_t index,
                                                 char* buffer, int32_t capacity);

/* Authoring. */
SKY_BRIDGE_API SkyObjectId sky_editor_create_primitive(SkyEditorContext* ctx,
                                                       SkyPrimitiveKind kind,
                                                       const char* name);
SKY_BRIDGE_API SkyObjectId sky_editor_duplicate(SkyEditorContext* ctx,
                                                SkyObjectId object);
SKY_BRIDGE_API void sky_editor_delete(SkyEditorContext* ctx, SkyObjectId object);

/* Viewport: attaches a Vulkan swapchain to a native window (X11 display +
 * window from the UI toolkit's embedded surface), renders the current scene
 * into it, and tears it down. Returns 1 on a successful attach, 0 otherwise. */
SKY_BRIDGE_API int32_t sky_editor_attach_viewport(SkyEditorContext* ctx,
                                                  void* x11Display,
                                                  uint64_t x11Window,
                                                  uint32_t width, uint32_t height);
SKY_BRIDGE_API void sky_editor_render_viewport(SkyEditorContext* ctx,
                                               uint32_t width, uint32_t height);
SKY_BRIDGE_API void sky_editor_detach_viewport(SkyEditorContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* SKY_EDITOR_BRIDGE_H */
