namespace SkyEngine.Tests;

/// <summary>
/// Test gameplay script: counts lifecycle calls so the native test suite
/// can observe that C# really executed inside the engine.
/// </summary>
public class Spinner : ScriptComponent, IProbe
{
    private long _updates;
    private bool _created;
    private bool _started;

    public long Probe => _updates + (_created ? 1000 : 0) + (_started ? 10000 : 0);

    public override void OnCreate() => _created = true;

    public override void OnStart() => _started = true;

    public override void OnUpdate(double deltaSeconds) => _updates++;
}

/// <summary>A script whose update always throws — failure isolation case.</summary>
public class Faulty : ScriptComponent
{
    public override void OnUpdate(double deltaSeconds) =>
        throw new InvalidOperationException("managed failure");
}
