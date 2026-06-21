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
            return Screenshot(args[shotIndex + 1]);
        return BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
    }

    private static int Screenshot(string path)
    {
        AppBuilder.Configure<App>()
            .UseSkia()
            .UseHeadless(new AvaloniaHeadlessPlatformOptions { UseHeadlessDrawing = false })
            .WithInterFont()
            .SetupWithoutStarting();

        var window = new MainWindow();
        window.Show();
        Dispatcher.UIThread.RunJobs();
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
