# Спринт 1 · День 1 — выдача фич (по одной на контур)

Первый день работы команды. Каждый из шести человек (E1–E6) берёт **первую фичу
своего контура** — фундамент, от которого зависит всё остальное в его зоне.
Одна фича = одна ветка `feature/<название>` = один запрос на слияние.

У методов ниже указаны **сигнатура**, **что делает**, **параметры** и **что
возвращает** — тексты скопированы 1:1 из ролевых ТЗ (`docs/role-E1.md` …
`docs/role-E6.md`), поэтому имена файлов и методов точно совпадают с проектом.

## Кто может стартовать в день 1

| Контур | Фича | Файлы (создаются) | Зависимости на день 1 |
|---|---|---|---|
| **E1** Ядро/данные | `feature/math-and-handles` | `core/math.hpp`, `core/handle.hpp` | нет — стартует первым |
| **E2** Рендеринг | `feature/render-contract` | `rendering/rendering.hpp`, `renderer_registry.hpp`, `null_renderer.*` | нужны `core::Vec3/Transform` из E1 (`RenderCommand`) |
| **E3** Редактор(.NET) | `feature/editor-shell` | `SkyEditor.csproj`, `Program.cs`, `App.axaml.cs`, `MainWindow.axaml.cs` | нет (C-интерфейс нужен только со 2-й фичи) |
| **E4** Рантайм/скриптинг | `feature/physics-world` | `physics/physics.hpp`, `physics_world.{hpp,cpp}` | нужны `core::Vec3/Transform` из E1 (заголовок) |
| **E5** Пайплайн/пакеты | `feature/build-system` | `CMakeLists.txt`, `engine/CMakeLists.txt`, `tests/CMakeLists.txt` | нет — включает модули по мере готовности |
| **E6** Data-oriented(ECS) | `feature/ecs-core` | `ecs/ecs.hpp`, `ecs_world.{hpp,cpp}` | нет |

**Полностью независимы в день 1:** E1, E5, E6. **E3** стартует независимо
(каркас окна), C-интерфейс подключается лишь со второй фичи. **E2 и E4 зависят
от математики E1:** `RenderCommand` (E2) и типы физики (E4) держат `core::Vec3`/
`core::Transform`. Поэтому E1 первым же коммитом отдаёт заголовок
`engine/core/include/sky/core/math.hpp` — по нему E2 и E4 стартуют, не дожидаясь
остальной части фичи E1.

---

## E1 · `feature/math-and-handles`

Математика и типобезопасные идентификаторы — фундамент, от которого зависят все.

### Файл `engine/core/include/sky/core/math.hpp`
Свободные функции над векторами, кватернионами и трансформами (все
`constexpr`). Структуры `Vec3{x,y,z}`, `Quat{x,y,z,w}`, `Transform{position,
rotation, scale}` объявляются здесь же.
- `constexpr Vec3 operator+(const Vec3& a, const Vec3& b)`
  Что делает: покомпонентное сложение векторов.
  Параметры: `a`, `b` — слагаемые. Возвращает: сумму `{a.x+b.x, …}`.
- `constexpr Vec3 operator*(const Vec3& a, float s)`
  Что делает: масштабирование вектора числом.
  Параметры: `a` — вектор, `s` — множитель. Возвращает: `{a.x*s, …}`.
- `constexpr Quat operator*(const Quat& a, const Quat& b)`
  Что делает: композиция двух поворотов (сначала `b`, потом `a`).
  Параметры: `a`, `b` — кватернионы. Возвращает: результирующий поворот.
- `constexpr Vec3 rotate(const Quat& q, const Vec3& v)`
  Что делает: поворачивает вектор кватернионом.
  Параметры: `q` — поворот, `v` — исходный вектор. Возвращает: повёрнутый вектор.
- `constexpr Transform compose(const Transform& parent, const Transform& child)`
  Что делает: переводит локальный трансформ ребёнка в систему координат родителя (нужно для мирового трансформа).
  Параметры: `parent` — трансформ родителя, `child` — локальный трансформ ребёнка. Возвращает: трансформ ребёнка в системе родителя.
- `constexpr Quat conjugate(const Quat& q)`
  Что делает: сопряжённый кватернион (обратный поворот для единичного).
  Параметры: `q` — поворот. Возвращает: `{-q.x,-q.y,-q.z,q.w}`.
- `constexpr Transform invCompose(const Transform& parent, const Transform& world)`
  Что делает: обратная к `compose` — переводит мировой трансформ в локальный относительно родителя (нужно при смене родителя).
  Параметры: `parent` — трансформ родителя, `world` — мировой трансформ объекта. Возвращает: локальный трансформ относительно родителя.

### Файл `engine/core/include/sky/core/handle.hpp`
- `template <typename Tag> struct Handle { std::uint64_t value; … }`
  Что делает: типобезопасный идентификатор. Разные теги (`ObjectTag`,
  `ComponentTag`) дают несовместимые типы — нельзя перепутать хэндл объекта с
  хэндлом компонента. Содержит `isValid()`, статический `invalid()`, `operator==`.

**Проверка фичи:** `rotate(поворот 90° вокруг Y, {0,0,1})` ≈ `{1,0,0}` (±1e-5);
`invCompose(parent, compose(parent, child)) == child`.

---

## E2 · `feature/render-contract`

Общий (не только Vulkan) контракт рендера — поток команд и интерфейс рендерера.

### Файл `engine/rendering/include/sky/rendering/rendering.hpp`
Перечисления и структура команды.
- `enum class RenderCommandType { BeginFrame, SetViewport, SetCamera, AddLight, SetSky, BindPipeline, DrawMesh, EndFrame }`
  Что делает: вид команды отрисовки в потоке.
- `enum class LightType { Directional = 0, Point = 1 }` — вид источника света.
- `struct RenderCommand { RenderCommandType type; core::Transform transform; core::Vec3 color; float fovDegrees; float orthoHeight; … }`
  Что делает: одна backend-независимая команда. Поля используются по-разному в зависимости от `type` (для `SetCamera` — поза камеры и проекция, для `AddLight` — поза и цвет света, для `DrawMesh` — материал и текстуры).

**`class IRenderer`** — контракт рендерера.
- `virtual std::string backendName() const = 0` — Возвращает: имя бэкенда ("vulkan"/"opengl").
- `virtual void attachSurface(IRenderSurface& surface) = 0` — привязывает поверхность вывода. Параметры: `surface`.
- `virtual void submit(std::span<const RenderCommand> commands) = 0` — принимает поток команд кадра. Параметры: `commands`.
- `virtual void renderFrame() = 0` — рисует накопленный кадр.

**`class IRenderResourceFactory`** — создание ресурсов GPU.
- `virtual RenderResourceHandle createMeshFromData(std::span<const float> interleavedPosNormalUv) = 0`
  Что делает: загружает меш из массива вершин (позиция+нормаль+uv). Параметры: `interleavedPosNormalUv`. Возвращает: хэндл ресурса.
- `virtual RenderResourceHandle createTextureFromData(std::uint32_t w, std::uint32_t h, std::span<const std::uint8_t> rgba) = 0`
  Что делает: загружает текстуру. Параметры: `w`, `h` — размеры, `rgba` — пиксели. Возвращает: хэндл.
- `virtual void destroy(RenderResourceHandle resource) = 0` — освобождает ресурс.

### Файл `engine/rendering/include/sky/rendering/renderer_registry.hpp`
`struct BackendInit { … }` — параметры инициализации бэкенда; `RendererFactory` —
тип функции-фабрики рендерера.

**`class IRendererRegistry`** — реестр бэкендов рендера.
- `virtual bool registerBackend(const std::string& name, RendererFactory factory) = 0` — регистрирует фабрику бэкенда. Возвращает: успех.
- `virtual std::unique_ptr<IRenderer> create(const std::string& name, const BackendInit&) = 0` — Возвращает: рендерер по имени бэкенда.
- `std::unique_ptr<IRendererRegistry> createRendererRegistry()` — фабрика реестра.

### Файлы `engine/rendering/include/sky/rendering/null_renderer.hpp`, `engine/rendering/src/{null_renderer,renderer_registry}.cpp`
Пустой рендерер (считает кадры/команды, ничего не рисует) — для тестов контракта, и реализация реестра.

**Проверка фичи:** null-рендерер регистрируется в реестре и создаётся по имени;
`submit` + `renderFrame` увеличивают счётчики кадров/команд.
Зависимость: `core::Vec3`/`core::Transform` из фичи E1 `feature/math-and-handles`
(поля `RenderCommand`).

---

## E3 · `feature/editor-shell`

### Файлы `editor/avalonia/SkyEditor.csproj`, `Program.cs`, `App.axaml.cs`, `MainWindow.axaml.cs`
- `csproj` — проект .NET 8 с пакетами Avalonia.
- `static int Main(string[] args)` — точка входа; ветка `--screenshot` (headless). Возвращает: код выхода.
- `App` — приложение Avalonia; `OnFrameworkInitializationCompleted()` открывает `MainWindow`.
- `MainWindow` — окно «Sky Engine», меню (File/Edit/GameObject), обработчики пунктов.

**Проверка фичи:** `dotnet build editor/avalonia` — 0 ошибок; окно «Sky Engine»
открывается с меню. C-интерфейс на этой фиче не требуется — он подключается со
второй фичи контура (`feature/engine-bridge`).

---

## E4 · `feature/physics-world`

### Файл `engine/physics/include/sky/physics/physics.hpp`
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

### Файлы `engine/physics/include/sky/physics/physics_world.hpp`, `engine/physics/src/physics_world.cpp`
- `class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService` — добавляет:
  - `virtual void setGravity(const core::Vec3& gravity) = 0` — задаёт гравитацию.
  - `virtual void setBodyVelocity(RigidBodyHandle body, const core::Vec3& velocity) = 0` — задаёт скорость тела.
  - `virtual core::Vec3 bodyVelocity(RigidBodyHandle body) const = 0` — Возвращает: скорость тела.
  - `virtual void setBodyTransform(RigidBodyHandle body, const core::Transform& transform) = 0` — переставляет тело.
- `std::unique_ptr<PhysicsWorld> createPhysicsWorld()` — фабрика.
  Внутри `.cpp`: интегрирование гравитации в `step`; `detectAndResolve` (расталкивание AABB, сбор `CollisionEvent`); `sampleHeightfield`/`resolveHeightfields` (удержание на террейне); `raycast` по коллайдерам и высотной поверхности.

**Проверка фичи:** тело за 1 с падает ≈4.9 м под гравитацией; куб замирает на
полу (расталкивание AABB); луч попадает в коллайдер.
Зависимость: `core::Vec3`/`core::Transform` из фичи E1 `feature/math-and-handles`.

---

## E5 · `feature/build-system`

### Файлы `CMakeLists.txt`, `engine/CMakeLists.txt`, `tests/CMakeLists.txt`
- `engine/CMakeLists.txt` — функция `sky_add_module(NAME DIR sources…)` создаёт
  статическую библиотеку с public-include-путями и стандартом C++20; регистрирует
  модули и связи между ними.
- корневой `CMakeLists.txt` — проект, опции, `add_subdirectory` для движка/редактора/плеера/тестов.

**Проверка фичи:** `cmake -S . -B build && cmake --build build` проходит с одним
модулем (напр. `sky_core`); `sky_add_module` подключает следующий модуль одной
строкой. Это скелет сборки, в который остальные контуры добавляют свои модули.

---

## E6 · `feature/ecs-core`

### Файл `engine/ecs/include/sky/ecs/ecs.hpp`
`struct EntityId { std::uint32_t index; std::uint32_t generation; }` — сущность
с поколением (защита от повторного использования id).

**`class IEcsComponentStore`** — хранилище одного типа компонента.
- `virtual std::type_index componentType() const = 0` — Возвращает: тип компонента.
- `virtual bool has(EntityId entity) const = 0` — Возвращает: есть ли компонент у сущности.
- `virtual void remove(EntityId entity) = 0` — удаляет компонент у сущности.
- `virtual std::size_t count() const = 0` — Возвращает: число компонентов.

**`class IEcsSystem`** — система, исполняемая каждый кадр.
- `virtual std::string name() const = 0` — Возвращает: имя системы.
- `virtual void update(double deltaSeconds) = 0` — обрабатывает подходящие сущности. Параметры: `deltaSeconds` — шаг времени.

**`class IEcsWorld`** — мир сущностей.
- `virtual EntityId createEntity() = 0` — Возвращает: новую сущность.
- `virtual void destroyEntity(EntityId entity) = 0` — уничтожает сущность.
- `virtual bool isAlive(EntityId entity) const = 0` — Возвращает: жива ли сущность.
- `virtual IEcsComponentStore& store(std::type_index componentType) = 0` — Возвращает: хранилище типа.

**`class IEcsSystemScheduler`** — планировщик.
- `virtual void registerSystem(IEcsSystem& system) = 0` — регистрирует систему.
- `virtual void unregisterSystem(IEcsSystem& system) = 0` — снимает систему.
- `virtual void tick(double deltaSeconds) = 0` — прогоняет все системы за такт. Параметры: `deltaSeconds`.

**`class IEcsQueryService`** — запросы.
- `virtual std::vector<EntityId> entitiesWith(std::set<std::type_index> types) const = 0`
  Что делает: находит сущности с заданным набором компонентов. Параметры: `types`. Возвращает: список сущностей.

### Файлы `engine/ecs/include/sky/ecs/ecs_world.hpp`, `engine/ecs/src/ecs_world.cpp`
- `class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService` — добавляет:
  - `template <typename T> TypedComponentStore<T>& storeFor()`
    Что делает: типобезопасный доступ к хранилищу компонента T (с методами `set(entity, value)`, `get(entity)→T*`). Возвращает: хранилище T.
- `std::unique_ptr<EcsWorld> createEcsWorld()` — фабрика.

**Проверка фичи:** создание/уничтожение сущности; `storeFor<T>().set/get`
работают; `entitiesWith({type})` возвращает только сущности с этим компонентом.

---

## Итог дня 1

Шесть параллельных веток `feature/*`, каждая — контракт своего слоя:
`core::Vec3/Quat/Transform` (E1), поток команд рендера (E2), окно редактора (E3),
физический мир (E4), скелет сборки (E5), ECS-мир (E6). Точки соприкосновения на
день 1 — заголовок `core/math.hpp` (E1→E2 и E1→E4: `RenderCommand` и типы физики
держат `core::Vec3`/`core::Transform`) и `engine/CMakeLists.txt` (E5 включает
модули по мере их появления). Полные пути следующих фич — в
`docs/role-E1.md` … `docs/role-E6.md`; кросс-срез по неделям — в
`docs/weeks-overview.md`.
