using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using SkyEditor.Docking;

namespace SkyEditor.Views;

public partial class HierarchyView : UserControl
{
    public HierarchyView() => AvaloniaXamlLoader.Load(this);

    private MainViewModel? Vm => (DataContext as EditorTool)?.Main;

    private void OnCreate(object? sender, RoutedEventArgs e) => Vm?.CreateCube();
}
