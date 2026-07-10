using System;
using System.Collections.Generic;
using Dock.Avalonia.Controls;
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
    private IRootDock? _root;

    public DockFactory(MainViewModel main) => _main = main;

    public override IRootDock CreateLayout()
    {
        var hierarchy = new HierarchyTool { Id = "Hierarchy", Title = "Hierarchy", Main = _main };

        var scene = new SceneDocument { Id = "Scene", Title = "Scene", Main = _main };
        var game = new GameDocument { Id = "Game", Title = "Game", Main = _main };

        var inspector = new InspectorTool { Id = "Inspector", Title = "Inspector", Main = _main };
        var terrain = new TerrainTool { Id = "Terrain", Title = "Terrain", Main = _main };
        var materials = new MaterialsTool { Id = "Materials", Title = "Materials", Main = _main };

        var project = new ProjectTool { Id = "Project", Title = "Project", Main = _main };
        var console = new ConsoleTool { Id = "Console", Title = "Console", Main = _main };
        var packages = new PackagesTool { Id = "Packages", Title = "Packages", Main = _main };

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
            VisibleDockables = CreateList<IDockable>(scene, game),
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
        _root = root;
        return root;
    }

    /// Wire up the locators Dock.Avalonia needs for live drag/dock/float.
    /// Without HostWindowLocator a panel dragged out of its dock cannot
    /// spawn its own floating window, so tabs feel "stuck".
    public override void InitLayout(IDockable layout)
    {
        ContextLocator = new Dictionary<string, Func<object?>>();

        DockableLocator = new Dictionary<string, Func<IDockable?>>
        {
            ["Root"] = () => _root,
        };

        HostWindowLocator = new Dictionary<string, Func<IHostWindow?>>
        {
            [nameof(IDockWindow)] = () => new HostWindow(),
        };

        base.InitLayout(layout);
    }
}
