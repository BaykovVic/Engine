namespace SkyEngine;

/// <summary>
/// Deterministic gameplay randomness (PCG32). Mirrors the native
/// sky::core::Pcg32 bit for bit: the same seed produces the same sequence on
/// both sides of the boundary, so recorded sessions replay exactly. Unlike
/// Unity, the default state is a FIXED seed — runs are reproducible unless
/// the game explicitly randomizes via <see cref="InitState"/>.
/// </summary>
public static class Random
{
    private static ulong _state;
    private static ulong _increment;

    static Random() => InitState(0);

    /// <summary>Reseeds the generator; the same seed replays the same
    /// sequence (and matches the native Pcg32 for that seed).</summary>
    public static void InitState(ulong seed, ulong sequence = 54)
    {
        _state = 0;
        _increment = (sequence << 1) | 1;
        NextUInt();
        _state += seed;
        NextUInt();
    }

    /// <summary>The raw 32-bit output, one generator step.</summary>
    public static uint NextUInt()
    {
        var old = _state;
        _state = unchecked(old * 6364136223846793005UL + _increment);
        var xorshifted = (uint)(((old >> 18) ^ old) >> 27);
        var rot = (int)(old >> 59);
        return (xorshifted >> rot) | (xorshifted << ((32 - rot) & 31));
    }

    /// <summary>Uniform float in [0, 1) from the top 24 bits (exactly a
    /// float mantissa, identical to the native computation).</summary>
    public static float Value => (NextUInt() >> 8) * (1.0f / 16777216.0f);

    /// <summary>Uniform integer in [minInclusive, maxExclusive).</summary>
    public static int Range(int minInclusive, int maxExclusive)
    {
        if (maxExclusive <= minInclusive)
            return minInclusive;
        return minInclusive +
               (int)(NextUInt() % (uint)(maxExclusive - minInclusive));
    }

    /// <summary>Uniform float in [min, max).</summary>
    public static float Range(float min, float max) => min + Value * (max - min);
}
