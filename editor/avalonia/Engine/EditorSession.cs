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

    public static string CategoryOf(string typeId) =>
        Categories.TryGetValue(typeId, out var cat) ? cat : "Other";
}

/// A component shown in the Inspector with its inspectable fields.
public sealed class ComponentView
{
    public ComponentView(int index, string typeId, string displayName)
    {
        Index = index;
        TypeId = typeId;
        DisplayName = displayName;
        Category = SkyComponent.CategoryOf(typeId);
    }

    public int Index { get; }
    public string TypeId { get; }
    public string DisplayName { get; }
    public string Category { get; }
    public System.Collections.Generic.List<ComponentField> Fields { get; } = new();
    public bool HasFields => Fields.Count > 0;
    public string Glyph => TypeId switch
    {
        "sky.camera" => "IconCamera",
        "sky.light" => "IconLight",
        "sky.mesh" => "IconMesh",
        "sky.script" => "IconFile",
        _ => "IconInspector",
    };
}

/// One editable inspectable field; the setter writes straight to the engine.
public sealed class ComponentField : System.ComponentModel.INotifyPropertyChanged
{
    private readonly EditorSession _session;
    private readonly ulong _object;
    private readonly int _component;
    private readonly int _field;
    private string _value;

    public ComponentField(EditorSession session, ulong obj, int component, int field,
        string name, string type, string value)
    {
        _session = session;
        _object = obj;
        _component = component;
        _field = field;
        Name = name;
        Type = type;
        _value = value;
    }

    public string Name { get; }
    public string Type { get; }

    // Field-type presentation, so the inspector renders like Unity: a plain box
    // for scalars/strings, X/Y/Z boxes for Vec3, a checkbox for bool, and a
    // mesh-reference picker for the Mesh Renderer's "mesh" field.
    public bool IsVec3 => Type == "Vec3";
    public bool IsBool => Type == "bool";
    public bool IsMeshRef => Name == "mesh";
    public bool IsScalar => !IsVec3 && !IsBool && !IsMeshRef;

    // --- Mesh reference (Unity-style asset picker) ---
    public System.Collections.Generic.List<MeshOption> MeshOptions =>
        _session.AvailableMeshes(_value);

    public MeshOption? SelectedMesh
    {
        get
        {
            foreach (var o in MeshOptions)
                if (o.Value == _value) return o;
            return null;
        }
        set
        {
            if (value != null) Value = value.Value;
            Raise(nameof(SelectedMesh));
        }
    }

    public string Value
    {
        get => _value;
        set
        {
            if (_value == value)
                return;
            _value = value;
            _session.SetComponentField(_object, _component, _field, value);
            Raise(nameof(Value));
        }
    }

    // --- Vec3 (stored as "x, y, z") ---
    public string X { get => Vec(0); set => SetVec(0, value); }
    public string Y { get => Vec(1); set => SetVec(1, value); }
    public string Z { get => Vec(2); set => SetVec(2, value); }

    private string Vec(int axis)
    {
        var parts = _value.Split(',');
        return axis < parts.Length ? parts[axis].Trim() : "0";
    }

    private void SetVec(int axis, string component)
    {
        var v = new[] { Vec(0), Vec(1), Vec(2) };
        v[axis] = component.Trim();
        Value = $"{v[0]}, {v[1]}, {v[2]}";
        Raise(axis == 0 ? nameof(X) : axis == 1 ? nameof(Y) : nameof(Z));
    }

    // --- bool (stored as "true"/"false") ---
    public bool BoolValue
    {
        get => _value == "true" || _value == "1";
        set { Value = value ? "true" : "false"; Raise(nameof(BoolValue)); }
    }

    private void Raise(string name) => PropertyChanged?.Invoke(this,
        new System.ComponentModel.PropertyChangedEventArgs(name));

    public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;
}

/// One entry in the mesh-reference picker: a friendly display name plus the
/// underlying reference stored in the component ("cube", "assets://...").
public sealed class MeshOption
{
    public MeshOption(string display, string value) { Display = display; Value = value; }
    public string Display { get; }
    public string Value { get; }
    public override string ToString() => Display;
}

/// One Console log line. Level: 0 Trace,1 Debug,2 Info,3 Warning,4 Error,5 Critical.
public sealed class LogLine
{
    public LogLine(int level, string text) { Level = level; Text = text; }
    public int Level { get; }
    public string Text { get; }
    public string Color => Level >= 4 ? "#F0626E" : Level == 3 ? "#E0B44A" : "#9AA1AC";
}

/// A discovered package shown in the Packages panel.
public sealed class PackageInfo
{
    public PackageInfo(int index, string id, string name, string version, bool active)
    {
        Index = index;
        Id = id;
        Name = string.IsNullOrEmpty(name) ? id : name;
        Version = version;
        Active = active;
    }
    public int Index { get; }
    public string Id { get; }
    public string Name { get; }
    public string Version { get; }
    public bool Active { get; }
    public string StatusText => Active ? "Active" : "Inactive";
    public string ToggleLabel => Active ? "Deactivate" : "Activate";
}

/// A registered component type offered in the Inspector's Add Component list.
public sealed class ComponentType
{
    public ComponentType(string typeId, string displayName, string category)
    {
        TypeId = typeId;
        DisplayName = displayName;
        Category = string.IsNullOrEmpty(category) ? "Other" : category;
    }
    public string TypeId { get; }
    public string DisplayName { get; }
    public string Category { get; }
    public override string ToString() => DisplayName;
}

/// A material in the Materials panel, with a colour swatch and PBR fields.
public sealed class MaterialView
{
    public MaterialView(int index, string name)
    {
        Index = index;
        Name = name;
    }

    public int Index { get; }
    public string Name { get; }
    public System.Collections.Generic.List<MaterialField> Fields { get; } = new();
    public Avalonia.Media.IBrush Swatch { get; private set; } = Avalonia.Media.Brushes.Gray;

    public void RefreshSwatch()
    {
        foreach (var f in Fields)
        {
            if (f.Name != "baseColor")
                continue;
            var parts = f.Value.Split(',');
            if (parts.Length == 3 &&
                float.TryParse(parts[0], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out var r) &&
                float.TryParse(parts[1], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out var g) &&
                float.TryParse(parts[2], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out var b))
            {
                Swatch = new Avalonia.Media.SolidColorBrush(Avalonia.Media.Color.FromRgb(
                    (byte)(System.Math.Clamp(r, 0, 1) * 255),
                    (byte)(System.Math.Clamp(g, 0, 1) * 255),
                    (byte)(System.Math.Clamp(b, 0, 1) * 255)));
            }
        }
    }
}

/// One editable material field; the setter writes straight to the engine.
public sealed class MaterialField : System.ComponentModel.INotifyPropertyChanged
{
    private readonly EditorSession _session;
    private readonly int _material;
    private readonly int _field;
    private string _value;

    public MaterialField(EditorSession session, int material, int field, string name, string value)
    {
        _session = session;
        _material = material;
        _field = field;
        Name = name;
        _value = value;
    }

    public string Name { get; }
    public string Value
    {
        get => _value;
        set
        {
            if (_value == value)
                return;
            _value = value;
            _session.SetMaterialField(_material, _field, value);
            PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(nameof(Value)));
        }
    }

    public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;
}

/// One Project-panel entry (folder or file) from the VFS listing.
public sealed class ProjectEntry
{
    public ProjectEntry(string raw)
    {
        IsDirectory = raw.EndsWith('/');
        Name = raw.TrimEnd('/');
        var ext = System.IO.Path.GetExtension(Name).ToLowerInvariant();
        Glyph = IsDirectory ? "IconFolder"
            : ext is ".png" or ".jpg" or ".jpeg" or ".tga" or ".bmp" or ".raw" ? "IconImage"
            : "IconFile";
    }

    /// Synthetic entry (the Assets/Packages roots and the ".." up entry).
    public ProjectEntry(string name, bool isDirectory, string glyph)
    {
        Name = name;
        IsDirectory = isDirectory;
        Glyph = glyph;
    }

    public string Name { get; }
    public bool IsDirectory { get; }
    public string Glyph { get; }
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

    public ulong CreateModel(string name, string meshRef)
    {
        var id = EngineInterop.sky_editor_create_mesh_object(_ctx, name, meshRef);
        Reload();
        return id;
    }

    public void RenameObject(ulong id, string name)
    {
        EngineInterop.sky_editor_rename_object(_ctx, id, name);
        Reload();
    }

    public List<ComponentType> AvailableTypes()
    {
        var list = new List<ComponentType>();
        var count = EngineInterop.sky_editor_available_type_count(_ctx);
        for (var i = 0; i < count; ++i)
        {
            var idx = i;
            var id = EngineInterop.ReadString((b, n) => EngineInterop.sky_editor_available_type_id(_ctx, idx, b, n));
            var name = EngineInterop.ReadString((b, n) => EngineInterop.sky_editor_available_type_name(_ctx, idx, b, n));
            var cat = EngineInterop.ReadString((b, n) => EngineInterop.sky_editor_available_type_category(_ctx, idx, b, n));
            if (!string.IsNullOrEmpty(id))
                list.Add(new ComponentType(id, name, cat));
        }
        return list;
    }

    public void AddComponent(ulong id, string typeId) =>
        EngineInterop.sky_editor_add_component(_ctx, id, typeId);

    public void RemoveComponent(ulong id, int component) =>
        EngineInterop.sky_editor_remove_component(_ctx, id, component);

    // --- Console log ---
    public int LogCount => EngineInterop.sky_editor_log_count(_ctx);
    public void ClearLogs() => EngineInterop.sky_editor_log_clear(_ctx);
    public List<LogLine> ReadLogs()
    {
        var list = new List<LogLine>();
        var count = EngineInterop.sky_editor_log_count(_ctx);
        for (var i = 0; i < count; ++i)
        {
            var idx = i;
            var level = EngineInterop.sky_editor_log_level(_ctx, idx);
            var text = EngineInterop.ReadString((b, n) => EngineInterop.sky_editor_log_text(_ctx, idx, b, n));
            list.Add(new LogLine(level, text));
        }
        return list;
    }

    // --- Packages ---
    public List<PackageInfo> ReadPackages()
    {
        var list = new List<PackageInfo>();
        var count = EngineInterop.sky_editor_package_count(_ctx);
        for (var i = 0; i < count; ++i)
        {
            var idx = i;
            var id = EngineInterop.ReadString((b, n) => EngineInterop.sky_editor_package_info(_ctx, idx, 0, b, n));
            var name = EngineInterop.ReadString((b, n) => EngineInterop.sky_editor_package_info(_ctx, idx, 1, b, n));
            var ver = EngineInterop.ReadString((b, n) => EngineInterop.sky_editor_package_info(_ctx, idx, 2, b, n));
            var active = EngineInterop.sky_editor_package_active(_ctx, idx) == 1;
            list.Add(new PackageInfo(idx, id, name, ver, active));
        }
        return list;
    }
    public void SetPackageActive(int index, bool active) =>
        EngineInterop.sky_editor_package_set_active(_ctx, index, active ? 1 : 0);
    public void RefreshPackages() => EngineInterop.sky_editor_package_refresh(_ctx);

    public void NewScene()
    {
        EngineInterop.sky_editor_new_scene(_ctx);
        Reload();
    }

    public bool SaveScene(string path) => EngineInterop.sky_editor_save_scene(_ctx, path) == 1;

    public bool OpenScene(string path)
    {
        var ok = EngineInterop.sky_editor_open_scene(_ctx, path) == 1;
        Reload();
        return ok;
    }

    // --- Undo / Redo ---
    public void CommitEdit() => EngineInterop.sky_editor_commit_edit(_ctx);
    public bool CanUndo => EngineInterop.sky_editor_can_undo(_ctx) == 1;
    public bool CanRedo => EngineInterop.sky_editor_can_redo(_ctx) == 1;

    public bool Undo()
    {
        var changed = EngineInterop.sky_editor_undo(_ctx) == 1;
        if (changed) Reload();
        return changed;
    }

    public bool Redo()
    {
        var changed = EngineInterop.sky_editor_redo(_ctx) == 1;
        if (changed) Reload();
        return changed;
    }

    public ulong Duplicate(ulong id)
    {
        var copy = EngineInterop.sky_editor_duplicate(_ctx, id);
        Reload();
        return copy;
    }

    public void Delete(ulong id)
    {
        EngineInterop.sky_editor_delete(_ctx, id);
        Reload();
    }

    public void SetPosition(ulong id, float x, float y, float z) =>
        EngineInterop.sky_editor_set_position(_ctx, id, x, y, z);

    public void SetLocalEuler(ulong id, float x, float y, float z) =>
        EngineInterop.sky_editor_set_local_euler(_ctx, id, x, y, z);

    public void SetScale(ulong id, float x, float y, float z) =>
        EngineInterop.sky_editor_set_scale(_ctx, id, x, y, z);

    /// Reads an object's components and their fields through the ABI.
    public List<ComponentView> ReadComponents(ulong id)
    {
        var result = new List<ComponentView>();
        var count = EngineInterop.sky_editor_component_count(_ctx, id);
        for (var c = 0; c < count; ++c)
        {
            var index = c;
            var typeId = EngineInterop.ReadString((b, n) =>
                EngineInterop.sky_editor_component_type(_ctx, id, index, b, n));
            var display = EngineInterop.ReadString((b, n) =>
                EngineInterop.sky_editor_component_display_name(_ctx, id, index, b, n));
            var view = new ComponentView(index, typeId,
                string.IsNullOrEmpty(display) ? typeId : display);

            var fieldCount = EngineInterop.sky_editor_component_field_count(_ctx, id, c);
            for (var f = 0; f < fieldCount; ++f)
            {
                var fi = f;
                var fname = EngineInterop.ReadString((b, n) =>
                    EngineInterop.sky_editor_component_field_name(_ctx, id, index, fi, b, n));
                var ftype = EngineInterop.ReadString((b, n) =>
                    EngineInterop.sky_editor_component_field_type(_ctx, id, index, fi, b, n));
                var fvalue = EngineInterop.ReadString((b, n) =>
                    EngineInterop.sky_editor_component_field_value(_ctx, id, index, fi, b, n));
                view.Fields.Add(new ComponentField(this, id, index, fi, fname, ftype, fvalue));
            }
            result.Add(view);
        }
        return result;
    }

    public void SetComponentField(ulong id, int component, int field, string value) =>
        EngineInterop.sky_editor_set_component_field(_ctx, id, component, field, value);

    /// Reads the material library and each material's PBR fields.
    public List<MaterialView> ReadMaterials()
    {
        var result = new List<MaterialView>();
        var count = EngineInterop.sky_editor_material_count(_ctx);
        var fieldCount = EngineInterop.sky_editor_material_field_count(_ctx);
        for (var i = 0; i < count; ++i)
        {
            var index = i;
            var name = EngineInterop.ReadString((b, n) =>
                EngineInterop.sky_editor_material_name(_ctx, index, b, n));
            var view = new MaterialView(index, name);
            for (var f = 0; f < fieldCount; ++f)
            {
                var fi = f;
                var fname = EngineInterop.ReadString((b, n) =>
                    EngineInterop.sky_editor_material_field_name(_ctx, fi, b, n));
                var fvalue = EngineInterop.ReadString((b, n) =>
                    EngineInterop.sky_editor_material_field_value(_ctx, index, fi, b, n));
                view.Fields.Add(new MaterialField(this, index, fi, fname, fvalue));
            }
            view.RefreshSwatch();
            result.Add(view);
        }
        return result;
    }

    public void SetMaterialField(int material, int field, string value) =>
        EngineInterop.sky_editor_set_material_field(_ctx, material, field, value);

    /// Regenerates the terrain from a seed and reloads the hierarchy (the
    /// scattered objects change).
    public int GenerateTerrain(ulong seed)
    {
        var count = EngineInterop.sky_editor_terrain_generate(_ctx, seed);
        Reload();
        return count;
    }

    /// Lists a VFS directory; entries ending in '/' are folders.
    /// Meshes selectable in the Mesh Renderer's reference picker: built-in
    /// primitives plus imported models under Assets/Models. `current` is kept
    /// in the list even if it lives elsewhere, so the picker always shows it.
    public List<MeshOption> AvailableMeshes(string current)
    {
        var list = new List<MeshOption>
        {
            new MeshOption("None", ""),
            new MeshOption("Cube", "cube"),
            new MeshOption("Plane", "plane"),
            new MeshOption("Sphere", "sphere"),
        };
        string[] modelExt = { ".obj", ".fbx", ".gltf", ".glb" };
        foreach (var e in ListProject("assets://Models"))
        {
            if (e.IsDirectory) continue;
            var lower = e.Name.ToLowerInvariant();
            if (System.Array.Exists(modelExt, x => lower.EndsWith(x)))
            {
                var value = "assets://Models/" + e.Name;
                list.Add(new MeshOption(MeshDisplayName(value), value));
            }
        }
        if (!string.IsNullOrEmpty(current) && !list.Exists(o => o.Value == current))
            list.Add(new MeshOption(MeshDisplayName(current), current));
        return list;
    }

    public static string MeshDisplayName(string value)
    {
        if (string.IsNullOrEmpty(value)) return "None";
        var name = value;
        var slash = name.LastIndexOf('/');
        if (slash >= 0) name = name.Substring(slash + 1);
        var dot = name.LastIndexOf('.');
        if (dot > 0) name = name.Substring(0, dot);
        return name.Length == 0 ? value : char.ToUpperInvariant(name[0]) + name.Substring(1);
    }

    public List<ProjectEntry> ListProject(string dir)
    {
        var entries = new List<ProjectEntry>();
        var count = EngineInterop.sky_editor_vfs_count(_ctx, dir);
        for (var i = 0; i < count; ++i)
        {
            var index = i;
            var raw = EngineInterop.ReadString((b, n) =>
                EngineInterop.sky_editor_vfs_entry(_ctx, dir, index, b, n));
            if (!string.IsNullOrEmpty(raw))
                entries.Add(new ProjectEntry(raw));
        }
        return entries;
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
