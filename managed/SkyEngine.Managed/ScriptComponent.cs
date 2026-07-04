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

    /// <summary>The object's world-space position.</summary>
    protected (float X, float Y, float Z) GetWorldPosition()
    {
        float x = 0, y = 0, z = 0;
        Engine.GetWorldPosition?.Invoke(Handle.Value, out x, out y, out z);
        return (x, y, z);
    }

    /// <summary>World-space position of any object by id — e.g. one this
    /// script spawned via Instantiate.</summary>
    protected static (float X, float Y, float Z) GetWorldPosition(ulong objectId)
    {
        float x = 0, y = 0, z = 0;
        Engine.GetWorldPosition?.Invoke(objectId, out x, out y, out z);
        return (x, y, z);
    }

    /// <summary>Sets the local position of any object by id.</summary>
    protected static void SetLocalPosition(ulong objectId, float x, float y, float z) =>
        Engine.SetLocalPosition?.Invoke(objectId, x, y, z);

    /// <summary>Sets the rigid-body velocity of any object by id.</summary>
    protected static void SetVelocity(ulong objectId, float x, float y, float z) =>
        Engine.SetVelocity?.Invoke(objectId, x, y, z);

    /// <summary>Spawns a prefab (.skyprefab path or "assets://..." reference)
    /// at a world position. Returns the new object's id (0 on failure). In
    /// play mode the spawned object's own scripts start on the next frame.</summary>
    protected ulong Instantiate(string prefabPath, float x, float y, float z) =>
        Engine.Instantiate?.Invoke(prefabPath, x, y, z) ?? 0;

    /// <summary>Destroys this script's object (with children).</summary>
    protected void Destroy() => Destroy(Handle.Value);

    /// <summary>Destroys a scene object by id (with children).</summary>
    protected void Destroy(ulong objectId) =>
        Engine.DestroyObject?.Invoke(objectId);

    /// <summary>Sets the linear velocity of the object's rigid body (no-op
    /// when the object has no physics body).</summary>
    protected void SetVelocity(float x, float y, float z) =>
        Engine.SetVelocity?.Invoke(Handle.Value, x, y, z);

    /// <summary>The linear velocity of the object's rigid body (zero when
    /// the object has no physics body).</summary>
    protected (float X, float Y, float Z) GetVelocity()
    {
        float x = 0, y = 0, z = 0;
        Engine.GetVelocity?.Invoke(Handle.Value, out x, out y, out z);
        return (x, y, z);
    }

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
