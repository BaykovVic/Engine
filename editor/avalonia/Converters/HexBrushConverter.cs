using System;
using System.Globalization;
using Avalonia.Data.Converters;
using Avalonia.Media;

namespace SkyEditor.Converters;

/// Converts a "#RRGGBB" string (e.g. a log level colour) to a brush.
public sealed class HexBrushConverter : IValueConverter
{
    public static readonly HexBrushConverter Instance = new();

    public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        if (value is string hex && Color.TryParse(hex, out var color))
            return new SolidColorBrush(color);
        return Brushes.Gray;
    }

    public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture) =>
        throw new NotSupportedException();
}
