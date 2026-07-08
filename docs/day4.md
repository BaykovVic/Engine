# День 4 — компоненты, физика, offscreen-рендер

Фичи дня (в порядке реализации). Одна фича = ветка `feature/<название>` = один PR в `develop`.

---

## feature/component-model (E1)
**Цель фичи:** компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.
**Описание фичи (для чего):** поля описываются данными (`variant`-map), что позже бесплатно даёт сериализацию, отмену и параметры скриптов.
**Пошаговое описание действий:**
- Сделай файл `engine/component/include/sky/component/component_model.hpp`.
- В файле должны быть `using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>`, `ComponentDescriptor`, `IComponentRegistry` (`registerComponentType`, `availableTypes`), `IComponentAttachmentService` (`attach`, `detach`), `IComponentQueryService` (`componentsOf`, `descriptorOf`, `ownerOf`).
- В методах должна быть реализована логика: регистрация типов; навешивание/снятие компонентов; запросы «компоненты объекта / описание типа / владелец».
- Сделай файлы `engine/component/include/sky/component/component_world.hpp`, `engine/component/src/component_world.cpp`.
- В файле должен быть `class ComponentWorld` + `setField`, `field`, `fields`, `detachAllFrom`, фабрика `createComponentWorld()`.
- В методах должна быть реализована логика: запись/чтение поля по имени; выдача всей карты полей; снятие всех компонентов объекта.
**На выходе должно получиться:** `component_model.hpp`, `component_world.hpp`, `component_world.cpp` (библиотека `sky_component`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** поле каждого из 5 типов записывается и читается без потерь.

## feature/physics-world (E4)
**Цель фичи:** физический мир — тела, коллайдеры, гравитация, столкновения, heightfield, луч.
**Описание фичи (для чего):** ядро симуляции; контракты управления и запросов + конкретная реализация. Без неё нет физических взаимодействий.
**Пошаговое описание действий:**
- Сделай файл `engine/physics/include/sky/physics/physics.hpp`.
- В файле должны быть типы `RigidBodyDesc`, `ColliderShape`, `ColliderDesc`, `HeightfieldDesc`, `RaycastHit`, `CollisionEvent`; `class IPhysicsWorld` (`createBody`, `destroyBody`, `attachCollider`, `step`, `drainCollisionEvents`); `class IPhysicsQueryService` (`raycast`, `bodyTransform`).
- В методах должна быть реализована логика: создание/удаление тела; навешивание коллайдера; шаг симуляции; выдача событий за кадр; луч возвращает попадание или `nullopt`; `bodyTransform` — трансформ тела.
- Сделай файлы `engine/physics/include/sky/physics/physics_world.hpp`, `engine/physics/src/physics_world.cpp`.
- В файле должен быть `class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService` (`setGravity`, `setBodyVelocity`, `bodyVelocity`, `setBodyTransform`) + фабрика `createPhysicsWorld()`.
- В `.cpp` должна быть реализована логика: интегрирование гравитации в `step`; `detectAndResolve` (расталкивание AABB, сбор `CollisionEvent`); `sampleHeightfield`/`resolveHeightfields`; `raycast` по коллайдерам и высотной поверхности.
**На выходе должно получиться:** `physics.hpp`, `physics_world.hpp`, `physics_world.cpp` (`sky::physics`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности.

## feature/vulkan-offscreen (E2)
**Цель фичи:** инициализировать Vulkan и отрисовать треугольник в закадровую цель с чтением кадра.
**Описание фичи (для чего):** первый реальный кадр + `readbackFrame()` — основа верификации всего проекта (скриншоты). Зависит от `feature/render-contract`.
**Пошаговое описание действий:**
- Сделай файл `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`.
- В файле должны быть `class VulkanRenderer : rendering::IRenderer` (`ready`, `readbackFrame`, `frameWidth`, `frameHeight`) и `createVulkanRenderer(width, height)`.
- В методах должна быть реализована логика: `ready` — инициализирован ли; `readbackFrame` — пиксели последнего кадра (RGBA); фабрика создаёт закадровый рендерер или `nullptr`, если Vulkan недоступен.
- Сделай файл `engine/rendering_vulkan/src/vulkan_renderer.cpp`.
- В файле должны быть `initInstanceAndDevice`, `initOffscreenTarget`, `initPipeline`, `renderFrame`, `readbackFrame` и вспомогательные `findMemoryType`/`createImage`/`createBuffer`/`createShader`.
- В методах должна быть реализована логика: создание инстанса/устройства/очереди; цвет (RGBA)+глубина (D32), проход и кадровый буфер; конвейер с вершиной позиция+нормаль+uv; запись команд и `vkCmdDraw` треугольника; копирование цвета в CPU-буфер.
- Сделай файлы `engine/rendering_vulkan/shaders/mesh.vert`, `mesh.frag` (компилируются в `mesh.vert.spv.h`/`mesh.frag.spv.h`).
**На выходе должно получиться:** `vulkan_backend.hpp`, `vulkan_renderer.cpp`, шейдеры + встроенные `.spv.h`; формируется `triangle.png`.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** на `triangle.png` центральный пиксель отличается от углового; `readbackFrame()` возвращает непустой массив.
