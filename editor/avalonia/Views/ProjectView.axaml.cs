using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using SkyEditor.Docking;
using SkyEditor.Engine;

namespace SkyEditor.Views;

public partial class ProjectView : UserControl
{
    public ProjectView() => AvaloniaXamlLoader.Load(this);

    private void OnOpen(object? sender, TappedEventArgs e)
    {
        if (sender is ListBox { SelectedItem: ProjectEntry entry })
            (DataContext as EditorTool)?.Main?.OpenProjectEntry(entry);
    }
}
