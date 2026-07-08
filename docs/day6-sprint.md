# Спринт 1 · День 6 — выдача фич (по одной на контур)

День 6: каждый контур берёт свою **6-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/scene-world` | Этап 2 (Неделя 2, веха M1). Сцена и сборочная точка |
| **E2** Рендеринг | `feature/editor-camera` | Этап 3 (Недели 3–4, веха M2). Выбор объекта, проекция, небо |
| **E3** Редактор(.NET) | `feature/gizmos` | Этап 3 (Недели 3–4, веха M2). Гизмо, инспектор, панели данных |
| **E4** Рантайм/скриптинг | `feature/dotnet-host` | Этап 4 (Недели 5–6, веха M3). Подсистема скриптинга |
| **E5** Пайплайн/пакеты | `feature/importers-obj-png` | Этап 3 (Недели 3–4, веха M2). Импортёры, виртуальная ФС, тесты интерфейса |
| **E6** Data-oriented(ECS) | `feature/ecs-profiling` | Этап 5 (Недели 7–8 · перспектива). Профилирование систем |

---

## E1 · `feature/scene-world`

*Этап: Этап 2 (Неделя 2, веха M1). Сцена и сборочная точка.*

#### Файлы `engine/scene/include/sky/scene/scene_world.hpp`, `engine/scene/src/scene_world.cpp`
`class SceneWorld` (наследует `ISceneRepository`, `ISceneRuntime`,
`ISceneQueryService`), создаётся через `SceneWorldDeps`.
- `virtual void addRootObject(SceneHandle scene, object::ObjectHandle object) = 0` — добавляет объект в корень сцены.
- `virtual bool saveSceneAs(SceneHandle scene, const std::filesystem::path& path, …) = 0` — сохраняет сцену (полный SKYB — на этапе 3). Возвращает: успех.
- `virtual std::vector<object::ObjectHandle> rootObjectsOf(SceneHandle scene) const = 0` — Возвращает: корневые объекты.
- `std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps)` — фабрика.

---

## E2 · `feature/editor-camera`

*Этап: Этап 3 (Недели 3–4, веха M2). Выбор объекта, проекция, небо.*

#### Файл `editor/shell/src/editor_camera.hpp`
`struct EditorCamera { float yawDegrees; float pitchDegrees; float distance;
core::Vec3 target; float fovDegrees; bool orthographic; … }`.
- `void orbit(float deltaYawDegrees, float deltaPitchDegrees)`
  Что делает: вращает камеру вокруг цели. Параметры: приращения углов в градусах. Возвращает: ничего.
- `void zoom(float factor)` — приближает/отдаляет. Параметры: `factor` — коэффициент. Возвращает: ничего.
- `void pan(float deltaRight, float deltaUp)` — сдвигает цель. Параметры: смещения. Возвращает: ничего.
- `void lookAlong(int axis)` — снапит вид к оси. Параметры: `axis` (0=+X,1=−X,2=+Y,3=−Y,…). Возвращает: ничего.
- `core::Transform pose() const` — Возвращает: мировую позу камеры.
- `float orthoHeight() const` — Возвращает: высоту орто-проекции (0 = перспектива).

Выбор объекта лучом (`pick`) и проекция мира в экран (`project`) реализуются в
мосте контура E5 поверх этой камеры (совместная фича).

---

## E3 · `feature/gizmos`

*Этап: Этап 3 (Недели 3–4, веха M2). Гизмо, инспектор, панели данных.*

#### Дополнение файла `editor/avalonia/Controls/VulkanViewport.cs`
- свойства `public ulong SelectedId`, `public bool LocalSpace`, `public GizmoTool Tool`.
- `DrawMoveGizmo`, `DrawRotateGizmo`, `DrawScaleGizmo`, `DrawSceneGizmo` — рисование манипуляторов поверх кадра.
- `OnPointerPressed/Moved/Released` — драг осей (перемещение/поворот/масштаб через C-интерфейс).

#### Файл `editor/avalonia/MainViewModel.cs`
- `enum GizmoTool { Hand, Move, Rotate, Scale }`; свойство `public GizmoTool Tool` (хоткеи Q/W/E/R).
- `public void CreateCube()`, `public void Undo()`, `public void Redo()`, `DuplicateSelected()`, `DeleteSelected()`.
- свойства трансформа `PositionX/Y/Z`, `RotationX/Y/Z`, `ScaleX/Y/Z` (двусторонние).

---

## E4 · `feature/dotnet-host`

*Этап: Этап 4 (Недели 5–6, веха M3). Подсистема скриптинга.*

#### Файлы `engine/scripting/include/sky/scripting/{scripting_boundary,script_host,dotnet_host}.hpp`, `engine/scripting/src/dotnet_host.cpp`
`enum class ScriptLifecycleEvent { OnCreate, OnStart, OnUpdate, OnFixedUpdate,
OnDestroy }`; `struct AssemblyRef { name; path; }`.

**`class IScriptHost`** — контракт хоста скриптов. Реализуется в `DotNetScriptHostImpl` (анонимный `namespace` в `.cpp`); переносимый слой hostfxr объявлен вручную (POSIX `char_t==char`), обёрнут `#ifdef SKY_HAS_DLOPEN`.
- `virtual bool start() = 0` — запускает среду .NET. Возвращает: успех.
- `virtual bool loadAssembly(const AssemblyRef& assembly) = 0` — загружает сборку. Возвращает: успех.
- `virtual std::uint64_t createInstance(const std::string& managedTypeName) = 0`
  Что делает: создаёт managed-инстанс класса. Параметры: `managedTypeName` — полное имя класса. Возвращает: id инстанса (0 при ошибке).
- `virtual void destroyInstance(std::uint64_t managedInstanceId) = 0` — уничтожает инстанс.
- `virtual bool invokeLifecycle(std::uint64_t id, ScriptLifecycleEvent event, double dt) = 0`
  Что делает: вызывает событие жизненного цикла. Параметры: `id`, `event`, `dt` — шаг времени. Возвращает: успех.

**`class DotNetScriptHost : IScriptHost`** — реализация через hostfxr, добавляет:
- `virtual void installEngineApi(const void* apiTable) = 0` — передаёт managed-стороне таблицу нативных функций (обратный API).
- `virtual void setInstanceObjectId(std::uint64_t id, std::uint64_t objectId) = 0` — связывает инстанс скрипта с объектом.
- `virtual void beginFrame(double totalSeconds, double deltaSeconds) = 0` — публикует время кадра managed-стороне.
- `virtual std::vector<std::string> scriptClassNames() = 0` — Возвращает: имена классов-наследников ScriptComponent.
- `std::unique_ptr<DotNetScriptHost> createDotNetScriptHost(const DotNetHostConfig&)` — фабрика (nullptr, если hostfxr не найден).
  Приватный хелпер `resolve(Fn& slot, const char* methodName)`: `loader_(bootstrapAssembly, "SkyEngine.Bootstrap, SkyEngine.Managed", methodName, kUnmanagedCallersOnly, nullptr, &fn)`, `slot = (Fn)fn`, успех при `rc==0 && slot`.

---

## E5 · `feature/importers-obj-png`

*Этап: Этап 3 (Недели 3–4, веха M2). Импортёры, виртуальная ФС, тесты интерфейса.*

#### Файлы `engine/asset/include/sky/asset/obj_importer.hpp`, `engine/asset/src/obj_importer.cpp`
- `std::unique_ptr<IAssetImporter> createObjImporter(...)` — импортёр OBJ (парсинг v/vn/vt/f).

#### Файлы `engine/asset/include/sky/asset/png_decoder.hpp`, `engine/asset/src/png_decoder.cpp`
`struct ImageData { std::uint32_t width, height; std::vector<std::uint8_t> pixels; }`.
- `std::optional<ImageData> decodePng(const std::vector<std::byte>& bytes)` — декодирует PNG. Возвращает: изображение или `nullopt`.
- `std::vector<std::byte> encodePngRgba(const ImageData& image)` — кодирует RGBA в PNG. Возвращает: байты файла.
- `std::unique_ptr<IAssetImporter> createPngImporter(platform::IFileSystem& fileSystem)` — импортёр PNG.

---

## E6 · `feature/ecs-profiling`

*Этап: Этап 5 (Недели 7–8 · перспектива). Профилирование систем.*

#### Дополнение файла `engine/ecs/src/ecs_world.cpp`
- измерение длительности `update` каждой системы за такт; накопление таймингов.

#### Совместно с E3/E5
- C-интерфейс выдачи таймингов систем; панель профилировщика в редакторе.

---
