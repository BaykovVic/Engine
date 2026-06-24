using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;
using SkyEditor.Engine;

namespace SkyEditor;

public sealed class MainViewModel : INotifyPropertyChanged
{
    private readonly EditorSession _session;
    private SkyObject? _selected;
    private float _px, _py, _pz;

    public MainViewModel()
    {
        _session = new EditorSession();
        if (Roots.Count > 0)
            SelectedObject = FirstWithComponents(Roots) ?? Roots[0];
    }

    public ObservableCollection<SkyObject> Roots => _session.Roots;

    /// Native session handle for the embedded Vulkan viewport.
    public System.IntPtr NativeContext => _session.Native;

    public SkyObject? SelectedObject
    {
        get => _selected;
        set
        {
            if (ReferenceEquals(_selected, value))
                return;
            _selected = value;
            if (_selected != null)
            {
                var (position, _, _) = _session.Transform(_selected.Id);
                _px = position[0];
                _py = position[1];
                _pz = position[2];
            }
            OnPropertyChanged();
            OnPropertyChanged(nameof(HasSelection));
            OnPropertyChanged(nameof(SelectedName));
            OnPropertyChanged(nameof(SelectedComponents));
            OnPropertyChanged(nameof(PositionX));
            OnPropertyChanged(nameof(PositionY));
            OnPropertyChanged(nameof(PositionZ));
        }
    }

    /// Selects the object with the given native id (e.g. from a viewport pick).
    public void SelectById(ulong id)
    {
        SelectedObject = id == 0 ? null : Find(Roots, id);
    }

    // --- Authoring commands (create / duplicate / delete) ---

    public void CreateCube()
    {
        SelectById(_session.CreateCube("Cube"));
    }

    public void DuplicateSelected()
    {
        if (_selected != null)
            SelectById(_session.Duplicate(_selected.Id));
    }

    public void DeleteSelected()
    {
        if (_selected == null)
            return;
        _session.Delete(_selected.Id);
        SelectedObject = Roots.Count > 0 ? Roots[0] : null;
    }

    private static SkyObject? Find(System.Collections.Generic.IEnumerable<SkyObject> objects, ulong id)
    {
        foreach (var o in objects)
        {
            if (o.Id == id)
                return o;
            var child = Find(o.Children, id);
            if (child != null)
                return child;
        }
        return null;
    }

    public bool HasSelection => _selected != null;
    public string SelectedName => _selected?.Name ?? string.Empty;
    public System.Collections.Generic.IReadOnlyList<SkyComponent> SelectedComponents =>
        _selected?.Components ?? (System.Collections.Generic.IReadOnlyList<SkyComponent>)System.Array.Empty<SkyComponent>();

    // Editable transform position: the getter formats the cached value, the
    // setter parses and writes it straight to the engine (the live viewport
    // then reflects the move on its next frame).
    public string PositionX
    {
        get => _px.ToString("0.###", CultureInfo.InvariantCulture);
        set => SetAxis(0, value);
    }
    public string PositionY
    {
        get => _py.ToString("0.###", CultureInfo.InvariantCulture);
        set => SetAxis(1, value);
    }
    public string PositionZ
    {
        get => _pz.ToString("0.###", CultureInfo.InvariantCulture);
        set => SetAxis(2, value);
    }

    private void SetAxis(int axis, string text)
    {
        if (_selected == null ||
            !float.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out var value))
            return;
        switch (axis)
        {
            case 0: _px = value; OnPropertyChanged(nameof(PositionX)); break;
            case 1: _py = value; OnPropertyChanged(nameof(PositionY)); break;
            default: _pz = value; OnPropertyChanged(nameof(PositionZ)); break;
        }
        _session.SetPosition(_selected.Id, _px, _py, _pz);
    }

    private static SkyObject? FirstWithComponents(System.Collections.Generic.IEnumerable<SkyObject> objects)
    {
        foreach (var o in objects)
        {
            if (o.Components.Count > 0)
                return o;
            var child = FirstWithComponents(o.Children);
            if (child != null)
                return child;
        }
        return null;
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? name = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
}
