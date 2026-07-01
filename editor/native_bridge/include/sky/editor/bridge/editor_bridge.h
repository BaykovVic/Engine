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
SKY_BRIDGE_API void sky_editor_set_scale(SkyEditorContext* ctx, SkyObjectId object,
                                         float x, float y, float z);

/* Components. */
SKY_BRIDGE_API int32_t sky_editor_component_count(SkyEditorContext* ctx,
                                                  SkyObjectId object);
/* Copies the component type id at `index` into `buffer`; returns its length. */
SKY_BRIDGE_API int32_t sky_editor_component_type(SkyEditorContext* ctx,
                                                 SkyObjectId object, int32_t index,
                                                 char* buffer, int32_t capacity);

/* Component fields (data-driven Inspector): display name, the field count and
 * each field's name / type / value as strings, and a string setter that parses
 * by the field's type. */
SKY_BRIDGE_API int32_t sky_editor_component_display_name(SkyEditorContext* ctx, SkyObjectId object, int32_t component, char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_component_field_count(SkyEditorContext* ctx, SkyObjectId object, int32_t component);
SKY_BRIDGE_API int32_t sky_editor_component_field_name(SkyEditorContext* ctx, SkyObjectId object, int32_t component, int32_t field, char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_component_field_type(SkyEditorContext* ctx, SkyObjectId object, int32_t component, int32_t field, char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_component_field_value(SkyEditorContext* ctx, SkyObjectId object, int32_t component, int32_t field, char* buffer, int32_t capacity);
SKY_BRIDGE_API void sky_editor_set_component_field(SkyEditorContext* ctx, SkyObjectId object, int32_t component, int32_t field, const char* value);

/* Terrain (Terrain panel): regenerate the terrain procedurally from a seed,
 * scattering objects; returns the scattered object count. */
SKY_BRIDGE_API int32_t sky_editor_terrain_generate(SkyEditorContext* ctx, uint64_t seed);

/* Materials (Materials panel): the material list and a data-driven field
 * surface (baseColor, roughness, metallic, emissive, the PBR map paths,
 * uvTiling, parallax) read/written as strings. */
SKY_BRIDGE_API int32_t sky_editor_material_count(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_material_name(SkyEditorContext* ctx, int32_t index, char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_material_field_count(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_material_field_name(SkyEditorContext* ctx, int32_t field, char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_material_field_value(SkyEditorContext* ctx, int32_t index, int32_t field, char* buffer, int32_t capacity);
SKY_BRIDGE_API void sky_editor_set_material_field(SkyEditorContext* ctx, int32_t index, int32_t field, const char* value);

/* Virtual file system listing (Project panel). Entries ending in '/' are
 * directories. dir is a virtual path like "project://" or "assets://textures". */
SKY_BRIDGE_API int32_t sky_editor_vfs_count(SkyEditorContext* ctx, const char* dir);
SKY_BRIDGE_API int32_t sky_editor_vfs_entry(SkyEditorContext* ctx, const char* dir, int32_t index, char* buffer, int32_t capacity);

/* Authoring. */
SKY_BRIDGE_API SkyObjectId sky_editor_create_primitive(SkyEditorContext* ctx,
                                                       SkyPrimitiveKind kind,
                                                       const char* name);
/* Creates a scene-root object with a Mesh Renderer referencing mesh_ref
 * (e.g. "assets://Models/ship.fbx"); used by Project -> Hierarchy drag-drop. */
SKY_BRIDGE_API SkyObjectId sky_editor_create_mesh_object(SkyEditorContext* ctx,
                                                         const char* name,
                                                         const char* mesh_ref);

/* Scene document lifecycle. new_scene clears to an empty scene; save_scene
 * writes the object graph to a .skybox (SKYB); open_scene replaces the scene
 * from a file. save/open return 1 on success, 0 on failure. */
SKY_BRIDGE_API void sky_editor_new_scene(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_save_scene(SkyEditorContext* ctx, const char* path);
SKY_BRIDGE_API int32_t sky_editor_open_scene(SkyEditorContext* ctx, const char* path);

/* Undo/redo. Edits (transform, field, create, duplicate, delete) are recorded
 * automatically; commit_edit ends a coalesced transform edit (call at the end
 * of a gizmo drag or after an inspector field commit). undo/redo return 1 when
 * they changed something; the *_label calls fill a human-readable action name. */
SKY_BRIDGE_API void sky_editor_commit_edit(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_undo(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_redo(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_can_undo(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_can_redo(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_undo_label(SkyEditorContext* ctx, char* buffer,
                                             int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_redo_label(SkyEditorContext* ctx, char* buffer,
                                             int32_t capacity);
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

/* Offscreen viewport: renders the scene (through the editor orbit camera)
 * into a width*height RGBA8 buffer the UI blits into a normal control — the
 * path the editor uses so the viewport receives input and hosts overlays.
 * `out_length` must be at least width*height*4. Returns 1 on success. */
SKY_BRIDGE_API int32_t sky_editor_render_offscreen(SkyEditorContext* ctx,
                                                   uint32_t width, uint32_t height,
                                                   uint8_t* out_rgba,
                                                   int32_t out_length);
/* Like render_offscreen but through the scene's Main Camera — the Game view. */
SKY_BRIDGE_API int32_t sky_editor_render_game_offscreen(SkyEditorContext* ctx,
                                                        uint32_t width,
                                                        uint32_t height,
                                                        uint8_t* out_rgba,
                                                        int32_t out_length);

/* Editor orbit-camera controls and click-to-pick. Orbit/pan deltas are in
 * the front-end's drag units (degrees for orbit, pixels for pan); zoom is a
 * multiplier (<1 closer, >1 farther). Pick returns the object id under the
 * viewport pixel, or 0 for empty space. */
SKY_BRIDGE_API void sky_editor_viewport_orbit(SkyEditorContext* ctx,
                                              float delta_yaw_degrees,
                                              float delta_pitch_degrees);
SKY_BRIDGE_API void sky_editor_viewport_pan(SkyEditorContext* ctx,
                                            float delta_right, float delta_up);
SKY_BRIDGE_API void sky_editor_viewport_zoom(SkyEditorContext* ctx, float factor);
SKY_BRIDGE_API SkyObjectId sky_editor_pick(SkyEditorContext* ctx, float pixel_x,
                                           float pixel_y, uint32_t width,
                                           uint32_t height);
SKY_BRIDGE_API void sky_editor_frame_object(SkyEditorContext* ctx,
                                            SkyObjectId object);

/* Fills the editor orbit camera's world position into a float[3]. Used by
 * the rotate gizmo to choose the screen-to-rotation sign. */
SKY_BRIDGE_API void sky_editor_camera_position(SkyEditorContext* ctx,
                                               float* out_xyz);

/* 2D view: an orthographic look at the YZ plane (along +X). Toggles the
 * editor/scene viewport only; the Game view keeps the Main Camera. */
SKY_BRIDGE_API void sky_editor_set_view_2d(SkyEditorContext* ctx, int32_t enabled);
SKY_BRIDGE_API int32_t sky_editor_view_2d(SkyEditorContext* ctx);
/* Snaps the orbit to an axis-aligned view (scene-gizmo cone click). axis:
 * 0=+X 1=-X 2=+Y 3=-Y 4=+Z 5=-Z. */
SKY_BRIDGE_API void sky_editor_look_along_axis(SkyEditorContext* ctx, int32_t axis);
/* Fills the camera's world-space basis vectors (each float[3]) so the corner
 * scene gizmo can orient its axes. Any pointer may be null. */
SKY_BRIDGE_API void sky_editor_camera_basis(SkyEditorContext* ctx, float* out_right,
                                            float* out_up, float* out_forward);

/* Projects a world point to a viewport pixel (inverse of the pick ray), so
 * the on-screen transform gizmo lines up with the render. Returns 1 when the
 * point is in front of the camera. world_position fills the object's world
 * position into a float[3]. */
SKY_BRIDGE_API int32_t sky_editor_project(SkyEditorContext* ctx, float world_x,
                                          float world_y, float world_z,
                                          uint32_t width, uint32_t height,
                                          float* out_x, float* out_y);
SKY_BRIDGE_API void sky_editor_world_position(SkyEditorContext* ctx,
                                              SkyObjectId object, float* out_xyz);

/* Transforms across spaces. get_world_transform fills float[3] position,
 * float[4] rotation (quaternion) and float[3] scale (any may be null).
 * set_world_position places the object at an absolute world position (correct
 * for nested objects). translate_self moves along the object's own axes.
 * set_local_euler sets the local (relative-to-parent) rotation from degrees.
 * get_transform / set_position remain the local (relative-to-parent) pair. */
SKY_BRIDGE_API void sky_editor_get_world_transform(SkyEditorContext* ctx,
                                                   SkyObjectId object,
                                                   float* out_position,
                                                   float* out_rotation,
                                                   float* out_scale);
SKY_BRIDGE_API void sky_editor_set_world_position(SkyEditorContext* ctx,
                                                  SkyObjectId object, float x,
                                                  float y, float z);
SKY_BRIDGE_API void sky_editor_translate_self(SkyEditorContext* ctx,
                                              SkyObjectId object, float dx,
                                              float dy, float dz);
SKY_BRIDGE_API void sky_editor_set_local_euler(SkyEditorContext* ctx,
                                               SkyObjectId object, float x_degrees,
                                               float y_degrees, float z_degrees);
/* World space: rotate the object about a world-space axis through its origin
 * by an angle in radians (axis need not be normalized). Correct for nested
 * objects (written back through the local-only model). */
SKY_BRIDGE_API void sky_editor_rotate_world_axis(SkyEditorContext* ctx,
                                                 SkyObjectId object, float axis_x,
                                                 float axis_y, float axis_z,
                                                 float radians);

/* Play mode: enter/pause/stop the running simulation (physics, scripts, ECS).
 * play_state returns 0 = editing, 1 = playing, 2 = paused. The simulation
 * advances each frame while a viewport is rendering. */
SKY_BRIDGE_API int32_t sky_editor_play(SkyEditorContext* ctx);
SKY_BRIDGE_API void sky_editor_pause(SkyEditorContext* ctx);
SKY_BRIDGE_API void sky_editor_stop(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_play_state(SkyEditorContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* SKY_EDITOR_BRIDGE_H */
