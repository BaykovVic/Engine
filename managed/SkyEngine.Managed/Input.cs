namespace SkyEngine;

/// <summary>
/// Portable key codes shared with the native side (letters/digits are their
/// ASCII uppercase values; named keys start at 256). Both the editor's Game
/// view and the standalone player map platform key events onto these.
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
}

/// <summary>
/// Keyboard state for gameplay scripts, Unity-style. The native side owns the
/// pressed-key set (fed by the editor's Game view or the player's window).
/// </summary>
public static class Input
{
    /// <summary>True while the key is held down.</summary>
    public static bool GetKey(KeyCode key) =>
        (Engine.IsKeyDown?.Invoke((int)key) ?? 0) != 0;
}
