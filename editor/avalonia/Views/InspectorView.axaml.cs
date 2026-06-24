using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace SkyEditor.Views;

public partial class InspectorView : UserControl
{
    public InspectorView() => AvaloniaXamlLoader.Load(this);
}
