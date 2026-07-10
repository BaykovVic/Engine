# Техническое задание · Контур E4 «Рантайм и скриптинг»

**Область ответственности.** Физическая симуляция, режим воспроизведения,
ввод, автономный проигрыватель и подсистема скриптинга (хостинг .NET,
управляемый рантайм). Скриптинг имеет наибольшую длину зависимостей — его
начинают не позднее пятой недели.

Каждый этап (неделя) разбит на **фичи** `feature/<название>` — логические
группы заданий недели. Одна фича = одна ветка в git и один запрос на слияние.
У каждого метода указаны: **сигнатура**, **что делает**, **параметры** и **что
возвращает**.

## Обозначения (C++ и .NET)

- `virtual … = 0` — чисто виртуальный метод (контракт).
- `std::unique_ptr<T>` — владеющий указатель; `std::optional<T>` — значение или «ничего»; `std::vector<T>` — динамический массив.
- **hostfxr** — библиотека, запускающая среду .NET внутри нативного процесса.
- **UnmanagedCallersOnly** — атрибут C#-метода, который можно вызвать напрямую из C (без обёрток).
- **AssemblyLoadContext (ALC)** — контекст загрузки .NET-сборки; «collectible» ALC можно выгрузить, заменив код между запусками.
- **Managed / native** — управляемая (C#) и нативная (C++) стороны; общаются через таблицу указателей на функции.

---

## Этап 1 (Неделя 1). Физическая симуляция

**Общее описание задач этапа.** Физический мир: тела, коллайдеры, гравитация,
столкновения, высотная поверхность, луч и синхронизация с объектным миром.
Две фичи.

### feature/physics-world

#### Файл `engine/physics/include/sky/physics/physics.hpp`
Типы: `RigidBodyDesc{type,mass,transform}`, `enum class ColliderShape{Box,Sphere,
Capsule,TerrainHeightfield}`, `ColliderDesc{shape,halfExtents,radius,heightfield}`,
`HeightfieldDesc{resolution,scale,heights}`, `RaycastHit{collider,point,normal,
distance}`, `CollisionEvent{first,second}`.

**`class IPhysicsWorld`** — управление симуляцией.
- `virtual RigidBodyHandle createBody(const RigidBodyDesc& desc) = 0`
  Что делает: создаёт физическое тело. Параметры: `desc` — тип/масса/поза. Возвращает: хэндл тела.
- `virtual void destroyBody(RigidBodyHandle body) = 0` — удаляет тело.
- `virtual ColliderHandle attachCollider(RigidBodyHandle body, const ColliderDesc& desc) = 0`
  Что делает: навешивает коллайдер на тело. Параметры: `body`, `desc` — форма коллайдера. Возвращает: хэндл коллайдера.
- `virtual void step(double fixedDeltaSeconds) = 0` — шаг симуляции. Параметры: `fixedDeltaSeconds` — шаг времени.
- `virtual std::vector<CollisionEvent> drainCollisionEvents() = 0` — Возвращает: события столкновений за кадр.

**`class IPhysicsQueryService`** — запросы.
- `virtual std::optional<RaycastHit> raycast(const core::Vec3& origin, const core::Vec3& direction, float maxDistance) const = 0`
  Что делает: пускает луч в физический мир. Параметры: `origin` — начало, `direction` — направление, `maxDistance` — предел. Возвращает: попадание или `nullopt`.
- `virtual core::Transform bodyTransform(RigidBodyHandle body) const = 0` — Возвращает: трансформ тела.

#### Файлы `engine/physics/include/sky/physics/physics_world.hpp`, `engine/physics/src/physics_world.cpp`
- `class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService` — добавляет:
  - `virtual void setGravity(const core::Vec3& gravity) = 0` — задаёт гравитацию.
  - `virtual void setBodyVelocity(RigidBodyHandle body, const core::Vec3& velocity) = 0` — задаёт скорость тела.
  - `virtual core::Vec3 bodyVelocity(RigidBodyHandle body) const = 0` — Возвращает: скорость тела.
  - `virtual void setBodyTransform(RigidBodyHandle body, const core::Transform& transform) = 0` — переставляет тело.
- `std::unique_ptr<PhysicsWorld> createPhysicsWorld()` — фабрика.
  Внутри `.cpp`: интегрирование гравитации в `step`; `detectAndResolve` (расталкивание AABB, сбор `CollisionEvent`); `sampleHeightfield`/`resolveHeightfields` (удержание на террейне); `raycast` по коллайдерам и высотной поверхности.

### feature/physics-object-sync

#### Дополнение файла `engine/physics/include/sky/physics/physics_world.hpp` + реализация
- `class ObjectPhysicsSync : IPhysicsSyncContract`
  - `virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0` — связывает тело с объектом.
  - `virtual void unbind(RigidBodyHandle body) = 0` — разрывает связь.
  - `virtual void pushKinematicState() = 0` — до шага переносит трансформы объектов в тела.
  - `virtual void pullSimulationResults() = 0` — после шага переносит результат обратно.
- `std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)` — фабрика.

#### Файл `tests/physics_tests.cpp`
Падение ≈4.9 м/с, куб на полу, тело на heightfield, синхронизация объекта.

**На выходе (артефакты):** библиотека `sky_physics` собрана; тест `physics_tests` зелёный.
**Критерий правильности этапа:** тело за 1 с падает ≈4.9 м; куб замирает на
полу; тело удерживается на высотной поверхности; привязанный объект синхронно
опускается в объектном мире.

---

## Этап 2 (Неделя 2, веха M1). Автономный проигрыватель

**Общее описание задач этапа.** Проигрыватель с собственным циклом и режимами
запуска. Одна фича.

### feature/player-runtime

#### Файл `player/src/main.cpp`
- `int main(int argc, char** argv)`
  Что делает: точка входа; разбирает `--frames N`, `--headless out.png`, `--scene path`. Возвращает: код выхода.
- `int runHeadless(EditorContext& context, int frames, const char* screenshotPath)`
  Что делает: безоконный прогон — цикл `tickFrame → build → renderFrame`, сохранение PNG. Параметры: `context`, `frames` — число кадров, `screenshotPath` — файл. Возвращает: код выхода.
- `int runWindowed(EditorContext& context, int frameLimit)`
  Что делает: оконный прогон (окно X11/Cocoa + swapchain-рендерер). Параметры: `context`, `frameLimit`. Возвращает: код выхода.
- `int mapPlatformKey(std::int32_t keysym)` — переводит платформенный код клавиши в переносимый (общий с C#). Возвращает: переносимый код или 0.

**На выходе:** бинарь `sky_player`; безоконный режим пишет PNG.
**Критерий правильности этапа:** `sky_player --headless out.png` формирует
изображение кадра.

---

## Этап 3 (Недели 3–4, веха M2). Режим воспроизведения и ввод

**Общее описание задач этапа.** Управление воспроизведением со снимком/
восстановлением сцены и приём ввода. Две фичи.

### feature/play-mode

#### Файлы `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`, `editor/viewport_bridge/src/play_mode_controller.cpp`
`enum class PlayModeState { Editing, Playing, Paused }`.
- `class PlayModeController`:
  - `virtual void setScene(scene::SceneHandle scene) = 0` — задаёт сцену для воспроизведения.
  - `virtual bool play() = 0` — запускает. Возвращает: успех (false = сцена не задана).
  - `virtual void pause() = 0` / `virtual void stop() = 0` — пауза/остановка.
  - `virtual void tickFrame(double deltaSeconds) = 0` — продвигает симуляцию на кадр.
  - `virtual PlayModeState state() const = 0` — Возвращает: текущее состояние.

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `void beginPlay()` — снимает локальные трансформы всех объектов (для восстановления).
- `void endPlay()` — восстанавливает снимок и пересаживает тела с нулевой скоростью.

### feature/input-state

#### Дополнение файла `editor/shell/src/editor_context.hpp` (методы объявлены inline)
- `void setKeyDown(int key, bool down)` — заносит/снимает клавишу. Параметры: `key` — переносимый код, `down` — нажата ли.
- `bool keyDown(int key) const` — Возвращает: нажата ли клавиша (читается скриптами через `Input`).

**На выходе:** вход в play снимает состояние, выход восстанавливает; состояние клавиш доступно движку.
**Критерий правильности этапа:** после `play → stop` сцена в исходном состоянии,
тела без остаточной скорости.

---

## Этап 4 (Недели 5–6, веха M3). Подсистема скриптинга

**Общее описание задач этапа.** Хостинг .NET, базовый класс скрипта, обратный
API движка и интеграция скриптов в цикл воспроизведения. Три фичи.

### feature/dotnet-host

#### Файлы `engine/scripting/include/sky/scripting/{scripting_boundary,script_host,dotnet_host}.hpp`, `engine/scripting/src/dotnet_host.cpp`
`enum class ScriptLifecycleEvent { OnCreate, OnStart, OnUpdate, OnFixedUpdate,
OnDestroy }`; `struct AssemblyRef { name; path; }`.

**`class IScriptHost`** — контракт хоста скриптов.
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

### feature/managed-runtime

#### Файлы `managed/SkyEngine.Managed/*.cs`
- `Bootstrap.cs` — `[UnmanagedCallersOnly]` точки входа, вызываемые из C++:
  `LoadAssembly`, `CreateInstance`, `DestroyInstance`, `InvokeLifecycle`,
  `Initialize` (ставит обратный API), `SetObjectId`, `TickFrame`.
- `ScriptComponent.cs` — базовый класс скрипта (аналог MonoBehaviour):
  `OnCreate/OnStart/OnUpdate/OnFixedUpdate/OnDestroy`, защищённые
  `SetLocalPosition/SetLocalEuler/SetLocalScale`, свойство `Handle`.
- `NativeHandle.cs` — обёртка над id объекта.
- `Engine.cs` — таблица делегатов обратного API (нативные функции движка).
- `Debug.cs` — `Debug.Log/LogWarning/LogError` в консоль редактора.
- `Time.cs` — `Time.TotalTime/DeltaTime`. `Input.cs` — `Input.GetKey(KeyCode)`.

### feature/scripting-integration

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `void initScripting()` — создаёт хост, загружает сборку, ставит обратный API (`installEngineApi`).
- `void startPlayScripts()` — создаёт инстансы для всех `sky.script`, вызывает OnCreate/OnStart.
- `void tickScripts(double deltaSeconds)` — вызывает OnUpdate каждый кадр.
- `void stopPlayScripts()` — OnDestroy + уничтожение инстансов.
Плюс файловые колбэки `scriptSetLocal*`, `scriptLogMessage`, `scriptIsKeyDown` и
структура `SkyScriptApi` (таблица нативных указателей).

#### Файлы `tests/dotnet_host_tests.cpp`, `tests/scripting_rendering_tests.cpp`

**На выходе:** скрипт на C# исполняется в редакторе и в проигрывателе; Debug.Log в консоль; Time/Input доступны.
**Критерий правильности этапа:** скрипт-вращатель даёт 90°/с (за 1 с = 45°) в
Play редактора и в плеере; тест `dotnet_host_tests` зелёный.

---

## Этап 5 (Недели 7–8, веха M4). Пользовательские сборки и игровой интерфейс

**Общее описание задач этапа.** Компиляция пользовательских скриптов и
расширение API движка функциями геймплея. Две фичи.

### feature/user-assemblies

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `bool reloadUserScripts()`
  Что делает: генерирует csproj над `Assets/Scripts/*.cs` (+ Runtime/*.cs пакетов), собирает через `dotnet build`, загружает сборку. Возвращает: успех.
- `std::vector<std::filesystem::path> scriptSourceDirs() const` — Возвращает: каталоги-источники скриптов.

#### Дополнение файла `managed/SkyEngine.Managed/Bootstrap.cs`
- `[UnmanagedCallersOnly] int LoadUserAssembly(IntPtr path)` — загружает пользовательскую сборку в collectible ALC. Возвращает: 1/0.
- `[UnmanagedCallersOnly] void UnloadUserAssembly()` — выгружает её (когда исходников не осталось).

### feature/gameplay-api

#### Дополнение файлов `managed/SkyEngine.Managed/{Engine,ScriptComponent,Physics}.cs`
- таблица обратного API растёт до 12 указателей: `GetWorldPosition`,
  `Instantiate`, `DestroyObject`, `SetVelocity`, `GetVelocity`, `Raycast`.
- `ScriptComponent`: защищённые `Instantiate(prefab, x,y,z)`, `Destroy()`,
  `SetVelocity(x,y,z)`, `GetWorldPosition()` (в т.ч. по id чужого объекта).
- `Physics.cs`: `Physics.Raycast(...) → RaycastHit`.

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- нативные колбэки `scriptGetWorldPosition/Instantiate/DestroyObject/SetVelocity/
  GetVelocity/Raycast`; raycast физики по heightfield (марш + бисекция + нормаль).

**На выходе:** скрипты из Assets/Scripts компилируются и работают в Play; доступны Instantiate/Destroy/velocity/raycast.
**Критерий правильности этапа:** правка `.cs` подхватывается при следующем Play;
скрипт спавнит/уничтожает объекты и читает физический луч.
