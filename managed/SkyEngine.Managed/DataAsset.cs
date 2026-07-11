using System.Collections.Generic;
using System.Globalization;

namespace SkyEngine;

/// <summary>
/// A read-only data asset (Assets/Data/*.skydata) — the ScriptableObject
/// analog. Load resolves the parent-chain inheritance on the native side,
/// so the fields seen here are the effective ones. Values are typed at
/// authoring time; the getters convert with an explicit fallback.
/// </summary>
public sealed class DataAsset
{
    private readonly Dictionary<string, (string Type, string Value)> _fields;

    private DataAsset(string assetRef,
                      Dictionary<string, (string, string)> fields)
    {
        Ref = assetRef;
        _fields = fields;
    }

    public string Ref { get; }
    public IReadOnlyCollection<string> FieldNames => _fields.Keys;

    /// <summary>Loads a data asset by its VFS reference
    /// ("assets://Data/enemy.skydata"); null when it does not exist.</summary>
    public static DataAsset? Load(string assetRef)
    {
        var raw = Engine.ReadDataAssetRaw(assetRef);
        if (string.IsNullOrEmpty(raw))
            return null;
        var fields = new Dictionary<string, (string, string)>();
        foreach (var record in raw.Split('\x1E'))
        {
            var parts = record.Split('\x1F');
            if (parts.Length == 3 && parts[0].Length > 0)
                fields[parts[0]] = (parts[1], parts[2]);
        }
        return fields.Count == 0 ? null : new DataAsset(assetRef, fields);
    }

    public bool Has(string name) => _fields.ContainsKey(name);

    public string TypeOf(string name) =>
        _fields.TryGetValue(name, out var field) ? field.Type : string.Empty;

    public string GetString(string name, string fallback = "") =>
        _fields.TryGetValue(name, out var field) ? field.Value : fallback;

    public float GetFloat(string name, float fallback = 0f) =>
        _fields.TryGetValue(name, out var field) &&
        float.TryParse(field.Value, NumberStyles.Float,
                       CultureInfo.InvariantCulture, out var value)
            ? value
            : fallback;

    public long GetInt(string name, long fallback = 0) =>
        _fields.TryGetValue(name, out var field) &&
        long.TryParse(field.Value, NumberStyles.Integer,
                      CultureInfo.InvariantCulture, out var value)
            ? value
            : fallback;

    public bool GetBool(string name, bool fallback = false) =>
        _fields.TryGetValue(name, out var field)
            ? field.Value == "true" || field.Value == "1"
            : fallback;

    /// <summary>Vec3 fields are stored "x, y, z".</summary>
    public bool TryGetVec3(string name, out float x, out float y, out float z)
    {
        x = y = z = 0f;
        if (!_fields.TryGetValue(name, out var field))
            return false;
        var parts = field.Value.Split(',');
        return parts.Length == 3 &&
               float.TryParse(parts[0].Trim(), NumberStyles.Float,
                              CultureInfo.InvariantCulture, out x) &&
               float.TryParse(parts[1].Trim(), NumberStyles.Float,
                              CultureInfo.InvariantCulture, out y) &&
               float.TryParse(parts[2].Trim(), NumberStyles.Float,
                              CultureInfo.InvariantCulture, out z);
    }
}
