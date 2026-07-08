# День 6 — синхронизация физики, тесты Vulkan

## feature/physics-object-sync

Цель фичи: синхронизация физических тел с объектами объектного мира до и после шага.
Описание фичи (для чего): переносит трансформы объектов в тела перед шагом и результаты обратно после — согласованное движение; зависит от physics-world и object-model. Контур E4.
Пошаговое описание действий:
Сделай файл engine/physics/include/sky/physics/physics_world.hpp (дополнение)
В файле engine/physics/include/sky/physics/physics_world.hpp должны быть class ObjectPhysicsSync : IPhysicsSyncContract (bind, unbind, pushKinematicState, pullSimulationResults) и фабрика createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&).
В методах должна быть реализована логика: bind/unbind связывают тело с объектом; pushKinematicState до шага переносит трансформы объектов в тела; pullSimulationResults после шага переносит результат обратно.
Сделай файл tests/physics_tests.cpp
В файле tests/physics_tests.cpp должны быть проверки падения, куба на полу, тела на heightfield и синхронизации объекта.
В тесте должна быть реализована логика: тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на heightfield; привязанный объект синхронно опускается.
На выходе должно получиться:
- дополненный engine/physics/include/sky/physics/physics_world.hpp с ObjectPhysicsSync и его реализация
- tests/physics_tests.cpp
- собранная библиотека sky_physics; тест physics_tests зелёный
КРИТЕРИЙ ПРАВИЛЬНОСТИ: падение ≈4.9 м; куб на полу; тело на heightfield; привязанный объект синхронно опускается в объектном мире.

## feature/vulkan-tests

Цель фичи: проверить Vulkan-рендерер на программном драйвере lavapipe.
Описание фичи (для чего): автотест закадрового рендера без видеокарты (в CI); зависит от vulkan-offscreen. Контур E2.
Пошаговое описание действий:
Сделай файл tests/vulkan_tests.cpp
В файле tests/vulkan_tests.cpp должна быть проверка рендера на lavapipe.
В тесте должна быть реализована логика: ready() истинно, кадр рендерится, readbackFrame() непустой, центральный пиксель отличается от углового.
На выходе должно получиться:
- tests/vulkan_tests.cpp
- собранная библиотека sky_rendering_vulkan; тест vulkan_tests зелёный на lavapipe
КРИТЕРИЙ ПРАВИЛЬНОСТИ: на triangle.png центральный пиксель отличается от углового; readbackFrame() возвращает непустой массив.
