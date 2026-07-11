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

    /// <summary>Simulation speed multiplier, Unity-style: scales DeltaTime
    /// and the fixed-step accumulator (0 pauses the simulation). The native
    /// side owns the value and resets it to 1 on every play start, so a
    /// slow-motion experiment never leaks into the next session.</summary>
    public static double TimeScale
    {
        get => Engine.TimeScale?.Invoke(0.0, 0) ?? 1.0;
        set => Engine.TimeScale?.Invoke(value, 1);
    }
}
