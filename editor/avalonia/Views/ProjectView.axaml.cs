using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using SkyEditor.Docking;
using SkyEditor.Engine;

namespace SkyEditor.Views;

public partial class ProjectView : UserControl
{
    /// Drag payload key for a model asset reference dragged into the Hierarchy.
    public const string AssetRefFormat = "sky-asset-ref";

    private Point _pressPoint;
    private ProjectEntry? _pressEntry;

    public ProjectView() => AvaloniaXamlLoader.Load(this);

    private void OnOpen(object? sender, TappedEventArgs e)
    {
        if (sender is ListBox { SelectedItem: ProjectEntry entry })
            (DataContext as EditorTool)?.Main?.OpenProjectEntry(entry);
    }

    private void OnPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        _pressPoint = e.GetPosition(this);
        _pressEntry = (e.Source as StyledElement)?.DataContext as ProjectEntry;
    }

    private async void OnPointerMoved(object? sender, PointerEventArgs e)
    {
        if (_pressEntry == null)
            return;
        if (!e.GetCurrentPoint(this).Properties.IsLeftButtonPressed)
        {
            _pressEntry = null;
            return;
        }
        var p = e.GetPosition(this);
        if (Math.Abs(p.X - _pressPoint.X) + Math.Abs(p.Y - _pressPoint.Y) < 5)
            return;

        var main = (DataContext as EditorTool)?.Main;
        var entry = _pressEntry;
        _pressEntry = null;
        if (main == null || !main.IsModelAsset(entry))
            return;

        var data = new DataObject();
        data.Set(AssetRefFormat, main.AssetRefFor(entry!));
        await DragDrop.DoDragDrop(e, data, DragDropEffects.Copy);
    }
}
