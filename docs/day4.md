# День 4

## feature/component-model
Цель фичи: компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов (контур E1).
Описание фичи (для чего): поля, описываемые данными (variant-map), позже бесплатно дают сериализацию, отмену и параметры скриптов.
Пошаговое описание действий:
Сделай файл engine/component/include/sky/component/component_model.hpp
В файле engine/component/include/sky/component/component_model.hpp должны быть using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>; ComponentDescriptor{typeId, displayName, fields, category}; class IComponentRegistry (registerComponentType, availableTypes); class IComponentAttachmentService (attach, detach); class IComponentQueryService (componentsOf, descriptorOf, ownerOf)
В методах должна быть реализована логика: registerComponentType регистрирует тип, availableTypes — типы для меню Add Component; attach навешивает компонент (возвращает хэндл), detach снимает; componentsOf — компоненты объекта, descriptorOf — описание типа, ownerOf — объект-владелец.
Сделай файл engine/component/include/sky/component/component_world.hpp
В файле engine/component/include/sky/component/component_world.hpp должны быть class ComponentWorld с методами setField, field, fields, detachAllFrom и фабрика std::unique_ptr<ComponentWorld> createComponentWorld()
В методах должна быть реализована логика: setField записывает поле по имени; field возвращает значение или nullopt; fields возвращает всю карту полей; detachAllFrom снимает все компоненты объекта.
Сделай файл engine/component/src/component_world.cpp
В файле engine/component/src/component_world.cpp должна быть реализация ComponentWorld
В методах должна быть реализована логика: хранение компонентов и их полей, привязанных к объекту-владельцу.
На выходе должно получиться:
- engine/component/include/sky/component/component_model.hpp
- engine/component/include/sky/component/component_world.hpp
- engine/component/src/component_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: поворот (0,0,1) на 90° вокруг Y = (1,0,0)±1e-5; ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире = (0,0,-1); поле каждого из 5 типов записывается и читается без потерь.

## feature/physics-world
Цель фичи: физический мир — тела, коллайдеры, гравитация, столкновения, высотная поверхность, луч (контур E4).
Описание фичи (для чего): ядро симуляции; контракты управления и запросов плюс конкретная реализация. Бэкенд заменяем по контракту.
Пошаговое описание действий:
Сделай файл engine/physics/include/sky/physics/physics.hpp
В файле engine/physics/include/sky/physics/physics.hpp должны быть типы RigidBodyDesc{type,mass,transform}, enum class ColliderShape{Box,Sphere,Capsule,TerrainHeightfield}, ColliderDesc{shape,halfExtents,radius,heightfield}, HeightfieldDesc{resolution,scale,heights}, RaycastHit{collider,point,normal,distance}, CollisionEvent{first,second}; class IPhysicsWorld (createBody(const RigidBodyDesc& desc), destroyBody(RigidBodyHandle body), attachCollider(RigidBodyHandle body, const ColliderDesc& desc), step(double fixedDeltaSeconds), drainCollisionEvents()); class IPhysicsQueryService (raycast(const core::Vec3& origin, const core::Vec3& direction, float maxDistance) const, bodyTransform(RigidBodyHandle body) const)
В методах должна быть реализована логика: createBody создаёт тело (тип/масса/поза), destroyBody удаляет; attachCollider навешивает коллайдер (форма), step — шаг симуляции, drainCollisionEvents — события за кадр; raycast пускает луч (начало, направление, предел) и возвращает попадание или nullopt; bodyTransform — трансформ тела.
Сделай файл engine/physics/include/sky/physics/physics_world.hpp
В файле engine/physics/include/sky/physics/physics_world.hpp должны быть class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService (setGravity(const core::Vec3& gravity), setBodyVelocity(RigidBodyHandle body, const core::Vec3& velocity), bodyVelocity(RigidBodyHandle body) const, setBodyTransform(RigidBodyHandle body, const core::Transform& transform)) и std::unique_ptr<PhysicsWorld> createPhysicsWorld()
В методах должна быть реализована логика: setGravity задаёт гравитацию; setBodyVelocity задаёт скорость; bodyVelocity возвращает скорость; setBodyTransform переставляет тело.
Сделай файл engine/physics/src/physics_world.cpp
В файле engine/physics/src/physics_world.cpp должна быть реализация PhysicsWorld
В методах должна быть реализована логика: интегрирование гравитации в step; detectAndResolve (расталкивание AABB, сбор CollisionEvent); sampleHeightfield/resolveHeightfields (удержание на террейне); raycast по коллайдерам и высотной поверхности.
На выходе должно получиться:
- engine/physics/include/sky/physics/physics.hpp
- engine/physics/include/sky/physics/physics_world.hpp
- engine/physics/src/physics_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап E4): тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности; привязанный объект синхронно опускается в объектном мире.

## feature/vulkan-offscreen
Цель фичи: инициализация Vulkan, закадровая цель, конвейер, отрисовка треугольника, чтение кадра (контур E2).
Описание фичи (для чего): первый реальный кадр и чтение его в изображение — основа верификации всего проекта.
Пошаговое описание действий:
Сделай файл engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp
В файле engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp должны быть class VulkanRenderer : public rendering::IRenderer (bool ready() const, std::vector<std::uint8_t> readbackFrame(), std::uint32_t frameWidth() const, std::uint32_t frameHeight() const) и std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width, std::uint32_t height)
В методах должна быть реализована логика: ready — инициализировался ли рендерер; readbackFrame — пиксели последнего кадра (RGBA); frameWidth/frameHeight — размеры кадра; createVulkanRenderer создаёт закадровый рендерер или nullptr, если Vulkan недоступен.
Сделай файл engine/rendering_vulkan/src/vulkan_renderer.cpp
В файле engine/rendering_vulkan/src/vulkan_renderer.cpp должны быть приватные методы initInstanceAndDevice(), initOffscreenTarget(), initPipeline(), renderFrame(), readbackFrame() и вспомогательные findMemoryType, createImage, createBuffer, createShader
В методах должна быть реализована логика: initInstanceAndDevice создаёт VkInstance, выбирает устройство и графическую очередь, создаёт VkDevice; initOffscreenTarget — изображения цвета (RGBA) и глубины (D32), проход и кадровый буфер; initPipeline — конвейер (вершина позиция+нормаль+uv); renderFrame записывает команды, очищает, рисует треугольник (vkCmdDraw); readbackFrame копирует изображение цвета в CPU-буфер.
Сделай файл engine/rendering_vulkan/shaders/mesh.vert
В файле engine/rendering_vulkan/shaders/mesh.vert должен быть минимальный вершинный шейдер (компилируется glslangValidator -V в mesh.vert.spv.h)
В шейдере должна быть реализована логика: минимальный вершинный шейдер (позже дорастёт до PBR).
Сделай файл engine/rendering_vulkan/shaders/mesh.frag
В файле engine/rendering_vulkan/shaders/mesh.frag должен быть минимальный фрагментный шейдер (компилируется glslangValidator -V в mesh.frag.spv.h)
В шейдере должна быть реализована логика: минимальный фрагментный шейдер (позже дорастёт до PBR).
На выходе должно получиться:
- engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp
- engine/rendering_vulkan/src/vulkan_renderer.cpp
- engine/rendering_vulkan/shaders/mesh.vert, mesh.frag (и встроенные mesh.vert.spv.h, mesh.frag.spv.h)
КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап E2): на triangle.png центральный пиксель отличается от углового; readbackFrame() возвращает непустой массив.
