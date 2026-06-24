using Dock.Model.Mvvm.Controls;

namespace SkyEditor.Docking;

/// Dockable tools/documents carry a reference to the shared editor view model
/// so their views (resolved by DataTemplate) bind to live engine state.
public class EditorTool : Tool
{
    public MainViewModel? Main { get; set; }
}

public class EditorDocument : Document
{
    public MainViewModel? Main { get; set; }
}

public sealed class HierarchyTool : EditorTool { }
public sealed class InspectorTool : EditorTool { }
public sealed class MaterialsTool : EditorTool { }
public sealed class ProjectTool : EditorTool { }
public sealed class SceneDocument : EditorDocument { }

public sealed class GameDocument : EditorDocument
{
    public string Caption { get; set; } = "Game view — runs in play mode";
}

/// A not-yet-built tab (Terrain, Materials, Console, Packages, Game).
public sealed class PlaceholderTool : EditorTool
{
    public string Caption { get; set; } = "";
}
