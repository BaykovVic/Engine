using System;
using System.Runtime.InteropServices;

namespace SkyEngine;

/// <summary>
/// The managed side of the reverse boundary: native engine functions a script
/// may call to affect its object. The native host installs the function
/// pointers once at startup via <see cref="Bootstrap.Initialize"/>; scripts
/// reach them through <see cref="ScriptComponent"/> keyed by their object id.
/// </summary>
public static class Engine
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void SetVec3Fn(ulong objectId, float x, float y, float z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void GetVec3Fn(ulong objectId, out float x, out float y, out float z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void LogFn(int level, [MarshalAs(UnmanagedType.LPUTF8Str)] string message);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int IsKeyDownFn(int key);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate ulong InstantiateFn(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string prefabPath, float x, float y, float z);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void DestroyFn(ulong objectId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int RaycastFn(float ox, float oy, float oz,
                                  float dx, float dy, float dz, float maxDistance,
                                  out float px, out float py, out float pz,
                                  out float nx, out float ny, out float nz,
                                  out float distance);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate int DataAssetFn(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string assetRef, byte[] buffer,
        int capacity);

    /// <summary>Get/set in one pointer: apply != 0 stores the (clamped)
    /// value; the current scale is always returned.</summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate double TimeScaleFn(double value, int apply);

    internal static SetVec3Fn? SetLocalPosition;
    internal static SetVec3Fn? SetLocalEuler;
    internal static SetVec3Fn? SetLocalScale;
    internal static LogFn? Log;
    internal static GetVec3Fn? GetLocalPosition;
    internal static IsKeyDownFn? IsKeyDown;
    internal static GetVec3Fn? GetWorldPosition;
    internal static InstantiateFn? Instantiate;
    internal static DestroyFn? DestroyObject;
    internal static SetVec3Fn? SetVelocity;
    internal static GetVec3Fn? GetVelocity;
    internal static RaycastFn? Raycast;
    internal static DataAssetFn? DataAssetFields;
    internal static TimeScaleFn? TimeScale;

    /// Layout must match the native SkyScriptApi struct (fourteen cdecl
    /// pointers; the native side static_asserts the same count).
    [StructLayout(LayoutKind.Sequential)]
    private struct Api
    {
        public IntPtr SetLocalPosition;
        public IntPtr SetLocalEuler;
        public IntPtr SetLocalScale;
        public IntPtr Log;
        public IntPtr GetLocalPosition;
        public IntPtr IsKeyDown;
        public IntPtr GetWorldPosition;
        public IntPtr Instantiate;
        public IntPtr DestroyObject;
        public IntPtr SetVelocity;
        public IntPtr GetVelocity;
        public IntPtr Raycast;
        public IntPtr DataAsset;
        public IntPtr TimeScale;
    }

    /// Both sides of the boundary must agree on the pointer count; keep in
    /// sync with the native static_assert on sizeof(SkyScriptApi).
    private const int ExpectedApiPointers = 14;

    internal static void Install(IntPtr apiPtr)
    {
        if (apiPtr == IntPtr.Zero) return;
        var api = Marshal.PtrToStructure<Api>(apiPtr);
        if (api.SetLocalPosition != IntPtr.Zero)
            SetLocalPosition = Marshal.GetDelegateForFunctionPointer<SetVec3Fn>(api.SetLocalPosition);
        if (api.SetLocalEuler != IntPtr.Zero)
            SetLocalEuler = Marshal.GetDelegateForFunctionPointer<SetVec3Fn>(api.SetLocalEuler);
        if (api.SetLocalScale != IntPtr.Zero)
            SetLocalScale = Marshal.GetDelegateForFunctionPointer<SetVec3Fn>(api.SetLocalScale);
        if (api.Log != IntPtr.Zero)
            Log = Marshal.GetDelegateForFunctionPointer<LogFn>(api.Log);
        if (api.GetLocalPosition != IntPtr.Zero)
            GetLocalPosition = Marshal.GetDelegateForFunctionPointer<GetVec3Fn>(api.GetLocalPosition);
        if (api.IsKeyDown != IntPtr.Zero)
            IsKeyDown = Marshal.GetDelegateForFunctionPointer<IsKeyDownFn>(api.IsKeyDown);
        if (api.GetWorldPosition != IntPtr.Zero)
            GetWorldPosition = Marshal.GetDelegateForFunctionPointer<GetVec3Fn>(api.GetWorldPosition);
        if (api.Instantiate != IntPtr.Zero)
            Instantiate = Marshal.GetDelegateForFunctionPointer<InstantiateFn>(api.Instantiate);
        if (api.DestroyObject != IntPtr.Zero)
            DestroyObject = Marshal.GetDelegateForFunctionPointer<DestroyFn>(api.DestroyObject);
        if (api.SetVelocity != IntPtr.Zero)
            SetVelocity = Marshal.GetDelegateForFunctionPointer<SetVec3Fn>(api.SetVelocity);
        if (api.GetVelocity != IntPtr.Zero)
            GetVelocity = Marshal.GetDelegateForFunctionPointer<GetVec3Fn>(api.GetVelocity);
        if (api.Raycast != IntPtr.Zero)
            Raycast = Marshal.GetDelegateForFunctionPointer<RaycastFn>(api.Raycast);
        if (api.DataAsset != IntPtr.Zero)
            DataAssetFields = Marshal.GetDelegateForFunctionPointer<DataAssetFn>(api.DataAsset);
        if (api.TimeScale != IntPtr.Zero)
            TimeScale = Marshal.GetDelegateForFunctionPointer<TimeScaleFn>(api.TimeScale);

        // Layout guard: a one-sided table edit must be loud, not a silent
        // misroute of every call after the mismatch.
        if (Marshal.SizeOf<Api>() != IntPtr.Size * ExpectedApiPointers)
            Log?.Invoke(3, "SkyScriptApi layout mismatch: managed Api pointer " +
                           "count diverged from the native table");
    }

    /// Raw serialized fields of a data asset (US/RS separated records) with
    /// the query-buffer retry the boundary contract promises.
    internal static string ReadDataAssetRaw(string assetRef)
    {
        var fn = DataAssetFields;
        if (fn == null || string.IsNullOrEmpty(assetRef))
            return string.Empty;
        var buffer = new byte[1024];
        var length = fn(assetRef, buffer, buffer.Length);
        if (length <= 0)
            return string.Empty;
        if (length >= buffer.Length)
        {
            buffer = new byte[length + 1];
            length = fn(assetRef, buffer, buffer.Length);
        }
        var copied = Math.Min(length, buffer.Length - 1);
        return copied <= 0
            ? string.Empty
            : System.Text.Encoding.UTF8.GetString(buffer, 0, copied);
    }
}
