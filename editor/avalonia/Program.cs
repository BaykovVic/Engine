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
        window.Show();
        Dispatcher.UIThread.RunJobs();

        // Exercise the editing path: create a cube and move it, so the capture
        // shows it both in the Hierarchy and in the live viewport.
        if (demo && window.DataContext is MainViewModel vm)
        {
            vm.CreateCube();
            vm.PositionX = "3";
            vm.PositionY = "3";
            Dispatcher.UIThread.RunJobs();
        }

        // Drive a few viewport frames so the offscreen scene is present.
        var viewport = window.FindControl<Controls.VulkanViewport>("Viewport");
        for (var i = 0; i < 4; ++i)
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
}
