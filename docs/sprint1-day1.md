Спринт 1. День 1


feature/build-system

Исполнитель: E5 (Пайплайн и QA)
Порядок реализации: 1 (первой в дне — без сборки ничего не компилируется)
Зависимости: нет

Цель фичи: скелет сборки CMake для движка, редактора, плеера и тестов.
Описание фичи: единый механизм сборки модулей на C++20 с одной функцией регистрации модуля; остальные контуры подключают свои модули по мере готовности.

Общий порядок реализации фичи:
1. Завести функцию sky_add_module в engine/CMakeLists.txt.
2. Объявить корневой проект и подключить подкаталоги в CMakeLists.txt.
3. Завести регистрацию тестов в tests/CMakeLists.txt.

Файлы фичи:
1. engine/CMakeLists.txt
2. CMakeLists.txt
3. tests/CMakeLists.txt

==============================
Файл: engine/CMakeLists.txt
==============================

Назначение файла: описывает движковые модули и связи между ними единообразно.

Пошаговое описание действий:
1. Объявить функцию sky_add_module(NAME DIR sources…).
2. Внутри создать статическую библиотеку из sources.
3. Задать public-include-путь модуля и стандарт C++20.
4. Зарегистрировать сами модули и их зависимости через target_link_libraries.

Что должно быть в файле:

Структуры / классы / enum:
- нет.

Функции / методы:
- sky_add_module(NAME DIR sources…)

Логика функций / методов:
- sky_add_module — создаёт статическую библиотеку с public-include-путями и стандартом C++20; регистрирует модули и связи между ними.

Результат по файлу: подключение нового модуля — одна строка sky_add_module.

Критерий правильности по файлу:
1. sky_add_module создаёт собираемую библиотеку.
2. Модуль виден зависимым модулям по своему include-пути.

==============================
Файл: CMakeLists.txt
==============================

Назначение файла: корневой конфиг проекта.

Пошаговое описание действий:
1. Объявить project(...) и стандарт C++20.
2. Объявить опции сборки.
3. Подключить add_subdirectory для движка/редактора/плеера/тестов.

Что должно быть в файле:

Структуры / классы / enum:
- нет.

Функции / методы:
- нет (декларативный CMake).

Логика функций / методов:
- проект, опции, add_subdirectory для движка/редактора/плеера/тестов.

Результат по файлу: проект конфигурируется и собирается.

Критерий правильности по файлу:
1. cmake -S . -B build проходит без ошибок конфигурации.

==============================
Файл: tests/CMakeLists.txt
==============================

Назначение файла: регистрация тестовых целей в ctest.

Пошаговое описание действий:
1. Объявить сборку тестовых исполняемых файлов.
2. Слинковать их с движком.
3. Зарегистрировать через add_test.

Что должно быть в файле:

Структуры / классы / enum:
- нет.

Функции / методы:
- нет (декларативный CMake).

Логика функций / методов:
- сборка тестов и их регистрация в ctest.

Результат по файлу: тесты запускаются через ctest.

Критерий правильности по файлу:
1. ctest видит зарегистрированные тесты.

==============================

На выходе должно получиться:

Список артефактов фичи:
1. CMakeLists.txt
2. engine/CMakeLists.txt
3. tests/CMakeLists.txt

Общий критерий правильности:
1. cmake -S . -B build && cmake --build build проходит хотя бы с одним модулем (напр. sky_core).
2. sky_add_module подключает следующий модуль одной строкой.


feature/math-and-handles

Исполнитель: E1 (Ядро и данные)
Порядок реализации: 2 (заголовок math.hpp отдаётся первым коммитом — по нему стартуют E2 и E4)
Зависимости: нет

Цель фичи: математика и типобезопасные идентификаторы — фундамент, от которого зависят все.
Описание фичи: Vec3/Quat/Transform и Handle используют все модули (рендер, физика, сцена держат позы и хендлы).

Общий порядок реализации фичи:
1. Объявить структуры Vec3/Quat/Transform в math.hpp.
2. Реализовать constexpr-операторы и rotate/compose/conjugate/invCompose.
3. Объявить шаблон Handle<Tag> в handle.hpp.

Файлы фичи:
1. engine/core/include/sky/core/math.hpp
2. engine/core/include/sky/core/handle.hpp

==============================
Файл: engine/core/include/sky/core/math.hpp
==============================

Назначение файла: базовые математические типы и свободные функции над ними.

Пошаговое описание действий:
1. Объявить структуры Vec3{x,y,z}, Quat{x,y,z,w}, Transform{position,rotation,scale}.
2. Реализовать операторы над векторами и кватернионами.
3. Реализовать rotate, compose, conjugate, invCompose.
4. Пометить всё constexpr.

Что должно быть в файле:

Структуры / классы / enum:
- Vec3{x,y,z}
- Quat{x,y,z,w}
- Transform{position, rotation, scale}

Функции / методы:
- constexpr Vec3 operator+(const Vec3& a, const Vec3& b)
- constexpr Vec3 operator*(const Vec3& a, float s)
- constexpr Quat operator*(const Quat& a, const Quat& b)
- constexpr Vec3 rotate(const Quat& q, const Vec3& v)
- constexpr Transform compose(const Transform& parent, const Transform& child)
- constexpr Quat conjugate(const Quat& q)
- constexpr Transform invCompose(const Transform& parent, const Transform& world)

Логика функций / методов:
- operator+(Vec3,Vec3) — покомпонентное сложение векторов. Параметры: a, b. Возвращает: {a.x+b.x, …}.
- operator*(Vec3,float) — масштабирование вектора числом. Параметры: a, s. Возвращает: {a.x*s, …}.
- operator*(Quat,Quat) — композиция двух поворотов (сначала b, потом a). Параметры: a, b. Возвращает: результирующий поворот.
- rotate(q,v) — поворачивает вектор кватернионом. Параметры: q, v. Возвращает: повёрнутый вектор.
- compose(parent,child) — переводит локальный трансформ ребёнка в систему координат родителя. Параметры: parent, child. Возвращает: трансформ ребёнка в системе родителя.
- conjugate(q) — сопряжённый кватернион (обратный поворот для единичного). Параметры: q. Возвращает: {-q.x,-q.y,-q.z,q.w}.
- invCompose(parent,world) — обратная к compose (мировой трансформ в локальный относительно родителя). Параметры: parent, world. Возвращает: локальный трансформ.

Результат по файлу: заголовок с математикой, готовый к использованию E2 и E4.

Критерий правильности по файлу:
1. rotate(поворот 90° вокруг Y, {0,0,1}) ≈ {1,0,0} (±1e-5).
2. invCompose(parent, compose(parent, child)) == child.

==============================
Файл: engine/core/include/sky/core/handle.hpp
==============================

Назначение файла: типобезопасный идентификатор для пересечения границ модулей.

Пошаговое описание действий:
1. Объявить шаблон Handle<Tag> с полем value.
2. Добавить isValid(), invalid(), operator==.

Что должно быть в файле:

Структуры / классы / enum:
- template <typename Tag> struct Handle { std::uint64_t value; … }

Функции / методы:
- bool isValid() const
- static Handle invalid()
- operator==

Логика функций / методов:
- Handle — типобезопасный идентификатор; разные теги (ObjectTag, ComponentTag) дают несовместимые типы. isValid() — валиден ли; invalid() — недействительный хендл; operator== — сравнение.

Результат по файлу: заголовок с Handle<Tag>.

Критерий правильности по файлу:
1. Handle<A> и Handle<B> — несовместимые типы (не компилируется присваивание).

==============================

На выходе должно получиться:

Список артефактов фичи:
1. engine/core/include/sky/core/math.hpp
2. engine/core/include/sky/core/handle.hpp

Общий критерий правильности:
1. rotate(поворот 90° вокруг Y, {0,0,1}) ≈ {1,0,0} (±1e-5).
2. invCompose(parent, compose(parent, child)) == child.


feature/ecs-core

Исполнитель: E6 (Data-oriented / ECS)
Порядок реализации: 3
Зависимости: sky_core (сборка модуля)

Цель фичи: ECS-ядро — сущности, типизированные хранилища компонентов, планировщик систем и запросы.
Описание фичи: сущности держат компоненты в хранилищах по типу, системы обрабатывают их пачками; на этом ядре строятся все этапы контура.

Общий порядок реализации фичи:
1. Объявить EntityId и контракты (store/system/world/scheduler/query) в ecs.hpp.
2. Объявить EcsWorld и storeFor<T>() в ecs_world.hpp.
3. Реализовать EcsWorld и фабрику в ecs_world.cpp.

Файлы фичи:
1. engine/ecs/include/sky/ecs/ecs.hpp
2. engine/ecs/include/sky/ecs/ecs_world.hpp
3. engine/ecs/src/ecs_world.cpp

==============================
Файл: engine/ecs/include/sky/ecs/ecs.hpp
==============================

Назначение файла: контракты ECS.

Пошаговое описание действий:
1. Объявить EntityId с поколением.
2. Объявить IEcsComponentStore, IEcsSystem, IEcsWorld, IEcsSystemScheduler, IEcsQueryService.

Что должно быть в файле:

Структуры / классы / enum:
- struct EntityId { std::uint32_t index; std::uint32_t generation; }
- class IEcsComponentStore
- class IEcsSystem
- class IEcsWorld
- class IEcsSystemScheduler
- class IEcsQueryService

Функции / методы:
- IEcsComponentStore: componentType(), has(EntityId), remove(EntityId), count()
- IEcsSystem: name(), update(double deltaSeconds)
- IEcsWorld: createEntity(), destroyEntity(EntityId), isAlive(EntityId), store(std::type_index)
- IEcsSystemScheduler: registerSystem(IEcsSystem&), unregisterSystem(IEcsSystem&), tick(double)
- IEcsQueryService: entitiesWith(std::set<std::type_index> types)

Логика функций / методов:
- EntityId — сущность с поколением (защита от повторного использования id).
- componentType/has/remove/count — доступ к хранилищу одного типа компонента.
- name/update — система обрабатывает подходящие сущности за кадр (deltaSeconds — шаг времени).
- createEntity/destroyEntity/isAlive/store — мир сущностей и доступ к хранилищу типа.
- registerSystem/unregisterSystem/tick — регистрация систем и прогон за такт.
- entitiesWith(types) — находит сущности с заданным набором компонентов; возвращает список сущностей.

Результат по файлу: контракты ECS зафиксированы.

Критерий правильности по файлу:
1. Заголовок компилируется (header_check).

==============================
Файл: engine/ecs/include/sky/ecs/ecs_world.hpp
==============================

Назначение файла: реализация-контракт мира ECS с типобезопасным хранилищем.

Пошаговое описание действий:
1. Объявить EcsWorld, наследующий три контракта.
2. Объявить шаблон storeFor<T>() и фабрику createEcsWorld().

Что должно быть в файле:

Структуры / классы / enum:
- class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService

Функции / методы:
- template <typename T> TypedComponentStore<T>& storeFor()
- std::unique_ptr<EcsWorld> createEcsWorld()

Логика функций / методов:
- storeFor<T>() — типобезопасный доступ к хранилищу компонента T (методы set(entity, value), get(entity)→T*); возвращает хранилище T.
- createEcsWorld() — фабрика мира.

Результат по файлу: доступ к типизированным хранилищам компонентов.

Критерий правильности по файлу:
1. storeFor<T>().set/get работают для произвольного типа T.

==============================
Файл: engine/ecs/src/ecs_world.cpp
==============================

Назначение файла: реализация EcsWorld.

Пошаговое описание действий:
1. Реализовать учёт живых сущностей.
2. Реализовать планировщик систем и запросы.
3. Дать фабрику createEcsWorld().

Что должно быть в файле:

Структуры / классы / enum:
- скрытый класс-реализация EcsWorld.

Функции / методы:
- createEntity, destroyEntity, isAlive, store, registerSystem, unregisterSystem, tick, entitiesWith, createEcsWorld.

Логика функций / методов:
- createEntity() — выдать id (с 1) и пометить сущность живой.
- destroyEntity(entity) — убрать из живых и удалить её компоненты во всех хранилищах.
- isAlive(entity) — проверить, жива ли сущность.
- store(type) — вернуть хранилище типа (бросить, если тип не зарегистрирован).
- registerSystem/unregisterSystem — вести список систем; tick(dt) — вызвать update у всех систем по порядку.
- entitiesWith(types) — вернуть живые сущности, у которых есть все указанные компоненты.

Результат по файлу: рабочий ECS-мир.

Критерий правильности по файлу:
1. Система обновляет только сущности с нужным компонентом.

==============================

На выходе должно получиться:

Список артефактов фичи:
1. engine/ecs/include/sky/ecs/ecs.hpp
2. engine/ecs/include/sky/ecs/ecs_world.hpp
3. engine/ecs/src/ecs_world.cpp

Общий критерий правильности:
1. Создание/уничтожение сущности работает.
2. storeFor<T>().set/get работают.
3. entitiesWith({type}) возвращает только сущности с этим компонентом.


feature/render-contract

Исполнитель: E2 (Рендеринг)
Порядок реализации: 4
Зависимости: core::Vec3/core::Transform из feature/math-and-handles (поля RenderCommand)

Цель фичи: общий (не только Vulkan) контракт рендера — поток команд и интерфейс рендерера.
Описание фичи: единый контракт, от которого зависят все бэкенды; остальные фичи опираются на интерфейсы, а не на реализацию.

Общий порядок реализации фичи:
1. Объявить поток команд и интерфейсы в rendering.hpp.
2. Объявить реестр бэкендов в renderer_registry.hpp.
3. Реализовать пустой рендерер и реестр (.cpp).

Файлы фичи:
1. engine/rendering/include/sky/rendering/rendering.hpp
2. engine/rendering/include/sky/rendering/renderer_registry.hpp
3. engine/rendering/include/sky/rendering/null_renderer.hpp
4. engine/rendering/src/null_renderer.cpp
5. engine/rendering/src/renderer_registry.cpp

==============================
Файл: engine/rendering/include/sky/rendering/rendering.hpp
==============================

Назначение файла: поток команд отрисовки и контракт рендерера.

Пошаговое описание действий:
1. Объявить перечисления и структуру RenderCommand.
2. Объявить IRenderer и IRenderResourceFactory.

Что должно быть в файле:

Структуры / классы / enum:
- enum class RenderCommandType { BeginFrame, SetViewport, SetCamera, AddLight, SetSky, BindPipeline, DrawMesh, EndFrame }
- enum class LightType { Directional = 0, Point = 1 }
- struct RenderCommand { RenderCommandType type; core::Transform transform; core::Vec3 color; float fovDegrees; float orthoHeight; … }
- class IRenderer
- class IRenderResourceFactory

Функции / методы:
- IRenderer: backendName() const, attachSurface(IRenderSurface&), submit(std::span<const RenderCommand>), renderFrame()
- IRenderResourceFactory: createMeshFromData(std::span<const float>), createTextureFromData(std::uint32_t w, std::uint32_t h, std::span<const std::uint8_t> rgba), destroy(RenderResourceHandle)

Логика функций / методов:
- RenderCommandType — вид команды в потоке; RenderCommand — одна backend-независимая команда (поля читаются по-разному в зависимости от type).
- backendName() — имя бэкенда ("vulkan"/"opengl"); attachSurface — привязывает поверхность вывода; submit — принимает поток команд кадра; renderFrame — рисует накопленный кадр.
- createMeshFromData — загружает меш (позиция+нормаль+uv), возвращает хэндл; createTextureFromData — загружает текстуру, возвращает хэндл; destroy — освобождает ресурс.

Результат по файлу: контракт рендера зафиксирован.

Критерий правильности по файлу:
1. Заголовок компилируется; поля RenderCommand используют core::Vec3/Transform.

==============================
Файл: engine/rendering/include/sky/rendering/renderer_registry.hpp
==============================

Назначение файла: реестр бэкендов рендера.

Пошаговое описание действий:
1. Объявить BackendInit и RendererFactory.
2. Объявить IRendererRegistry и createRendererRegistry().

Что должно быть в файле:

Структуры / классы / enum:
- struct BackendInit { … }
- class IRendererRegistry

Функции / методы:
- using RendererFactory = функция-фабрика рендерера
- IRendererRegistry: registerBackend(const std::string& name, RendererFactory), create(const std::string& name, const BackendInit&)
- std::unique_ptr<IRendererRegistry> createRendererRegistry()

Логика функций / методов:
- registerBackend — регистрирует фабрику бэкенда по имени; возвращает успех.
- create — возвращает рендерер по имени бэкенда.
- createRendererRegistry — фабрика реестра (с предрегистрированным "null").

Результат по файлу: реестр, позволяющий подключать бэкенды по имени.

Критерий правильности по файлу:
1. Заголовок компилируется.

==============================
Файл: engine/rendering/include/sky/rendering/null_renderer.hpp
==============================

Назначение файла: пустой рендерер для тестов контракта.

Пошаговое описание действий:
1. Объявить NullRenderer и фабрики createNullRenderer, createOffscreenSurface.

Что должно быть в файле:

Структуры / классы / enum:
- class NullRenderer (реализует IRenderer/IRenderResourceFactory)

Функции / методы:
- frameCount(), commandsInLastFrame(), liveResourceCount()
- createNullRenderer(), createOffscreenSurface(w, h)

Логика функций / методов:
- пустой рендерер считает кадры/команды, ничего не рисует.

Результат по файлу: контракт можно проверять без GPU.

Критерий правильности по файлу:
1. Заголовок компилируется.

==============================
Файл: engine/rendering/src/null_renderer.cpp
==============================

Назначение файла: реализация пустого рендерера.

Пошаговое описание действий:
1. Реализовать скрытый класс рендерера.
2. Реализовать фабрики.

Что должно быть в файле:

Структуры / классы / enum:
- скрытый класс-реализация NullRenderer, скрытый класс поверхности.

Функции / методы:
- submit, renderFrame, createMeshFromData, createTextureFromData, destroy, frameCount, commandsInLastFrame, liveResourceCount, createNullRenderer, createOffscreenSurface.

Логика функций / методов:
- submit(commands) — дописать команды во внутренний буфер.
- renderFrame() — запомнить число команд (commandsInLastFrame), очистить буфер, увеличить frameCount, вызвать present() у поверхности.
- createMeshFromData/createTextureFromData — проверить вход и завести хэндл ресурса; destroy — убрать ресурс.
- frameCount/commandsInLastFrame/liveResourceCount — чтение счётчиков.

Результат по файлу: рабочий null-рендерер.

Критерий правильности по файлу:
1. submit + renderFrame увеличивают счётчики кадров/команд.

==============================
Файл: engine/rendering/src/renderer_registry.cpp
==============================

Назначение файла: реализация реестра бэкендов.

Пошаговое описание действий:
1. Реализовать хранение фабрик по имени.
2. Предрегистрировать "null" и реализовать create.

Что должно быть в файле:

Структуры / классы / enum:
- скрытый класс-реализация реестра.

Функции / методы:
- registerBackend, create, createRendererRegistry.

Логика функций / методов:
- конструктор предрегистрирует бэкенд "null".
- registerBackend(name, factory) — сохранить фабрику; отклонить пустое имя/фабрику/дубликат.
- create(name, init) — найти фабрику по имени и создать рендерер; для неизвестного имени вернуть nullptr.

Результат по файлу: рабочий реестр.

Критерий правильности по файлу:
1. null-рендерер создаётся по имени "null".

==============================

На выходе должно получиться:

Список артефактов фичи:
1. engine/rendering/include/sky/rendering/rendering.hpp
2. engine/rendering/include/sky/rendering/renderer_registry.hpp
3. engine/rendering/include/sky/rendering/null_renderer.hpp
4. engine/rendering/src/null_renderer.cpp
5. engine/rendering/src/renderer_registry.cpp

Общий критерий правильности:
1. null-рендерер регистрируется в реестре и создаётся по имени.
2. submit + renderFrame увеличивают счётчики кадров/команд.


feature/physics-world

Исполнитель: E4 (Рантайм и физика)
Порядок реализации: 5
Зависимости: core::Vec3/core::Transform из feature/math-and-handles

Цель фичи: физический мир — тела, коллайдеры, гравитация, столкновения, высотная поверхность, луч.
Описание фичи: ядро симуляции; контракты управления и запросов плюс конкретная реализация.

Общий порядок реализации фичи:
1. Объявить типы и контракты в physics.hpp.
2. Объявить PhysicsWorld и фабрику в physics_world.hpp.
3. Реализовать солвер в physics_world.cpp.

Файлы фичи:
1. engine/physics/include/sky/physics/physics.hpp
2. engine/physics/include/sky/physics/physics_world.hpp
3. engine/physics/src/physics_world.cpp

==============================
Файл: engine/physics/include/sky/physics/physics.hpp
==============================

Назначение файла: типы и контракты физики.

Пошаговое описание действий:
1. Объявить типы описаний и событий.
2. Объявить IPhysicsWorld и IPhysicsQueryService.

Что должно быть в файле:

Структуры / классы / enum:
- RigidBodyDesc{type,mass,transform}
- enum class ColliderShape{Box,Sphere,Capsule,TerrainHeightfield}
- ColliderDesc{shape,halfExtents,radius,heightfield}
- HeightfieldDesc{resolution,scale,heights}
- RaycastHit{collider,point,normal,distance}
- CollisionEvent{first,second}
- class IPhysicsWorld
- class IPhysicsQueryService

Функции / методы:
- IPhysicsWorld: createBody(const RigidBodyDesc&), destroyBody(RigidBodyHandle), attachCollider(RigidBodyHandle, const ColliderDesc&), step(double fixedDeltaSeconds), drainCollisionEvents()
- IPhysicsQueryService: raycast(const core::Vec3& origin, const core::Vec3& direction, float maxDistance) const, bodyTransform(RigidBodyHandle) const

Логика функций / методов:
- createBody — создаёт тело (тип/масса/поза), возвращает хэндл; destroyBody — удаляет тело.
- attachCollider — навешивает коллайдер, возвращает хэндл; step — шаг симуляции; drainCollisionEvents — события столкновений за кадр.
- raycast — пускает луч (origin, direction, maxDistance), возвращает попадание или nullopt; bodyTransform — трансформ тела.

Результат по файлу: контракт физики зафиксирован.

Критерий правильности по файлу:
1. Заголовок компилируется; типы держат core::Vec3/Transform.

==============================
Файл: engine/physics/include/sky/physics/physics_world.hpp
==============================

Назначение файла: конкретный мир физики.

Пошаговое описание действий:
1. Объявить PhysicsWorld с расширенными методами.
2. Объявить фабрику createPhysicsWorld().

Что должно быть в файле:

Структуры / классы / enum:
- class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService

Функции / методы:
- setGravity(const core::Vec3&), setBodyVelocity(RigidBodyHandle, const core::Vec3&), bodyVelocity(RigidBodyHandle) const, setBodyTransform(RigidBodyHandle, const core::Transform&)
- std::unique_ptr<PhysicsWorld> createPhysicsWorld()

Логика функций / методов:
- setGravity — задаёт гравитацию; setBodyVelocity — задаёт скорость; bodyVelocity — возвращает скорость; setBodyTransform — переставляет тело.
- createPhysicsWorld — фабрика солвера.

Результат по файлу: интерфейс солвера с фабрикой.

Критерий правильности по файлу:
1. Заголовок компилируется.

==============================
Файл: engine/physics/src/physics_world.cpp
==============================

Назначение файла: реализация солвера.

Пошаговое описание действий:
1. Реализовать хранилища тел/коллайдеров.
2. Реализовать step (гравитация + разбор столкновений).
3. Реализовать raycast.

Что должно быть в файле:

Структуры / классы / enum:
- скрытый класс-реализация PhysicsWorld; внутренние BodyRecord, ColliderRecord, Aabb.

Функции / методы:
- createBody, destroyBody, attachCollider, detachCollider, step, drainCollisionEvents, raycast, bodyTransform, setGravity, setBodyVelocity, bodyVelocity, setBodyTransform, приватные detectAndResolve, resolveHeightfields, sampleHeightfield, createPhysicsWorld.

Логика функций / методов:
- createBody/destroyBody/attachCollider/detachCollider — ведут хранилища (id с 1); destroyBody каскадно удаляет коллайдеры тела.
- step(dt) — для каждого Dynamic-тела: к скорости прибавить gravity·dt, затем к позиции velocity·dt (явный Эйлер); Static/Kinematic не двигать; затем resolveHeightfields() и detectAndResolve().
- detectAndResolve() — AABB каждого коллайдера (позиция ± halfExtents); для пар РАЗНЫХ тел с пересечением записать CollisionEvent и вытолкнуть Dynamic из статического по оси наименьшего проникновения, обнулив скорость вдоль неё.
- resolveHeightfields() — держать Dynamic-тела над террейном: если низ тела ниже высоты heightfield — поднять и обнулить отрицательную вертикальную скорость, добавить событие.
- sampleHeightfield(field, x, z) — билинейная выборка высоты.
- raycast(origin, direction, maxDistance) — луч vs AABB (слэбы) или vs heightfield (марш+бисекция); ближайшее попадание, нормаль — грань оси входа.

Результат по файлу: рабочий физический солвер.

Критерий правильности по файлу:
1. Тело падает под гравитацией; куб замирает на полу; луч попадает в коллайдер.

==============================

На выходе должно получиться:

Список артефактов фичи:
1. engine/physics/include/sky/physics/physics.hpp
2. engine/physics/include/sky/physics/physics_world.hpp
3. engine/physics/src/physics_world.cpp

Общий критерий правильности:
1. Тело за 1 с падает ≈4.9 м под гравитацией.
2. Куб замирает на полу (расталкивание AABB).
3. Луч попадает в коллайдер.


feature/editor-shell

Исполнитель: E3 (Редактор .NET)
Порядок реализации: 6
Зависимости: нет (C-интерфейс подключается со второй фичи контура)

Цель фичи: каркас приложения редактора на Avalonia (.NET 8) с окном «Sky Engine».
Описание фичи: стартовый скелет — точка входа, приложение Avalonia и главное окно с меню.

Общий порядок реализации фичи:
1. Завести проект .NET 8 (csproj) с пакетами Avalonia.
2. Реализовать точку входа и приложение.
3. Собрать главное окно с меню.

Файлы фичи:
1. editor/avalonia/SkyEditor.csproj
2. editor/avalonia/Program.cs
3. editor/avalonia/App.axaml.cs
4. editor/avalonia/MainWindow.axaml.cs

==============================
Файл: editor/avalonia/SkyEditor.csproj
==============================

Назначение файла: описание проекта редактора.

Пошаговое описание действий:
1. Объявить проект .NET 8 (исполняемый).
2. Подключить пакеты Avalonia.

Что должно быть в файле:

Структуры / классы / enum:
- нет.

Функции / методы:
- нет (описание проекта).

Логика функций / методов:
- проект .NET 8 с пакетами Avalonia.

Результат по файлу: проект собирается.

Критерий правильности по файлу:
1. dotnet build проекта — 0 ошибок.

==============================
Файл: editor/avalonia/Program.cs
==============================

Назначение файла: точка входа приложения.

Пошаговое описание действий:
1. Реализовать Main.
2. Добавить ветку --screenshot (headless).

Что должно быть в файле:

Структуры / классы / enum:
- нет.

Функции / методы:
- static int Main(string[] args)

Логика функций / методов:
- Main — точка входа; ветка --screenshot (headless); возвращает код выхода.

Результат по файлу: приложение запускается.

Критерий правильности по файлу:
1. Приложение стартует; ветка --screenshot доступна.

==============================
Файл: editor/avalonia/App.axaml.cs
==============================

Назначение файла: приложение Avalonia.

Пошаговое описание действий:
1. Реализовать OnFrameworkInitializationCompleted().

Что должно быть в файле:

Структуры / классы / enum:
- class App

Функции / методы:
- OnFrameworkInitializationCompleted()

Логика функций / методов:
- OnFrameworkInitializationCompleted() — открывает MainWindow.

Результат по файлу: приложение открывает главное окно.

Критерий правильности по файлу:
1. MainWindow открывается при старте.

==============================
Файл: editor/avalonia/MainWindow.axaml.cs
==============================

Назначение файла: главное окно.

Пошаговое описание действий:
1. Собрать окно «Sky Engine».
2. Добавить меню File/Edit/GameObject и обработчики.

Что должно быть в файле:

Структуры / классы / enum:
- class MainWindow

Функции / методы:
- обработчики пунктов меню File/Edit/GameObject.

Логика функций / методов:
- окно «Sky Engine» с меню и обработчиками пунктов меню.

Результат по файлу: окно с меню.

Критерий правильности по файлу:
1. Окно «Sky Engine» открывается с меню File/Edit/GameObject.

==============================

На выходе должно получиться:

Список артефактов фичи:
1. editor/avalonia/SkyEditor.csproj
2. editor/avalonia/Program.cs
3. editor/avalonia/App.axaml.cs
4. editor/avalonia/MainWindow.axaml.cs

Общий критерий правильности:
1. dotnet build editor/avalonia — 0 ошибок.
2. Окно «Sky Engine» открывается с меню File/Edit/GameObject.
