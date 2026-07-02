using System;
using System.Collections.Generic;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using SkyEditor.Controls;
using SkyEditor.Docking;
using SkyEditor.Engine;

namespace SkyEditor.Views;

public partial class GameView : UserControl
{
    private VulkanViewport? _viewport;
    private MainViewModel? _vm;
    // Keys currently reported down to the engine, so losing focus can
    // release them (otherwise a held key would stick forever).
    private readonly HashSet<int> _held = new();

    public GameView()
    {
        AvaloniaXamlLoader.Load(this);
        _viewport = this.FindControl<VulkanViewport>("Viewport");
        Focusable = true;
        DataContextChanged += (_, _) =>
        {
            _vm = (DataContext as EditorDocument)?.Main;
            if (_vm != null && _viewport != null)
                _viewport.SetContext(_vm.NativeContext);
        };
        // Clicking the Game view gives it keyboard focus, Unity-style.
        PointerPressed += (_, _) => Focus();
    }

    protected override void OnKeyDown(KeyEventArgs e)
    {
        if (SendKey(e.Key, down: true)) e.Handled = true;
        else base.OnKeyDown(e);
    }

    protected override void OnKeyUp(KeyEventArgs e)
    {
        if (SendKey(e.Key, down: false)) e.Handled = true;
        else base.OnKeyUp(e);
    }

    protected override void OnLostFocus(Avalonia.Interactivity.RoutedEventArgs e)
    {
        base.OnLostFocus(e);
        if (_vm == null) return;
        foreach (var key in _held)
            EngineInterop.sky_editor_set_key_state(_vm.NativeContext, key, 0);
        _held.Clear();
    }

    private bool SendKey(Key key, bool down)
    {
        if (_vm == null) return false;
        var code = MapKey(key);
        if (code == 0) return false;
        EngineInterop.sky_editor_set_key_state(_vm.NativeContext, code, down ? 1 : 0);
        if (down) _held.Add(code);
        else _held.Remove(code);
        return true;
    }

    /// Avalonia keys -> the engine's portable key codes (SkyEngine.KeyCode).
    private static int MapKey(Key key) => key switch
    {
        >= Key.A and <= Key.Z => 'A' + (key - Key.A),
        >= Key.D0 and <= Key.D9 => '0' + (key - Key.D0),
        Key.Space => 32,
        Key.Escape => 256,
        Key.Return => 257,
        Key.Tab => 258,
        Key.LeftShift => 259,
        Key.LeftCtrl => 260,
        Key.LeftAlt => 261,
        Key.Left => 262,
        Key.Right => 263,
        Key.Up => 264,
        Key.Down => 265,
        _ => 0,
    };
}
