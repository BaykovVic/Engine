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
        PointerPressed += (_, e) =>
        {
            Focus();
            SendMouseButton(e.GetCurrentPoint(this).Properties.PointerUpdateKind,
                            down: true);
            SendMousePosition(e.GetPosition(this));
        };
        PointerReleased += (_, e) =>
        {
            SendMouseButton(MapReleased(e.InitialPressMouseButton), down: false);
            SendMousePosition(e.GetPosition(this));
        };
        PointerMoved += (_, e) => SendMousePosition(e.GetPosition(this));
        PointerWheelChanged += (_, e) =>
        {
            if (_vm != null)
                EngineInterop.sky_editor_add_mouse_wheel(
                    _vm.NativeContext, (float)e.Delta.Y);
        };
    }

    /// Mouse position travels in this control's pixel space; gameplay code
    /// reads it through Input.MousePosition.
    private void SendMousePosition(Avalonia.Point position)
    {
        if (_vm == null) return;
        EngineInterop.sky_editor_set_mouse_position(
            _vm.NativeContext, (float)position.X, (float)position.Y);
    }

    /// Engine buttons: 0 = left, 1 = right, 2 = middle.
    private void SendMouseButton(int button, bool down)
    {
        if (_vm == null || button < 0) return;
        EngineInterop.sky_editor_set_mouse_button(_vm.NativeContext, button,
                                                  down ? 1 : 0);
    }

    private void SendMouseButton(PointerUpdateKind kind, bool down) =>
        SendMouseButton(kind switch
        {
            PointerUpdateKind.LeftButtonPressed => 0,
            PointerUpdateKind.RightButtonPressed => 1,
            PointerUpdateKind.MiddleButtonPressed => 2,
            _ => -1,
        }, down);

    private static int MapReleased(MouseButton button) => button switch
    {
        MouseButton.Left => 0,
        MouseButton.Right => 1,
        MouseButton.Middle => 2,
        _ => -1,
    };

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
        Key.Back => 266,
        Key.Delete => 267,
        Key.RightShift => 268,
        Key.RightCtrl => 269,
        Key.RightAlt => 270,
        >= Key.F1 and <= Key.F12 => 271 + (key - Key.F1),
        _ => 0,
    };
}
