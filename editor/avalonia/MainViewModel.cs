using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;
using SkyEditor.Engine;

namespace SkyEditor;

/// Active manipulation tool, shared between the toolbar (which sets it) and
/// the scene viewport (which draws the matching gizmo). Hand = camera only.
public enum GizmoTool { Hand, Move, Rotate, Scale }

public sealed class MainViewModel : INotifyPropertyChanged
{
    private readonly EditorSession _session;
    private SkyObject? _selected;
    private readonly float[] _pos = new float[3];
    private readonly float[] _rot = new float[3]; // Euler degrees
    private readonly float[] _scale = { 1, 1, 1 };
    private List<ComponentView> _components = new();

    private MaterialView? _selectedMaterial;

    public MainViewModel()
    {
        _session = new EditorSession();
        RefreshProject();
        RefreshMaterials();
        if (Roots.Count > 0)
            SelectedObject = FirstWithComponents(Roots) ?? Roots[0];
    }

    // --- Materials ---
    public ObservableCollection<MaterialView> Materials { get; } = new();

    public MaterialView? SelectedMaterial
    {
        get => _selectedMaterial;
        set
        {
            if (ReferenceEquals(_selectedMaterial, value)) return;
            _selectedMaterial = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(SelectedMaterialFields));
            OnPropertyChanged(nameof(SelectedMaterialName));
        }
    }

    public string SelectedMaterialName => _selectedMaterial?.Name ?? string.Empty;
    public IReadOnlyList<MaterialField> SelectedMaterialFields =>
        _selectedMaterial?.Fields ?? (IReadOnlyList<MaterialField>)System.Array.Empty<MaterialField>();

    public void RefreshMaterials()
    {
        Materials.Clear();
        foreach (var m in _session.ReadMaterials())
            Materials.Add(m);
        SelectedMaterial = Materials.Count > 0 ? Materials[0] : null;
    }

    // --- Terrain ---
    private string _terrainSeed = "1337";
    public string TerrainSeed
    {
        get => _terrainSeed;
        set { _terrainSeed = value; OnPropertyChanged(); }
    }

    public void GenerateTerrain()
    {
        ulong.TryParse(_terrainSeed, out var seed);
        _session.GenerateTerrain(seed);
        SelectedObject = Roots.Count > 0 ? Roots[0] : null;
    }

    public ObservableCollection<SkyObject> Roots => _session.Roots;
    public IntPtr NativeContext => _session.Native;
    public string SceneTitle => "SampleScene";

    public SkyObject? SelectedObject
    {
        get => _selected;
        set
        {
            if (ReferenceEquals(_selected, value))
                return;
            _selected = value;
            LoadSelection();
            OnPropertyChanged();
            OnPropertyChanged(nameof(HasSelection));
            OnPropertyChanged(nameof(SelectedName));
            OnPropertyChanged(nameof(SelectedComponents));
            foreach (var p in new[] { nameof(PositionX), nameof(PositionY), nameof(PositionZ),
                nameof(RotationX), nameof(RotationY), nameof(RotationZ),
                nameof(ScaleX), nameof(ScaleY), nameof(ScaleZ) })
                OnPropertyChanged(p);
        }
    }

    private void LoadSelection()
    {
        if (_selected == null)
            return;
        var pos = new float[3];
        var quat = new float[4];
        var scale = new float[3];
        EngineInterop.sky_editor_get_transform(NativeContext, _selected.Id, pos, quat, scale);
        Array.Copy(pos, _pos, 3);
        Array.Copy(scale, _scale, 3);
        var (rx, ry, rz) = QuatToEuler(quat[0], quat[1], quat[2], quat[3]);
        _rot[0] = rx; _rot[1] = ry; _rot[2] = rz;
        _components = _session.ReadComponents(_selected.Id);
    }

    public void SelectById(ulong id) => SelectedObject = id == 0 ? null : Find(Roots, id);

    // --- Authoring ---
    public void CreateCube() => SelectById(_session.CreateCube("Cube"));
    public void DuplicateSelected() { if (_selected != null) SelectById(_session.Duplicate(_selected.Id)); }
    public void DeleteSelected()
    {
        if (_selected == null) return;
        _session.Delete(_selected.Id);
        SelectedObject = Roots.Count > 0 ? Roots[0] : null;
    }

    public bool HasSelection => _selected != null;
    public string SelectedName => _selected?.Name ?? string.Empty;
    public IReadOnlyList<ComponentView> SelectedComponents => _components;

    // --- Transform (Position / Rotation / Scale) ---
    public string PositionX { get => Fmt(_pos[0]); set => SetPos(0, value); }
    public string PositionY { get => Fmt(_pos[1]); set => SetPos(1, value); }
    public string PositionZ { get => Fmt(_pos[2]); set => SetPos(2, value); }
    public string RotationX { get => Fmt(_rot[0]); set => SetRot(0, value); }
    public string RotationY { get => Fmt(_rot[1]); set => SetRot(1, value); }
    public string RotationZ { get => Fmt(_rot[2]); set => SetRot(2, value); }
    public string ScaleX { get => Fmt(_scale[0]); set => SetScale(0, value); }
    public string ScaleY { get => Fmt(_scale[1]); set => SetScale(1, value); }
    public string ScaleZ { get => Fmt(_scale[2]); set => SetScale(2, value); }

    private void SetPos(int axis, string text)
    {
        if (_selected == null || !TryParse(text, out var v)) return;
        _pos[axis] = v;
        _session.SetPosition(_selected.Id, _pos[0], _pos[1], _pos[2]);
        OnPropertyChanged(axis == 0 ? nameof(PositionX) : axis == 1 ? nameof(PositionY) : nameof(PositionZ));
    }
    private void SetRot(int axis, string text)
    {
        if (_selected == null || !TryParse(text, out var v)) return;
        _rot[axis] = v;
        _session.SetLocalEuler(_selected.Id, _rot[0], _rot[1], _rot[2]);
        OnPropertyChanged(axis == 0 ? nameof(RotationX) : axis == 1 ? nameof(RotationY) : nameof(RotationZ));
    }
    private void SetScale(int axis, string text)
    {
        if (_selected == null || !TryParse(text, out var v)) return;
        _scale[axis] = v;
        _session.SetScale(_selected.Id, _scale[0], _scale[1], _scale[2]);
        OnPropertyChanged(axis == 0 ? nameof(ScaleX) : axis == 1 ? nameof(ScaleY) : nameof(ScaleZ));
    }

    // --- Play transport (Unity-like toggles) ---
    // PlayModeState: 0 = Editing, 1 = Playing, 2 = Paused.
    private int PlayState => EngineInterop.sky_editor_play_state(NativeContext);

    /// Play toggles play mode: enter from Editing, exit (stop) while running.
    public void Play()
    {
        if (PlayState == 0)
            EngineInterop.sky_editor_play(NativeContext);
        else
            EngineInterop.sky_editor_stop(NativeContext);
    }

    /// Pause toggles between Playing and Paused; ignored while editing.
    public void Pause()
    {
        switch (PlayState)
        {
            case 1: EngineInterop.sky_editor_pause(NativeContext); break;
            case 2: EngineInterop.sky_editor_play(NativeContext); break; // resume
        }
    }

    public void Stop() => EngineInterop.sky_editor_stop(NativeContext);

    // --- 2D / 3D view (orthographic YZ plane vs perspective orbit) ---
    public void SetView2D(bool enabled) =>
        EngineInterop.sky_editor_set_view_2d(NativeContext, enabled ? 1 : 0);

    // --- Active gizmo tool (toolbar <-> viewport) ---
    private GizmoTool _tool = GizmoTool.Move;
    public GizmoTool Tool
    {
        get => _tool;
        set { if (_tool == value) return; _tool = value; OnPropertyChanged(); }
    }

    // --- Project browser (Unity-like: scoped to Assets and Packages) ---
    private readonly List<string> _projectPath = new();
    public ObservableCollection<ProjectEntry> ProjectEntries { get; } = new();

    public string ProjectPath =>
        _projectPath.Count == 0 ? "project://" : "project:// " + string.Join("  ›  ", _projectPath);

    private string CurrentVfsDir()
    {
        if (_projectPath.Count == 0)
            return "";
        var alias = _projectPath[0] == "Assets" ? "assets://" : "packages://";
        return _projectPath.Count > 1
            ? alias + string.Join("/", _projectPath.GetRange(1, _projectPath.Count - 1))
            : alias;
    }

    public void RefreshProject()
    {
        ProjectEntries.Clear();
        if (_projectPath.Count == 0)
        {
            // Roots — the only visible scope, like Unity.
            ProjectEntries.Add(new ProjectEntry("Assets", true, "IconFolder"));
            ProjectEntries.Add(new ProjectEntry("Packages", true, "IconFolder"));
        }
        else
        {
            ProjectEntries.Add(new ProjectEntry("..", true, "IconChevronRight"));
            foreach (var e in _session.ListProject(CurrentVfsDir()))
                ProjectEntries.Add(e);
        }
        OnPropertyChanged(nameof(ProjectPath));
    }

    public void OpenProjectEntry(ProjectEntry? entry)
    {
        if (entry is not { IsDirectory: true })
            return;
        if (entry.Name == "..")
        {
            if (_projectPath.Count > 0)
                _projectPath.RemoveAt(_projectPath.Count - 1);
        }
        else
        {
            _projectPath.Add(entry.Name);
        }
        RefreshProject();
    }

    private static SkyObject? Find(IEnumerable<SkyObject> objects, ulong id)
    {
        foreach (var o in objects)
        {
            if (o.Id == id) return o;
            var c = Find(o.Children, id);
            if (c != null) return c;
        }
        return null;
    }
    private static SkyObject? FirstWithComponents(IEnumerable<SkyObject> objects)
    {
        foreach (var o in objects)
        {
            if (o.Components.Count > 0) return o;
            var c = FirstWithComponents(o.Children);
            if (c != null) return c;
        }
        return null;
    }

    private static string Fmt(float v) => v.ToString("0.###", CultureInfo.InvariantCulture);
    private static bool TryParse(string s, out float v) =>
        float.TryParse(s, NumberStyles.Float, CultureInfo.InvariantCulture, out v);

    private static (float, float, float) QuatToEuler(float x, float y, float z, float w)
    {
        double pitch = Math.Asin(Math.Clamp(2 * (w * x - y * z), -1, 1));
        double yaw = Math.Atan2(2 * (w * y + x * z), 1 - 2 * (x * x + y * y));
        double roll = Math.Atan2(2 * (w * z + x * y), 1 - 2 * (x * x + z * z));
        const double r = 180.0 / Math.PI;
        return ((float)(pitch * r), (float)(yaw * r), (float)(roll * r));
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
