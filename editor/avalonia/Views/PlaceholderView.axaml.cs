using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace SkyEditor.Views;

public partial class PlaceholderView : UserControl
{
    public PlaceholderView() => AvaloniaXamlLoader.Load(this);
}
