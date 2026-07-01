using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace SkyEditor.Engine;

/// P/Invoke surface over libsky_editor_bridge.so — the native C ABI that
/// drives the C++ engine. A custom resolver finds the .so in the CMake build
/// tree (or SKY_BRIDGE_PATH), so the managed editor runs straight from source.
internal static class EngineInterop
{
    private const string Lib = "sky_editor_bridge";

    static EngineInterop()
    {
        NativeLibrary.SetDllImportResolver(typeof(EngineInterop).Assembly, Resolve);
    }

    private static IntPtr Resolve(string name, Assembly assembly, DllImportSearchPath? path)
    {
        if (name != Lib)
            return IntPtr.Zero;
        foreach (var candidate in Candidates())
        {
            if (File.Exists(candidate) && NativeLibrary.TryLoad(candidate, out var handle))
                return handle;
        }
        return IntPtr.Zero;
    }

    private static string[] Candidates()
    {
        var file = OperatingSystem.IsMacOS() ? "libsky_editor_bridge.dylib"
            : OperatingSystem.IsWindows() ? "sky_editor_bridge.dll"
            : "libsky_editor_bridge.so";
        var env = Environment.GetEnvironmentVariable("SKY_BRIDGE_PATH");
        var dir = Path.GetDirectoryName(typeof(EngineInterop).Assembly.Location) ?? ".";
        return new[]
        {
            env ?? "",
            Path.Combine(dir, file),
            // From editor/avalonia/bin/<cfg>/net8.0 up to the repo build tree.
            Path.GetFullPath(Path.Combine(dir, "..", "..", "..", "..", "..",
                "build", "editor", "native_bridge", file)),
        };
    }

    // --- Lifecycle ---
    [DllImport(Lib)] public static extern IntPtr sky_editor_create();
    [DllImport(Lib)] public static extern void sky_editor_destroy(IntPtr ctx);

    // --- Hierarchy ---
    [DllImport(Lib)] public static extern int sky_editor_root_count(IntPtr ctx);
    [DllImport(Lib)] public static extern ulong sky_editor_root_at(IntPtr ctx, int index);
    [DllImport(Lib)] public static extern int sky_editor_child_count(IntPtr ctx, ulong obj);
    [DllImport(Lib)] public static extern ulong sky_editor_child_at(IntPtr ctx, ulong obj, int index);
    [DllImport(Lib)] public static extern int sky_editor_object_name(IntPtr ctx, ulong obj, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_object_exists(IntPtr ctx, ulong obj);

    // --- Transform ---
    [DllImport(Lib)] public static extern void sky_editor_get_transform(IntPtr ctx, ulong obj, float[]? position, float[]? rotation, float[]? scale);
    [DllImport(Lib)] public static extern void sky_editor_set_position(IntPtr ctx, ulong obj, float x, float y, float z);
    [DllImport(Lib)] public static extern void sky_editor_set_scale(IntPtr ctx, ulong obj, float x, float y, float z);

    // --- Components ---
    [DllImport(Lib)] public static extern int sky_editor_component_count(IntPtr ctx, ulong obj);
    [DllImport(Lib)] public static extern int sky_editor_component_type(IntPtr ctx, ulong obj, int index, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_component_display_name(IntPtr ctx, ulong obj, int component, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_component_field_count(IntPtr ctx, ulong obj, int component);
    [DllImport(Lib)] public static extern int sky_editor_component_field_name(IntPtr ctx, ulong obj, int component, int field, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_component_field_type(IntPtr ctx, ulong obj, int component, int field, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_component_field_value(IntPtr ctx, ulong obj, int component, int field, byte[] buffer, int capacity);
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern void sky_editor_set_component_field(IntPtr ctx, ulong obj, int component, int field, string value);

    // --- Terrain ---
    [DllImport(Lib)] public static extern int sky_editor_terrain_generate(IntPtr ctx, ulong seed);

    // --- Materials ---
    [DllImport(Lib)] public static extern int sky_editor_material_count(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_material_name(IntPtr ctx, int index, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_material_field_count(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_material_field_name(IntPtr ctx, int field, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_material_field_value(IntPtr ctx, int index, int field, byte[] buffer, int capacity);
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern void sky_editor_set_material_field(IntPtr ctx, int index, int field, string value);

    // --- Project / VFS ---
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern int sky_editor_vfs_count(IntPtr ctx, string dir);
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern int sky_editor_vfs_entry(IntPtr ctx, string dir, int index, byte[] buffer, int capacity);

    // --- Authoring ---
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern ulong sky_editor_create_primitive(IntPtr ctx, int kind, string name);
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern ulong sky_editor_create_mesh_object(IntPtr ctx, string name, string meshRef);
    [DllImport(Lib)] public static extern int sky_editor_log_count(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_log_level(IntPtr ctx, int index);
    [DllImport(Lib)] public static extern int sky_editor_log_text(IntPtr ctx, int index, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern void sky_editor_log_clear(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_package_count(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_package_info(IntPtr ctx, int index, int which, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_package_active(IntPtr ctx, int index);
    [DllImport(Lib)] public static extern void sky_editor_package_set_active(IntPtr ctx, int index, int active);
    [DllImport(Lib)] public static extern int sky_editor_package_refresh(IntPtr ctx);
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern void sky_editor_rename_object(IntPtr ctx, ulong obj, string name);
    [DllImport(Lib)] public static extern int sky_editor_available_type_count(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_available_type_id(IntPtr ctx, int index, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_available_type_name(IntPtr ctx, int index, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_available_type_category(IntPtr ctx, int index, byte[] buffer, int capacity);
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern void sky_editor_add_component(IntPtr ctx, ulong obj, string typeId);
    [DllImport(Lib)] public static extern void sky_editor_remove_component(IntPtr ctx, ulong obj, int component);
    [DllImport(Lib)] public static extern void sky_editor_new_scene(IntPtr ctx);
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern int sky_editor_save_scene(IntPtr ctx, string path);
    [DllImport(Lib, CharSet = CharSet.Ansi)] public static extern int sky_editor_open_scene(IntPtr ctx, string path);
    [DllImport(Lib)] public static extern void sky_editor_commit_edit(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_undo(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_redo(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_can_undo(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_can_redo(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_undo_label(IntPtr ctx, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern int sky_editor_redo_label(IntPtr ctx, byte[] buffer, int capacity);
    [DllImport(Lib)] public static extern ulong sky_editor_duplicate(IntPtr ctx, ulong obj);
    [DllImport(Lib)] public static extern void sky_editor_delete(IntPtr ctx, ulong obj);

    // --- Viewport (Vulkan swapchain bound to a native window; player path) ---
    [DllImport(Lib)] public static extern int sky_editor_attach_viewport(IntPtr ctx, IntPtr x11Display, ulong x11Window, uint width, uint height);
    [DllImport(Lib)] public static extern void sky_editor_render_viewport(IntPtr ctx, uint width, uint height);
    [DllImport(Lib)] public static extern void sky_editor_detach_viewport(IntPtr ctx);

    // --- Offscreen viewport + orbit camera + pick (the editor path) ---
    [DllImport(Lib)] public static extern int sky_editor_render_offscreen(IntPtr ctx, uint width, uint height, byte[] outRgba, int outLength);
    [DllImport(Lib)] public static extern int sky_editor_render_game_offscreen(IntPtr ctx, uint width, uint height, byte[] outRgba, int outLength);
    [DllImport(Lib)] public static extern void sky_editor_viewport_orbit(IntPtr ctx, float deltaYawDegrees, float deltaPitchDegrees);
    [DllImport(Lib)] public static extern void sky_editor_viewport_pan(IntPtr ctx, float deltaRight, float deltaUp);
    [DllImport(Lib)] public static extern void sky_editor_viewport_zoom(IntPtr ctx, float factor);
    [DllImport(Lib)] public static extern ulong sky_editor_pick(IntPtr ctx, float pixelX, float pixelY, uint width, uint height);
    [DllImport(Lib)] public static extern void sky_editor_frame_object(IntPtr ctx, ulong obj);
    [DllImport(Lib)] public static extern int sky_editor_project(IntPtr ctx, float worldX, float worldY, float worldZ, uint width, uint height, out float outX, out float outY);
    [DllImport(Lib)] public static extern void sky_editor_world_position(IntPtr ctx, ulong obj, float[] outXyz);
    [DllImport(Lib)] public static extern void sky_editor_camera_position(IntPtr ctx, float[] outXyz);
    [DllImport(Lib)] public static extern void sky_editor_set_view_2d(IntPtr ctx, int enabled);
    [DllImport(Lib)] public static extern int sky_editor_view_2d(IntPtr ctx);
    [DllImport(Lib)] public static extern void sky_editor_look_along_axis(IntPtr ctx, int axis);
    [DllImport(Lib)] public static extern void sky_editor_camera_basis(IntPtr ctx, float[] outRight, float[] outUp, float[] outForward);

    // --- Transforms across spaces (world / self / parent) ---
    [DllImport(Lib)] public static extern void sky_editor_get_world_transform(IntPtr ctx, ulong obj, float[] outPosition, float[] outRotation, float[] outScale);
    [DllImport(Lib)] public static extern void sky_editor_set_world_position(IntPtr ctx, ulong obj, float x, float y, float z);
    [DllImport(Lib)] public static extern void sky_editor_translate_self(IntPtr ctx, ulong obj, float dx, float dy, float dz);
    [DllImport(Lib)] public static extern void sky_editor_set_local_euler(IntPtr ctx, ulong obj, float xDegrees, float yDegrees, float zDegrees);
    [DllImport(Lib)] public static extern void sky_editor_rotate_world_axis(IntPtr ctx, ulong obj, float axisX, float axisY, float axisZ, float radians);

    // --- Play mode ---
    [DllImport(Lib)] public static extern int sky_editor_play(IntPtr ctx);
    [DllImport(Lib)] public static extern void sky_editor_pause(IntPtr ctx);
    [DllImport(Lib)] public static extern void sky_editor_stop(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_play_state(IntPtr ctx);

    /// Reads a name/type string through the caller-owned-buffer ABI idiom.
    public static string ReadString(Func<byte[], int, int> call)
    {
        var buffer = new byte[256];
        var length = call(buffer, buffer.Length);
        var copied = Math.Min(length, buffer.Length - 1);
        return copied <= 0 ? string.Empty : Encoding.UTF8.GetString(buffer, 0, copied);
    }
}
