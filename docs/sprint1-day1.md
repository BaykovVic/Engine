# Спринт 1. День 1

## Контур E1 (Ядро и данные)

feature/math-and-handles

Цель фичи: математика и типобезопасные идентификаторы — фундамент, от которого зависят все.
Описание фичи (для чего): Vec3/Quat/Transform и Handle используют все модули; заголовок math.hpp отдаётся первым коммитом — по нему стартуют E2 и E4.
Пошаговое описание действий:
Сделай файл engine/core/include/sky/core/math.hpp
В файле engine/core/include/sky/core/math.hpp должны быть структуры Vec3{x,y,z}, Quat{x,y,z,w}, Transform{position,rotation,scale} и функции constexpr Vec3 operator+(const Vec3& a, const Vec3& b), constexpr Vec3 operator*(const Vec3& a, float s), constexpr Quat operator*(const Quat& a, const Quat& b), constexpr Vec3 rotate(const Quat& q, const Vec3& v), constexpr Transform compose(const Transform& parent, const Transform& child), constexpr Quat conjugate(const Quat& q), constexpr Transform invCompose(const Transform& parent, const Transform& world)
В методах должна быть реализована логика: покомпонентное сложение векторов; масштабирование вектора числом; композиция двух поворотов (сначала b, потом a); rotate поворачивает вектор кватернионом; compose переводит локальный трансформ ребёнка в систему координат родителя; conjugate — сопряжённый кватернион; invCompose — обратная к compose (мировой трансформ в локальный относительно родителя).
Сделай файл engine/core/include/sky/core/handle.hpp
В файле engine/core/include/sky/core/handle.hpp должен быть template <typename Tag> struct Handle { std::uint64_t value; … } с методами isValid(), invalid(), operator==
В методах должна быть реализована логика: типобезопасный идентификатор — разные теги (ObjectTag, ComponentTag) дают несовместимые типы.
На выходе должно получиться:
- engine/core/include/sky/core/math.hpp
- engine/core/include/sky/core/handle.hpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: rotate(поворот 90° вокруг Y, {0,0,1}) ≈ {1,0,0} (±1e-5); invCompose(parent, compose(parent, child)) == child.

## Контур E2 (Рендеринг)

feature/render-contract

Цель фичи: общий (не только Vulkan) контракт рендера — поток команд и интерфейс рендерера.
Описание фичи (для чего): единый контракт, от которого зависят все бэкенды; остальные фичи опираются на интерфейсы, а не на реализацию.
Пошаговое описание действий:
Сделай файл engine/rendering/include/sky/rendering/rendering.hpp
В файле engine/rendering/include/sky/rendering/rendering.hpp должны быть enum class RenderCommandType { BeginFrame, SetViewport, SetCamera, AddLight, SetSky, BindPipeline, DrawMesh, EndFrame }, enum class LightType { Directional = 0, Point = 1 }, struct RenderCommand { RenderCommandType type; core::Transform transform; core::Vec3 color; float fovDegrees; float orthoHeight; … }, class IRenderer (backendName, attachSurface, submit, renderFrame), class IRenderResourceFactory (createMeshFromData, createTextureFromData, destroy)
В методах должна быть реализована логика: RenderCommand — одна backend-независимая команда (поля используются по-разному в зависимости от type); backendName возвращает имя бэкенда; attachSurface привязывает поверхность; submit принимает поток команд кадра; renderFrame рисует накопленный кадр; createMeshFromData загружает меш (позиция+нормаль+uv), createTextureFromData загружает текстуру, destroy освобождает ресурс.
Сделай файл engine/rendering/include/sky/rendering/renderer_registry.hpp
В файле engine/rendering/include/sky/rendering/renderer_registry.hpp должны быть struct BackendInit { … }, тип RendererFactory, class IRendererRegistry (registerBackend, create), std::unique_ptr<IRendererRegistry> createRendererRegistry()
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
КРИТЕРИЙ ПРАВИЛЬНОСТИ: null-рендерер регистрируется в реестре и создаётся по имени; submit + renderFrame увеличивают счётчики кадров/команд. Зависимость: core::Vec3/core::Transform из feature/math-and-handles.

## Контур E3 (Редактор .NET)

feature/editor-shell

Цель фичи: каркас приложения редактора на Avalonia (.NET 8) с окном «Sky Engine».
Описание фичи (для чего): проект редактора и первичное окно, без которого нет панелей и вызовов движка; C-интерфейс подключается со второй фичи контура.
Пошаговое описание действий:
Сделай файл editor/avalonia/SkyEditor.csproj
В файле editor/avalonia/SkyEditor.csproj должно быть описание проекта .NET 8 с пакетами Avalonia
В файле должна быть реализована логика: конфигурация проекта .NET 8 и зависимости Avalonia.
Сделай файл editor/avalonia/Program.cs
В файле editor/avalonia/Program.cs должен быть static int Main(string[] args)
В методе должна быть реализована логика: точка входа; ветка --screenshot (headless); возвращает код выхода.
Сделай файл editor/avalonia/App.axaml.cs
В файле editor/avalonia/App.axaml.cs должен быть класс App с методом OnFrameworkInitializationCompleted()
В методе должна быть реализована логика: приложение Avalonia открывает MainWindow.
Сделай файл editor/avalonia/MainWindow.axaml.cs
В файле editor/avalonia/MainWindow.axaml.cs должен быть класс MainWindow с меню (File/Edit/GameObject) и обработчиками пунктов
В классе должна быть реализована логика: окно «Sky Engine» с меню и обработчиками пунктов меню.
На выходе должно получиться:
- editor/avalonia/SkyEditor.csproj
- editor/avalonia/Program.cs
- editor/avalonia/App.axaml.cs
- editor/avalonia/MainWindow.axaml.cs
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build editor/avalonia — 0 ошибок; окно «Sky Engine» открывается с меню.

## Контур E4 (Рантайм и физика)

feature/physics-world

Цель фичи: физический мир — тела, коллайдеры, гравитация, столкновения, высотная поверхность, луч.
Описание фичи (для чего): ядро симуляции; контракты управления и запросов плюс конкретная реализация. Бэкенд заменяем по контракту.
Пошаговое описание действий:
Сделай файл engine/physics/include/sky/physics/physics.hpp
В файле engine/physics/include/sky/physics/physics.hpp должны быть типы RigidBodyDesc{type,mass,transform}, enum class ColliderShape{Box,Sphere,Capsule,TerrainHeightfield}, ColliderDesc{shape,halfExtents,radius,heightfield}, HeightfieldDesc{resolution,scale,heights}, RaycastHit{collider,point,normal,distance}, CollisionEvent{first,second}; class IPhysicsWorld (createBody(const RigidBodyDesc& desc), destroyBody(RigidBodyHandle body), attachCollider(RigidBodyHandle body, const ColliderDesc& desc), step(double fixedDeltaSeconds), drainCollisionEvents()); class IPhysicsQueryService (raycast(const core::Vec3& origin, const core::Vec3& direction, float maxDistance) const, bodyTransform(RigidBodyHandle body) const)
В методах должна быть реализована логика: createBody создаёт тело (тип/масса/поза), destroyBody удаляет; attachCollider навешивает коллайдер, step — шаг симуляции, drainCollisionEvents — события за кадр; raycast пускает луч и возвращает попадание или nullopt; bodyTransform — трансформ тела.
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
КРИТЕРИЙ ПРАВИЛЬНОСТИ: тело за 1 с падает ≈4.9 м под гравитацией; куб замирает на полу (расталкивание AABB); луч попадает в коллайдер. Зависимость: core::Vec3/core::Transform из feature/math-and-handles.

## Контур E5 (Пайплайн и QA)

feature/build-system

Цель фичи: скелет сборки CMake для движка, редактора, плеера и тестов.
Описание фичи (для чего): единый механизм сборки модулей; в него остальные контуры добавляют свои модули по мере готовности.
Пошаговое описание действий:
Сделай файл engine/CMakeLists.txt
В файле engine/CMakeLists.txt должна быть функция sky_add_module(NAME DIR sources…)
В функции должна быть реализована логика: создаёт статическую библиотеку с public-include-путями и стандартом C++20; регистрирует модули и связи между ними.
Сделай файл CMakeLists.txt (корневой)
В файле CMakeLists.txt должны быть объявление проекта, опции и add_subdirectory для движка/редактора/плеера/тестов
В файле должна быть реализована логика: конфигурация проекта и подключение подкаталогов.
Сделай файл tests/CMakeLists.txt
В файле tests/CMakeLists.txt должна быть регистрация тестовых целей
В файле должна быть реализована логика: сборка тестов и их регистрация в ctest.
На выходе должно получиться:
- CMakeLists.txt
- engine/CMakeLists.txt
- tests/CMakeLists.txt
КРИТЕРИЙ ПРАВИЛЬНОСТИ: cmake -S . -B build && cmake --build build проходит с одним модулем (напр. sky_core); sky_add_module подключает следующий модуль одной строкой.

## Контур E6 (Data-oriented / ECS)

feature/ecs-core

Цель фичи: ECS-ядро — сущности, типизированные хранилища компонентов, планировщик систем и запросы.
Описание фичи (для чего): сущности держат компоненты в хранилищах по типу, системы обрабатывают их пачками; на этом ядре строятся все этапы контура.
Пошаговое описание действий:
Сделай файл engine/ecs/include/sky/ecs/ecs.hpp
В файле engine/ecs/include/sky/ecs/ecs.hpp должны быть struct EntityId { std::uint32_t index; std::uint32_t generation; }; class IEcsComponentStore (componentType, has, remove, count); class IEcsSystem (name, update); class IEcsWorld (createEntity, destroyEntity, isAlive, store); class IEcsSystemScheduler (registerSystem, unregisterSystem, tick); class IEcsQueryService (entitiesWith(std::set<std::type_index> types))
В методах должна быть реализована логика: EntityId — сущность с поколением (защита от повторного использования id); componentType/has/remove/count — хранилище одного типа; system update обрабатывает подходящие сущности; createEntity/destroyEntity/isAlive/store — мир сущностей; registerSystem/unregisterSystem/tick — планировщик; entitiesWith находит сущности с заданным набором компонентов.
Сделай файл engine/ecs/include/sky/ecs/ecs_world.hpp
В файле engine/ecs/include/sky/ecs/ecs_world.hpp должны быть class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService с template <typename T> TypedComponentStore<T>& storeFor() и std::unique_ptr<EcsWorld> createEcsWorld()
В методах должна быть реализована логика: storeFor<T>() — типобезопасный доступ к хранилищу компонента T (методы set(entity, value), get(entity)→T*).
Сделай файл engine/ecs/src/ecs_world.cpp
В файле engine/ecs/src/ecs_world.cpp должна быть реализация EcsWorld и createEcsWorld()
В методах должна быть реализована логика: учёт живых сущностей; destroyEntity удаляет компоненты во всех хранилищах; tick прогоняет системы; entitiesWith отбирает сущности со всеми указанными компонентами.
На выходе должно получиться:
- engine/ecs/include/sky/ecs/ecs.hpp
- engine/ecs/include/sky/ecs/ecs_world.hpp
- engine/ecs/src/ecs_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: создание/уничтожение сущности; storeFor<T>().set/get работают; entitiesWith({type}) возвращает только сущности с этим компонентом.
