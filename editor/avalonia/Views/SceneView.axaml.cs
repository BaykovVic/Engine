using System;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using SkyEditor.Controls;
using SkyEditor.Docking;
using SkyEditor.Engine;

namespace SkyEditor.Views;

public partial class SceneView : UserControl
{
    private VulkanViewport? _viewport;
    private MainViewModel? _vm;
    private Border? _badge;
    private TextBlock? _badgeText;
    private DispatcherTimer? _stateTimer;

    public SceneView()
    {
        AvaloniaXamlLoader.Load(this);
        _viewport = this.FindControl<VulkanViewport>("Viewport");
        _badge = this.FindControl<Border>("PausedBadge");
        _badgeText = this.FindControl<TextBlock>("PausedText");
        DataContextChanged += (_, _) => Bind();
        AttachedToVisualTree += (_, _) =>
        {
            _stateTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(200) };
            _stateTimer.Tick += (_, _) => UpdatePlayBadge();
            _stateTimer.Start();
        };
        DetachedFromVisualTree += (_, _) => _stateTimer?.Stop();
    }

    private void UpdatePlayBadge()
    {
        if (_vm == null || _badge == null || _badgeText == null)
            return;
        var state = EngineInterop.sky_editor_play_state(_vm.NativeContext);
        _badge.IsVisible = state != 0;
        if (_badgeText != null)
            _badgeText.Text = state == 1 ? "Playing" : "Paused";
    }

    private void Bind()
    {
        var vm = (DataContext as EditorDocument)?.Main;
        if (vm == null || _viewport == null || ReferenceEquals(vm, _vm))
            return;
        _vm = vm;
        _viewport.SetContext(vm.NativeContext);
        _viewport.ObjectPicked += vm.SelectById;
        _viewport.TransformChanged += vm.ReloadTransform;
        _viewport.SelectedId = vm.SelectedObject?.Id ?? 0;
        _viewport.Tool = vm.Tool;
        vm.PropertyChanged += (_, e) =>
        {
            if (e.PropertyName == nameof(MainViewModel.SelectedObject))
                _viewport.SelectedId = vm.SelectedObject?.Id ?? 0;
            else if (e.PropertyName == nameof(MainViewModel.Tool))
                _viewport.Tool = vm.Tool;
        };
    }

    private void OnSpaceToggle(object? sender, RoutedEventArgs e)
    {
        if (_viewport != null && sender is ToggleButton toggle)
            _viewport.LocalSpace = toggle.IsChecked == true;
    }

    private void OnView3D(object? sender, RoutedEventArgs e) => SetView2D(false);
    private void OnView2D(object? sender, RoutedEventArgs e) => SetView2D(true);

    /// Switches between the perspective orbit view and the orthographic YZ
    /// plane, keeping the two toggles mutually exclusive.
    private void SetView2D(bool enabled)
    {
        _vm?.SetView2D(enabled);
        this.FindControl<ToggleButton>("view3D")!.IsChecked = !enabled;
        this.FindControl<ToggleButton>("view2D")!.IsChecked = enabled;
    }
}
