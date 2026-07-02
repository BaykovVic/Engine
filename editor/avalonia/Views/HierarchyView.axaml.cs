using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using SkyEditor.Docking;

namespace SkyEditor.Views;

public partial class HierarchyView : UserControl
{
    public HierarchyView()
    {
        AvaloniaXamlLoader.Load(this);
        // Accept model assets dragged from the Project panel.
        DragDrop.SetAllowDrop(this, true);
        AddHandler(DragDrop.DragOverEvent, OnDragOver);
        AddHandler(DragDrop.DropEvent, OnDrop);
    }

    private MainViewModel? Vm => (DataContext as EditorTool)?.Main;

    private void OnCreate(object? sender, RoutedEventArgs e) => Vm?.CreateCube();

    private static void OnDragOver(object? sender, DragEventArgs e)
    {
        e.DragEffects = e.Data.Contains(ProjectView.AssetRefFormat)
            ? DragDropEffects.Copy
            : DragDropEffects.None;
        e.Handled = true;
    }

    private void OnDrop(object? sender, DragEventArgs e)
    {
        if (e.Data.Get(ProjectView.AssetRefFormat) is string assetRef)
        {
            if (assetRef.ToLowerInvariant().EndsWith(".skyprefab"))
                Vm?.InstantiatePrefabFromAsset(assetRef);
            else
                Vm?.CreateModelFromAsset(assetRef);
        }
        e.Handled = true;
    }
}
