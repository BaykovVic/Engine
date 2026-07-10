namespace SkyEngine;

/// <summary>
/// Script logging, Unity-style. Messages land in the editor's Console panel
/// (or the player's standard output). Levels carry the engine's LogLevel
/// values across the boundary: Info = 2, Warning = 3, Error = 4.
/// </summary>
public static class Debug
{
    public static void Log(object? message) => Write(2, message);
    public static void LogWarning(object? message) => Write(3, message);
    public static void LogError(object? message) => Write(4, message);

    private static void Write(int level, object? message)
    {
        try { Engine.Log?.Invoke(level, message?.ToString() ?? "null"); }
        catch { /* logging must never take the script down */ }
    }
}
