using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using SkyEditor.Controls;
using SkyEditor.Docking;

namespace SkyEditor.Views;

public partial class GameView : UserControl
{
    private VulkanViewport? _viewport;

    public GameView()
    {
        AvaloniaXamlLoader.Load(this);
        _viewport = this.FindControl<VulkanViewport>("Viewport");
        DataContextChanged += (_, _) =>
        {
            var vm = (DataContext as EditorDocument)?.Main;
            if (vm != null && _viewport != null)
                _viewport.SetContext(vm.NativeContext);
        };
    }
}
