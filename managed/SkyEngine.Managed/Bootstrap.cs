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

    // The user-scripts assembly lives in its own collectible load context, so
    // recompiling the project's scripts can swap it between play sessions.
    private static AssemblyLoadContext? _userContext;
    private static Assembly? _userAssembly;

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

    /// <summary>Loads (or replaces) the project's compiled user-scripts
    /// assembly. Read into memory and hosted in a collectible load context:
    /// the file stays unlocked and a recompile swaps the old code out. Engine
    /// types resolve back to this assembly's context, so ScriptComponent
    /// keeps one identity.</summary>
    [UnmanagedCallersOnly]
    public static int LoadUserAssembly(IntPtr pathUtf8)
    {
        try
        {
            var path = Marshal.PtrToStringUTF8(pathUtf8);
            if (string.IsNullOrEmpty(path) || !File.Exists(path))
            {
                return 0;
            }
            var bootstrapContext = AssemblyLoadContext.GetLoadContext(typeof(Bootstrap).Assembly)
                                   ?? AssemblyLoadContext.Default;
            var context = new AssemblyLoadContext("SkyUserScripts", isCollectible: true);
            context.Resolving += (_, name) =>
            {
                try { return bootstrapContext.LoadFromAssemblyName(name); }
                catch { return null; }
            };
            using var stream = new MemoryStream(File.ReadAllBytes(Path.GetFullPath(path)));
            var assembly = context.LoadFromStream(stream);

            _userContext?.Unload();
            _userContext = context;
            _userAssembly = assembly;
            return 1;
        }
        catch
        {
            return 0;
        }
    }

    /// <summary>Unloads the user-scripts assembly (its collectible context
    /// goes away). Used when the last user script source disappears — e.g.
    /// deactivating the only code-carrying package.</summary>
    [UnmanagedCallersOnly]
    public static void UnloadUserAssembly()
    {
        _userContext?.Unload();
        _userContext = null;
        _userAssembly = null;
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
            if (_userAssembly != null)
            {
                assemblies.Add(_userAssembly);
            }
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

    /// <summary>Writes the class's serializable script fields — public
    /// instance fields of float/int/bool/string — as "name\ttype\tdefault"
    /// lines (UTF-8) into the caller's buffer. Defaults come from a throwaway
    /// instance, so the Inspector shows what the script author declared.</summary>
    [UnmanagedCallersOnly]
    public static int GetScriptFields(IntPtr classNameUtf8, IntPtr buffer, int capacity)
    {
        try
        {
            var typeName = Marshal.PtrToStringUTF8(classNameUtf8);
            if (string.IsNullOrEmpty(typeName))
            {
                return 0;
            }
            var type = ResolveType(typeName);
            if (type == null || !typeof(ScriptComponent).IsAssignableFrom(type) || type.IsAbstract)
            {
                return 0;
            }
            object? defaults = null;
            try { defaults = Activator.CreateInstance(type); } catch { /* no defaults */ }
            var text = new System.Text.StringBuilder();
            foreach (var field in type.GetFields(BindingFlags.Public | BindingFlags.Instance))
            {
                var kind = FieldKind(field.FieldType);
                if (kind == null)
                {
                    continue;
                }
                var value = defaults != null ? field.GetValue(defaults) : null;
                var defText = value switch
                {
                    float f => f.ToString(System.Globalization.CultureInfo.InvariantCulture),
                    int i => i.ToString(System.Globalization.CultureInfo.InvariantCulture),
                    bool b => b ? "true" : "false",
                    string s => s,
                    _ => string.Empty,
                };
                text.Append(field.Name).Append('\t').Append(kind).Append('\t')
                    .Append(defText).Append('\n');
            }
            var bytes = System.Text.Encoding.UTF8.GetBytes(text.ToString());
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

    /// <summary>Sets a serializable field on a live script instance from its
    /// string form (authored in the Inspector, stored in the component).</summary>
    [UnmanagedCallersOnly]
    public static int SetScriptField(ulong id, IntPtr nameUtf8, IntPtr valueUtf8)
    {
        try
        {
            if (!Instances.TryGetValue(id, out var script))
            {
                return 0;
            }
            var name = Marshal.PtrToStringUTF8(nameUtf8);
            var text = Marshal.PtrToStringUTF8(valueUtf8) ?? string.Empty;
            if (string.IsNullOrEmpty(name))
            {
                return 0;
            }
            var field = script.GetType().GetField(name, BindingFlags.Public | BindingFlags.Instance);
            if (field == null || FieldKind(field.FieldType) == null)
            {
                return 0;
            }
            var culture = System.Globalization.CultureInfo.InvariantCulture;
            object? parsed =
                field.FieldType == typeof(float) ? float.Parse(text, culture) :
                field.FieldType == typeof(int) ? (int)long.Parse(text, culture) :
                field.FieldType == typeof(bool) ? text is "true" or "1" :
                field.FieldType == typeof(string) ? text : null;
            if (parsed == null)
            {
                return 0;
            }
            field.SetValue(script, parsed);
            return 1;
        }
        catch
        {
            return 0;
        }
    }

    private static string? FieldKind(Type type) =>
        type == typeof(float) ? "float" :
        type == typeof(int) ? "int" :
        type == typeof(bool) ? "bool" :
        type == typeof(string) ? "string" : null;

    [UnmanagedCallersOnly]
    public static long GetProbe(ulong id)
    {
        return Instances.TryGetValue(id, out var script) && script is IProbe probe
                   ? probe.Probe
                   : -1;
    }

    private static Type? ResolveType(string typeName)
    {
        // User scripts win: a project class shadows same-named engine samples.
        if (_userAssembly?.GetType(typeName) is { } userType)
        {
            return userType;
        }
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
