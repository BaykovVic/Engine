namespace SkyEngine;

/// <summary>
/// Base class for user gameplay scripts (the Unity-like MonoBehaviour
/// analogue). Lifecycle callbacks are invoked by the native Scripting
/// Boundary through the lifecycle bridge.
/// </summary>
public abstract class ScriptComponent
{
    /// <summary>Handle of the native component this script is bound to.</summary>
    public NativeHandle Handle { get; internal set; } = NativeHandle.Invalid;

    /// <summary>Called once right after the managed peer is created.</summary>
    public virtual void OnCreate() { }

    /// <summary>Called before the first update in play mode.</summary>
    public virtual void OnStart() { }

    /// <summary>Called every runtime frame.</summary>
    public virtual void OnUpdate(double deltaSeconds) { }

    /// <summary>Called on every fixed simulation step.</summary>
    public virtual void OnFixedUpdate(double fixedDeltaSeconds) { }

    /// <summary>Called before the native component is destroyed.</summary>
    public virtual void OnDestroy() { }
}
