using Dock.Model.Controls;
using Dock.Model.Core;
using Dock.Model.Mvvm;
using Dock.Model.Mvvm.Controls;

namespace SkyEditor.Docking;

/// Builds the editor's dockable layout: Hierarchy (left), Scene/Game
/// (documents, centre), Inspector/Terrain/Materials (right) and
/// Project/Console/Packages (bottom). All tabs are draggable, floatable,
/// closable and resizable through Dock.Avalonia.
public sealed class DockFactory : Factory
{
    private readonly MainViewModel _main;

    public DockFactory(MainViewModel main) => _main = main;

    public override IRootDock CreateLayout()
    {
        var hierarchy = new HierarchyTool { Id = "Hierarchy", Title = "Hierarchy", Main = _main };

        var scene = new SceneDocument { Id = "Scene", Title = "Scene", Main = _main };
        var game = new PlaceholderTool { Id = "Game", Title = "Game", Caption = "Game view", Main = _main };

        var inspector = new InspectorTool { Id = "Inspector", Title = "Inspector", Main = _main };
        var terrain = new PlaceholderTool { Id = "Terrain", Title = "Terrain", Caption = "Terrain tools", Main = _main };
        var materials = new PlaceholderTool { Id = "Materials", Title = "Materials", Caption = "Material editor", Main = _main };

        var project = new ProjectTool { Id = "Project", Title = "Project", Main = _main };
        var console = new PlaceholderTool { Id = "Console", Title = "Console", Caption = "Console output", Main = _main };
        var packages = new PlaceholderTool { Id = "Packages", Title = "Packages", Caption = "Packages", Main = _main };

        var leftDock = new ToolDock
        {
            Id = "LeftDock", Alignment = Alignment.Left, Proportion = 0.2,
            ActiveDockable = hierarchy,
            VisibleDockables = CreateList<IDockable>(hierarchy),
        };
        var documentDock = new DocumentDock
        {
            Id = "Documents", IsCollapsable = false,
            ActiveDockable = scene,
            VisibleDockables = CreateList<IDockable>(scene),
        };
        var rightDock = new ToolDock
        {
            Id = "RightDock", Alignment = Alignment.Right, Proportion = 0.26,
            ActiveDockable = inspector,
            VisibleDockables = CreateList<IDockable>(inspector, terrain, materials),
        };
        var bottomDock = new ToolDock
        {
            Id = "BottomDock", Alignment = Alignment.Bottom, Proportion = 0.28,
            ActiveDockable = project,
            VisibleDockables = CreateList<IDockable>(project, console, packages),
        };

        var centre = new ProportionalDock
        {
            Orientation = Orientation.Horizontal,
            VisibleDockables = CreateList<IDockable>(
                leftDock, new ProportionalDockSplitter(), documentDock,
                new ProportionalDockSplitter(), rightDock),
        };
        var workspace = new ProportionalDock
        {
            Orientation = Orientation.Vertical,
            VisibleDockables = CreateList<IDockable>(
                centre, new ProportionalDockSplitter(), bottomDock),
        };

        var root = CreateRootDock();
        root.Id = "Root";
        root.VisibleDockables = CreateList<IDockable>(workspace);
        root.ActiveDockable = workspace;
        root.DefaultDockable = workspace;
        return root;
    }
}
