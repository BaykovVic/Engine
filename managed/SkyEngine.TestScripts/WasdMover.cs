namespace SkyEngine.Tests;

/// <summary>
/// Demo gameplay script: WASD moves the object in the XZ plane at 3 units/s.
/// Exercises the full input + timing loop — Input.GetKey reads native key
/// state, Time.DeltaTime scales the step, GetLocalPosition/SetLocalPosition
/// round-trip the transform.
/// </summary>
public class WasdMover : SkyEngine.ScriptComponent
{
    private const float Speed = 3.0f;

    public override void OnUpdate(double deltaSeconds)
    {
        var step = Speed * (float)SkyEngine.Time.DeltaTime;
        var (x, y, z) = GetLocalPosition();
        if (SkyEngine.Input.GetKey(SkyEngine.KeyCode.W)) z += step;
        if (SkyEngine.Input.GetKey(SkyEngine.KeyCode.S)) z -= step;
        if (SkyEngine.Input.GetKey(SkyEngine.KeyCode.A)) x -= step;
        if (SkyEngine.Input.GetKey(SkyEngine.KeyCode.D)) x += step;
        SetLocalPosition(x, y, z);
    }
}
