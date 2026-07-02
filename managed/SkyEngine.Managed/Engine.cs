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
    public delegate void LogFn(int level, [MarshalAs(UnmanagedType.LPUTF8Str)] string message);

    internal static SetVec3Fn? SetLocalPosition;
    internal static SetVec3Fn? SetLocalEuler;
    internal static SetVec3Fn? SetLocalScale;
    internal static LogFn? Log;

    /// Layout must match the native SkyScriptApi struct (four cdecl pointers).
    [StructLayout(LayoutKind.Sequential)]
    private struct Api
    {
        public IntPtr SetLocalPosition;
        public IntPtr SetLocalEuler;
        public IntPtr SetLocalScale;
        public IntPtr Log;
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
    }
}
