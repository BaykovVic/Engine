namespace SkyEngine.Tests;

/// <summary>
/// Spawns a prefab above itself every Interval seconds — exercises
/// Instantiate from script code plus Time-based scheduling. Spawned objects
/// bring their own components (and scripts) with them.
/// </summary>
public class Spawner : SkyEngine.ScriptComponent
{
    public string PrefabPath = "";
    public float Interval = 0.5f;

    private double _nextSpawn;

    public override void OnUpdate(double deltaSeconds)
    {
        if (PrefabPath.Length == 0 || SkyEngine.Time.TotalTime < _nextSpawn)
        {
            return;
        }
        _nextSpawn = SkyEngine.Time.TotalTime + Interval;
        var (x, y, z) = GetWorldPosition();
        Instantiate(PrefabPath, x, y + 2.0f, z);
    }
}
