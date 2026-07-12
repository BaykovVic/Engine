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
/* Prefabs: save writes an object subtree (with components, fields and
 * physics binding) to a .skyprefab; instantiate rebuilds it as a scene root
 * (undoable). Paths may be plain or "assets://..." VFS references. */
SKY_BRIDGE_API int32_t sky_editor_save_prefab(SkyEditorContext* ctx,
                                              SkyObjectId object,
                                              const char* path);
SKY_BRIDGE_API SkyObjectId sky_editor_instantiate_prefab(SkyEditorContext* ctx,
                                                         const char* path);

/* Renames an object (undoable). */
SKY_BRIDGE_API void sky_editor_rename_object(SkyEditorContext* ctx,
                                             SkyObjectId object, const char* name);

/* Enable/disable an object (Unity's active checkbox): a disabled object is not
 * rendered and its lights do not contribute. */
SKY_BRIDGE_API void sky_editor_set_object_enabled(SkyEditorContext* ctx,
                                                  SkyObjectId object, int32_t enabled);
SKY_BRIDGE_API int32_t sky_editor_object_enabled(SkyEditorContext* ctx,
                                                 SkyObjectId object);

/* Registered component types, for the Inspector's Add Component list. */
SKY_BRIDGE_API int32_t sky_editor_available_type_count(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_available_type_id(SkyEditorContext* ctx,
                                                    int32_t index, char* buffer,
                                                    int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_available_type_name(SkyEditorContext* ctx,
                                                      int32_t index, char* buffer,
                                                      int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_available_type_category(SkyEditorContext* ctx,
                                                          int32_t index, char* buffer,
                                                          int32_t capacity);
/* The project's Assets folder on disk (user scripts live in
 * <assets>/Scripts). Returns the byte length written. */
SKY_BRIDGE_API int32_t sky_editor_assets_root(SkyEditorContext* ctx, char* buffer,
                                              int32_t capacity);

/* Compiles Assets/Scripts/*.cs into the project scripts assembly and
 * (re)loads it; the class list below refreshes. Returns 1 on success, 0 when
 * there are no scripts or the build failed (errors land in the Console).
 * Entering play also recompiles automatically when a source changed. */
SKY_BRIDGE_API int32_t sky_editor_reload_scripts(SkyEditorContext* ctx);

/* Managed script classes (ScriptComponent subclasses in the loaded
 * assemblies) — the choices for a sky.script component's "class" field. */
SKY_BRIDGE_API int32_t sky_editor_script_class_count(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_script_class_name(SkyEditorContext* ctx,
                                                    int32_t index, char* buffer,
                                                    int32_t capacity);

/* Serializable script fields of a sky.script component's class: public
 * float/int/bool/string fields declared by the managed script. Values read
 * back the authored value stored on the component, falling back to the
 * script's declared default; the setter stores on the component (undoable,
 * persisted in the scene, pushed to the instance at play start). */
SKY_BRIDGE_API int32_t sky_editor_script_field_count(SkyEditorContext* ctx,
                                                     SkyObjectId object,
                                                     int32_t component);
SKY_BRIDGE_API int32_t sky_editor_script_field_name(SkyEditorContext* ctx,
                                                    SkyObjectId object,
                                                    int32_t component, int32_t field,
                                                    char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_script_field_type(SkyEditorContext* ctx,
                                                    SkyObjectId object,
                                                    int32_t component, int32_t field,
                                                    char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_script_field_value(SkyEditorContext* ctx,
                                                     SkyObjectId object,
                                                     int32_t component, int32_t field,
                                                     char* buffer, int32_t capacity);
SKY_BRIDGE_API void sky_editor_set_script_field(SkyEditorContext* ctx,
                                                SkyObjectId object,
                                                int32_t component, int32_t field,
                                                const char* value);
/* Attach a component type / detach the component at an index (both undoable). */
SKY_BRIDGE_API void sky_editor_add_component(SkyEditorContext* ctx, SkyObjectId object,
                                             const char* type_id);
SKY_BRIDGE_API void sky_editor_remove_component(SkyEditorContext* ctx,
                                                SkyObjectId object, int32_t component);

/* Console log buffer. text combines "category: message"; level is a
 * sky::core::LogLevel (0=Trace..5=Critical). */
SKY_BRIDGE_API int32_t sky_editor_log_count(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_log_level(SkyEditorContext* ctx, int32_t index);
SKY_BRIDGE_API int32_t sky_editor_log_text(SkyEditorContext* ctx, int32_t index,
                                           char* buffer, int32_t capacity);
SKY_BRIDGE_API void sky_editor_log_clear(SkyEditorContext* ctx);

/* Packages panel. package_info which: 0=id, 1=displayName, 2=version. */
SKY_BRIDGE_API int32_t sky_editor_package_count(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_package_info(SkyEditorContext* ctx, int32_t index,
                                               int32_t which, char* buffer,
                                               int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_package_active(SkyEditorContext* ctx, int32_t index);
SKY_BRIDGE_API void sky_editor_package_set_active(SkyEditorContext* ctx, int32_t index,
                                                  int32_t active);
SKY_BRIDGE_API int32_t sky_editor_package_refresh(SkyEditorContext* ctx);
/* Installs a package from a source: a local package directory, a tarball
 * (.tar/.tar.gz/.tgz) or a git URL/path ("url#tag" pins a tag). The package
 * goes through the global version-addressed cache into the project's
 * Packages directory; the list refreshes. 1 on success. */
SKY_BRIDGE_API int32_t sky_editor_package_install(SkyEditorContext* ctx,
                                                  const char* source);

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
/* Advances one simulation frame (physics + scripts) without rendering — for
 * headless stepping in tests/tools. Rendering viewports tick on their own. */
SKY_BRIDGE_API void sky_editor_tick_play(SkyEditorContext* ctx, double dt);

/* Keyboard state for gameplay scripts (Input.GetKey). Key codes are the
 * engine's portable set: ASCII uppercase for letters/digits, Space = 32,
 * named keys from 256 (Escape, Enter, Tab, LShift, LCtrl, LAlt, arrows).
 * The Game view feeds this from UI key events; a lost focus should clear
 * held keys by sending up-events. */
SKY_BRIDGE_API void sky_editor_set_key_state(SkyEditorContext* ctx, int32_t key,
                                             int32_t down);

/* Mouse state for gameplay scripts (Input.MousePosition / GetMouseButton /
 * MouseWheelDelta). Position is in the pixels of the surface feeding it
 * (Game view control or player window). `button`: 0 = left, 1 = right,
 * 2 = middle — internally these are key codes 323..325, so edge queries
 * (GetMouseButtonDown/Up) ride the keyboard latch mechanism. The wheel
 * accumulates within a frame and resets each play frame. */
SKY_BRIDGE_API void sky_editor_set_mouse_position(SkyEditorContext* ctx, float x,
                                                  float y);
SKY_BRIDGE_API void sky_editor_set_mouse_button(SkyEditorContext* ctx,
                                                int32_t button, int32_t down);
SKY_BRIDGE_API void sky_editor_add_mouse_wheel(SkyEditorContext* ctx,
                                               float delta);

/* Data assets (ScriptableObject analog): typed field bags stored as
 * Assets/Data/<name>.skydata. `ref` is a VFS-style path
 * ("assets://Data/enemy.skydata"). Values travel as strings; `type` is one
 * of float/int/bool/string/Vec3 (Vec3 formatted "x, y, z"). Setting a field
 * persists immediately. Enumeration reflects the file itself; the engine
 * resolves parent-chain inheritance at load time for gameplay reads. */
SKY_BRIDGE_API int32_t sky_editor_data_asset_create(SkyEditorContext* ctx,
                                                    const char* name,
                                                    const char* typeId);
SKY_BRIDGE_API int32_t sky_editor_data_type_id(SkyEditorContext* ctx, const char* ref,
                                               char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_data_field_count(SkyEditorContext* ctx,
                                                   const char* ref);
SKY_BRIDGE_API int32_t sky_editor_data_field_name(SkyEditorContext* ctx,
                                                  const char* ref, int32_t index,
                                                  char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_data_field_type(SkyEditorContext* ctx,
                                                  const char* ref, int32_t index,
                                                  char* buffer, int32_t capacity);
SKY_BRIDGE_API int32_t sky_editor_data_field_value(SkyEditorContext* ctx,
                                                   const char* ref, int32_t index,
                                                   char* buffer, int32_t capacity);
SKY_BRIDGE_API void sky_editor_set_data_field(SkyEditorContext* ctx, const char* ref,
                                              const char* name, const char* type,
                                              const char* value);

#ifdef __cplusplus
}
#endif

#endif /* SKY_EDITOR_BRIDGE_H */
