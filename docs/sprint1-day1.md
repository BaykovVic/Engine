# Спринт 1. День 1

## Контур E1 (Ядро и данные)

feature/math-and-handles

Цель фичи: математика и типобезопасные идентификаторы — фундамент, от которого зависят все.
Описание фичи (для чего): Vec3/Quat/Transform и Handle используют все модули; заголовок math.hpp отдаётся первым коммитом — по нему стартуют E2 и E4.
Пошаговое описание действий:

Сделай файл engine/core/include/sky/core/math.hpp
Свободные функции над векторами, кватернионами и трансформами (все constexpr). Структуры Vec3{x,y,z}, Quat{x,y,z,w}, Transform{position, rotation, scale} объявляются здесь же. В файле должны быть функции/методы:
- `constexpr Vec3 operator+(const Vec3& a, const Vec3& b)`
  Что делает: покомпонентное сложение векторов. Параметры: a, b — слагаемые. Возвращает: сумму {a.x+b.x, …}.
- `constexpr Vec3 operator*(const Vec3& a, float s)`
  Что делает: масштабирование вектора числом. Параметры: a — вектор, s — множитель. Возвращает: {a.x*s, …}.
- `constexpr Quat operator*(const Quat& a, const Quat& b)`
  Что делает: композиция двух поворотов (сначала b, потом a). Параметры: a, b — кватернионы. Возвращает: результирующий поворот.
- `constexpr Vec3 rotate(const Quat& q, const Vec3& v)`
  Что делает: поворачивает вектор кватернионом. Параметры: q — поворот, v — исходный вектор. Возвращает: повёрнутый вектор.
- `constexpr Transform compose(const Transform& parent, const Transform& child)`
  Что делает: переводит локальный трансформ ребёнка в систему координат родителя (нужно для мирового трансформа). Параметры: parent — трансформ родителя, child — локальный трансформ ребёнка. Возвращает: трансформ ребёнка в системе родителя.
- `constexpr Quat conjugate(const Quat& q)`
  Что делает: сопряжённый кватернион (обратный поворот для единичного). Параметры: q — поворот. Возвращает: {-q.x,-q.y,-q.z,q.w}.
- `constexpr Transform invCompose(const Transform& parent, const Transform& world)`
  Что делает: обратная к compose — переводит мировой трансформ в локальный относительно родителя (нужно при смене родителя). Параметры: parent — трансформ родителя, world — мировой трансформ объекта. Возвращает: локальный трансформ относительно родителя.

Сделай файл engine/core/include/sky/core/handle.hpp
В файле должны быть функции/методы:
- `template <typename Tag> struct Handle { std::uint64_t value; … }`
  Что делает: типобезопасный идентификатор. Разные теги (ObjectTag, ComponentTag) дают несовместимые типы — нельзя перепутать хэндл объекта с хэндлом компонента. Содержит isValid(), статический invalid(), operator==.

На выходе должно получиться:
- engine/core/include/sky/core/math.hpp
- engine/core/include/sky/core/handle.hpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: rotate(поворот 90° вокруг Y, {0,0,1}) ≈ {1,0,0} (±1e-5); invCompose(parent, compose(parent, child)) == child.

## Контур E2 (Рендеринг)

feature/render-contract

Цель фичи: общий (не только Vulkan) контракт рендера — поток команд и интерфейс рендерера.
Описание фичи (для чего): единый контракт, от которого зависят все бэкенды; остальные фичи опираются на интерфейсы, а не на реализацию. Зависимость: core::Vec3/core::Transform из feature/math-and-handles.
Пошаговое описание действий:

Сделай файл engine/rendering/include/sky/rendering/rendering.hpp
Перечисления и структура команды. В файле должны быть функции/методы:
- `enum class RenderCommandType { BeginFrame, SetViewport, SetCamera, AddLight, SetSky, BindPipeline, DrawMesh, EndFrame }`
  Что делает: вид команды отрисовки в потоке.
- `enum class LightType { Directional = 0, Point = 1 }` — вид источника света.
- `struct RenderCommand { RenderCommandType type; core::Transform transform; core::Vec3 color; float fovDegrees; float orthoHeight; … }`
  Что делает: одна backend-независимая команда. Поля используются по-разному в зависимости от type (для SetCamera — поза камеры и проекция, для AddLight — поза и цвет света, для DrawMesh — материал и текстуры).

class IRenderer — контракт рендерера:
- `virtual std::string backendName() const = 0` — Возвращает: имя бэкенда ("vulkan"/"opengl").
- `virtual void attachSurface(IRenderSurface& surface) = 0` — привязывает поверхность вывода. Параметры: surface.
- `virtual void submit(std::span<const RenderCommand> commands) = 0` — принимает поток команд кадра. Параметры: commands.
- `virtual void renderFrame() = 0` — рисует накопленный кадр.

class IRenderResourceFactory — создание ресурсов GPU:
- `virtual RenderResourceHandle createMeshFromData(std::span<const float> interleavedPosNormalUv) = 0`
  Что делает: загружает меш из массива вершин (позиция+нормаль+uv). Параметры: interleavedPosNormalUv. Возвращает: хэндл ресурса.
- `virtual RenderResourceHandle createTextureFromData(std::uint32_t w, std::uint32_t h, std::span<const std::uint8_t> rgba) = 0`
  Что делает: загружает текстуру. Параметры: w, h — размеры, rgba — пиксели. Возвращает: хэндл.
- `virtual void destroy(RenderResourceHandle resource) = 0` — освобождает ресурс.

Сделай файл engine/rendering/include/sky/rendering/renderer_registry.hpp
struct BackendInit { … } — параметры инициализации бэкенда; RendererFactory — тип функции-фабрики рендерера. В файле должны быть функции/методы:
class IRendererRegistry — реестр бэкендов рендера:
- `virtual bool registerBackend(const std::string& name, RendererFactory factory) = 0` — регистрирует фабрику бэкенда. Возвращает: успех.
- `virtual std::unique_ptr<IRenderer> create(const std::string& name, const BackendInit&) = 0` — Возвращает: рендерер по имени бэкенда.
- `std::unique_ptr<IRendererRegistry> createRendererRegistry()` — фабрика реестра.

Сделай файл engine/rendering/include/sky/rendering/null_renderer.hpp
В файле должен быть пустой рендерер (считает кадры/команды, ничего не рисует) — для тестов контракта.

Сделай файл engine/rendering/src/null_renderer.cpp
Реализация пустого рендерера (скрытый класс) и фабрики createNullRenderer, createOffscreenSurface. В методах должна быть реализована логика:
- submit(commands) — дописать команды во внутренний буфер.
- renderFrame() — запомнить число команд в буфере (commandsInLastFrame), очистить буфер, увеличить счётчик кадров (frameCount), вызвать present() у привязанной поверхности.
- createMeshFromData / createTextureFromData — проверить вход и завести хэндл ресурса (учитывается в множестве живых); destroy — убрать ресурс.
- frameCount / commandsInLastFrame / liveResourceCount — чтение счётчиков.

Сделай файл engine/rendering/src/renderer_registry.cpp
Реализация реестра (скрытый класс) и фабрика createRendererRegistry(). В методах должна быть реализована логика:
- конструктор реестра предрегистрирует бэкенд "null".
- registerBackend(name, factory) — сохранить фабрику по имени; отклонить пустое имя, пустую фабрику и дубликат.
- create(name, init) — найти фабрику по имени и создать рендерер; для неизвестного имени вернуть nullptr.

На выходе должно получиться:
- engine/rendering/include/sky/rendering/rendering.hpp
- engine/rendering/include/sky/rendering/renderer_registry.hpp
- engine/rendering/include/sky/rendering/null_renderer.hpp
- engine/rendering/src/null_renderer.cpp
- engine/rendering/src/renderer_registry.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: null-рендерер регистрируется в реестре и создаётся по имени; submit + renderFrame увеличивают счётчики кадров/команд.

## Контур E3 (Редактор .NET)

feature/editor-shell

Цель фичи: каркас приложения редактора на Avalonia (.NET 8) с окном «Sky Engine».
Описание фичи (для чего): проект редактора и первичное окно, без которого нет панелей и вызовов движка; C-интерфейс подключается со второй фичи контура.
Пошаговое описание действий:

Сделай файл editor/avalonia/SkyEditor.csproj
В файле должны быть функции/методы:
- `csproj` — проект .NET 8 с пакетами Avalonia.

Сделай файл editor/avalonia/Program.cs
В файле должны быть функции/методы:
- `static int Main(string[] args)` — точка входа; ветка --screenshot (headless). Возвращает: код выхода.

Сделай файл editor/avalonia/App.axaml.cs
В файле должны быть функции/методы:
- `App` — приложение Avalonia; `OnFrameworkInitializationCompleted()` открывает MainWindow.

Сделай файл editor/avalonia/MainWindow.axaml.cs
В файле должны быть функции/методы:
- `MainWindow` — окно «Sky Engine», меню (File/Edit/GameObject), обработчики пунктов.

На выходе должно получиться:
- editor/avalonia/SkyEditor.csproj
- editor/avalonia/Program.cs
- editor/avalonia/App.axaml.cs
- editor/avalonia/MainWindow.axaml.cs
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build editor/avalonia — 0 ошибок; окно «Sky Engine» открывается с меню.

## Контур E4 (Рантайм и физика)

feature/physics-world

Цель фичи: физический мир — тела, коллайдеры, гравитация, столкновения, высотная поверхность, луч.
Описание фичи (для чего): ядро симуляции; контракты управления и запросов плюс конкретная реализация. Зависимость: core::Vec3/core::Transform из feature/math-and-handles.
Пошаговое описание действий:

Сделай файл engine/physics/include/sky/physics/physics.hpp
Типы: RigidBodyDesc{type,mass,transform}, enum class ColliderShape{Box,Sphere,Capsule,TerrainHeightfield}, ColliderDesc{shape,halfExtents,radius,heightfield}, HeightfieldDesc{resolution,scale,heights}, RaycastHit{collider,point,normal,distance}, CollisionEvent{first,second}. В файле должны быть функции/методы:
class IPhysicsWorld — управление симуляцией:
- `virtual RigidBodyHandle createBody(const RigidBodyDesc& desc) = 0`
  Что делает: создаёт физическое тело. Параметры: desc — тип/масса/поза. Возвращает: хэндл тела.
- `virtual void destroyBody(RigidBodyHandle body) = 0` — удаляет тело.
- `virtual ColliderHandle attachCollider(RigidBodyHandle body, const ColliderDesc& desc) = 0`
  Что делает: навешивает коллайдер на тело. Параметры: body, desc — форма коллайдера. Возвращает: хэндл коллайдера.
- `virtual void step(double fixedDeltaSeconds) = 0` — шаг симуляции. Параметры: fixedDeltaSeconds — шаг времени.
- `virtual std::vector<CollisionEvent> drainCollisionEvents() = 0` — Возвращает: события столкновений за кадр.
class IPhysicsQueryService — запросы:
- `virtual std::optional<RaycastHit> raycast(const core::Vec3& origin, const core::Vec3& direction, float maxDistance) const = 0`
  Что делает: пускает луч в физический мир. Параметры: origin — начало, direction — направление, maxDistance — предел. Возвращает: попадание или nullopt.
- `virtual core::Transform bodyTransform(RigidBodyHandle body) const = 0` — Возвращает: трансформ тела.

Сделай файл engine/physics/include/sky/physics/physics_world.hpp
class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService — добавляет функции/методы:
- `virtual void setGravity(const core::Vec3& gravity) = 0` — задаёт гравитацию.
- `virtual void setBodyVelocity(RigidBodyHandle body, const core::Vec3& velocity) = 0` — задаёт скорость тела.
- `virtual core::Vec3 bodyVelocity(RigidBodyHandle body) const = 0` — Возвращает: скорость тела.
- `virtual void setBodyTransform(RigidBodyHandle body, const core::Transform& transform) = 0` — переставляет тело.
- `std::unique_ptr<PhysicsWorld> createPhysicsWorld()` — фабрика.

Сделай файл engine/physics/src/physics_world.cpp
Реализация PhysicsWorld (скрытый класс) и фабрика createPhysicsWorld(). В методах должна быть реализована логика:
- createBody / destroyBody / attachCollider / detachCollider — ведут хранилища тел и коллайдеров (id с 1); destroyBody каскадно удаляет коллайдеры тела.
- step(fixedDeltaSeconds) — для каждого Dynamic-тела: к скорости прибавить gravity·dt, затем к позиции прибавить velocity·dt (явный Эйлер); Static/Kinematic не двигать. После движения вызвать resolveHeightfields() и detectAndResolve().
- detectAndResolve() — построить осевую коробку (AABB) каждого коллайдера (позиция тела ± halfExtents); для пар РАЗНЫХ тел, чьи коробки пересеклись, записать CollisionEvent и, если одно тело Dynamic, а другое — нет, вытолкнуть Dynamic по оси наименьшего проникновения и обнулить его скорость вдоль этой оси.
- resolveHeightfields() — удержание Dynamic-тел над террейном: под телом взять высоту heightfield; если низ тела ниже поверхности — поднять тело на поверхность и обнулить отрицательную вертикальную скорость, добавить CollisionEvent.
- sampleHeightfield(field, x, z) — билинейная выборка высоты heightfield в точке (x, z).
- raycast(origin, direction, maxDistance) — для каждого коллайдера пересечь луч с AABB (метод слэбов) либо с heightfield (марш с бисекцией); вернуть ближайшее попадание в пределах maxDistance, нормаль — грань оси входа.

На выходе должно получиться:
- engine/physics/include/sky/physics/physics.hpp
- engine/physics/include/sky/physics/physics_world.hpp
- engine/physics/src/physics_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: тело за 1 с падает ≈4.9 м под гравитацией; куб замирает на полу (расталкивание AABB); луч попадает в коллайдер.

## Контур E5 (Пайплайн и QA)

feature/build-system

Цель фичи: скелет сборки CMake для движка, редактора, плеера и тестов.
Описание фичи (для чего): единый механизм сборки модулей; в него остальные контуры добавляют свои модули по мере готовности.
Пошаговое описание действий:

Сделай файл engine/CMakeLists.txt
В файле должны быть функции/методы:
- `sky_add_module(NAME DIR sources…)`
  Что делает: создаёт статическую библиотеку с public-include-путями и стандартом C++20; регистрирует модули и связи между ними.

Сделай файл CMakeLists.txt (корневой)
В файле должны быть функции/методы:
- корневой проект, опции, `add_subdirectory` для движка/редактора/плеера/тестов.

Сделай файл tests/CMakeLists.txt
В файле должны быть функции/методы:
- регистрация тестовых целей.

На выходе должно получиться:
- CMakeLists.txt
- engine/CMakeLists.txt
- tests/CMakeLists.txt
КРИТЕРИЙ ПРАВИЛЬНОСТИ: cmake -S . -B build && cmake --build build проходит с одним модулем (напр. sky_core); sky_add_module подключает следующий модуль одной строкой.

## Контур E6 (Data-oriented / ECS)

feature/ecs-core

Цель фичи: ECS-ядро — сущности, типизированные хранилища компонентов, планировщик систем и запросы.
Описание фичи (для чего): сущности держат компоненты в хранилищах по типу, системы обрабатывают их пачками; на этом ядре строятся все этапы контура.
Пошаговое описание действий:

Сделай файл engine/ecs/include/sky/ecs/ecs.hpp
`struct EntityId { std::uint32_t index; std::uint32_t generation; }` — сущность с поколением (защита от повторного использования id). В файле должны быть функции/методы:
class IEcsComponentStore — хранилище одного типа компонента:
- `virtual std::type_index componentType() const = 0` — Возвращает: тип компонента.
- `virtual bool has(EntityId entity) const = 0` — Возвращает: есть ли компонент у сущности.
- `virtual void remove(EntityId entity) = 0` — удаляет компонент у сущности.
- `virtual std::size_t count() const = 0` — Возвращает: число компонентов.
class IEcsSystem — система, исполняемая каждый кадр:
- `virtual std::string name() const = 0` — Возвращает: имя системы.
- `virtual void update(double deltaSeconds) = 0` — обрабатывает подходящие сущности. Параметры: deltaSeconds — шаг времени.
class IEcsWorld — мир сущностей:
- `virtual EntityId createEntity() = 0` — Возвращает: новую сущность.
- `virtual void destroyEntity(EntityId entity) = 0` — уничтожает сущность.
- `virtual bool isAlive(EntityId entity) const = 0` — Возвращает: жива ли сущность.
- `virtual IEcsComponentStore& store(std::type_index componentType) = 0` — Возвращает: хранилище типа.
class IEcsSystemScheduler — планировщик:
- `virtual void registerSystem(IEcsSystem& system) = 0` — регистрирует систему.
- `virtual void unregisterSystem(IEcsSystem& system) = 0` — снимает систему.
- `virtual void tick(double deltaSeconds) = 0` — прогоняет все системы за такт. Параметры: deltaSeconds.
class IEcsQueryService — запросы:
- `virtual std::vector<EntityId> entitiesWith(std::set<std::type_index> types) const = 0`
  Что делает: находит сущности с заданным набором компонентов. Параметры: types. Возвращает: список сущностей.

Сделай файл engine/ecs/include/sky/ecs/ecs_world.hpp
class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService — добавляет функции/методы:
- `template <typename T> TypedComponentStore<T>& storeFor()`
  Что делает: типобезопасный доступ к хранилищу компонента T (с методами set(entity, value), get(entity)→T*). Возвращает: хранилище T.
- `std::unique_ptr<EcsWorld> createEcsWorld()` — фабрика.

Сделай файл engine/ecs/src/ecs_world.cpp
Реализация EcsWorld (скрытый класс) и фабрика createEcsWorld(). В методах должна быть реализована логика:
- createEntity() — выдать id (с 1) и пометить сущность живой.
- destroyEntity(entity) — убрать из живых и удалить её компоненты во всех хранилищах.
- isAlive(entity) — проверить, жива ли сущность.
- store(type) — вернуть хранилище типа (бросить, если тип не зарегистрирован).
- registerSystem/unregisterSystem — вести список систем; tick(dt) — вызвать update у всех систем по порядку.
- entitiesWith(types) — вернуть живые сущности, у которых есть все указанные компоненты.

На выходе должно получиться:
- engine/ecs/include/sky/ecs/ecs.hpp
- engine/ecs/include/sky/ecs/ecs_world.hpp
- engine/ecs/src/ecs_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: создание/уничтожение сущности; storeFor<T>().set/get работают; entitiesWith({type}) возвращает только сущности с этим компонентом.
