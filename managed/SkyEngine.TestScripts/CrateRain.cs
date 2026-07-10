namespace SkyEngine.Tests;

/// <summary>
/// Мини-игра: с неба падают ящики, игрок (WASD) уклоняется. Полный dogfood
/// скриптового API — Instantiate префабов, чтение позиций чужих объектов,
/// Destroy, Time-план спавна, жизни и счёт через Debug.Log в Console.
/// </summary>
public class CrateRain : SkyEngine.ScriptComponent
{
    public string CratePrefab = "assets://Prefabs/crate.skyprefab";
    public string PlayerPrefab = "assets://Prefabs/player.skyprefab";
    public float Interval = 0.8f;   // секунд между ящиками
    public float Arena = 6.0f;      // полуразмер арены по X/Z
    public int Lives = 3;

    private readonly System.Collections.Generic.List<ulong> _crates = new();
    private readonly System.Random _rng = new(1337);
    private ulong _player;
    private double _nextSpawn;
    private int _score;
    private int _lives;

    public override void OnStart()
    {
        _lives = Lives;
        _player = Instantiate(PlayerPrefab, 0.0f, 0.5f, 0.0f);
        SkyEngine.Debug.Log($"CrateRain: dodge the falling crates! lives={_lives}");
    }

    public override void OnUpdate(double deltaSeconds)
    {
        if (SkyEngine.Time.TotalTime >= _nextSpawn)
        {
            _nextSpawn = SkyEngine.Time.TotalTime + Interval;
            var x = (float)(_rng.NextDouble() * 2 - 1) * Arena;
            var z = (float)(_rng.NextDouble() * 2 - 1) * Arena;
            var crate = Instantiate(CratePrefab, x, 9.0f, z);
            if (crate != 0) _crates.Add(crate);
        }

        var (px, _, pz) = GetWorldPosition(_player);
        for (var i = _crates.Count - 1; i >= 0; --i)
        {
            var (cx, cy, cz) = GetWorldPosition(_crates[i]);
            if (cy > 1.1f)
                continue; // ещё в воздухе
            // Ящик у земли: попал в игрока или промахнулся.
            var dx = cx - px;
            var dz = cz - pz;
            var hit = dx * dx + dz * dz < 1.6f * 1.6f;
            Destroy(_crates[i]);
            _crates.RemoveAt(i);
            if (hit)
            {
                _lives--;
                SkyEngine.Debug.LogWarning($"CrateRain: HIT! lives={_lives}");
                if (_lives <= 0)
                {
                    SkyEngine.Debug.Log($"CrateRain: GAME OVER — score {_score}. Restarting.");
                    _lives = Lives;
                    _score = 0;
                }
            }
            else
            {
                _score++;
                SkyEngine.Debug.Log($"CrateRain: dodged, score={_score}");
            }
        }
    }
}
