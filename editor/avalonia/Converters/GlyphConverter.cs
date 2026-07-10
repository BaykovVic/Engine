using System;
using System.Globalization;
using Avalonia;
using Avalonia.Data.Converters;

namespace SkyEditor.Converters;

/// Resolves an icon resource key (e.g. "IconLight") to its Geometry from the
/// merged design tokens, so list/tree/inspector rows can bind an icon by name.
public sealed class GlyphConverter : IValueConverter
{
    public static readonly GlyphConverter Instance = new();

    public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        if (value is string key && key.Length > 0 &&
            Application.Current?.Resources.TryGetResource(key, null, out var resource) == true)
            return resource;
        return null;
    }

    public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) =>
        throw new NotSupportedException();
}
