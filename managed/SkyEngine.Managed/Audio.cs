namespace SkyEngine;

/// <summary>
/// Gameplay audio, Unity-style entry point. Clips are WAV assets referenced
/// by VFS path; the native mixer decodes once, caches, and mixes on its own
/// device thread. Voices started during play are stopped when play ends.
/// </summary>
public static class Audio
{
    /// <summary>Starts a clip; returns a voice id (0 when the clip is
    /// missing or undecodable). A looping voice plays until stopped.</summary>
    public static ulong Play(string clipRef, float volume = 1.0f, bool loop = false) =>
        Engine.PlaySound?.Invoke(clipRef, volume, loop ? 1 : 0) ?? 0;

    /// <summary>Stops a voice returned by Play. Unknown ids are a no-op.</summary>
    public static void Stop(ulong voice) => Engine.StopSound?.Invoke(voice);
}
