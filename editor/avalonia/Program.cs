using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Headless;
using Avalonia.Threading;

namespace SkyEditor;

internal static class Program
{
    public static AppBuilder BuildAvaloniaApp() =>
        AppBuilder.Configure<App>().UsePlatformDetect().WithInterFont().LogToTrace();

    [STAThread]
    public static int Main(string[] args)
    {
        var shotIndex = Array.IndexOf(args, "--screenshot");
        if (shotIndex >= 0 && shotIndex + 1 < args.Length)
            return Screenshot(args[shotIndex + 1], Array.IndexOf(args, "--demo") >= 0);
        return BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
    }

    private static int Screenshot(string path, bool demo)
    {
        AppBuilder.Configure<App>()
            .UseSkia()
            .UseHeadless(new AvaloniaHeadlessPlatformOptions { UseHeadlessDrawing = false })
            .WithInterFont()
            .SetupWithoutStarting();

        var window = new MainWindow();

        // --size WxH: capture at a custom window size (e.g. tall Inspectors).
        var cmdArgs = Environment.GetCommandLineArgs();
        var sizeIndex = Array.IndexOf(cmdArgs, "--size");
        if (sizeIndex >= 0 && sizeIndex + 1 < cmdArgs.Length)
        {
            var parts = cmdArgs[sizeIndex + 1].Split('x');
            if (parts.Length == 2 &&
                double.TryParse(parts[0], out var w) && double.TryParse(parts[1], out var h))
            {
                window.Width = w;
                window.Height = h;
            }
        }

        window.Show();
        Dispatcher.UIThread.RunJobs();

        // --select <name>: select a scene object by name, so captures can show
        // its Inspector (components, script class picker, ...).
        var selectIndex = Array.IndexOf(cmdArgs, "--select");
        if (selectIndex >= 0 && selectIndex + 1 < cmdArgs.Length &&
            window.DataContext is MainViewModel svm)
        {
            var found = FindByName(svm.Roots, cmdArgs[selectIndex + 1]);
            if (found != null)
            {
                svm.SelectedObject = found;
                Dispatcher.UIThread.RunJobs();
            }
        }

        // Exercise the editing path: create a cube and move it, so the capture
        // shows it both in the Hierarchy and in the live viewport.
        if (demo && window.DataContext is MainViewModel vm)
        {
            vm.CreateCube();
            vm.PositionX = "3";
            vm.PositionY = "3";
            SkyEditor.Engine.EngineInterop.sky_editor_set_local_euler(
                vm.NativeContext, vm.SelectedObject!.Id, 0, 40, 0);
            Dispatcher.UIThread.RunJobs();
        }

        // Drive viewport frames so the offscreen scene is present. With --play,
        // enter play mode and run long enough for the rigidbody crates to fall.
        var viewport = window.FindControl<Controls.VulkanViewport>("Viewport");
        var play = demo && System.Array.IndexOf(System.Environment.GetCommandLineArgs(), "--play") >= 0;
        if (play && window.DataContext is MainViewModel pvm)
            SkyEditor.Engine.EngineInterop.sky_editor_play(pvm.NativeContext);
        for (var i = 0; i < (play ? 60 : 4); ++i)
        {
            viewport?.RenderOnce();
            Dispatcher.UIThread.RunJobs();
        }

        var frame = window.CaptureRenderedFrame();
        if (frame is null)
        {
            Console.Error.WriteLine("capture failed");
            return 1;
        }
        frame.Save(path);
        Console.WriteLine($"wrote {path}");
        return 0;
    }

    private static Engine.SkyObject? FindByName(
        System.Collections.Generic.IEnumerable<Engine.SkyObject> nodes, string name)
    {
        foreach (var node in nodes)
        {
            if (node.Name == name)
                return node;
            if (FindByName(node.Children, name) is { } hit)
                return hit;
        }
        return null;
    }
}
