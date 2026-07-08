# День 3

## feature/object-model
Цель фичи: единственный владелец иерархии сцены и трансформов (контур E1).
Описание фичи (для чего): объекты, их дерево и трансформы; остальные модули держат только хендлы.
Пошаговое описание действий:
Сделай файл engine/object/include/sky/object/object_model.hpp
В файле engine/object/include/sky/object/object_model.hpp должны быть using ObjectHandle = core::Handle<ObjectTag>; class IObjectFactory (createObject(const std::string& name), destroyObject(ObjectHandle object)); class IObjectHierarchyAccess (setParent, parentOf, childrenOf, setLocalTransform, localTransform, worldTransform); class IObjectQueryService (exists, nameOf, findByName); свободная inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)
В методах должна быть реализована логика: createObject создаёт объект, destroyObject удаляет объект и его поддерево; setParent перевешивает child под parent (invalid = корень); worldTransform — композиция локальных вверх по цепочке; setWorldTransform задаёт мировой трансформ, пересчитывая локальный через invCompose.
Сделай файл engine/object/include/sky/object/object_world.hpp
В файле engine/object/include/sky/object/object_world.hpp должны быть class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService с методом renameObject(ObjectHandle object, const std::string& name) и фабрика std::unique_ptr<ObjectWorld> createObjectWorld()
В методах должна быть реализована логика: единый владелец мира объектов; renameObject переименовывает объект.
Сделай файл engine/object/src/object_world.cpp
В файле engine/object/src/object_world.cpp должна быть реализация ObjectWorld
В методах должна быть реализована логика: хранилище id → {локальный трансформ, родитель, дети, имя}; worldTransform = compose вверх; destroyObject рекурсивно удаляет поддерево.
На выходе должно получиться:
- engine/object/include/sky/object/object_model.hpp
- engine/object/include/sky/object/object_world.hpp
- engine/object/src/object_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: поворот (0,0,1) на 90° вокруг Y = (1,0,0)±1e-5; ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире = (0,0,-1); поле каждого из 5 типов записывается и читается без потерь.

## feature/render-contract
Цель фичи: общий (не только Vulkan) контракт рендера — поток команд и интерфейс рендерера (контур E2).
Описание фичи (для чего): единый контракт, от которого зависят все бэкенды; остальные фичи опираются на интерфейсы, а не на реализацию.
Пошаговое описание действий:
Сделай файл engine/rendering/include/sky/rendering/rendering.hpp
В файле engine/rendering/include/sky/rendering/rendering.hpp должны быть enum class RenderCommandType { BeginFrame, SetViewport, SetCamera, AddLight, SetSky, BindPipeline, DrawMesh, EndFrame }, enum class LightType { Directional = 0, Point = 1 }, struct RenderCommand { RenderCommandType type; core::Transform transform; core::Vec3 color; float fovDegrees; float orthoHeight; … }, class IRenderer (backendName, attachSurface, submit, renderFrame), class IRenderResourceFactory (createMeshFromData, createTextureFromData, destroy)
В методах должна быть реализована логика: RenderCommandType — вид команды в потоке; RenderCommand — одна backend-независимая команда (поля используются по-разному в зависимости от type); backendName возвращает имя бэкенда; attachSurface привязывает поверхность вывода; submit принимает поток команд кадра; renderFrame рисует накопленный кадр; createMeshFromData загружает меш (позиция+нормаль+uv), createTextureFromData загружает текстуру, destroy освобождает ресурс.
Сделай файл engine/rendering/include/sky/rendering/renderer_registry.hpp
В файле engine/rendering/include/sky/rendering/renderer_registry.hpp должны быть struct BackendInit { … }, тип RendererFactory, class IRendererRegistry (registerBackend, create) и std::unique_ptr<IRendererRegistry> createRendererRegistry()
В методах должна быть реализована логика: registerBackend регистрирует фабрику бэкенда (возвращает успех); create возвращает рендерер по имени бэкенда; createRendererRegistry — фабрика реестра.
Сделай файл engine/rendering/include/sky/rendering/null_renderer.hpp
В файле engine/rendering/include/sky/rendering/null_renderer.hpp должен быть пустой рендерер (считает кадры/команды, ничего не рисует) — для тестов контракта
В методах должна быть реализована логика: подсчёт кадров и команд без реальной отрисовки.
Сделай файл engine/rendering/src/null_renderer.cpp
В файле engine/rendering/src/null_renderer.cpp должна быть реализация пустого рендерера
В методах должна быть реализована логика: подсчёт кадров/команд без отрисовки.
Сделай файл engine/rendering/src/renderer_registry.cpp
В файле engine/rendering/src/renderer_registry.cpp должна быть реализация реестра бэкендов
В методах должна быть реализована логика: registerBackend (хранение фабрик по имени), create (создание рендерера по имени), createRendererRegistry.
На выходе должно получиться:
- engine/rendering/include/sky/rendering/rendering.hpp
- engine/rendering/include/sky/rendering/renderer_registry.hpp
- engine/rendering/include/sky/rendering/null_renderer.hpp
- engine/rendering/src/null_renderer.cpp
- engine/rendering/src/renderer_registry.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап E2): на triangle.png центральный пиксель отличается от углового; readbackFrame() возвращает непустой массив.

## feature/ecs-core
Цель фичи: ECS-ядро — сущности, типизированные хранилища компонентов, планировщик систем и запросы (контур E6).
Описание фичи (для чего): сущности держат компоненты в хранилищах по типу, системы обрабатывают их пачками; на этом ядре строятся все этапы контура.
Пошаговое описание действий:
Сделай файл engine/ecs/include/sky/ecs/ecs.hpp
В файле engine/ecs/include/sky/ecs/ecs.hpp должны быть struct EntityId { std::uint32_t index; std::uint32_t generation; }; class IEcsComponentStore (componentType, has, remove, count); class IEcsSystem (name, update); class IEcsWorld (createEntity, destroyEntity, isAlive, store); class IEcsSystemScheduler (registerSystem, unregisterSystem, tick); class IEcsQueryService (entitiesWith(std::set<std::type_index> types))
В методах должна быть реализована логика: EntityId — сущность с поколением (защита от повторного использования id); componentType/has/remove/count — доступ к хранилищу одного типа; system update обрабатывает подходящие сущности; createEntity/destroyEntity/isAlive/store — мир сущностей; registerSystem/unregisterSystem/tick — планировщик; entitiesWith находит сущности с заданным набором компонентов.
Сделай файл engine/ecs/include/sky/ecs/ecs_world.hpp
В файле engine/ecs/include/sky/ecs/ecs_world.hpp должны быть class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService с template <typename T> TypedComponentStore<T>& storeFor() и фабрика std::unique_ptr<EcsWorld> createEcsWorld()
В методах должна быть реализована логика: storeFor<T>() — типобезопасный доступ к хранилищу компонента T (методы set(entity, value), get(entity)→T*).
Сделай файл engine/ecs/src/ecs_world.cpp
В файле engine/ecs/src/ecs_world.cpp должна быть реализация EcsWorld и createEcsWorld()
В методах должна быть реализована логика: учёт живых сущностей; destroyEntity удаляет компоненты во всех хранилищах; tick прогоняет системы; entitiesWith отбирает сущности со всеми указанными компонентами.
На выходе должно получиться:
- engine/ecs/include/sky/ecs/ecs.hpp
- engine/ecs/include/sky/ecs/ecs_world.hpp
- engine/ecs/src/ecs_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: создание/уничтожение сущности; система обновляет только сущности с нужным компонентом; запрос по типам корректен.
