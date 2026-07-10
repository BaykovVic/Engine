using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using SkyEditor.Docking;
using SkyEditor.Engine;

namespace SkyEditor.Views;

public partial class InspectorView : UserControl
{
    public InspectorView() => AvaloniaXamlLoader.Load(this);

    private MainViewModel? Vm => (DataContext as EditorTool)?.Main;

    private void OnRemoveComponent(object? sender, RoutedEventArgs e)
    {
        if (sender is Control { DataContext: ComponentView view })
            Vm?.RemoveComponent(view.Index);
    }

    private void OnAddComponentSelected(object? sender, SelectionChangedEventArgs e)
    {
        if (sender is not ListBox list || list.SelectedItem is not ComponentType type)
            return;
        list.SelectedItem = null; // let the same type be picked again next time
        this.FindControl<Button>("AddComponentButton")?.Flyout?.Hide();
        Vm?.AddComponent(type.TypeId);
    }
}
