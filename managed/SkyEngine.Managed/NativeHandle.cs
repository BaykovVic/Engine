namespace SkyEngine;

/// <summary>
/// Opaque handle to a native engine object. The managed side never owns
/// engine state: every interaction goes through engine API calls keyed by
/// this handle, and the authoritative state changes only on the native side.
/// </summary>
public readonly record struct NativeHandle(ulong Value)
{
    public static readonly NativeHandle Invalid = new(0);

    public bool IsValid => Value != 0;
}
