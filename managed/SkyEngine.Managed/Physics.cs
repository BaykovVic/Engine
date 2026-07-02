namespace SkyEngine;

/// <summary>What a raycast hit: the world-space point, the surface normal
/// and the distance from the ray origin.</summary>
public struct RaycastHit
{
    public float X, Y, Z;
    public float NormalX, NormalY, NormalZ;
    public float Distance;
}

/// <summary>Physics queries for gameplay scripts, Unity-style.</summary>
public static class Physics
{
    /// <summary>Casts a ray against the physics world (colliders and the
    /// terrain). True when something was hit within maxDistance.</summary>
    public static bool Raycast(float originX, float originY, float originZ,
                               float dirX, float dirY, float dirZ,
                               float maxDistance, out RaycastHit hit)
    {
        hit = default;
        if (Engine.Raycast == null) return false;
        var result = Engine.Raycast(originX, originY, originZ, dirX, dirY, dirZ,
                                    maxDistance,
                                    out hit.X, out hit.Y, out hit.Z,
                                    out hit.NormalX, out hit.NormalY, out hit.NormalZ,
                                    out hit.Distance);
        return result != 0;
    }
}
