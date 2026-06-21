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

        var viewport = this.FindControl<VulkanViewport>("Viewport");
        if (viewport != null)
        {
            viewport.SetContext(vm.NativeContext);
            viewport.ObjectPicked += vm.SelectById;
        }
    }
}
