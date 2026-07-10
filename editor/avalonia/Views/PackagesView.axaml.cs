using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using SkyEditor.Docking;
using SkyEditor.Engine;

namespace SkyEditor.Views;

public partial class PackagesView : UserControl
{
    public PackagesView() => AvaloniaXamlLoader.Load(this);

    private MainViewModel? Vm => (DataContext as EditorTool)?.Main;

    private void OnRefresh(object? sender, RoutedEventArgs e) => Vm?.RediscoverPackages();

    private void OnToggle(object? sender, RoutedEventArgs e)
    {
        if (sender is Control { DataContext: PackageInfo package })
            Vm?.TogglePackage(package);
    }

    private void OnInstall(object? sender, RoutedEventArgs e)
    {
        var box = this.FindControl<TextBox>("InstallSource");
        var source = box?.Text?.Trim();
        if (string.IsNullOrEmpty(source))
            return;
        if (Vm?.InstallPackage(source) == true && box != null)
            box.Text = string.Empty;
    }
}
