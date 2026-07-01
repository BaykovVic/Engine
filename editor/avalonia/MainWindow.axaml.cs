using System;
using System.Collections.Generic;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Platform.Storage;
using Avalonia.Threading;
using Dock.Avalonia.Controls;
using SkyEditor.Docking;
using SkyEditor.Engine;

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

        // Reflect the live play state on the transport buttons: play lit while
        // playing, pause lit while paused.
        var transport = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(200) };
        transport.Tick += (_, _) => UpdateTransport();
        transport.Start();
    }

    private void UpdateTransport()
    {
        var state = EngineInterop.sky_editor_play_state(_vm.NativeContext); // 0=edit,1=play,2=pause
        SetClass(this.FindControl<Button>("playButton"), state == 1);
        SetClass(this.FindControl<Button>("pauseButton"), state == 2);
    }

    private static void SetClass(Button? button, bool on)
    {
        if (button == null) return;
        if (on && !button.Classes.Contains("on")) button.Classes.Add("on");
        else if (!on && button.Classes.Contains("on")) button.Classes.Remove("on");
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

    // --- Scene document (File menu) ---
    private static readonly FilePickerFileType SkyboxType =
        new("Sky Scene") { Patterns = new[] { "*.skybox" } };

    private void OnNewScene(object? sender, RoutedEventArgs e) => _vm.NewScene();

    private async void OnOpenScene(object? sender, RoutedEventArgs e)
    {
        var top = TopLevel.GetTopLevel(this);
        if (top == null) return;
        var files = await top.StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title = "Open Scene",
            AllowMultiple = false,
            FileTypeFilter = new List<FilePickerFileType> { SkyboxType },
        });
        var path = files.Count > 0 ? files[0].TryGetLocalPath() : null;
        if (!string.IsNullOrEmpty(path)) _vm.OpenScene(path);
    }

    private async void OnSaveScene(object? sender, RoutedEventArgs e)
    {
        if (_vm.HasScenePath) { _vm.SaveScene(_vm.CurrentScenePath); return; }
        await SaveAs();
    }

    private async void OnSaveSceneAs(object? sender, RoutedEventArgs e) => await SaveAs();

    private async System.Threading.Tasks.Task SaveAs()
    {
        var top = TopLevel.GetTopLevel(this);
        if (top == null) return;
        var file = await top.StorageProvider.SaveFilePickerAsync(new FilePickerSaveOptions
        {
            Title = "Save Scene",
            SuggestedFileName = _vm.SceneTitle + ".skybox",
            DefaultExtension = "skybox",
            FileTypeChoices = new List<FilePickerFileType> { SkyboxType },
        });
        var path = file?.TryGetLocalPath();
        if (!string.IsNullOrEmpty(path)) _vm.SaveScene(path);
    }

    private void OnQuit(object? sender, RoutedEventArgs e) => Close();

    private void OnUndo(object? sender, RoutedEventArgs e) => _vm.Undo();
    private void OnRedo(object? sender, RoutedEventArgs e) => _vm.Redo();

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
        var ctrl = e.KeyModifiers.HasFlag(KeyModifiers.Control);
        if (e.Key == Key.Delete) { _vm.DeleteSelected(); e.Handled = true; }
        else if (e.Key == Key.D && ctrl) { _vm.DuplicateSelected(); e.Handled = true; }
        else if (e.Key == Key.N && ctrl) { _vm.NewScene(); e.Handled = true; }
        else if (e.Key == Key.O && ctrl) { OnOpenScene(this, e); e.Handled = true; }
        else if (e.Key == Key.S && ctrl) { OnSaveScene(this, e); e.Handled = true; }
        else if (e.Key == Key.Z && ctrl) { _vm.Undo(); e.Handled = true; }
        else if (e.Key == Key.Y && ctrl) { _vm.Redo(); e.Handled = true; }
        else if (e.Key == Key.Q) { SelectTool(GizmoTool.Hand); e.Handled = true; }
        else if (e.Key == Key.W) { SelectTool(GizmoTool.Move); e.Handled = true; }
        else if (e.Key == Key.E) { SelectTool(GizmoTool.Rotate); e.Handled = true; }
        else if (e.Key == Key.R) { SelectTool(GizmoTool.Scale); e.Handled = true; }
    }
}
