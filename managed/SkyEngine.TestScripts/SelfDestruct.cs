namespace SkyEngine.Tests;

/// <summary>
/// Destroys its own object after Life seconds — exercises Destroy from
/// script code. The engine sweeps the orphaned managed instance (OnDestroy
/// fires) at the end of the frame.
/// </summary>
public class SelfDestruct : SkyEngine.ScriptComponent
{
    public float Life = 0.5f;

    public override void OnUpdate(double deltaSeconds)
    {
        if (SkyEngine.Time.TotalTime >= Life)
        {
            Destroy();
        }
    }
}
