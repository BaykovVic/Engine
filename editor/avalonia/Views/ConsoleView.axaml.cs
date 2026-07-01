using System;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using SkyEditor.Docking;

namespace SkyEditor.Views;

public partial class ConsoleView : UserControl
{
    private DispatcherTimer? _timer;
    private ScrollViewer? _scroll;

    public ConsoleView()
    {
        AvaloniaXamlLoader.Load(this);
        _scroll = this.FindControl<ScrollViewer>("Scroll");
        AttachedToVisualTree += (_, _) =>
        {
            _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(400) };
            _timer.Tick += (_, _) => Poll();
            _timer.Start();
        };
        DetachedFromVisualTree += (_, _) => _timer?.Stop();
    }

    private MainViewModel? Vm => (DataContext as EditorTool)?.Main;

    private void Poll()
    {
        var vm = Vm;
        if (vm == null) return;
        var before = vm.ConsoleLog.Count;
        vm.RefreshConsole();
        if (vm.ConsoleLog.Count != before)
            _scroll?.ScrollToEnd();
    }

    private void OnClear(object? sender, RoutedEventArgs e) => Vm?.ClearConsole();
}
