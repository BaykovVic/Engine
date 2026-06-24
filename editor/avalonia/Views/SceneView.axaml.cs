using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using SkyEditor.Controls;
using SkyEditor.Docking;

namespace SkyEditor.Views;

public partial class SceneView : UserControl
{
    private VulkanViewport? _viewport;
    private MainViewModel? _vm;

    public SceneView()
    {
        AvaloniaXamlLoader.Load(this);
        _viewport = this.FindControl<VulkanViewport>("Viewport");
        DataContextChanged += (_, _) => Bind();
    }

    private void Bind()
    {
        var vm = (DataContext as EditorDocument)?.Main;
        if (vm == null || _viewport == null || ReferenceEquals(vm, _vm))
            return;
        _vm = vm;
        _viewport.SetContext(vm.NativeContext);
        _viewport.ObjectPicked += vm.SelectById;
        _viewport.SelectedId = vm.SelectedObject?.Id ?? 0;
        vm.PropertyChanged += (_, e) =>
        {
            if (e.PropertyName == nameof(MainViewModel.SelectedObject))
                _viewport.SelectedId = vm.SelectedObject?.Id ?? 0;
        };
    }

    private void OnSpaceToggle(object? sender, RoutedEventArgs e)
    {
        if (_viewport != null && sender is ToggleButton toggle)
            _viewport.LocalSpace = toggle.IsChecked == true;
    }
}
