# День 7

## E5 · `feature/c-abi-seed`

### Файлы `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`, `editor/native_bridge/src/editor_bridge.cpp`
Плоский C-интерфейс (совместно с E1). На этом этапе — минимум:
- `SkyEditorContext* sky_editor_create(void)` — Возвращает: указатель на сессию (собирает движок и демо-сцену).
- `void sky_editor_destroy(SkyEditorContext* ctx)` — уничтожает сессию.
- `int32_t sky_editor_root_count(SkyEditorContext* ctx)` — Возвращает: число корневых объектов.
- `SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index)` — Возвращает: id корневого объекта.
- `int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)` — пишет имя в буфер. Возвращает: длину.

**На выходе:** проект собирается; CI прогоняет сборку и тесты; C-интерфейс create/destroy/enumerate работает.
**Критерий правильности этапа:** красный CI блокирует слияние; сломанный тест
краснеет; редактор E3 вызывает `sky_editor_create`.

---

## E3 · `feature/engine-bridge`

### Файлы `editor/avalonia/Engine/EngineInterop.cs`, `Engine/EditorSession.cs`
- `static class EngineInterop` — резолвер нативной библиотеки и P/Invoke-объявления. На этом этапе:
  - `[DllImport] static extern IntPtr sky_editor_create()` — Возвращает: указатель на сессию движка.
  - `[DllImport] static extern void sky_editor_destroy(IntPtr ctx)` — уничтожает сессию.
  - `static IntPtr Resolve(...)`, `static string[] Candidates()` — находят `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в дереве сборки.
- `class EditorSession : IDisposable` — обёртка над сессией: конструктор вызывает `sky_editor_create` и проверяет не-null; `Dispose()` → `destroy`; свойство `IntPtr Native`.

**На выходе:** `dotnet build` без ошибок; окно открывается; панели перетаскиваются.
**Критерий правильности этапа:** `dotnet build` = 0 ошибок; сессия движка
создаётся из .NET (не-null); есть сброс компоновки.
