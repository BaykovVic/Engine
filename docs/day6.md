# День 6

## E4 · `feature/physics-object-sync`

### Дополнение файла `engine/physics/include/sky/physics/physics_world.hpp` + реализация
- `class ObjectPhysicsSync : IPhysicsSyncContract`
  - `virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0` — связывает тело с объектом.
  - `virtual void unbind(RigidBodyHandle body) = 0` — разрывает связь.
  - `virtual void pushKinematicState() = 0` — до шага переносит трансформы объектов в тела.
  - `virtual void pullSimulationResults() = 0` — после шага переносит результат обратно.
- `std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)` — фабрика.

### Файл `tests/physics_tests.cpp`
Падение ≈4.9 м/с, куб на полу, тело на heightfield, синхронизация объекта.

**На выходе (артефакты):** библиотека `sky_physics` собрана; тест `physics_tests` зелёный.
**Критерий правильности этапа:** тело за 1 с падает ≈4.9 м; куб замирает на
полу; тело удерживается на высотной поверхности; привязанный объект синхронно
опускается в объектном мире.

---

## E2 · `feature/vulkan-tests`

### Файл `tests/vulkan_tests.cpp`
На lavapipe: `ready()` истинно, кадр рендерится, `readbackFrame()` непустой,
центральный пиксель отличается от углового.

**На выходе должно получиться (список артефактов):**
- Библиотека `sky_rendering_vulkan` собрана; формируется `triangle.png`.
- Тест `vulkan_tests` зелёный на lavapipe.

**Критерий правильности этапа:** на `triangle.png` центральный пиксель
отличается от углового; `readbackFrame()` возвращает непустой массив.
