# Sky Engine · Недели 7–8 (M4 «Продукт») — пофайловая детализация

Продолжение [weeks5-6-files.md](weeks5-6-files.md). Легенда: `[H]` заголовок,
`[S]` исходник, `[T]` тест, `[B]` сборка. Цель: пользовательские скрипты с
сериализуемыми полями, префабы, тени, мини-игра — движок, в котором делают.

---

## E1 · Префабы, терраин в SKYB

#### `[H+S] editor/shell/src/editor_context.{hpp,cpp}` (дополнить)
- `bool savePrefab(ObjectHandle, path);` — SKYP (magic "SKYP") поверх `snapshotObject`;
- `ObjectHandle instantiatePrefab(path);` `ObjectHandle spawnPrefabAt(path, worldPos);` (для скриптов);
- анонимный namespace: `writeSnapshot(ByteWriter&, ObjectSnapshot)`, `readSnapshot(ByteReader&, out)`, `resolveAssetPath("assets://...")`.

#### `[S] engine/scene/src/scene_world.cpp` (дополнить)
- сериализация heightfield террейна и `enabled`-состояния в SKYB (сейчас — фикстура).

## E2 · Тени directional (shadow mapping)

#### `[H+B] engine/rendering_vulkan/shaders/shadow.vert` + `shadow.vert.spv.h`
- depth-only вершинный шейдер: `lightViewProjection * model * pos`; обе матрицы в push-константах (`struct ShadowPush`).

#### `[S] engine/rendering_vulkan/src/vulkan_renderer.cpp` (дополнить)
- `initShadowResources()` — D32 2048² сэмплируемая карта, clamp-to-border sampler, отдельный render pass, пайплайн (slope-scaled bias), дескриптор set 7;
- в `renderFrame`: сперва depth-only shadow pass по всем DrawMesh, затем основной pass с биндом карты;
- `FrameUbo` растёт: `lightViewProjection[16]`, `counts.y/z` (флаг теней, индекс источника);
- `mesh.frag`: `shadowVisibility(worldPos, ndl)` — 3×3 PCF + normal-bias.

## E3 · Пикер классов, сериализуемые поля скриптов

#### `[S] managed/SkyEngine.Managed/Bootstrap.cs` (дополнить)
- `[UnmanagedCallersOnly] GetScriptClasses(buffer,cap)` — рефлексия по наследникам `ScriptComponent`;
- `GetScriptFields(className,buffer,cap)` — публичные float/int/bool/string поля с дефолтами;
- `SetScriptField(id,name,value)` — запись в живой инстанс.

#### `[S] editor/native_bridge/src/editor_bridge.cpp` (дополнить)
- ABI `script_class_count/name`, `script_field_count/name/type/value`, `set_script_field` (кэш классов/полей в `BridgeSession`).

#### `[S] editor/avalonia/Engine/EditorSession.cs` + `Views/InspectorView.axaml` (дополнить)
- `ComponentField.IsScriptClass` → ComboBox классов; `ScriptClasses(current)`;
- поля скрипта (`scriptParam`) добавляются к списку у компонента `sky.script`; сеттер через `SetScriptField`.

## E4 · Пользовательские сборки, игровой API

#### `[S] editor/shell/src/editor_context.cpp` (дополнить)
- `reloadUserScripts()` — сгенерировать csproj над `Assets/Scripts/*.cs` (+ `Runtime/*.cs` активных пакетов) → `dotnet build` → `loadUserAssembly`;
- `scriptSourceDirs()`; recompile на входе в Play по mtime-штампу.

#### `[S] managed/SkyEngine.Managed/Bootstrap.cs` (дополнить)
- `LoadUserAssembly(path)` / `UnloadUserAssembly()` — collectible `AssemblyLoadContext`, загрузка из памяти.

#### `[S] managed/SkyEngine.Managed/Engine.cs` + `ScriptComponent.cs` + `Physics.cs` (дополнить)
- таблица reverse-API растёт до 12 указателей: `GetWorldPosition`, `Instantiate`, `DestroyObject`, `SetVelocity`, `GetVelocity`, `Raycast`;
- `ScriptComponent`: `Instantiate/Destroy/Set-GetVelocity/GetWorldPosition` (в т.ч. по id чужого объекта);
- `Physics.Raycast(...) → RaycastHit`.

#### `[S] editor/shell/src/editor_context.cpp` (дополнить)
- нативные колбэки `scriptGetWorldPosition/Instantiate/DestroyObject/SetVelocity/GetVelocity/Raycast`;
- физика: `raycast` по heightfield (марш + бисекция + градиентная нормаль).

## E5 · Демо-игра, cooking, RuntimeContext

#### `[S] managed/SkyEngine.TestScripts/CrateRain.cs`
- `class CrateRain : ScriptComponent` — спавн ящиков-префабов, пешка игрока, жизни/счёт в Console; dogfood всего API.

#### `[H+S] engine/platform/…/pak_archive` (задел)
- `.skypak`: упаковка сцен+ассетов в один архив (cooking).

#### `[S] editor/shell/src/*` (задел RuntimeContext)
- отделение рантайм-состояния от `EditorContext`, чтобы в билд не ехали демо-ассеты и редакторская инфраструктура.

## E6 · Профилирование систем

#### `[S] engine/ecs/src/ecs_world.cpp` (дополнить)
- измерение длительности `update` каждой системы за такт; накопление таймингов.

#### `[S] editor/native_bridge` + `editor/avalonia` (совместно с E3/E5)
- ABI выдачи таймингов систем; панель профилировщика в редакторе.

**На выходе:** длительность такта каждой системы измеряется и отображается в панели профилировщика.

**Ворота M4:** пользовательский `.cs` из Assets/Scripts работает в Play;
префаб drag-drop'ится; сцена отбрасывает тени; мини-игра запускается в
плеере из сохранённой сцены.

---

## Итог по всем неделям

| Неделя | Документ | Ворота |
|---|---|---|
| 1 | [week1-files.md](week1-files.md) | зелёный CI + `triangle.png` |
| 2 | [week2-files.md](week2-files.md) | M1 демо-сцена в панели |
| 3–4 | [weeks3-4-files.md](weeks3-4-files.md) | M2 собрал→save→open→play→restore |
| 5–6 | [weeks5-6-files.md](weeks5-6-files.md) | M3 скрипт в редакторе и в билде |
| 7–8 | [weeks7-8-files.md](weeks7-8-files.md) | M4 пользовательские скрипты, префабы, тени, демка |
