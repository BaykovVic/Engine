namespace SkyEngine.Tests;

/// <summary>
/// Demo gameplay script: spins its object around Y at 90°/s. Proves the full
/// scripting loop — the native play tick invokes OnUpdate, and the script calls
/// back into the engine to move the object it is attached to.
/// </summary>
public class Rotator : SkyEngine.ScriptComponent
{
    /// Degrees per second around Y — serializable, editable in the Inspector.
    public float Speed = 90.0f;

    private float _angle;

    public override void OnStart()
    {
        SkyEngine.Debug.Log($"Rotator started on object {Handle.Value}");
    }

    public override void OnUpdate(double deltaSeconds)
    {
        _angle += (float)(deltaSeconds * Speed);
        SetLocalEuler(0.0f, _angle, 0.0f);
    }
}
