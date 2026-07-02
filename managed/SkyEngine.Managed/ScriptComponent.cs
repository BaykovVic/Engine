namespace SkyEngine;

/// <summary>
/// Base class for user gameplay scripts (the Unity-like MonoBehaviour
/// analogue). Lifecycle callbacks are invoked by the native Scripting
/// Boundary through the lifecycle bridge.
/// </summary>
public abstract class ScriptComponent
{
    /// <summary>Handle of the native object this script drives (its id).</summary>
    public NativeHandle Handle { get; internal set; } = NativeHandle.Invalid;

    /// <summary>Sets the object's local position (relative to its parent).</summary>
    protected void SetLocalPosition(float x, float y, float z) =>
        Engine.SetLocalPosition?.Invoke(Handle.Value, x, y, z);

    /// <summary>The object's local position (relative to its parent).</summary>
    protected (float X, float Y, float Z) GetLocalPosition()
    {
        float x = 0, y = 0, z = 0;
        Engine.GetLocalPosition?.Invoke(Handle.Value, out x, out y, out z);
        return (x, y, z);
    }

    /// <summary>Sets the object's local rotation from Euler angles in degrees.</summary>
    protected void SetLocalEuler(float x, float y, float z) =>
        Engine.SetLocalEuler?.Invoke(Handle.Value, x, y, z);

    /// <summary>Sets the object's local scale.</summary>
    protected void SetLocalScale(float x, float y, float z) =>
        Engine.SetLocalScale?.Invoke(Handle.Value, x, y, z);

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
