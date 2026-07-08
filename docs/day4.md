# День 4 — компоненты, физика, offscreen-рендер

## feature/component-model

Цель фичи: компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.
Описание фичи (для чего): поля описываются данными (variant-map), что позже бесплатно даёт сериализацию, отмену и параметры скриптов. Контур E1.
Пошаговое описание действий:
Сделай файл engine/component/include/sky/component/component_model.hpp
В файле engine/component/include/sky/component/component_model.hpp должны быть using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>, ComponentDescriptor, IComponentRegistry (registerComponentType, availableTypes), IComponentAttachmentService (attach, detach), IComponentQueryService (componentsOf, descriptorOf, ownerOf).
В методах должна быть реализована логика: регистрация типов; навешивание и снятие компонентов; запросы «компоненты объекта / описание типа / владелец».
Сделай файл engine/component/include/sky/component/component_world.hpp
В файле engine/component/include/sky/component/component_world.hpp должны быть класс ComponentWorld с методами setField, field, fields, detachAllFrom и фабрика createComponentWorld().
В методах должна быть реализована логика: запись/чтение поля по имени; выдача всей карты полей; снятие всех компонентов объекта.
Сделай файл engine/component/src/component_world.cpp
В файле engine/component/src/component_world.cpp должна быть реализация ComponentWorld.
В методах должна быть реализована логика: хранение компонентов и их полей, привязанных к объекту-владельцу.
На выходе должно получиться:
- engine/component/include/sky/component/component_model.hpp
- engine/component/include/sky/component/component_world.hpp
- engine/component/src/component_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: поле каждого из 5 типов записывается и читается без потерь.

## feature/physics-world

Цель фичи: физический мир — тела, коллайдеры, гравитация, столкновения, heightfield, луч.
Описание фичи (для чего): ядро симуляции; контракты управления и запросов плюс конкретная реализация. Без неё нет физических взаимодействий. Контур E4.
Пошаговое описание действий:
Сделай файл engine/physics/include/sky/physics/physics.hpp
В файле engine/physics/include/sky/physics/physics.hpp должны быть типы RigidBodyDesc, ColliderShape, ColliderDesc, HeightfieldDesc, RaycastHit, CollisionEvent; class IPhysicsWorld (createBody, destroyBody, attachCollider, step, drainCollisionEvents); class IPhysicsQueryService (raycast, bodyTransform).
В методах должна быть реализована логика: создание/удаление тела; навешивание коллайдера; шаг симуляции; выдача событий за кадр; луч возвращает попадание или nullopt; bodyTransform — трансформ тела.
Сделай файл engine/physics/include/sky/physics/physics_world.hpp
В файле engine/physics/include/sky/physics/physics_world.hpp должны быть class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService (setGravity, setBodyVelocity, bodyVelocity, setBodyTransform) и фабрика createPhysicsWorld().
В методах должна быть реализована логика: задать гравитацию; задать/прочитать скорость тела; переставить тело.
Сделай файл engine/physics/src/physics_world.cpp
В файле engine/physics/src/physics_world.cpp должна быть реализация PhysicsWorld.
В методах должна быть реализована логика: интегрирование гравитации в step; detectAndResolve (расталкивание AABB, сбор CollisionEvent); sampleHeightfield/resolveHeightfields; raycast по коллайдерам и высотной поверхности.
На выходе должно получиться:
- engine/physics/include/sky/physics/physics.hpp
- engine/physics/include/sky/physics/physics_world.hpp
- engine/physics/src/physics_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности.

## feature/vulkan-offscreen

Цель фичи: инициализировать Vulkan и отрисовать треугольник в закадровую цель с чтением кадра.
Описание фичи (для чего): первый реальный кадр и readbackFrame() — основа верификации всего проекта (скриншоты). Зависит от feature/render-contract. Контур E2.
Пошаговое описание действий:
Сделай файл engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp
В файле engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp должны быть class VulkanRenderer : rendering::IRenderer (ready, readbackFrame, frameWidth, frameHeight) и функция createVulkanRenderer(width, height).
В методах должна быть реализована логика: ready — инициализирован ли рендерер; readbackFrame — пиксели последнего кадра (RGBA); фабрика создаёт закадровый рендерер или nullptr, если Vulkan недоступен.
Сделай файл engine/rendering_vulkan/src/vulkan_renderer.cpp
В файле engine/rendering_vulkan/src/vulkan_renderer.cpp должны быть методы initInstanceAndDevice, initOffscreenTarget, initPipeline, renderFrame, readbackFrame и вспомогательные findMemoryType, createImage, createBuffer, createShader.
В методах должна быть реализована логика: создание инстанса/устройства/очереди; цель цвет (RGBA) + глубина (D32), проход и кадровый буфер; конвейер с вершиной позиция+нормаль+uv; запись команд и vkCmdDraw треугольника; копирование цвета в CPU-буфер.
Сделай файл engine/rendering_vulkan/shaders/mesh.vert
В файле engine/rendering_vulkan/shaders/mesh.vert должен быть минимальный вершинный шейдер (компилируется в mesh.vert.spv.h).
В шейдере должна быть реализована логика: трансформация вершины позиция+нормаль+uv.
Сделай файл engine/rendering_vulkan/shaders/mesh.frag
В файле engine/rendering_vulkan/shaders/mesh.frag должен быть минимальный фрагментный шейдер (компилируется в mesh.frag.spv.h).
В шейдере должна быть реализована логика: вывод цвета фрагмента.
На выходе должно получиться:
- engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp
- engine/rendering_vulkan/src/vulkan_renderer.cpp
- engine/rendering_vulkan/shaders/mesh.vert, mesh.frag (и встроенные mesh.vert.spv.h, mesh.frag.spv.h)
КРИТЕРИЙ ПРАВИЛЬНОСТИ: на triangle.png центральный пиксель отличается от углового; readbackFrame() возвращает непустой массив.
