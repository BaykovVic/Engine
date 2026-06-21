using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace SkyEditor.Engine;

/// One inspectable component on an object (currently just its type identity).
public sealed class SkyComponent
{
    public SkyComponent(string typeId)
    {
        TypeId = typeId;
        DisplayName = FriendlyNames.TryGetValue(typeId, out var name) ? name : typeId;
        Category = Categories.TryGetValue(typeId, out var cat) ? cat : "Other";
    }

    public string TypeId { get; }
    public string DisplayName { get; }
    public string Category { get; }

    private static readonly Dictionary<string, string> FriendlyNames = new()
    {
        ["sky.mesh"] = "Mesh Renderer",
        ["sky.camera"] = "Camera",
        ["sky.light"] = "Light",
        ["sky.collider.box"] = "Box Collider",
        ["sky.rigidbody"] = "Rigidbody",
        ["sky.script"] = "Script",
    };

    private static readonly Dictionary<string, string> Categories = new()
    {
        ["sky.mesh"] = "Rendering",
        ["sky.camera"] = "Rendering",
        ["sky.light"] = "Rendering",
        ["sky.collider.box"] = "Physics",
        ["sky.rigidbody"] = "Physics",
        ["sky.script"] = "Scripting",
    };
}

/// A scene object mirrored from the native engine for display and editing.
public sealed class SkyObject
{
    public SkyObject(ulong id, string name)
    {
        Id = id;
        Name = name;
    }

    public ulong Id { get; }
    public string Name { get; }
    public ObservableCollection<SkyObject> Children { get; } = new();
    public List<SkyComponent> Components { get; } = new();

    /// A single representative glyph key for the hierarchy row, inferred from
    /// the most characteristic component (mirrors the design's type glyphs).
    public string Glyph
    {
        get
        {
            foreach (var component in Components)
            {
                switch (component.TypeId)
                {
                    case "sky.camera": return "IconCamera";
                    case "sky.light": return "IconLight";
                    case "sky.mesh": return "IconMesh";
                }
            }
            return "IconEmpty";
        }
    }
}

/// Managed session over the native editor context: assembles the engine,
/// mirrors its scene graph, and forwards authoring/edits through the C ABI.
public sealed class EditorSession : IDisposable
{
    private IntPtr _ctx;

    public EditorSession()
    {
        _ctx = EngineInterop.sky_editor_create();
        if (_ctx == IntPtr.Zero)
            throw new InvalidOperationException("sky_editor_create returned null");
        Reload();
    }

    /// The native session handle, shared with the viewport control so the
    /// embedded renderer draws the very scene the panels edit.
    public IntPtr Native => _ctx;

    public ObservableCollection<SkyObject> Roots { get; } = new();

    public void Reload()
    {
        Roots.Clear();
        var count = EngineInterop.sky_editor_root_count(_ctx);
        for (var i = 0; i < count; ++i)
            Roots.Add(Load(EngineInterop.sky_editor_root_at(_ctx, i)));
    }

    private SkyObject Load(ulong id)
    {
        var name = EngineInterop.ReadString((buffer, capacity) =>
            EngineInterop.sky_editor_object_name(_ctx, id, buffer, capacity));
        var node = new SkyObject(id, name);

        var components = EngineInterop.sky_editor_component_count(_ctx, id);
        for (var i = 0; i < components; ++i)
        {
            var index = i;
            var typeId = EngineInterop.ReadString((buffer, capacity) =>
                EngineInterop.sky_editor_component_type(_ctx, id, index, buffer, capacity));
            node.Components.Add(new SkyComponent(typeId));
        }

        var children = EngineInterop.sky_editor_child_count(_ctx, id);
        for (var i = 0; i < children; ++i)
            node.Children.Add(Load(EngineInterop.sky_editor_child_at(_ctx, id, i)));

        return node;
    }

    public (float[] position, float[] rotation, float[] scale) Transform(ulong id)
    {
        var position = new float[3];
        var rotation = new float[4];
        var scale = new float[3];
        EngineInterop.sky_editor_get_transform(_ctx, id, position, rotation, scale);
        return (position, rotation, scale);
    }

    public ulong CreateCube(string name)
    {
        var id = EngineInterop.sky_editor_create_primitive(_ctx, 0, name);
        Reload();
        return id;
    }

    public void Dispose()
    {
        if (_ctx != IntPtr.Zero)
        {
            EngineInterop.sky_editor_destroy(_ctx);
            _ctx = IntPtr.Zero;
        }
    }
}
