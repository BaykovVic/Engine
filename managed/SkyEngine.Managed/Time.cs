namespace SkyEngine;

/// <summary>
/// Frame timing, Unity-style. The native play loop pushes fresh values once
/// per frame before any script's OnUpdate runs.
/// </summary>
public static class Time
{
    /// <summary>Seconds since play started.</summary>
    public static double TotalTime { get; internal set; }

    /// <summary>Seconds the current frame advanced the simulation.</summary>
    public static double DeltaTime { get; internal set; }
}
