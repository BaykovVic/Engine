using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using SkyEditor.Controls;

namespace SkyEditor;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        AvaloniaXamlLoader.Load(this);
        var vm = new MainViewModel();
        DataContext = vm;
        this.FindControl<VulkanViewport>("Viewport")?.SetContext(vm.NativeContext);
    }
}
