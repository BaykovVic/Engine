using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using SkyEditor.Engine;

namespace SkyEditor;

public sealed class MainViewModel : INotifyPropertyChanged
{
    private readonly EditorSession _session;
    private SkyObject? _selected;

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

    public string PositionX => Position(0);
    public string PositionY => Position(1);
    public string PositionZ => Position(2);

    private string Position(int axis)
    {
        if (_selected == null)
            return "0";
        var (position, _, _) = _session.Transform(_selected.Id);
        return position[axis].ToString("0.###");
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
