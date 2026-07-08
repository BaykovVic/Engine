# День 6 — синхронизация физики, тесты Vulkan

Фичи дня (в порядке реализации). Одна фича = ветка `feature/<название>` = один PR в `develop`.

---

## feature/physics-object-sync (E4)
**Цель фичи:** синхронизация физических тел с объектами объектного мира до/после шага.
**Описание фичи (для чего):** переносит трансформы объектов в тела перед шагом и результаты обратно после — согласованное движение; зависит от `physics-world` и `object-model`.
**Пошаговое описание действий:**
- Дополни `engine/physics/include/sky/physics/physics_world.hpp` (+ реализация).
- В файле должен быть `class ObjectPhysicsSync : IPhysicsSyncContract` (`bind`, `unbind`, `pushKinematicState`, `pullSimulationResults`) + фабрика `createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)`.
- В методах должна быть реализована логика: `bind`/`unbind` связывают тело с объектом; `pushKinematicState` до шага переносит трансформы объектов в тела; `pullSimulationResults` после шага — обратно.
- Сделай файл `tests/physics_tests.cpp` — проверки падения, куба на полу, тела на heightfield, синхронизации объекта.
**На выходе должно получиться:** дополненный `physics_world.hpp` с `ObjectPhysicsSync`; `tests/physics_tests.cpp`; библиотека `sky_physics` собрана, тест зелёный.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** падение ≈4.9 м; куб на полу; тело на heightfield; привязанный объект синхронно опускается в объектном мире.

## feature/vulkan-tests (E2)
**Цель фичи:** проверить Vulkan-рендерер на программном драйвере lavapipe.
**Описание фичи (для чего):** автотест закадрового рендера без видеокарты (в CI); зависит от `vulkan-offscreen`.
**Пошаговое описание действий:**
- Сделай файл `tests/vulkan_tests.cpp`.
- В файле должна быть реализована логика: `ready()` истинно, кадр рендерится, `readbackFrame()` непустой, центральный пиксель отличается от углового.
**На выходе должно получиться:** библиотека `sky_rendering_vulkan` собрана; тест `vulkan_tests` зелёный на lavapipe.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `triangle.png` — центр ≠ угол; `readbackFrame()` непустой.
