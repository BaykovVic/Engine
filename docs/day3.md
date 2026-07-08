# День 3 — объектная модель, контракт рендера, ядро ECS

## feature/object-model

Цель фичи: единственный владелец иерархии сцены и трансформов.
Описание фичи (для чего): объекты, их дерево и трансформы; остальные модули держат только хендлы. Контур E1.
Пошаговое описание действий:
Сделай файл engine/object/include/sky/object/object_model.hpp
В файле engine/object/include/sky/object/object_model.hpp должны быть IObjectFactory (createObject, destroyObject), IObjectHierarchyAccess (setParent, parentOf, childrenOf, setLocalTransform, localTransform, worldTransform), IObjectQueryService (exists, nameOf, findByName) и свободная функция setWorldTransform.
В методах должна быть реализована логика: создание/удаление поддерева; смена родителя; worldTransform — композиция локальных вверх по цепочке; setWorldTransform пересчитывает локальный через invCompose.
Сделай файл engine/object/include/sky/object/object_world.hpp
В файле engine/object/include/sky/object/object_world.hpp должны быть класс ObjectWorld (наследует три контракта), метод renameObject и фабрика createObjectWorld().
В методах должна быть реализована логика: единый владелец мира объектов; renameObject меняет имя.
Сделай файл engine/object/src/object_world.cpp
В файле engine/object/src/object_world.cpp должна быть реализация ObjectWorld.
В методах должна быть реализована логика: хранилище id → {локальный трансформ, родитель, дети, имя}; worldTransform = compose вверх; destroyObject рекурсивно удаляет поддерево.
На выходе должно получиться:
- engine/object/include/sky/object/object_model.hpp
- engine/object/include/sky/object/object_world.hpp
- engine/object/src/object_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире = (0,0,-1).

## feature/render-contract

Цель фичи: общий backend-независимый контракт рендера — поток команд и интерфейсы рендерера.
Описание фичи (для чего): единый контракт, от которого зависят все бэкенды (Vulkan/OpenGL); остальные фичи опираются на интерфейсы, а не на реализацию. Контур E2.
Пошаговое описание действий:
Сделай файл engine/rendering/include/sky/rendering/rendering.hpp
В файле engine/rendering/include/sky/rendering/rendering.hpp должны быть enum class RenderCommandType {BeginFrame,SetViewport,SetCamera,AddLight,SetSky,BindPipeline,DrawMesh,EndFrame}, enum class LightType {Directional,Point}, struct RenderCommand, class IRenderer (backendName, attachSurface, submit, renderFrame), class IRenderResourceFactory (createMeshFromData, createTextureFromData, destroy).
В методах должна быть реализована логика: RenderCommand — одна backend-независимая команда (поля читаются по type); submit принимает поток команд; renderFrame рисует накопленный кадр; фабрика ресурсов грузит меш/текстуру и возвращает хэндл.
Сделай файл engine/rendering/include/sky/rendering/renderer_registry.hpp
В файле engine/rendering/include/sky/rendering/renderer_registry.hpp должны быть struct BackendInit, тип RendererFactory, class IRendererRegistry (registerBackend, create) и функция createRendererRegistry().
В методах должна быть реализована логика: регистрация фабрики по имени; создание рендерера по имени; реестр создаётся с предрегистрированным "null".
Сделай файл engine/rendering/include/sky/rendering/null_renderer.hpp
В файле engine/rendering/include/sky/rendering/null_renderer.hpp должны быть класс NullRenderer (frameCount, commandsInLastFrame, liveResourceCount) и функции createNullRenderer, createOffscreenSurface.
В методах должна быть реализована логика: пустой рендерер считает кадры/команды и ничего не рисует.
Сделай файл engine/rendering/src/null_renderer.cpp
В файле engine/rendering/src/null_renderer.cpp должна быть реализация NullRenderer и фабрик.
В методах должна быть реализована логика: submit копит команды; renderFrame фиксирует их число, очищает буфер и увеличивает счётчик кадров; ресурсы учитываются в множестве.
Сделай файл engine/rendering/src/renderer_registry.cpp
В файле engine/rendering/src/renderer_registry.cpp должна быть реализация реестра.
В методах должна быть реализована логика: хранение фабрик по имени; предрегистрация "null"; create ищет фабрику по имени.
На выходе должно получиться:
- engine/rendering/include/sky/rendering/rendering.hpp
- engine/rendering/include/sky/rendering/renderer_registry.hpp
- engine/rendering/include/sky/rendering/null_renderer.hpp
- engine/rendering/src/null_renderer.cpp
- engine/rendering/src/renderer_registry.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: null-рендерер регистрируется и создаётся по имени; submit + renderFrame увеличивают счётчики кадров/команд.

## feature/ecs-core

Цель фичи: ядро ECS — сущности, типизированные хранилища, планировщик систем, запросы.
Описание фичи (для чего): сущности держат компоненты в хранилищах по типу, системы обрабатывают их пачками; на этом ядре строятся все этапы контура. Контур E6.
Пошаговое описание действий:
Сделай файл engine/ecs/include/sky/ecs/ecs.hpp
В файле engine/ecs/include/sky/ecs/ecs.hpp должны быть struct EntityId, IEcsComponentStore (componentType, has, remove, count), IEcsSystem (name, update), IEcsWorld (createEntity, destroyEntity, isAlive, store), IEcsSystemScheduler (registerSystem, unregisterSystem, tick), IEcsQueryService (entitiesWith).
В методах должна быть реализована логика: сущность с защитой от повторного id; хранилище одного типа; система обрабатывает подходящие сущности; мир создаёт/уничтожает сущности; планировщик прогоняет системы; запрос находит сущности с набором компонентов.
Сделай файл engine/ecs/include/sky/ecs/ecs_world.hpp
В файле engine/ecs/include/sky/ecs/ecs_world.hpp должны быть класс EcsWorld (наследует IEcsWorld, IEcsSystemScheduler, IEcsQueryService), шаблон storeFor<T>() (методы set, get) и фабрика createEcsWorld().
В методах должна быть реализована логика: типобезопасный доступ к хранилищу T; set пишет компонент, get возвращает T*.
Сделай файл engine/ecs/src/ecs_world.cpp
В файле engine/ecs/src/ecs_world.cpp должна быть реализация EcsWorld и createEcsWorld().
В методах должна быть реализована логика: учёт живых сущностей; destroyEntity удаляет компоненты во всех хранилищах; tick вызывает update систем; entitiesWith отбирает сущности со всеми указанными компонентами.
На выходе должно получиться:
- engine/ecs/include/sky/ecs/ecs.hpp
- engine/ecs/include/sky/ecs/ecs_world.hpp
- engine/ecs/src/ecs_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: создание/уничтожение сущности; система обновляет только сущности с нужным компонентом; запрос по типам корректен.
