# День 3 — объектная модель, контракт рендера, ядро ECS

Фичи дня (в порядке реализации). Одна фича = ветка `feature/<название>` = один PR в `develop`.

---

## feature/object-model (E1)
**Цель фичи:** единственный владелец иерархии сцены и трансформов.
**Описание фичи (для чего):** объекты, их дерево и трансформы; остальные модули держат только хендлы.
**Пошаговое описание действий:**
- Сделай файл `engine/object/include/sky/object/object_model.hpp` (`using ObjectHandle = core::Handle<ObjectTag>`).
- В файле должны быть `IObjectFactory` (`createObject`, `destroyObject`), `IObjectHierarchyAccess` (`setParent`, `parentOf`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`), `IObjectQueryService` (`exists`, `nameOf`, `findByName`) и свободная `setWorldTransform`.
- В методах должна быть реализована логика: создание/удаление поддерева; смена родителя; `worldTransform` — композиция локальных вверх по цепочке; `setWorldTransform` пересчитывает локальный через `invCompose`.
- Сделай файлы `engine/object/include/sky/object/object_world.hpp`, `engine/object/src/object_world.cpp`.
- В файле должен быть `class ObjectWorld` (наследует три контракта) + `renameObject`, фабрика `createObjectWorld()`.
- В методах должна быть реализована логика: хранилище `id → {локальный трансформ, родитель, дети, имя}`; `worldTransform` = `compose` вверх; `destroyObject` рекурсивно удаляет поддерево.
**На выходе должно получиться:** `object_model.hpp`, `object_world.hpp`, `object_world.cpp` (библиотека `sky_object`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** ребёнок `(1,0,0)` под родителем, повёрнутым на 90° вокруг Y, в мире = `(0,0,-1)`.

## feature/render-contract (E2)
**Цель фичи:** общий backend-независимый контракт рендера — поток команд и интерфейсы рендерера.
**Описание фичи (для чего):** единый контракт, от которого зависят все бэкенды (Vulkan/OpenGL); остальные фичи опираются на интерфейсы, а не на реализацию.
**Пошаговое описание действий:**
- Сделай файл `engine/rendering/include/sky/rendering/rendering.hpp`.
- В файле должны быть `enum class RenderCommandType {BeginFrame,SetViewport,SetCamera,AddLight,SetSky,BindPipeline,DrawMesh,EndFrame}`, `enum class LightType {Directional,Point}`, `struct RenderCommand`, `class IRenderer` (`backendName`, `attachSurface`, `submit`, `renderFrame`), `class IRenderResourceFactory` (`createMeshFromData`, `createTextureFromData`, `destroy`).
- В методах должна быть реализована логика: `RenderCommand` — одна backend-независимая команда (поля читаются по `type`); `submit` принимает поток команд, `renderFrame` рисует накопленный кадр; фабрика ресурсов грузит меш/текстуру и возвращает хэндл.
- Сделай файл `engine/rendering/include/sky/rendering/renderer_registry.hpp`.
- В файле должны быть `struct BackendInit`, тип `RendererFactory`, `class IRendererRegistry` (`registerBackend`, `create`), функция `createRendererRegistry()`.
- В методах должна быть реализована логика: регистрация фабрики по имени; создание рендерера по имени; реестр создаётся с предрегистрированным `"null"`.
- Сделай файлы `engine/rendering/include/sky/rendering/null_renderer.hpp`, `engine/rendering/src/null_renderer.cpp`, `engine/rendering/src/renderer_registry.cpp`.
- В файлах должна быть реализована логика: пустой рендерер считает кадры/команды и ничего не рисует; реализация реестра.
**На выходе должно получиться:** `rendering.hpp`, `renderer_registry.hpp`, `null_renderer.hpp`, `null_renderer.cpp`, `renderer_registry.cpp` (`sky::rendering`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** null-рендерер регистрируется и создаётся по имени; `submit` + `renderFrame` увеличивают счётчики кадров/команд.

## feature/ecs-core (E6)
**Цель фичи:** ядро ECS — сущности, типизированные хранилища, планировщик систем, запросы.
**Описание фичи (для чего):** сущности держат компоненты в хранилищах по типу, системы обрабатывают их пачками; на этом ядре строятся все этапы E6.
**Пошаговое описание действий:**
- Сделай файл `engine/ecs/include/sky/ecs/ecs.hpp`.
- В файле должны быть `struct EntityId`; `IEcsComponentStore` (`componentType`, `has`, `remove`, `count`); `IEcsSystem` (`name`, `update`); `IEcsWorld` (`createEntity`, `destroyEntity`, `isAlive`, `store`); `IEcsSystemScheduler` (`registerSystem`, `unregisterSystem`, `tick`); `IEcsQueryService` (`entitiesWith`).
- В методах должна быть реализована логика: сущность с защитой от повторного id; хранилище одного типа; система обрабатывает подходящие сущности; мир создаёт/уничтожает сущности; планировщик прогоняет системы; запрос находит сущности с набором компонентов.
- Сделай файлы `engine/ecs/include/sky/ecs/ecs_world.hpp`, `engine/ecs/src/ecs_world.cpp`.
- В файле должен быть `class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService`, шаблон `storeFor<T>()` (методы `set`, `get`) и фабрика `createEcsWorld()`.
- В методах должна быть реализована логика: типобезопасный доступ к хранилищу `T`; `set` пишет компонент, `get` возвращает `T*`; фабрика создаёт мир.
**На выходе должно получиться:** `ecs.hpp`, `ecs_world.hpp`, `ecs_world.cpp` (`sky::ecs`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** создание/уничтожение сущности; система обновляет только сущности с нужным компонентом; запрос по типам корректен.
