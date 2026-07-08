# Спринт 1 · День 1 — выдача фич (по одной на контур)

Первый день работы команды. Каждый из шести человек (E1–E6) берёт **первую фичу
своего контура** — фундамент, от которого зависит всё остальное в его зоне.
Одна фича = одна ветка `feature/<название>` = один запрос на слияние.

У методов ниже указаны **сигнатура**, **что делает**, **параметры**, **что
возвращает** и **Реализация** (что писать внутри тела: алгоритм, какие поля/
структуры трогает, какие хелперы зовёт). Тексты скопированы 1:1 из ролевых ТЗ
(`docs/role-E1.md` … `docs/role-E6.md`) — имена файлов, методов и классов точно
совпадают с проектом, а «Реализация» взята из реального кода.

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
  Реализация: одна строка `return {a.x + b.x, a.y + b.y, a.z + b.z};` — агрегатный литерал `Vec3` из трёх покомпонентных сумм.
- `constexpr Vec3 operator*(const Vec3& a, float s)`
  Что делает: масштабирование вектора числом.
  Параметры: `a` — вектор, `s` — множитель. Возвращает: `{a.x*s, …}`.
  Реализация: `return {a.x * s, a.y * s, a.z * s};` — каждая компонента домножается на скаляр `s`.
- `constexpr Quat operator*(const Quat& a, const Quat& b)`
  Что делает: композиция двух поворотов (сначала `b`, потом `a`).
  Параметры: `a`, `b` — кватернионы. Возвращает: результирующий поворот.
  Реализация: возвращает четыре компоненты по формуле произведения кватернионов Гамильтона (`w*x + x*w + y*z − z*y` и т.д. для x,y,z и `w*w − x*x − y*y − z*z` для w).
- `constexpr Vec3 rotate(const Quat& q, const Vec3& v)`
  Что делает: поворачивает вектор кватернионом.
  Параметры: `q` — поворот, `v` — исходный вектор. Возвращает: повёрнутый вектор.
  Реализация: быстрая форма `q * v * q^-1` без промежуточного кватерниона: берётся `u = {q.x,q.y,q.z}`, считается `t = 2·cross(u,v)`, затем `v + q.w·t + cross(u,t)`.
- `constexpr Transform compose(const Transform& parent, const Transform& child)`
  Что делает: переводит локальный трансформ ребёнка в систему координат родителя (нужно для мирового трансформа).
  Параметры: `parent` — трансформ родителя, `child` — локальный трансформ ребёнка. Возвращает: трансформ ребёнка в системе родителя.
  Реализация: позиция = `parent.position + rotate(parent.rotation, child.position * parent.scale)`, поворот = `parent.rotation * child.rotation`, масштаб = покомпонентное произведение `parent.scale * child.scale`.
- `constexpr Quat conjugate(const Quat& q)`
  Что делает: сопряжённый кватернион (обратный поворот для единичного).
  Параметры: `q` — поворот. Возвращает: `{-q.x,-q.y,-q.z,q.w}`.
  Реализация: одна строка `return {-q.x, -q.y, -q.z, q.w};` — инвертируются знаки мнимой части, скалярная `w` сохраняется.
- `constexpr Transform invCompose(const Transform& parent, const Transform& world)`
  Что делает: обратная к `compose` — переводит мировой трансформ в локальный относительно родителя (нужно при смене родителя).
  Параметры: `parent` — трансформ родителя, `world` — мировой трансформ объекта. Возвращает: локальный трансформ относительно родителя.
  Реализация: `inverse = conjugate(parent.rotation)`; позиция = `divide(rotate(inverse, world.position − parent.position), parent.scale)`, поворот = `inverse * world.rotation`, масштаб = покомпонентное `divide(world.scale, parent.scale)`.

### Файл `engine/core/include/sky/core/handle.hpp`
- `template <typename Tag> struct Handle { std::uint64_t value; … }`
  Что делает: типобезопасный идентификатор. Разные теги (`ObjectTag`,
  `ComponentTag`) дают несовместимые типы — нельзя перепутать хэндл объекта с
  хэндлом компонента. Содержит `isValid()`, статический `invalid()`, `operator==`.
  Реализация: шаблон-структура с единственным полем `std::uint64_t value = 0`; `isValid()` возвращает `value != 0`, `invalid()` — `constexpr Handle{0}`, сравнения генерируются через `operator<=>(const Handle&) const = default`. Тег в теле не используется — служит лишь для разделения типов на этапе компиляции.

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
  Реализация: просто возвращает строковый литерал имени бэкенда (`return "null";` в `NullRendererImpl`); состояние не трогает.
- `virtual void attachSurface(IRenderSurface& surface) = 0` — привязывает поверхность вывода. Параметры: `surface`.
  Реализация: сохраняет адрес поверхности в поле-указатель (`surface_ = &surface;`), чтобы в конце `renderFrame` вызвать `present()`.
- `virtual void submit(std::span<const RenderCommand> commands) = 0` — принимает поток команд кадра. Параметры: `commands`.
  Реализация: дописывает пришедшие команды в конец накопителя `pending_` — `pending_.insert(pending_.end(), commands.begin(), commands.end())`; отрисовки не выполняет.
- `virtual void renderFrame() = 0` — рисует накопленный кадр.
  Реализация (null-эталон): запоминает `commandsInLastFrame_ = pending_.size()`, очищает `pending_`, инкрементирует `frameCount_` и, если поверхность привязана, зовёт `surface_->present()`.

**`class IRenderResourceFactory`** — создание ресурсов GPU.
- `virtual RenderResourceHandle createMeshFromData(std::span<const float> interleavedPosNormalUv) = 0`
  Что делает: загружает меш из массива вершин (позиция+нормаль+uv). Параметры: `interleavedPosNormalUv`. Возвращает: хэндл ресурса.
  Реализация (null-эталон): проверяет, что массив не пуст и кратен 24 float (8 float на вершину × 3 вершины треугольника), иначе `RenderResourceHandle::invalid()`; иначе выдаёт свежий хэндл `nextId_++` и заносит его в набор `resources_`.
- `virtual RenderResourceHandle createTextureFromData(std::uint32_t w, std::uint32_t h, std::span<const std::uint8_t> rgba) = 0`
  Что делает: загружает текстуру. Параметры: `w`, `h` — размеры, `rgba` — пиксели. Возвращает: хэндл.
  Реализация (null-эталон): проверяет `w,h > 0` и `rgba.size() == w*h*4`, иначе `invalid()`; иначе выдаёт хэндл `nextId_++` и кладёт в `resources_`.
- `virtual void destroy(RenderResourceHandle resource) = 0` — освобождает ресурс.
  Реализация (null-эталон): `resources_.erase(resource.value)`.

### Файл `engine/rendering/include/sky/rendering/renderer_registry.hpp`
`struct BackendInit { … }` — параметры инициализации бэкенда; `RendererFactory` —
тип функции-фабрики рендерера.

**`class IRendererRegistry`** — реестр бэкендов рендера.
- `virtual bool registerBackend(const std::string& name, RendererFactory factory) = 0` — регистрирует фабрику бэкенда. Возвращает: успех.
  Реализация: отвергает пустое имя или пустую `factory` (`return false`), иначе `factories_.emplace(name, std::move(factory)).second` — возвращает `false`, если имя уже занято (`std::map<std::string, RendererFactory>`).
- `virtual std::unique_ptr<IRenderer> create(const std::string& name, const BackendInit&) = 0` — Возвращает: рендерер по имени бэкенда.
  Реализация: ищет имя в `factories_`; при попадании вызывает сохранённую функцию-фабрику `it->second(init)`, иначе возвращает `nullptr`.
- `std::unique_ptr<IRendererRegistry> createRendererRegistry()` — фабрика реестра.
  Реализация: `std::make_unique<RendererRegistryImpl>()`; конструктор impl сразу регистрирует встроенный бэкенд `"null"` лямбдой, возвращающей `createNullRenderer()`.

### Файлы `engine/rendering/include/sky/rendering/null_renderer.hpp`, `engine/rendering/src/{null_renderer,renderer_registry}.cpp`
Пустой рендерер (считает кадры/команды, ничего не рисует) — для тестов контракта, и реализация реестра.
Реализация: `NullRendererImpl` в анонимном namespace хранит `pending_`, счётчики `frameCount_`/`commandsInLastFrame_` и набор `resources_`; `createNullRenderer()` и `createOffscreenSurface()` — фабрики `make_unique`. Счётчики отдаются через `frameCount()`, `commandsInLastFrame()`, `liveResourceCount()`.

**Проверка фичи:** null-рендерер регистрируется в реестре и создаётся по имени;
`submit` + `renderFrame` увеличивают счётчики кадров/команд.
Зависимость: `core::Vec3`/`core::Transform` из фичи E1 `feature/math-and-handles`
(поля `RenderCommand`).

---

## E3 · `feature/editor-shell`

### Файлы `editor/avalonia/SkyEditor.csproj`, `Program.cs`, `App.axaml.cs`, `MainWindow.axaml.cs`
- `csproj` — проект .NET 8 с пакетами Avalonia.
  Реализация: `<OutputType>Exe`, `TargetFramework net8.0`, `Nullable enable`, `AvaloniaUseCompiledBindingsByDefault true`, `AllowUnsafeBlocks true`; `PackageReference` на Avalonia 11.2.1 (Avalonia, .Desktop, .Themes.Fluent, .Fonts.Inter, .Headless) и Dock.Avalonia / Dock.Model.Mvvm 11.2.0.
- `static int Main(string[] args)` — точка входа; ветка `--screenshot` (headless). Возвращает: код выхода.
  Реализация: `Array.IndexOf(args, "--screenshot")`; если флаг и путь есть — вызывает приватный `Screenshot(path, есть ли "--demo")` (конфигурирует `AppBuilder<App>().UseSkia().UseHeadless(...)`, создаёт `MainWindow`, показывает, гоняет `Dispatcher.UIThread.RunJobs()`, при `--demo` создаёт куб и двигает его, крутит кадры `viewport.RenderOnce()`, сохраняет `window.CaptureRenderedFrame()`); иначе `BuildAvaloniaApp().StartWithClassicDesktopLifetime(args)`. `BuildAvaloniaApp` = `AppBuilder.Configure<App>().UsePlatformDetect().WithInterFont().LogToTrace()`.
- `App` — приложение Avalonia; `OnFrameworkInitializationCompleted()` открывает `MainWindow`.
  Реализация: `Initialize()` грузит XAML через `AvaloniaXamlLoader.Load(this)`; `OnFrameworkInitializationCompleted()` при `IClassicDesktopStyleApplicationLifetime desktop` присваивает `desktop.MainWindow = new MainWindow()` и вызывает `base`.
- `MainWindow` — окно «Sky Engine», меню (File/Edit/GameObject), обработчики пунктов.
  Реализация: конструктор создаёт `MainViewModel _vm`, ставит `DataContext`, находит `DockControl`, создаёт `DockFactory(_vm)` и `ResetLayout()`; вешает `OnKeyDown` (хоткеи Delete/Ctrl+D/N/O/S/Z/Y и Q/W/E/R, но не при фокусе в `TextBox`) и `DispatcherTimer` 200 мс → `UpdateTransport()` (подсветка `playButton`/`pauseButton` классом `on` по `sky_editor_play_state`). Пункты меню — обёртки над `_vm` (`NewScene`/`OpenScene`/`SaveScene`/`Undo`/`Redo`/`CreateCube`/`DuplicateSelected`/`DeleteSelected`/`Play`/`Pause`/`Stop`) и `SelectTool` (держит четыре тогла взаимоисключающими).

**Проверка фичи:** `dotnet build editor/avalonia` — 0 ошибок; окно «Sky Engine»
открывается с меню. C-интерфейс на этой фиче не требуется — он подключается со
второй фичи контура (`feature/engine-bridge`).

---

## E4 · `feature/physics-world`

**Порядок реализации.** Крупный `physics_world.cpp` собирается подшагами внутри
этой фичи; после каждого шаг компилируется и закрывается свой пункт теста:
1. **Каркас + хранилище.** Анонимный `namespace` в `.cpp`: записи `BodyRecord{desc, transform, velocity, colliders}`, `ColliderRecord{desc, body}`, `Aabb{min,max}` с `overlaps()`; класс `PhysicsWorldImpl : PhysicsWorld` с полями `nextId_`, `gravity_{0,-9.81,0}`, `bodies_`, `colliders_`, `events_`. Реализуй `createBody/destroyBody/attachCollider/detachCollider`, `bodyTransform`, сеттеры/геттеры и фабрику. Компилируется `sky_physics`; закрывает создание тел.
2. **Гравитация.** Тело `step` без столкновений: интегрирование скорости и позиции для Dynamic-тел. Закрывает пункт «падение ≈4.9 м/с».
3. **Столкновения.** `detectAndResolve` + `resolve` (AABB-расталкивание, сбор `CollisionEvent`); в `step` вызвать после интегрирования. Закрывает «куб на полу».
4. **Террейн.** `sampleHeightfield` + `resolveHeightfields`; вызвать в `step` до `detectAndResolve`. Закрывает «тело на heightfield».
5. **Луч.** `worldAabb`, `rayVsAabb`, `rayVsHeightfield` и `raycast`. Закрывает запросы луча (используется на этапе 5).

### Файл `engine/physics/include/sky/physics/physics.hpp`
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
  Реализация: перебирает все `colliders_`, хранит `best` (ближайшее). Для `TerrainHeightfield` зовёт `rayVsHeightfield(...)`; для остальных строит `worldAabb(...)` и зовёт `rayVsAabb(...)`. При успехе, если ближе `best->distance`, обновляет `best`. Возвращает `best`.
- `virtual core::Transform bodyTransform(RigidBodyHandle body) const = 0` — Возвращает: трансформ тела.
  Реализация: `bodies_.find(body.value)`; вернуть `it->second.transform`, либо `core::Transform{}` если тела нет.

### Файлы `engine/physics/include/sky/physics/physics_world.hpp`, `engine/physics/src/physics_world.cpp`
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
  Реализация: держит Dynamic-тела над каждым `TerrainHeightfield`-коллайдером. Для каждого heightfield-коллайдера (с его `fieldOrigin`) и каждого Dynamic-тела: взять `halfHeight`, посчитать `ground = fieldOrigin.y + sampleHeightfield(...localX,localZ...)`; если `position.y - halfHeight < ground` — поднять `position.y = ground + halfHeight`, обнулить `velocity.y` при падении вниз и добавить `CollisionEvent` в `events_`.
- `bool rayVsHeightfield(ColliderHandle, const ColliderRecord&, origin, direction, maxDistance, float& distance, Vec3& point, Vec3& normal) const`
  Реализация: марш фиксированным шагом до первой точки под поверхностью, затем бисекция. `heightAt(p)=fieldOrigin.y+sampleHeightfield(...)`, `above(t)` сравнивает `p.y` с `heightAt(p)`. Если старт уже под поверхностью — `false`. Шаг `step=clamp(maxDistance/256, 0.05, 0.5)`; при пересечении между `previous` и `t` 16 итераций бисекции дают `distance`, `point`, а нормаль — из центральных разностей высоты (градиент) с нормировкой.
- `void detectAndResolve()`
  Реализация: собрать `boxes` (id + `worldAabb`) по всем коллайдерам, пропуская `TerrainHeightfield`. Двойным циклом по парам: пропустить пары одного тела и непересекающиеся `overlaps`; иначе добавить `CollisionEvent` в `events_`, вызвать `resolve(...)` и обновить `boxes[i]/[j]` (тело могло сдвинуться).
- `void resolve(const ColliderRecord& a, const Aabb& boxA, const ColliderRecord& b, const Aabb& boxB)`
  Реализация: позиционно расталкивается только пара Dynamic-vs-(Static|Kinematic) (иначе выход — только событие). Посчитать проникновения `penX/penY/penZ`, выбрать ось наименьшего проникновения, сдвинуть `dynamicBody->transform.position` по ней со знаком и обнулить соответствующую компоненту `velocity`.
- `static bool rayVsAabb(origin, direction, const Aabb& box, maxDistance, float& outDistance, Vec3& outNormal)`
  Реализация: слэб-метод по трём осям. Вести `tMin=0`, `tMax=maxDistance`; для каждой оси при почти нулевом направлении проверить попадание в диапазон, иначе посчитать `t1,t2` (с перестановкой и знаком нормали), поднять `tMin` (запомнив нормаль), опустить `tMax`; при `tMin>tMax` — `false`. Записать `outDistance=tMin`, `outNormal`.

**Проверка фичи:** тело за 1 с падает ≈4.9 м под гравитацией; куб замирает на
полу (расталкивание AABB); луч попадает в коллайдер.
Зависимость: `core::Vec3`/`core::Transform` из фичи E1 `feature/math-and-handles`.

---

## E5 · `feature/build-system`

### Файлы `CMakeLists.txt`, `engine/CMakeLists.txt`, `tests/CMakeLists.txt`
- `engine/CMakeLists.txt` — функция `sky_add_module(NAME DIR sources…)` создаёт
  статическую библиотеку с public-include-путями и стандартом C++20; регистрирует
  модули и связи между ними.
  Реализация: `set(SOURCES ${ARGN})`; если исходники есть — `add_library(${NAME} STATIC ${SOURCES})`, `target_include_directories(... PUBLIC .../${DIR}/include)`, `target_compile_features(... PUBLIC cxx_std_20)`; иначе `add_library(${NAME} INTERFACE)` с теми же include/features через `INTERFACE`. В конце всегда `add_library(sky::${NAME} ALIAS ${NAME})`. Ниже функции идут вызовы `sky_add_module` для всех модулей и `target_link_libraries` для рёбер зависимостей.
- корневой `CMakeLists.txt` — проект, опции, `add_subdirectory` для движка/редактора/плеера/тестов.
  Реализация: `cmake_minimum_required(3.20)`, `project(SkyEngine … LANGUAGES C CXX)`, задать `CMAKE_CXX_STANDARD 20`/`_REQUIRED ON`/`_EXTENSIONS OFF` и `CMAKE_POSITION_INDEPENDENT_CODE ON` (движок линкуется в .so-мост редактора); объявить `option(...)` (`SKY_BUILD_EDITOR/OPENGL/VULKAN/TESTS`); через `find_program(SKY_DOTNET dotnet)` собрать managed-цель; затем `add_subdirectory(engine|editor|player)` и, если `SKY_BUILD_TESTS`, — `enable_testing()` + `add_subdirectory(tests)`.

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
  Реализация: в `TypedComponentStore<T>` (ecs_world.hpp) вернуть `typeid(T)`.
- `virtual bool has(EntityId entity) const = 0` — Возвращает: есть ли компонент у сущности.
  Реализация: вернуть `data_.contains(entity.value)`, где `data_` — `std::unordered_map<std::uint64_t, T>`, ключ — `EntityId::value`.
- `virtual void remove(EntityId entity) = 0` — удаляет компонент у сущности.
  Реализация: `data_.erase(entity.value)`.
- `virtual std::size_t count() const = 0` — Возвращает: число компонентов.
  Реализация: вернуть `data_.size()`.

**`class IEcsSystem`** — система, исполняемая каждый кадр.
- `virtual std::string name() const = 0` — Возвращает: имя системы.
  Реализация: конкретная система (напр. `SpinSystem` в `tests/runtime_tests.cpp`) возвращает свой строковый литерал-имя.
- `virtual void update(double deltaSeconds) = 0` — обрабатывает подходящие сущности. Параметры: `deltaSeconds` — шаг времени.
  Реализация: получить список сущностей через `world_.entitiesWith({typeid(Comp)})`, для каждой взять компонент через `world_.storeFor<Comp>().get(entity)` и изменить его поля по `deltaSeconds` (в `SpinSystem` — прибавляет `90.0f * deltaSeconds` к `angle`).

**`class IEcsWorld`** — мир сущностей.
- `virtual EntityId createEntity() = 0` — Возвращает: новую сущность.
  Реализация: в `EcsWorldImpl` (ecs_world.cpp) собрать `EntityId{nextId_++}` (счётчик стартует с 1), вставить `entity.value` в множество `alive_` (`std::unordered_set<std::uint64_t>`), вернуть сущность.
- `virtual void destroyEntity(EntityId entity) = 0` — уничтожает сущность.
  Реализация: `alive_.erase(entity.value)`; если элемент удалён (был жив), пройти по всем хранилищам `stores_` и вызвать `store->remove(entity)` для каждого.
- `virtual bool isAlive(EntityId entity) const = 0` — Возвращает: жива ли сущность.
  Реализация: вернуть `alive_.contains(entity.value)`.
- `virtual IEcsComponentStore& store(std::type_index componentType) = 0` — Возвращает: хранилище типа.
  Реализация: искать `componentType` в `stores_` (`unordered_map<std::type_index, unique_ptr<IEcsComponentStore>>`); если не найдено — бросить `std::out_of_range`, иначе разыменовать `*it->second`.

**`class IEcsSystemScheduler`** — планировщик.
- `virtual void registerSystem(IEcsSystem& system) = 0` — регистрирует систему.
  Реализация: `systems_.push_back(&system)`, где `systems_` — `std::vector<IEcsSystem*>` (хранит указатели в порядке регистрации).
- `virtual void unregisterSystem(IEcsSystem& system) = 0` — снимает систему.
  Реализация: `std::erase(systems_, &system)` — убрать указатель из вектора.
- `virtual void tick(double deltaSeconds) = 0` — прогоняет все системы за такт. Параметры: `deltaSeconds`.
  Реализация: последовательный цикл по `systems_`, для каждой вызвать `system->update(deltaSeconds)` (порядок регистрации).

**`class IEcsQueryService`** — запросы.
- `virtual std::vector<EntityId> entitiesWith(std::set<std::type_index> types) const = 0`
  Что делает: находит сущности с заданным набором компонентов. Параметры: `types`. Возвращает: список сущностей.
  Реализация: перебрать все id из `alive_`; для каждой сущности проверить `std::ranges::all_of` по `types` — что тип есть в `stores_` и `store->has(entity)`; совпавшие добавить в `result` (линейный перебор живых сущностей).

### Файлы `engine/ecs/include/sky/ecs/ecs_world.hpp`, `engine/ecs/src/ecs_world.cpp`
- `class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService` — добавляет:
  - `template <typename T> TypedComponentStore<T>& storeFor()`
    Что делает: типобезопасный доступ к хранилищу компонента T (с методами `set(entity, value)`, `get(entity)→T*`). Возвращает: хранилище T.
    Реализация: в `stores()` (map `type_index→unique_ptr<IEcsComponentStore>`) взять слот по ключу `std::type_index(typeid(T))`; если пуст — создать `std::make_unique<TypedComponentStore<T>>()`; вернуть `static_cast<TypedComponentStore<T>&>(*slot)`. `set` делает `data_.insert_or_assign`, `get` — `data_.find` с возвратом указателя или `nullptr`.
- `std::unique_ptr<EcsWorld> createEcsWorld()` — фабрика.
  Реализация: `return std::make_unique<EcsWorldImpl>();` (`EcsWorldImpl` — приватная реализация в анонимном namespace ecs_world.cpp).

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
