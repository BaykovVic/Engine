# Sky Engine · Недели 5–6 (M3 «Замкнутые петли») — пофайловая детализация

Продолжение [weeks3-4-files.md](weeks3-4-files.md). Легенда: `[H]` заголовок,
`[S]` исходник, `[T]` тест, `[B]` сборка. Цель: standalone-плеер запускает
авторские сцены; C#-скрипт работает и в редакторе, и в билде.

---

## E1 · Граница контекста, физика из компонентов

#### `[S] editor/shell/src/editor_context.cpp` (дополнить)
- `reattachPhysics()` — восстановление тел из `sky.rigidbody`/`sky.collider.box` компонентов при `openScene`;
- ревизия границы: пометить, что уедет в рантайм-контекст (задел под RuntimeContext, неделя 8).

## E2 · Текстуры, PBR-слоты, процедурные примитивы

#### `[S] engine/rendering_vulkan/src/vulkan_renderer.cpp` (дополнить)
- `uploadTexture(w,h,rgba)` → `GpuTexture{image,view,set}`; дескрипторные сеты 1..6 (albedo/normal/roughness/metallic/occlusion/height);
- `bindMaterial(command)` — привязка шести сетов с нейтральными дефолтами (белый / плоская нормаль).

#### `[S] engine/scene/src/scene_authoring.cpp` (дополнить)
- процедурная геометрия для `PrimitiveKind::Sphere`/`Plane` (генерация вершин), а не куб-заглушка.

## E3 · Пикер мешей, drag-drop, scene-гизмо, 2D

#### `[S] editor/avalonia/Engine/EditorSession.cs` (дополнить)
- `class MeshOption { Display; Value; }`; `AvailableMeshes(current)` — примитивы + модели из `assets://Models`;
- `CreateModel(name, meshRef)`, `MeshDisplayName(value)`.

#### `[S] editor/avalonia/Views/ProjectView.axaml.cs`, `HierarchyView.axaml.cs`
- drag модели из Project (`DataObject` с `AssetRefFormat`), drop в Hierarchy → `CreateModelFromAsset`.

#### `[S] editor/avalonia/Controls/VulkanViewport.cs` (дополнить)
- scene-гизмо в углу (оси X/Y/Z по камере, клик по конусу → `look_along_axis`, Persp/Iso); 2D ортографический режим (тумблер 3D/2D).

## E4 · Скриптинг (критический путь)

#### `[H] engine/scripting/include/sky/scripting/scripting_boundary.hpp`
- `enum class ScriptLifecycleEvent { OnCreate, OnStart, OnUpdate, OnFixedUpdate, OnDestroy };`
- `struct AssemblyRef { name; path; }`; типы границы.

#### `[H] engine/scripting/include/sky/scripting/script_host.hpp`
- `class IScriptHost { bool start(); void shutdown(); bool loadAssembly(AssemblyRef); createInstance(typeName)→uint64; destroyInstance(id); bool invokeLifecycle(id, event, dt); }`

#### `[H] engine/scripting/include/sky/scripting/dotnet_host.hpp` `[S] dotnet_host.cpp`
- `struct DotNetHostConfig { hostfxrPath; bootstrapAssembly; }`
- `class DotNetScriptHost : IScriptHost { installEngineApi(apiTable); setInstanceObjectId(mid,objId); beginFrame(total,dt); scriptClassNames(); ... }`
- `createDotNetScriptHost(config)`; реализация через `hostfxr_*` + `load_assembly_and_get_function_pointer` + `UnmanagedCallersOnly`-энтрипоинты.

#### `[S] managed/SkyEngine.Managed/*.cs`
- `Bootstrap.cs` — `[UnmanagedCallersOnly]` энтрипоинты: `LoadAssembly`, `CreateInstance`, `DestroyInstance`, `InvokeLifecycle`, `Initialize`, `SetObjectId`, `TickFrame`.
- `ScriptComponent.cs` — базовый класс: `OnCreate/OnStart/OnUpdate/OnFixedUpdate/OnDestroy`, `SetLocalPosition/Euler/Scale`, `Handle`.
- `NativeHandle.cs`, `Engine.cs` (таблица reverse-API), `Debug.cs`, `Time.cs`, `Input.cs`.

#### `[S] editor/shell/src/editor_context.cpp` (дополнить)
- `initScripting()` (под `#ifdef SKY_MANAGED_DIR`): создать host, `loadAssembly`, `installEngineApi(&g_scriptApi)`;
- `startPlayScripts()`, `tickScripts(dt)`, `stopPlayScripts()`; файл-scope колбэки `scriptSetLocal*`, `scriptLogMessage`, `scriptIsKeyDown`, структура `SkyScriptApi`.
- регистрация компонента `sky.script` с полем `class`.

#### `[B] CMakeLists.txt` (корень)
- `find_program(SKY_DOTNET dotnet)`; `add_custom_command` собирает `SkyEngine.TestScripts` в `SKY_MANAGED_DIR`; `add_custom_target(sky_managed)`; прокинуть `SKY_MANAGED_DIR` в bridge/player/тесты.

## E5 · FBX/glTF, запуск сцены плеером

#### `[H+S] engine/asset/include/sky/asset/{fbx_importer,gltf_importer}.hpp` + `.cpp`
- `class FbxImporter : IAssetImporter`, `class GltfImporter : IAssetImporter`; `mini_json.hpp` для glTF.

#### `[S] player/src/main.cpp` (дополнить)
- `--scene <path>` / позиционный аргумент → `context.openScene(path)` до цикла; `context.beginPlay()` + `context.tickScripts(dt)` в цикле.

#### `[B] .github/workflows/ci.yml` (дополнить)
- шаг «player smoke»: `sky_player --headless player-smoke.png --frames 60` + артефакт.

**Ворота M3:** C#-скрипт на объекте работает и в Play редактора, и в
standalone-плеере (проверка точная: 90°/с × 1 с = 45°).
