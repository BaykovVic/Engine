# День 3

## E1 · `feature/object-model`

Единственный владелец иерархии сцены и трансформов.

### Файл `engine/object/include/sky/object/object_model.hpp`
Три контракта. `using ObjectHandle = core::Handle<ObjectTag>`.

**`class IObjectFactory`** — создание и удаление объектов.
- `virtual ObjectHandle createObject(const std::string& name) = 0`
  Что делает: создаёт объект. Параметры: `name`. Возвращает: хэндл нового объекта.
- `virtual void destroyObject(ObjectHandle object) = 0`
  Что делает: удаляет объект и его поддерево. Параметры: `object`. Возвращает: ничего.

**`class IObjectHierarchyAccess`** — иерархия и трансформы.
- `virtual void setParent(ObjectHandle child, ObjectHandle parent) = 0` — перевешивает `child` под `parent` (invalid = корень).
- `virtual ObjectHandle parentOf(ObjectHandle object) const = 0` — Возвращает: родителя (или invalid).
- `virtual std::vector<ObjectHandle> childrenOf(ObjectHandle object) const = 0` — Возвращает: прямых детей.
- `virtual void setLocalTransform(ObjectHandle object, const core::Transform& transform) = 0` — задаёт локальный трансформ.
- `virtual core::Transform localTransform(ObjectHandle object) const = 0` — Возвращает: локальный трансформ.
- `virtual core::Transform worldTransform(ObjectHandle object) const = 0` — Возвращает: мировой трансформ (композиция локальных вверх по цепочке).

**`class IObjectQueryService`** — запросы для чтения.
- `virtual bool exists(ObjectHandle object) const = 0` — Возвращает: жив ли объект.
- `virtual std::string nameOf(ObjectHandle object) const = 0` — Возвращает: имя.
- `virtual std::vector<ObjectHandle> findByName(const std::string& name) const = 0` — Возвращает: объекты с таким именем.

**Свободная функция:**
- `inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)`
  Что делает: задаёт мировой трансформ, пересчитывая локальный через `invCompose`. Параметры: `access`, `object`, `world`.

### Файлы `engine/object/include/sky/object/object_world.hpp`, `engine/object/src/object_world.cpp`
- `class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService` — единый владелец, добавляет:
  - `virtual void renameObject(ObjectHandle object, const std::string& name) = 0` — переименовывает объект.
- `std::unique_ptr<ObjectWorld> createObjectWorld()` — Возвращает: реализацию мира объектов.
  Внутри `.cpp`: хранилище `id → {локальный трансформ, родитель, дети, имя}`; `worldTransform` = `compose` вверх; `destroyObject` рекурсивно удаляет поддерево.

**Проверка (этап E1):** ребёнок `(1,0,0)` под родителем, повёрнутым на 90° вокруг Y,
в мире = `(0,0,-1)`.

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
