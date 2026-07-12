namespace SkyEngine;

/// <summary>
/// Portable key codes shared with the native side (letters/digits are their
/// ASCII uppercase values; named keys start at 256; mouse buttons live in
/// the same space from 323, mirroring Unity's Mouse0..2). Both the editor's
/// Game view and the standalone player map platform key events onto these.
/// </summary>
public enum KeyCode
{
    Space = 32,
    Alpha0 = 48, Alpha1, Alpha2, Alpha3, Alpha4, Alpha5, Alpha6, Alpha7, Alpha8, Alpha9,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    LeftShift = 259,
    LeftControl = 260,
    LeftAlt = 261,
    LeftArrow = 262,
    RightArrow = 263,
    UpArrow = 264,
    DownArrow = 265,
    Backspace = 266,
    Delete = 267,
    RightShift = 268,
    RightControl = 269,
    RightAlt = 270,
    F1 = 271, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Mouse0 = 323,
    Mouse1 = 324,
    Mouse2 = 325,
}

/// <summary>
/// Keyboard and mouse state for gameplay scripts, Unity-style. The native
/// side owns all state (fed by the editor's Game view or the player's
/// window). Edge queries (GetKeyDown/GetKeyUp) are latched per play frame:
/// every OnFixedUpdate inside one frame sees the same edge state, so a
/// press is never lost between fixed steps.
/// </summary>
public static class Input
{
    /// <summary>True while the key (or mouse button) is held down.</summary>
    public static bool GetKey(KeyCode key) =>
        (Engine.KeyEvent?.Invoke((int)key, 0) ?? 0) != 0;

    /// <summary>True only during the frame the key went down.</summary>
    public static bool GetKeyDown(KeyCode key) =>
        (Engine.KeyEvent?.Invoke((int)key, 1) ?? 0) != 0;

    /// <summary>True only during the frame the key went up.</summary>
    public static bool GetKeyUp(KeyCode key) =>
        (Engine.KeyEvent?.Invoke((int)key, 2) ?? 0) != 0;

    /// <summary>Mouse buttons: 0 = left, 1 = right, 2 = middle.</summary>
    public static bool GetMouseButton(int button) => GetKey(KeyCode.Mouse0 + button);

    public static bool GetMouseButtonDown(int button) =>
        GetKeyDown(KeyCode.Mouse0 + button);

    public static bool GetMouseButtonUp(int button) =>
        GetKeyUp(KeyCode.Mouse0 + button);

    /// <summary>Cursor position in the pixels of the surface feeding it
    /// (the Game view control, or the player window).</summary>
    public static (float X, float Y) MousePosition
    {
        get
        {
            float x = 0, y = 0, wheel = 0;
            Engine.MouseState?.Invoke(out x, out y, out wheel);
            return (x, y);
        }
    }

    /// <summary>Wheel movement accumulated over the current frame
    /// (positive away from the user); resets every play frame.</summary>
    public static float MouseWheelDelta
    {
        get
        {
            float x = 0, y = 0, wheel = 0;
            Engine.MouseState?.Invoke(out x, out y, out wheel);
            return wheel;
        }
    }
}
