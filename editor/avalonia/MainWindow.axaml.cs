using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Dock.Avalonia.Controls;
using SkyEditor.Docking;

namespace SkyEditor;

public partial class MainWindow : Window
{
    private readonly MainViewModel _vm = new();
    private DockControl? _dock;
    private DockFactory? _factory;

    public MainWindow()
    {
        AvaloniaXamlLoader.Load(this);
        DataContext = _vm;

        _dock = this.FindControl<DockControl>("DockControl");
        _factory = new DockFactory(_vm);
        if (_dock != null)
            _dock.Factory = _factory;
        ResetLayout();

        AddHandler(KeyDownEvent, OnKeyDown, RoutingStrategies.Bubble);
    }

    private void ResetLayout()
    {
        if (_dock == null || _factory == null)
            return;
        var layout = _factory.CreateLayout();
        _factory.InitLayout(layout);
        _dock.Layout = layout;
    }

    private void OnResetLayout(object? sender, RoutedEventArgs e) => ResetLayout();

    private void OnCreateCube(object? sender, RoutedEventArgs e) => _vm.CreateCube();
    private void OnDuplicate(object? sender, RoutedEventArgs e) => _vm.DuplicateSelected();
    private void OnDelete(object? sender, RoutedEventArgs e) => _vm.DeleteSelected();
    private void OnPlay(object? sender, RoutedEventArgs e) => _vm.Play();
    private void OnPause(object? sender, RoutedEventArgs e) => _vm.Pause();
    private void OnStop(object? sender, RoutedEventArgs e) => _vm.Stop();

    private void OnToolHand(object? sender, RoutedEventArgs e) => SelectTool(GizmoTool.Hand);
    private void OnToolMove(object? sender, RoutedEventArgs e) => SelectTool(GizmoTool.Move);
    private void OnToolRotate(object? sender, RoutedEventArgs e) => SelectTool(GizmoTool.Rotate);
    private void OnToolScale(object? sender, RoutedEventArgs e) => SelectTool(GizmoTool.Scale);

    /// Sets the active tool and keeps the four toggles mutually exclusive.
    private void SelectTool(GizmoTool tool)
    {
        _vm.Tool = tool;
        this.FindControl<ToggleButton>("HandTool")!.IsChecked = tool == GizmoTool.Hand;
        this.FindControl<ToggleButton>("MoveTool")!.IsChecked = tool == GizmoTool.Move;
        this.FindControl<ToggleButton>("RotateTool")!.IsChecked = tool == GizmoTool.Rotate;
        this.FindControl<ToggleButton>("ScaleTool")!.IsChecked = tool == GizmoTool.Scale;
    }

    private void OnKeyDown(object? sender, KeyEventArgs e)
    {
        if (TopLevel.GetTopLevel(this)?.FocusManager?.GetFocusedElement() is TextBox)
            return;
        if (e.Key == Key.Delete) { _vm.DeleteSelected(); e.Handled = true; }
        else if (e.Key == Key.D && e.KeyModifiers.HasFlag(KeyModifiers.Control)) { _vm.DuplicateSelected(); e.Handled = true; }
        else if (e.Key == Key.Q) { SelectTool(GizmoTool.Hand); e.Handled = true; }
        else if (e.Key == Key.W) { SelectTool(GizmoTool.Move); e.Handled = true; }
        else if (e.Key == Key.E) { SelectTool(GizmoTool.Rotate); e.Handled = true; }
        else if (e.Key == Key.R) { SelectTool(GizmoTool.Scale); e.Handled = true; }
    }
}
