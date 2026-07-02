using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace SkyEngine;

/// <summary>
/// Optional capability for tests and diagnostics: a script exposes one
/// observable value the native side can read back.
/// </summary>
public interface IProbe
{
    long Probe { get; }
}

/// <summary>
/// The managed entry points the native Scripting Boundary calls through
/// hostfxr. This is the only surface crossing the boundary: instances are
/// identified by opaque ids, all state stays on whichever side owns it.
/// </summary>
public static class Bootstrap
{
    private static readonly Dictionary<ulong, ScriptComponent> Instances = new();
    private static readonly List<Assembly> LoadedAssemblies = new();
    private static ulong _nextId = 1;

    [UnmanagedCallersOnly]
    public static int LoadAssembly(IntPtr pathUtf8)
    {
        try
        {
            var path = Marshal.PtrToStringUTF8(pathUtf8);
            if (string.IsNullOrEmpty(path))
            {
                return 0;
            }
            // Load into the same AssemblyLoadContext hostfxr put this
            // assembly into, so ScriptComponent is one identity everywhere.
            var context = AssemblyLoadContext.GetLoadContext(typeof(Bootstrap).Assembly)
                          ?? AssemblyLoadContext.Default;
            LoadedAssemblies.Add(context.LoadFromAssemblyPath(Path.GetFullPath(path)));
            return 1;
        }
        catch
        {
            return 0;
        }
    }

    /// <summary>Installs the native engine API (reverse-call function table).</summary>
    [UnmanagedCallersOnly]
    public static void Initialize(IntPtr apiPtr)
    {
        try { Engine.Install(apiPtr); } catch { /* isolated */ }
    }

    /// <summary>Publishes frame timing before the frame's OnUpdate dispatches.</summary>
    [UnmanagedCallersOnly]
    public static void TickFrame(double totalSeconds, double deltaSeconds)
    {
        Time.TotalTime = totalSeconds;
        Time.DeltaTime = deltaSeconds;
    }

    /// <summary>Points a live script instance at the native object it drives,
    /// so its transform helpers act on that object.</summary>
    [UnmanagedCallersOnly]
    public static void SetObjectId(ulong instanceId, ulong objectId)
    {
        if (Instances.TryGetValue(instanceId, out var script))
        {
            script.Handle = new NativeHandle(objectId);
        }
    }

    [UnmanagedCallersOnly]
    public static ulong CreateInstance(IntPtr typeNameUtf8)
    {
        try
        {
            var typeName = Marshal.PtrToStringUTF8(typeNameUtf8);
            if (string.IsNullOrEmpty(typeName))
            {
                return 0;
            }
            var type = ResolveType(typeName);
            if (type == null || !typeof(ScriptComponent).IsAssignableFrom(type))
            {
                return 0;
            }
            if (Activator.CreateInstance(type) is not ScriptComponent instance)
            {
                return 0;
            }
            var id = _nextId++;
            instance.Handle = new NativeHandle(id);
            Instances[id] = instance;
            return id;
        }
        catch
        {
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static void DestroyInstance(ulong id)
    {
        Instances.Remove(id);
    }

    /// <summary>Event order mirrors the native ScriptLifecycleEvent enum.</summary>
    [UnmanagedCallersOnly]
    public static int InvokeLifecycle(ulong id, int lifecycleEvent, double deltaSeconds)
    {
        if (!Instances.TryGetValue(id, out var script))
        {
            return 0;
        }
        try
        {
            switch (lifecycleEvent)
            {
                case 0: script.OnCreate(); break;
                case 1: script.OnStart(); break;
                case 2: script.OnUpdate(deltaSeconds); break;
                case 3: script.OnFixedUpdate(deltaSeconds); break;
                case 4: script.OnDestroy(); break;
                default: return 0;
            }
            return 1;
        }
        catch
        {
            // Managed failures are isolated; they never corrupt native state.
            return 0;
        }
    }

    /// <summary>Writes the full names of all instantiable ScriptComponent
    /// subclasses (newline-separated, UTF-8) into the caller's buffer, for the
    /// editor's script-class picker. Returns the byte length written.</summary>
    [UnmanagedCallersOnly]
    public static int GetScriptClasses(IntPtr buffer, int capacity)
    {
        try
        {
            var names = new List<string>();
            var assemblies = new List<Assembly>(LoadedAssemblies) { typeof(Bootstrap).Assembly };
            foreach (var assembly in assemblies)
            {
                foreach (var type in assembly.GetTypes())
                {
                    if (typeof(ScriptComponent).IsAssignableFrom(type) &&
                        !type.IsAbstract && type.FullName != null)
                    {
                        names.Add(type.FullName);
                    }
                }
            }
            names.Sort(StringComparer.Ordinal);
            var joined = string.Join('\n', names);
            var bytes = System.Text.Encoding.UTF8.GetBytes(joined);
            var length = Math.Min(bytes.Length, Math.Max(capacity - 1, 0));
            Marshal.Copy(bytes, 0, buffer, length);
            Marshal.WriteByte(buffer, length, 0);
            return length;
        }
        catch
        {
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    public static long GetProbe(ulong id)
    {
        return Instances.TryGetValue(id, out var script) && script is IProbe probe
                   ? probe.Probe
                   : -1;
    }

    private static Type? ResolveType(string typeName)
    {
        var type = Type.GetType(typeName);
        if (type != null)
        {
            return type;
        }
        foreach (var assembly in LoadedAssemblies)
        {
            type = assembly.GetType(typeName);
            if (type != null)
            {
                return type;
            }
        }
        foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
        {
            type = assembly.GetType(typeName);
            if (type != null)
            {
                return type;
            }
        }
        return null;
    }
}
