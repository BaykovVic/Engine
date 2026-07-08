# День 4

## E1 · `feature/component-model`

Компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.

### Файл `engine/component/include/sky/component/component_model.hpp`
`using FieldValue = std::variant<float, std::int64_t, bool, std::string,
core::Vec3>`. `ComponentDescriptor{typeId, displayName, fields, category}`.

**`class IComponentRegistry`** — реестр типов.
- `virtual void registerComponentType(const ComponentDescriptor& descriptor) = 0` — регистрирует тип.
- `virtual std::vector<ComponentDescriptor> availableTypes() const = 0` — Возвращает: типы для меню Add Component.

**`class IComponentAttachmentService`** — навешивание.
- `virtual ComponentHandle attach(object::ObjectHandle object, const std::string& typeId) = 0` — Возвращает: хэндл компонента.
- `virtual void detach(ComponentHandle component) = 0` — снимает компонент.

**`class IComponentQueryService`** — запросы.
- `virtual std::vector<ComponentHandle> componentsOf(object::ObjectHandle object) const = 0` — Возвращает: компоненты объекта.
- `virtual const ComponentDescriptor& descriptorOf(ComponentHandle component) const = 0` — Возвращает: описание типа.
- `virtual object::ObjectHandle ownerOf(ComponentHandle component) const = 0` — Возвращает: объект-владелец.

### Файлы `engine/component/include/sky/component/component_world.hpp`, `engine/component/src/component_world.cpp`
`class ComponentWorld` наследует интерфейсы выше и добавляет доступ к данным:
- `virtual void setField(ComponentHandle component, const std::string& name, FieldValue value) = 0` — записывает поле по имени.
- `virtual std::optional<FieldValue> field(ComponentHandle component, const std::string& name) const = 0` — Возвращает: значение или `nullopt`.
- `virtual std::map<std::string, FieldValue> fields(ComponentHandle component) const = 0` — Возвращает: всю карту полей.
- `virtual void detachAllFrom(object::ObjectHandle object) = 0` — снимает все компоненты объекта.
- `std::unique_ptr<ComponentWorld> createComponentWorld()` — фабрика.

**Проверка (этап E1):** поле каждого из 5 типов записывается и читается без потерь.

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

## E2 · `feature/vulkan-offscreen`

Инициализация Vulkan, закадровая цель, конвейер, отрисовка треугольника, чтение кадра.

### Файл `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
- `class VulkanRenderer : public rendering::IRenderer` — добавляет:
  - `virtual bool ready() const = 0` — Возвращает: инициализировался ли рендерер.
  - `virtual std::vector<std::uint8_t> readbackFrame() = 0` — Возвращает: пиксели последнего кадра (RGBA).
  - `virtual std::uint32_t frameWidth() const = 0` / `frameHeight() const = 0` — Возвращает: размеры кадра.
- `std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width, std::uint32_t height)`
  Что делает: создаёт закадровый рендерер. Параметры: `width`, `height`. Возвращает: рендерер или `nullptr`, если Vulkan недоступен.

### Файл `engine/rendering_vulkan/src/vulkan_renderer.cpp`
Реализация. Приватные методы (реализуются в этой фиче):
- `initInstanceAndDevice()` — создаёт `VkInstance`, выбирает устройство и графическую очередь, создаёт `VkDevice`.
- `initOffscreenTarget()` — создаёт изображения цвета (RGBA) и глубины (D32), проход рендеринга и кадровый буфер.
- `initPipeline()` — создаёт графический конвейер; формат вершины — позиция+нормаль+uv.
- `renderFrame()` — записывает команды, очищает, рисует треугольник (`vkCmdDraw`).
- `readbackFrame()` — копирует изображение цвета в буфер, доступный CPU, возвращает пиксели.
- вспомогательные `findMemoryType`, `createImage`, `createBuffer`, `createShader`.

### Файлы `engine/rendering_vulkan/shaders/mesh.vert`, `mesh.frag` (+ встроенные `.spv.h`)
Минимальные вершинный и фрагментный шейдеры (позже дорастут до PBR). Компиляция
`glslangValidator -V` → массив `std::uint32_t` встраивается в `.spv.h`.

**Проверка (этап E2):** на `triangle.png` центральный пиксель отличается от
углового; `readbackFrame()` возвращает непустой массив.
