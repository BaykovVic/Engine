# День 6

## feature/physics-object-sync
Цель фичи: явная синхронизация физического мира с объектным до и после шага (контур E4).
Описание фичи (для чего): переносит трансформы объектов в тела перед шагом и результаты обратно после — без неявного двойного владения.
Пошаговое описание действий:
Сделай файл engine/physics/include/sky/physics/physics_world.hpp (дополнение) и его реализацию
В файле engine/physics/include/sky/physics/physics_world.hpp должны быть class ObjectPhysicsSync : IPhysicsSyncContract (bind(RigidBodyHandle body, object::ObjectHandle object), unbind(RigidBodyHandle body), pushKinematicState(), pullSimulationResults()) и std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)
В методах должна быть реализована логика: bind связывает тело с объектом, unbind разрывает связь; pushKinematicState до шага переносит трансформы объектов в тела; pullSimulationResults после шага переносит результат обратно.
Сделай файл tests/physics_tests.cpp
В файле tests/physics_tests.cpp должны быть проверки: падение ≈4.9 м/с, куб на полу, тело на heightfield, синхронизация объекта
В тесте должна быть реализована логика: тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности; привязанный объект синхронно опускается.
На выходе должно получиться:
- дополненный engine/physics/include/sky/physics/physics_world.hpp с ObjectPhysicsSync и его реализация
- tests/physics_tests.cpp
- библиотека sky_physics собрана; тест physics_tests зелёный
КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап E4): тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности; привязанный объект синхронно опускается в объектном мире.

## feature/vulkan-tests
Цель фичи: проверить Vulkan-рендерер на программном драйвере lavapipe (контур E2).
Описание фичи (для чего): автотест закадрового рендера без видеокарты (в CI).
Пошаговое описание действий:
Сделай файл tests/vulkan_tests.cpp
В файле tests/vulkan_tests.cpp должна быть проверка на lavapipe
В тесте должна быть реализована логика: ready() истинно, кадр рендерится, readbackFrame() непустой, центральный пиксель отличается от углового.
На выходе должно получиться:
- tests/vulkan_tests.cpp
- библиотека sky_rendering_vulkan собрана; формируется triangle.png; тест vulkan_tests зелёный на lavapipe
КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап E2): на triangle.png центральный пиксель отличается от углового; readbackFrame() возвращает непустой массив.
