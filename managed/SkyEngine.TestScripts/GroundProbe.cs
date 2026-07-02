namespace SkyEngine.Tests;

/// <summary>
/// Raycasts straight down at start and reports what it stands on —
/// exercises Physics.Raycast against the terrain and colliders.
/// </summary>
public class GroundProbe : SkyEngine.ScriptComponent
{
    public override void OnStart()
    {
        var (x, y, z) = GetWorldPosition();
        if (SkyEngine.Physics.Raycast(x, y + 10.0f, z, 0.0f, -1.0f, 0.0f, 100.0f,
                                      out var hit))
        {
            // Rounded to whole units: culture-independent log output.
            SkyEngine.Debug.Log(
                $"GroundProbe hit y={(int)System.Math.Round(hit.Y)}" +
                $" d={(int)System.Math.Round(hit.Distance)}");
        }
        else
        {
            SkyEngine.Debug.LogWarning("GroundProbe found no ground");
        }
    }
}
