using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using SkyEditor.Docking;

namespace SkyEditor.Views;

public partial class TerrainView : UserControl
{
    public TerrainView() => AvaloniaXamlLoader.Load(this);

    private void OnGenerate(object? sender, RoutedEventArgs e) =>
        (DataContext as EditorTool)?.Main?.GenerateTerrain();
}
