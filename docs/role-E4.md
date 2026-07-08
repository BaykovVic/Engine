# Техническое задание · Контур E4 «Рантайм и скриптинг»

**Область ответственности.** Физическая симуляция, режим воспроизведения,
ввод, автономный проигрыватель и подсистема скриптинга (хостинг .NET,
управляемый рантайм). Скриптинг имеет наибольшую длину зависимостей — его
начинают не позднее пятой недели.

Каждый этап (неделя) разбит на **фичи** `feature/<название>` — логические
группы заданий недели. Одна фича = одна ветка в git и один запрос на слияние.
У каждого метода указаны: **сигнатура**, **что делает**, **параметры**, **что
возвращает** и **как реализовать** («Реализация:»).

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

**Порядок реализации.** Крупный `physics_world.cpp` собирается подшагами внутри
этой фичи; после каждого шаг компилируется и закрывается свой пункт теста:
1. **Каркас + хранилище.** Анонимный `namespace` в `.cpp`: записи `BodyRecord{desc, transform, velocity, colliders}`, `ColliderRecord{desc, body}`, `Aabb{min,max}` с `overlaps()`; класс `PhysicsWorldImpl : PhysicsWorld` с полями `nextId_`, `gravity_{0,-9.81,0}`, `bodies_`, `colliders_`, `events_`. Реализуй `createBody/destroyBody/attachCollider/detachCollider`, `bodyTransform`, сеттеры/геттеры и фабрику. Компилируется `sky_physics`; закрывает создание тел.
2. **Гравитация.** Тело `step` без столкновений: интегрирование скорости и позиции для Dynamic-тел. Закрывает пункт «падение ≈4.9 м/с».
3. **Столкновения.** `detectAndResolve` + `resolve` (AABB-расталкивание, сбор `CollisionEvent`); в `step` вызвать после интегрирования. Закрывает «куб на полу».
4. **Террейн.** `sampleHeightfield` + `resolveHeightfields`; вызвать в `step` до `detectAndResolve`. Закрывает «тело на heightfield».
5. **Луч.** `worldAabb`, `rayVsAabb`, `rayVsHeightfield` и `raycast`. Закрывает запросы луча (используется на этапе 5).

#### Файл `engine/physics/include/sky/physics/physics.hpp`
Типы: `RigidBodyDesc{type,mass,transform}`, `enum class ColliderShape{Box,Sphere,
Capsule,TerrainHeightfield}`, `ColliderDesc{shape,halfExtents,radius,heightfield}`,
`HeightfieldDesc{resolution,scale,heights}`, `RaycastHit{collider,point,normal,
distance}`, `CollisionEvent{first,second}`.

**`class IPhysicsWorld`** — управление симуляцией.
- `virtual RigidBodyHandle createBody(const RigidBodyDesc& desc) = 0`
  Что делает: создаёт физическое тело. Параметры: `desc` — тип/масса/поза. Возвращает: хэндл тела.
  Реализация: в `PhysicsWorldImpl` завести `RigidBodyHandle handle{nextId_++}`, положить `bodies_.emplace(handle.value, BodyRecord{desc, desc.initialTransform})` (скорость нулевая, список коллайдеров пуст), вернуть `handle`.
- `virtual void destroyBody(RigidBodyHandle body) = 0` — удаляет тело.
  Реализация: `bodies_.find(body.value)`; если найдено — пробежать `it->second.colliders` и стереть каждый из `colliders_`, затем `bodies_.erase(it)`; если нет — тихо выйти.
- `virtual ColliderHandle attachCollider(RigidBodyHandle body, const ColliderDesc& desc) = 0`
  Что делает: навешивает коллайдер на тело. Параметры: `body`, `desc` — форма коллайдера. Возвращает: хэндл коллайдера.
  Реализация: найти тело в `bodies_`; если нет — вернуть `ColliderHandle::invalid()`. Иначе `ColliderHandle handle{nextId_++}`, `colliders_.emplace(handle.value, ColliderRecord{desc, body})`, добавить хэндл в `it->second.colliders`, вернуть `handle`.
- `virtual void detachCollider(ColliderHandle collider) = 0` — снимает коллайдер.
  Реализация: найти запись в `colliders_`; по её `body` найти `BodyRecord` и `std::erase(bodyIt->second.colliders, collider)`; затем `colliders_.erase(it)`.
- `virtual void step(double fixedDeltaSeconds) = 0` — шаг симуляции. Параметры: `fixedDeltaSeconds` — шаг времени.
  Реализация: `dt = (float)fixedDeltaSeconds`; пробегает `bodies_`, для Dynamic-тел `velocity += gravity_*dt; position += velocity*dt`; затем зовёт `resolveHeightfields()` и `detectAndResolve()` (порядок важен: террейн держит тело, потом AABB-пары расталкиваются).
- `virtual std::vector<CollisionEvent> drainCollisionEvents() = 0` — Возвращает: события столкновений за кадр.
  Реализация: `return std::exchange(events_, {})` — отдать накопленный `events_` и очистить его на текущий кадр.

**`class IPhysicsQueryService`** — запросы.
- `virtual std::optional<RaycastHit> raycast(const core::Vec3& origin, const core::Vec3& direction, float maxDistance) const = 0`
  Что делает: пускает луч в физический мир. Параметры: `origin` — начало, `direction` — направление, `maxDistance` — предел. Возвращает: попадание или `nullopt`.
  Реализация: перебирает все `colliders_`, хранит `best` (ближайшее). Для `TerrainHeightfield` зовёт `rayVsHeightfield(...)`; для остальных строит `worldAabb(...)` и зовёт `rayVsAabb(...)`. При успехе, если ближе `best->distance`, обновляет `best` (`{handle, point/origin+direction*distance, normal, distance}`). Возвращает `best`.
- `virtual core::Transform bodyTransform(RigidBodyHandle body) const = 0` — Возвращает: трансформ тела.
  Реализация: `bodies_.find(body.value)`; вернуть `it->second.transform`, либо `core::Transform{}` если тела нет.

#### Файлы `engine/physics/include/sky/physics/physics_world.hpp`, `engine/physics/src/physics_world.cpp`
- `class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService` — добавляет:
  - `virtual void setGravity(const core::Vec3& gravity) = 0` — задаёт гравитацию.
    Реализация: `gravity_ = gravity` (поле-член, по умолчанию `{0,-9.81,0}`).
  - `virtual void setBodyVelocity(RigidBodyHandle body, const core::Vec3& velocity) = 0` — задаёт скорость тела.
    Реализация: найти `BodyRecord` в `bodies_`, если есть — `it->second.velocity = velocity`.
  - `virtual core::Vec3 bodyVelocity(RigidBodyHandle body) const = 0` — Возвращает: скорость тела.
    Реализация: `bodies_.find(body.value)`; вернуть `it->second.velocity`, иначе `core::Vec3{}`.
  - `virtual void setBodyTransform(RigidBodyHandle body, const core::Transform& transform) = 0` — переставляет тело.
    Реализация: найти `BodyRecord`, если есть — `it->second.transform = transform` (скорость не трогается).
- `std::unique_ptr<PhysicsWorld> createPhysicsWorld()` — фабрика.
  Реализация: `return std::make_unique<PhysicsWorldImpl>()` (реализация живёт в анонимном `namespace` в `.cpp`).

**Приватные хелперы `PhysicsWorldImpl` внутри `.cpp`** (это тело `createPhysicsWorld`):
- `Aabb worldAabb(ColliderHandle, const ColliderRecord& collider) const`
  Реализация: взять центр = позиция `BodyRecord` коллайдера (или `{}`), `extents = collider.desc.halfExtents`; для `Sphere`/`Capsule` заменить на `{radius,radius,radius}`; вернуть `Aabb{center-extents, center+extents}`.
- `static float sampleHeightfield(const HeightfieldDesc& field, float x, float z)`
  Реализация: билинейная выборка высоты. При `resolution<2` или недостатке `heights` вернуть 0. Перевести `(x,z)` в сетку через `x/scale.x`, `z/scale.z` с `std::clamp` в `[0,resolution-1]`, взять четыре узла `heights[sz*resolution+sx]`, интерполировать по дробным частям `fx,fz` и домножить на `scale.y`.
- `void resolveHeightfields()`
  Реализация: держит Dynamic-тела над каждым `TerrainHeightfield`-коллайдером. Для каждого heightfield-коллайдера (с его `fieldOrigin`) и каждого Dynamic-тела: взять `halfHeight` (радиус первого коллайдера или `halfExtents.y`, иначе 0.5), посчитать `ground = fieldOrigin.y + sampleHeightfield(...localX,localZ...)`; если `position.y - halfHeight < ground` — поднять `position.y = ground + halfHeight`, обнулить `velocity.y` при падении вниз и добавить `CollisionEvent` в `events_`.
- `bool rayVsHeightfield(ColliderHandle, const ColliderRecord&, origin, direction, maxDistance, float& distance, Vec3& point, Vec3& normal) const`
  Реализация: марш фиксированным шагом до первой точки под поверхностью, затем бисекция. `heightAt(p)=fieldOrigin.y+sampleHeightfield(...)`, `above(t)` сравнивает `p.y` с `heightAt(p)`. Если старт уже под поверхностью — `false`. Шаг `step=clamp(maxDistance/256, 0.05, 0.5)`; при пересечении между `previous` и `t` 16 итераций бисекции дают `distance`, `point` (с `y=heightAt`), а нормаль — из центральных разностей высоты (градиент) с последующей нормировкой.
- `void detectAndResolve()`
  Реализация: собрать `boxes` (id + `worldAabb`) по всем коллайдерам, пропуская `TerrainHeightfield`. Двойным циклом по парам: пропустить пары одного тела и непересекающиеся `overlaps`; иначе добавить `CollisionEvent` в `events_`, вызвать `resolve(...)` и обновить `boxes[i]/[j]` (тело могло сдвинуться).
- `void resolve(const ColliderRecord& a, const Aabb& boxA, const ColliderRecord& b, const Aabb& boxB)`
  Реализация: позиционно расталкивается только пара Dynamic-vs-(Static|Kinematic) (иначе выход — только событие). Посчитать проникновения `penX/penY/penZ`, выбрать ось наименьшего проникновения, сдвинуть `dynamicBody->transform.position` по ней со знаком (по сравнению центров боксов) и обнулить соответствующую компоненту `velocity`.
- `static bool rayVsAabb(origin, direction, const Aabb& box, maxDistance, float& outDistance, Vec3& outNormal)`
  Реализация: слэб-метод по трём осям. Вести `tMin=0`, `tMax=maxDistance`; для каждой оси при почти нулевом направлении проверить попадание в диапазон, иначе посчитать `t1,t2` (с перестановкой и знаком нормали), поднять `tMin` (запомнив нормаль), опустить `tMax`; при `tMin>tMax` — `false`. Записать `outDistance=tMin`, `outNormal`.

### feature/physics-object-sync

#### Дополнение файла `engine/physics/include/sky/physics/physics_world.hpp` + реализация
- `class ObjectPhysicsSync : IPhysicsSyncContract`
  - `virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0` — связывает тело с объектом.
    Реализация: в `ObjectPhysicsSyncImpl` — `bindings_[body.value] = object` (map id тела → `ObjectHandle`).
  - `virtual void unbind(RigidBodyHandle body) = 0` — разрывает связь.
    Реализация: `bindings_.erase(body.value)`.
  - `virtual void pushKinematicState() = 0` — до шага переносит трансформы объектов в тела.
    Реализация: пробежать `bindings_`, для каждой пары `physics_.setBodyTransform(RigidBodyHandle{bodyId}, hierarchy_.worldTransform(object))` — авторская правка объекта переносится в тело перед `step`.
  - `virtual void pullSimulationResults() = 0` — после шага переносит результат обратно.
    Реализация: пробежать `bindings_`, для каждой пары `hierarchy_.setLocalTransform(object, physics_.bodyTransform(RigidBodyHandle{bodyId}))` — итог симуляции переносится в объект.
- `std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)` — фабрика.
  Реализация: `return std::make_unique<ObjectPhysicsSyncImpl>(physics, hierarchy)`; конструктор запоминает ссылки `physics_`, `hierarchy_`.

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
  Реализация: цикл по `argv` со `std::strcmp` наполняет `frames`, `headless`+`headlessScreenshot`, `scenePath` (плюс позиционный аргумент — путь сцены). Создать `EditorContext context`; если задан `scenePath` — `context.openScene(...)` (при неудаче печать в `stderr` и `return 1`). Диспетчеризовать `runHeadless(context, frames?:120, screenshot)` или `runWindowed(context, frames)`.
- `int runHeadless(EditorContext& context, int frames, const char* screenshotPath)`
  Что делает: безоконный прогон — цикл `tickFrame → build → renderFrame`, сохранение PNG. Параметры: `context`, `frames` — число кадров, `screenshotPath` — файл. Возвращает: код выхода.
  Реализация: `createVulkanRenderer(kWidth,kHeight)` (при nullptr — ошибка, `return 1`), `FrameBuilder builder(context, *renderer)`. `playMode->setScene(activeScene); playMode->play(); context.beginPlay()`. Цикл `frames` раз: `playMode->tickFrame(1.0/60)`, `context.tickScripts(1.0/60)`, `renderer->submit(builder.build(...))`, `renderer->renderFrame()`. Если задан путь — `readbackFrame()`, собрать `ImageData`, `encodePngRgba`, `fileSystem->writeAll(path, png)`.
- `int runWindowed(EditorContext& context, int frameLimit)`
  Что делает: оконный прогон (окно X11/Cocoa + swapchain-рендерер). Параметры: `context`, `frameLimit`. Возвращает: код выхода.
  Реализация: создать оконную систему (`createCocoaWindowSystem`/`createX11WindowSystem`), окно `960×540`, заполнить `VulkanPresentTarget` (metalLayer или x11Display/Window), `createVulkanRendererForWindow(...)`. Поставить `setEventCallback`: ESC (keysym `0xff1b`) → `running=false`; на KeyDown/KeyUp через `mapPlatformKey` вызвать `context.setKeyDown(key, down)`. `setScene/play/beginPlay`. Цикл `pumpEvents`: измерить `dt` через `steady_clock`, ограничить `step=min(dt,0.1)`, `tickFrame(step)`, `tickScripts(step)`, `submit(build)`, `renderFrame()`, выйти по `frameLimit`.
- `int mapPlatformKey(std::int32_t keysym)` — переводит платформенный код клавиши в переносимый (общий с C#). Возвращает: переносимый код или 0.
  Реализация: `a..z` → ASCII-верхний регистр (`keysym-'a'+'A'`); `A..Z`, `0..9`, пробел — как есть; `switch` по X11-keysym для именованных клавиш (`0xff1b→256` Escape, `0xff0d→257` Enter, `0xff09→258` Tab, Shift/Ctrl/Alt/стрелки до 265); прочее — `0`.

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
    Реализация: в `PlayModeControllerImpl` — если состояние не `Editing`, сначала `stop()` (смена сцены посреди Play завершает сессию); затем `scene_ = scene`.
  - `virtual bool play() = 0` — запускает. Возвращает: успех (false = сцена не задана).
    Реализация: если `!scene_.isValid()` или уже `Playing` — `false`; если было `Editing` — `sceneRuntime_.activate(scene_)`; `transition(PlayModeState::Playing)`; `true`.
  - `virtual void pause() = 0` / `virtual void stop() = 0` — пауза/остановка.
    Реализация: `pause` — только из `Playing`, иначе `false`; `transition(Paused)`. `stop` — из `Editing` вернуть `false`, иначе `sceneRuntime_.deactivate(scene_)` и `transition(Editing)`.
  - `virtual void tickFrame(double deltaSeconds) = 0` — продвигает симуляцию на кадр.
    Реализация: только в `Playing` — `sceneRuntime_.tick(deltaSeconds)`; в паузе/редактировании ничего не делает.
  - `virtual PlayModeState state() const = 0` — Возвращает: текущее состояние.
    Реализация: `return state_`. Приватный `transition(next)` пишет `state_=next` и оповещает подписчиков `callbacks_`; фабрика `createPlayModeController` отдаёт `make_unique<PlayModeControllerImpl>(sceneRuntime)`.

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `void beginPlay()` — снимает локальные трансформы всех объектов (для восстановления).
  Реализация: очистить `playSnapshot_`; обойти дерево от `roots_` стеком, для каждого объекта `playSnapshot_[object.value] = objects->localTransform(object)` и положить детей `objects->childrenOf`. Затем: если `newestUserScriptStamp(scriptSourceDirs()) != userScriptsStamp_` — `reloadUserScripts()`; вызвать `startPlayScripts()`.
- `void endPlay()` — восстанавливает снимок и пересаживает тела с нулевой скоростью.
  Реализация: `stopPlayScripts()`; пройти `playSnapshot_` и вернуть `objects->setLocalTransform(object, transform)` для существующих; затем пройти `bodies_` — для живых объектов `physics->setBodyTransform(body, objects->worldTransform(object))` и `physics->setBodyVelocity(body, {0,0,0})`; очистить `playSnapshot_`.

### feature/input-state

#### Дополнение файла `editor/shell/src/editor_context.hpp` (методы объявлены inline)
- `void setKeyDown(int key, bool down)` — заносит/снимает клавишу. Параметры: `key` — переносимый код, `down` — нажата ли.
  Реализация: inline — при `down` `keysDown_.insert(key)`, иначе `keysDown_.erase(key)` (`keysDown_` — `std::unordered_set<int>`).
- `bool keyDown(int key) const` — Возвращает: нажата ли клавиша (читается скриптами через `Input`).
  Реализация: inline — `return keysDown_.contains(key)`.

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

**`class IScriptHost`** — контракт хоста скриптов. Реализуется в `DotNetScriptHostImpl` (анонимный `namespace` в `.cpp`); переносимый слой hostfxr объявлен вручную (POSIX `char_t==char`), обёрнут `#ifdef SKY_HAS_DLOPEN`.
- `virtual bool start() = 0` — запускает среду .NET. Возвращает: успех.
  Реализация: если уже `started_` — `true`. `dlopen(config_.hostfxrPath)`; через `dlsym` взять `hostfxr_initialize_for_runtime_config`, `hostfxr_get_runtime_delegate`, `hostfxr_close`. Построить путь `*.runtimeconfig.json` из `bootstrapAssembly`, вызвать `initialize(...)` (коды 0..2 — успех), получить делегат `kHdtLoadAssemblyAndGetFunctionPointer(=5)` в `loader_`. Через хелпер `resolve(...)` привязать managed-точки `LoadAssembly/CreateInstance/DestroyInstance/InvokeLifecycle/GetProbe` (обязательные) и обратные `Initialize/SetObjectId/TickFrame/GetScriptClasses/GetScriptFields/SetScriptField/LoadUserAssembly/UnloadUserAssembly`; выставить `started_=true`.
- `virtual bool loadAssembly(const AssemblyRef& assembly) = 0` — загружает сборку. Возвращает: успех.
  Реализация: при `started_` вызвать `managedLoadAssembly_(assembly.path.string().c_str())`; при успехе `assemblies_.push_back(assembly)` и `true`.
- `virtual std::uint64_t createInstance(const std::string& managedTypeName) = 0`
  Что делает: создаёт managed-инстанс класса. Параметры: `managedTypeName` — полное имя класса. Возвращает: id инстанса (0 при ошибке).
  Реализация: `return started_ ? managedCreateInstance_(managedTypeName.c_str()) : 0` — вызов `[UnmanagedCallersOnly]`-точки `CreateInstance`.
- `virtual void destroyInstance(std::uint64_t managedInstanceId) = 0` — уничтожает инстанс.
  Реализация: при `started_` — `managedDestroyInstance_(managedInstanceId)`.
- `virtual bool invokeLifecycle(std::uint64_t id, ScriptLifecycleEvent event, double dt) = 0`
  Что делает: вызывает событие жизненного цикла. Параметры: `id`, `event`, `dt` — шаг времени. Возвращает: успех.
  Реализация: `return started_ && managedInvokeLifecycle_(id, (int)event, dt) != 0` — `event` кастуется в `std::int32_t`, порядок совпадает с managed `switch`.

**`class DotNetScriptHost : IScriptHost`** — реализация через hostfxr, добавляет:
- `virtual void installEngineApi(const void* apiTable) = 0` — передаёт managed-стороне таблицу нативных функций (обратный API).
  Реализация: при `started_ && managedInitialize_` — `managedInitialize_(const_cast<void*>(apiTable))` (managed `Bootstrap.Initialize → Engine.Install`).
- `virtual void setInstanceObjectId(std::uint64_t id, std::uint64_t objectId) = 0` — связывает инстанс скрипта с объектом.
  Реализация: при `started_ && managedSetObjectId_` — `managedSetObjectId_(id, objectId)` (managed кладёт в `script.Handle`).
- `virtual void beginFrame(double totalSeconds, double deltaSeconds) = 0` — публикует время кадра managed-стороне.
  Реализация: при `started_ && managedTickFrame_` — `managedTickFrame_(totalSeconds, deltaSeconds)` (managed пишет `Time.TotalTime/DeltaTime`).
- `virtual std::vector<std::string> scriptClassNames() = 0` — Возвращает: имена классов-наследников ScriptComponent.
  Реализация: выделить буфер `std::string(8192,'\0')`, вызвать `managedGetScriptClasses_(buffer.data(), size)`, обрезать по возвращённой длине и разбить хелпером `splitLines` (`'\n'`-разделитель) в вектор имён.
- `std::unique_ptr<DotNetScriptHost> createDotNetScriptHost(const DotNetHostConfig&)` — фабрика (nullptr, если hostfxr не найден).
  Реализация: собрать `make_unique<DotNetScriptHostImpl>(config)` (конструктор при пустом `hostfxrPath` зовёт `discoverHostfxr()` — ищет `libhostfxr.so` под `DOTNET_ROOT`/`/usr/lib/dotnet`/`/usr/share/dotnet`, берёт старшую версию); вернуть хост, если `available()` (путь найден), иначе `nullptr`. На не-POSIX платформе — заглушка `return nullptr`.
  Приватный хелпер `resolve(Fn& slot, const char* methodName)`: `loader_(bootstrapAssembly, "SkyEngine.Bootstrap, SkyEngine.Managed", methodName, kUnmanagedCallersOnly, nullptr, &fn)`, `slot = (Fn)fn`, успех при `rc==0 && slot`.

### feature/managed-runtime

#### Файлы `managed/SkyEngine.Managed/*.cs`
- `Bootstrap.cs` — `[UnmanagedCallersOnly]` точки входа, вызываемые из C++:
  `LoadAssembly`, `CreateInstance`, `DestroyInstance`, `InvokeLifecycle`,
  `Initialize` (ставит обратный API), `SetObjectId`, `TickFrame`.
  Реализация: статический класс с `Dictionary<ulong,ScriptComponent> Instances`, `List<Assembly> LoadedAssemblies`, счётчик `_nextId`. `LoadAssembly` — `Marshal.PtrToStringUTF8`, грузит в тот же ALC, что и `Bootstrap` (`context.LoadFromAssemblyPath`), возвращает 1/0. `CreateInstance` — `ResolveType`, проверка наследования от `ScriptComponent`, `Activator.CreateInstance`, назначить `Handle=new NativeHandle(_nextId++)`, положить в `Instances`. `DestroyInstance` — `Instances.Remove(id)`. `InvokeLifecycle` — по индексу `switch` (0→OnCreate…4→OnDestroy) в `try/catch` (managed-сбой изолирован, `return 0`). `Initialize` — `Engine.Install(apiPtr)`. `SetObjectId` — `Instances[id].Handle=new NativeHandle(objectId)`. `TickFrame` — пишет `Time.TotalTime/DeltaTime`. Всё в `try/catch`, id — непрозрачные.
- `ScriptComponent.cs` — базовый класс скрипта (аналог MonoBehaviour):
  `OnCreate/OnStart/OnUpdate/OnFixedUpdate/OnDestroy`, защищённые
  `SetLocalPosition/SetLocalEuler/SetLocalScale`, свойство `Handle`.
  Реализация: `abstract class` со свойством `Handle {get; internal set;}` (стартует `NativeHandle.Invalid`). Жизненные методы — пустые `virtual`, переопределяются скриптом. Защищённые сеттеры дёргают делегаты `Engine`: `SetLocalPosition → Engine.SetLocalPosition?.Invoke(Handle.Value,x,y,z)` и аналогично Euler/Scale; геттеры (`GetLocalPosition`, `GetWorldPosition`) вызывают `GetVec3Fn` с `out`-параметрами и возвращают кортеж.
- `NativeHandle.cs` — обёртка над id объекта.
  Реализация: `readonly record struct NativeHandle(ulong Value)` со статическим `Invalid=new(0)` и `IsValid => Value!=0`. Managed-сторона не владеет нативным состоянием — только ключ.
- `Engine.cs` — таблица делегатов обратного API (нативные функции движка).
  Реализация: статический класс с `[UnmanagedFunctionPointer(Cdecl)]`-делегатами (`SetVec3Fn`, `GetVec3Fn`, `LogFn`, `IsKeyDownFn`, …) и полями-делегатами. `Install(IntPtr)` — `Marshal.PtrToStructure<Api>` (последовательная раскладка, совпадает с нативным `SkyScriptApi`), каждый ненулевой указатель превращается в делегат через `Marshal.GetDelegateForFunctionPointer<...>`.
- `Debug.cs` — `Debug.Log/LogWarning/LogError` в консоль редактора.
  Реализация: `Log/LogWarning/LogError` зовут приватный `Write(level, message)` с уровнями 2/3/4; `Write` в `try/catch` вызывает `Engine.Log?.Invoke(level, message?.ToString() ?? "null")` (логирование не должно ронять скрипт).
- `Time.cs` — `Time.TotalTime/DeltaTime`. `Input.cs` — `Input.GetKey(KeyCode)`.
  Реализация: `Time` — статические свойства `TotalTime/DeltaTime` с `internal set` (пишет `Bootstrap.TickFrame`). `Input.cs` — `enum KeyCode` (буквы/цифры=ASCII, именованные с 256) и `Input.GetKey(KeyCode key) => (Engine.IsKeyDown?.Invoke((int)key) ?? 0) != 0`.

### feature/scripting-integration

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `void initScripting()` — создаёт хост, загружает сборку, ставит обратный API (`installEngineApi`).
  Реализация: поставить дефолтный `scriptLog` (печать в stdout/stderr). Под `#ifdef SKY_MANAGED_DIR`: `DotNetHostConfig{bootstrapAssembly = SKY_MANAGED_DIR/"SkyEngine.Managed.dll"}`, `scriptHost = createDotNetScriptHost(config)`; если `nullptr` или `!start()` — `scriptHost.reset()` и выход (среды .NET нет). Иначе `loadAssembly("SkyEngine.TestScripts")`, выставить файловые указатели `g_scriptObjects=objects.get()`, `g_scriptContext=this`, `installEngineApi(&g_scriptApi)`, затем `reloadUserScripts()`.
- `void startPlayScripts()` — создаёт инстансы для всех `sky.script`, вызывает OnCreate/OnStart.
  Реализация: очистить `playScripts_`, `playTime_=0`; при `scriptHost==nullptr` выход. Переустановить `g_scriptObjects/g_scriptContext` на этот контекст; для каждого `roots_` вызвать `startScriptsFor(root)` — тот обходит поддерево, у компонентов `sky.script` читает поле `class`, `createInstance(className)`, `setInstanceObjectId`, прогоняет авторские поля через `setInstanceField`, вызывает `OnCreate`+`OnStart` и добавляет `(mid, objectId)` в `playScripts_`.
- `void tickScripts(double deltaSeconds)` — вызывает OnUpdate каждый кадр.
  Реализация: при `scriptHost==nullptr` выход. `playTime_+=deltaSeconds`, `scriptHost->beginFrame(playTime_, deltaSeconds)`. Пройти `playScripts_` по индексу до снимка размера `liveCount` (скрипт мог заспавнить новые), для живых объектов `invokeLifecycle(mid, OnUpdate, dt)`. Затем «подмести» инстансы, чей объект умер: `OnDestroy`, `destroyInstance`, `erase`.
- `void stopPlayScripts()` — OnDestroy + уничтожение инстансов.
  Реализация: при живом `scriptHost` пройти `playScripts_` и на каждом вызвать `invokeLifecycle(mid, OnDestroy, 0)` + `destroyInstance(mid)`; очистить `playScripts_`.
Плюс файловые колбэки `scriptSetLocal*`, `scriptLogMessage`, `scriptIsKeyDown` и
структура `SkyScriptApi` (таблица нативных указателей).
  Реализация колбэков (файловые функции в анонимном `namespace`, через `g_scriptObjects`/`g_scriptContext`): `scriptSetLocalPosition/Scale` читают `localTransform`, меняют `position`/`scale`, пишут `setLocalTransform`; `scriptSetLocalEuler` собирает кватернион из углов (лямбда `axisAngle`) и ставит `rotation`; `scriptLogMessage(level,msg)` → `g_scriptContext->scriptLog(level,msg)`; `scriptIsKeyDown(key)` → `g_scriptContext->keyDown(key) ? 1 : 0`. `struct SkyScriptApi` — 12 `void*` в порядке, совпадающем с managed `Engine.Api`; глобальный `g_scriptApi` инициализируется адресами колбэков и передаётся в `installEngineApi`.

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
  Реализация: под `#ifdef SKY_MANAGED_DIR` (иначе `false`). Пересчитать `scriptSourceDirs()`, посчитать `*.cs` по каждой папке; если источников 0 — `scriptHost->unloadUserAssembly()`, сбросить `userScriptsStamp_`, `false`. Иначе создать `Library/ScriptBuild`, записать сгенерированный `SkyProject.Scripts.csproj` (net8.0, `Compile Include` по каждой папке-источнику, `Reference` на `SkyEngine.Managed.dll`), запустить `std::system("dotnet build ... -c Release -o out ...")`; при ненулевом коде — залогировать ошибку, `false`. При успехе `scriptHost->loadUserAssembly(out/"SkyProject.Scripts.dll")`, обновить `userScriptsStamp_=newestUserScriptStamp(dirs)`, залогировать и вернуть результат загрузки.
- `std::vector<std::filesystem::path> scriptSourceDirs() const` — Возвращает: каталоги-источники скриптов.
  Реализация: начать с `assetsRoot/"Scripts"`; для каждого активного (`activePackages_`) кодового пакета добавить `packages->manifest(handle).rootPath/"Runtime"`; вернуть вектор.

#### Дополнение файла `managed/SkyEngine.Managed/Bootstrap.cs`
- `[UnmanagedCallersOnly] int LoadUserAssembly(IntPtr path)` — загружает пользовательскую сборку в collectible ALC. Возвращает: 1/0.
  Реализация: `Marshal.PtrToStringUTF8` + `File.Exists`; создать `new AssemblyLoadContext("SkyUserScripts", isCollectible:true)` с `Resolving`, который дозагружает движковые сборки из ALC `Bootstrap`; прочитать файл в `MemoryStream` и `context.LoadFromStream` (файл не блокируется). Прежний `_userContext?.Unload()`, запомнить `_userContext/_userAssembly`, вернуть 1; всё в `try/catch → 0`.
- `[UnmanagedCallersOnly] void UnloadUserAssembly()` — выгружает её (когда исходников не осталось).
  Реализация: `_userContext?.Unload(); _userContext=null; _userAssembly=null` (collectible ALC уходит, `ResolveType` перестаёт видеть пользовательские типы).

### feature/gameplay-api

#### Дополнение файлов `managed/SkyEngine.Managed/{Engine,ScriptComponent,Physics}.cs`
- таблица обратного API растёт до 12 указателей: `GetWorldPosition`,
  `Instantiate`, `DestroyObject`, `SetVelocity`, `GetVelocity`, `Raycast`.
  Реализация: в `Engine.cs` добавить делегаты `InstantiateFn`/`DestroyFn`/`RaycastFn` (плюс существующие `SetVec3Fn`/`GetVec3Fn`) и поля; расширить `struct Api` и тело `Install` до 12 полей строго в порядке нативного `SkyScriptApi` (`GetWorldPosition/Instantiate/DestroyObject/SetVelocity/GetVelocity/Raycast` в хвосте), каждое ненулевое — `GetDelegateForFunctionPointer`.
- `ScriptComponent`: защищённые `Instantiate(prefab, x,y,z)`, `Destroy()`,
  `SetVelocity(x,y,z)`, `GetWorldPosition()` (в т.ч. по id чужого объекта).
  Реализация: тонкие обёртки над `Engine`-делегатами по `Handle.Value` (либо по переданному `objectId` в статических перегрузках): `Instantiate → Engine.Instantiate?.Invoke(prefabPath,x,y,z) ?? 0`; `Destroy()/Destroy(id) → Engine.DestroyObject?.Invoke(id)`; `SetVelocity → Engine.SetVelocity?.Invoke(...)`; `GetVelocity/GetWorldPosition` вызывают `GetVec3Fn` с `out` и возвращают кортеж.
- `Physics.cs`: `Physics.Raycast(...) → RaycastHit`.
  Реализация: `struct RaycastHit{X,Y,Z, NormalX/Y/Z, Distance}`; `Physics.Raycast(origin, dir, maxDistance, out hit)` — при `Engine.Raycast==null` вернуть `false`, иначе вызвать делегат с 7 `out float`, вернуть `result != 0`.

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- нативные колбэки `scriptGetWorldPosition/Instantiate/DestroyObject/SetVelocity/
  GetVelocity/Raycast`; raycast физики по heightfield (марш + бисекция + нормаль).
  Реализация колбэков (через `g_scriptContext`/`g_scriptObjects`, добавляются в `g_scriptApi`): `scriptGetWorldPosition` → `worldTransform(handle).position` в `out`-указатели; `scriptInstantiate(path,x,y,z)` → `g_scriptContext->spawnPrefabAt(path,{x,y,z}).value` (0 при ошибке); `scriptDestroyObject` → если объект существует, `destroyObject(handle)`; `scriptSetVelocity/scriptGetVelocity` → `setObjectVelocity`/`objectVelocity`; `scriptRaycast(...)` → `g_scriptContext->physics->raycast(origin,dir,maxDistance)`, при попадании разложить `point/normal/distance` в `out` и вернуть 1, иначе 0. Сам raycast по террейну реализован в `PhysicsWorldImpl::rayVsHeightfield` (см. feature/physics-world): марш фиксированным шагом, 16-шаговая бисекция и нормаль из градиента высоты.

**На выходе:** скрипты из Assets/Scripts компилируются и работают в Play; доступны Instantiate/Destroy/velocity/raycast.
**Критерий правильности этапа:** правка `.cs` подхватывается при следующем Play;
скрипт спавнит/уничтожает объекты и читает физический луч.
