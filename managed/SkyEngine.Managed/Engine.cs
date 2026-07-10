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

    /// Layout must match the native SkyScriptApi struct (twelve cdecl pointers).
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
    }

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
    }
}
