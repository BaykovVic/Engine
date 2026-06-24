using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using SkyEditor.Controls;

namespace SkyEditor;

public partial class MainWindow : Window
{
    private readonly MainViewModel _vm = new();
    private VulkanViewport? _viewport;

    public MainWindow()
    {
        AvaloniaXamlLoader.Load(this);
        DataContext = _vm;

        _viewport = this.FindControl<VulkanViewport>("Viewport");
        if (_viewport != null)
        {
            _viewport.SetContext(_vm.NativeContext);
            _viewport.ObjectPicked += _vm.SelectById;
            _viewport.SelectedId = _vm.SelectedObject?.Id ?? 0;
            _vm.PropertyChanged += (_, e) =>
            {
                if (e.PropertyName == nameof(MainViewModel.SelectedObject))
                    _viewport.SelectedId = _vm.SelectedObject?.Id ?? 0;
            };
        }

        AddHandler(KeyDownEvent, OnKeyDown, RoutingStrategies.Bubble);
    }

    private void OnSpaceToggle(object? sender, RoutedEventArgs e)
    {
        if (_viewport != null && sender is ToggleButton toggle)
            _viewport.LocalSpace = toggle.IsChecked == true;
    }

    private void OnCreateCube(object? sender, RoutedEventArgs e) => _vm.CreateCube();
    private void OnDuplicate(object? sender, RoutedEventArgs e) => _vm.DuplicateSelected();
    private void OnDelete(object? sender, RoutedEventArgs e) => _vm.DeleteSelected();

    private void OnKeyDown(object? sender, KeyEventArgs e)
    {
        // Let text fields keep their own Delete / typing.
        if (TopLevel.GetTopLevel(this)?.FocusManager?.GetFocusedElement() is TextBox)
            return;

        if (e.Key == Key.Delete)
        {
            _vm.DeleteSelected();
            e.Handled = true;
        }
        else if (e.Key == Key.D && e.KeyModifiers.HasFlag(KeyModifiers.Control))
        {
            _vm.DuplicateSelected();
            e.Handled = true;
        }
    }
}
