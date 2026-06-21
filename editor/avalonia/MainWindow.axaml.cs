using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace SkyEditor;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        AvaloniaXamlLoader.Load(this);
        DataContext = new MainViewModel();
    }
}
