# День 1–7 (Неделя 1) — фундамент. Фичи по контурам E1–E6

Источник — `role-E1.md` … `role-E6.md` (Этап 1 каждого контура). Одна фича =
ветка `feature/<название>` = один запрос на слияние в `develop`. Порядок внутри
контура — порядок реализации. Точки соприкосновения дня 1: `core/math.hpp`
(E1→E2, E4) и `engine/CMakeLists.txt` (E5).

---

## Контур E1 — Ядро и данные

### feature/math-and-handles
**Цель фичи:** математические типы и типобезопасные идентификаторы — фундамент для всех.
**Описание фичи (для чего):** `Vec3/Quat/Transform` и `Handle` используют все модули; заголовок `math.hpp` отдаётся первым коммитом — по нему стартуют E2 и E4.
**Пошаговое описание действий:**
- Сделай файл `engine/core/include/sky/core/math.hpp` (`namespace sky::core`).
- В файле должны быть структуры `Vec3`, `Quat`, `Transform` и функции `operator+`, `operator*`(вектор·число), `operator*`(кватернион), `rotate`, `compose`, `conjugate`, `invCompose` (все `constexpr`).
- В функциях должна быть реализована логика: сложение/масштаб векторов; композиция поворотов; `rotate` — поворот вектора кватернионом; `compose` — локальный трансформ ребёнка в систему родителя; `conjugate` — сопряжённый кватернион; `invCompose` — обратная к `compose`.
- Сделай файл `engine/core/include/sky/core/handle.hpp` (`namespace sky::core`).
- В файле должен быть шаблон `Handle<Tag>` с `isValid()`, `invalid()`, `operator==`.
- В методах должна быть реализована логика: хранит `value`; разные теги (`ObjectTag`, `ComponentTag`) дают несовместимые типы.
**На выходе должно получиться:** `math.hpp`, `handle.hpp` (header-only, `sky::core`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `rotate(90° вокруг Y, {0,0,1}) ≈ {1,0,0}` (±1e-5); `invCompose(parent, compose(parent,child)) == child`.

### feature/core-services
**Цель фичи:** единый журнал и служба конфигурации для всех подсистем.
**Описание фичи (для чего):** логирование и конфиг нужны всем модулям; выносятся за интерфейсы с фабриками.
**Пошаговое описание действий:**
- Сделай файл `engine/core/include/sky/core/logger.hpp`.
- В файле должны быть `enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}` и методы `log(level, category, message)`, сокращения `info/warning/error`.
- В методах должна быть реализована логика: `log` записывает строку журнала; сокращения вызывают `log` с нужным уровнем.
- Сделай файл `engine/core/include/sky/core/config_service.hpp`.
- В файле должны быть методы `getString`, `getInt`, `getBool`, `set`.
- В методах должна быть реализована логика: чтение значения по ключу (`nullopt`, если нет); `set` кладёт значение.
- Сделай файлы `engine/core/src/console_logger.cpp`, `engine/core/src/memory_config_service.cpp`.
- В файлах должны быть фабрики `createConsoleLogger()` (журнал в stdout/stderr) и `createInMemoryConfigService()` (конфиг на `map`).
**На выходе должно получиться:** `logger.hpp`, `config_service.hpp`, `console_logger.cpp`, `memory_config_service.cpp`.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** записанные конфиг-значения читаются обратно нужного типа; см. `tests/core_tests.cpp`.

### feature/object-model
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
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** ребёнок `(1,0,0)` под родителем, повёрнутым на 90° вокруг Y, в мире = `(0,0,-1)`.

### feature/component-model
**Цель фичи:** компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.
**Описание фичи (для чего):** поля описываются данными (`variant`-map), что позже бесплатно даёт сериализацию, отмену и параметры скриптов.
**Пошаговое описание действий:**
- Сделай файл `engine/component/include/sky/component/component_model.hpp`.
- В файле должны быть `using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>`, `ComponentDescriptor`, `IComponentRegistry` (`registerComponentType`, `availableTypes`), `IComponentAttachmentService` (`attach`, `detach`), `IComponentQueryService` (`componentsOf`, `descriptorOf`, `ownerOf`).
- В методах должна быть реализована логика: регистрация типов; навешивание/снятие компонентов; запросы «компоненты объекта / описание типа / владелец».
- Сделай файлы `engine/component/include/sky/component/component_world.hpp`, `engine/component/src/component_world.cpp`.
- В файле должен быть `class ComponentWorld` + `setField`, `field`, `fields`, `detachAllFrom`, фабрика `createComponentWorld()`.
- В методах должна быть реализована логика: запись/чтение поля по имени; выдача всей карты полей; снятие всех компонентов объекта.
**На выходе должно получиться:** `component_model.hpp`, `component_world.hpp`, `component_world.cpp` (библиотека `sky_component`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** поле каждого из 5 типов записывается и читается без потерь.

### feature/core-tests
**Цель фичи:** проверить математику, объектный и компонентный миры.
**Описание фичи (для чего):** зафиксировать корректность фундамента тестами.
**Пошаговое описание действий:**
- Сделай файлы `tests/core_tests.cpp`, `tests/world_tests.cpp`.
- В файлах должны быть проверки математики, `ObjectWorld`, `ComponentWorld`.
- В тестах должна быть реализована логика: повороты/композиция; иерархия и мировые трансформы; запись/чтение полей всех 5 типов.
**На выходе должно получиться:** библиотеки `sky_core`, `sky_object`, `sky_component` линкуются; тесты `core_tests`, `world_tests` зелёные.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** поворот `(0,0,1)`→`(1,0,0)`±1e-5; ребёнок в мире = `(0,0,-1)`; поля 5 типов без потерь.

---

## Контур E2 — Рендеринг

### feature/render-contract
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

### feature/vulkan-offscreen
**Цель фичи:** инициализировать Vulkan и отрисовать треугольник в закадровую цель с чтением кадра.
**Описание фичи (для чего):** первый реальный кадр + `readbackFrame()` — основа верификации всего проекта (скриншоты).
**Пошаговое описание действий:**
- Сделай файл `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`.
- В файле должны быть `class VulkanRenderer : rendering::IRenderer` (`ready`, `readbackFrame`, `frameWidth`, `frameHeight`) и `createVulkanRenderer(width, height)`.
- В методах должна быть реализована логика: `ready` — инициализирован ли; `readbackFrame` — пиксели последнего кадра (RGBA); фабрика создаёт закадровый рендерер или `nullptr`, если Vulkan недоступен.
- Сделай файл `engine/rendering_vulkan/src/vulkan_renderer.cpp`.
- В файле должны быть `initInstanceAndDevice`, `initOffscreenTarget`, `initPipeline`, `renderFrame`, `readbackFrame` и вспомогательные `findMemoryType`/`createImage`/`createBuffer`/`createShader`.
- В методах должна быть реализована логика: создание инстанса/устройства/очереди; цвет (RGBA)+глубина (D32), проход и кадровый буфер; конвейер с вершиной позиция+нормаль+uv; запись команд и `vkCmdDraw` треугольника; копирование цвета в CPU-буфер.
- Сделай файлы `engine/rendering_vulkan/shaders/mesh.vert`, `mesh.frag`.
- В файлах должны быть минимальные вершинный/фрагментный шейдеры (компилируются в `mesh.vert.spv.h`/`mesh.frag.spv.h`).
**На выходе должно получиться:** `vulkan_backend.hpp`, `vulkan_renderer.cpp`, шейдеры + встроенные `.spv.h`; формируется `triangle.png`.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** на `triangle.png` центральный пиксель отличается от углового; `readbackFrame()` возвращает непустой массив.

### feature/vulkan-tests
**Цель фичи:** проверить Vulkan-рендерер на программном драйвере lavapipe.
**Описание фичи (для чего):** автотест закадрового рендера без видеокарты (в CI).
**Пошаговое описание действий:**
- Сделай файл `tests/vulkan_tests.cpp`.
- В файле должна быть реализована логика: `ready()` истинно, кадр рендерится, `readbackFrame()` непустой, центральный пиксель отличается от углового.
**На выходе должно получиться:** библиотека `sky_rendering_vulkan` собрана; тест `vulkan_tests` зелёный на lavapipe.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `triangle.png` — центр ≠ угол; `readbackFrame()` непустой.

---

## Контур E3 — Редактор (.NET / Avalonia)

### feature/editor-shell
**Цель фичи:** каркас приложения редактора на Avalonia (.NET 8) с окном «Sky Engine».
**Описание фичи (для чего):** стартовый скелет, без которого нет панелей и вызовов движка; точка входа + окно с меню + headless-режим для скриншотов.
**Пошаговое описание действий:**
- Сделай файл `editor/avalonia/SkyEditor.csproj` — проект .NET 8 с пакетами Avalonia.
- Сделай файл `editor/avalonia/Program.cs` — метод `static int Main(string[] args)` с веткой `--screenshot` (headless), возвращает код выхода.
- Сделай файл `editor/avalonia/App.axaml.cs` — класс `App` с `OnFrameworkInitializationCompleted()` (открывает `MainWindow`).
- Сделай файл `editor/avalonia/MainWindow.axaml.cs` — окно «Sky Engine», меню File/Edit/GameObject и обработчики.
**На выходе должно получиться:** `SkyEditor.csproj`, `Program.cs`, `App.axaml.cs`, `MainWindow.axaml.cs`; окно с меню.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `dotnet build` = 0 ошибок; сессия движка создаётся из .NET (не-null); есть сброс компоновки. *(C-интерфейс — фича `feature/engine-bridge`.)*

### feature/docking-layout
**Цель фичи:** компоновка перетаскиваемых панелей-заглушек редактора.
**Описание фичи (для чего):** система докинга задаёт структуру рабочего пространства (Hierarchy/Scene/Inspector…); на место заглушек позже встанут живые панели.
**Пошаговое описание действий:**
- Сделай файл `editor/avalonia/Docking/DockFactory.cs` — класс `DockFactory` с `CreateLayout()` (собирает раскладку панелей).
- Сделай файл `editor/avalonia/Docking/Tools.cs` — классы-инструменты `HierarchyTool`, `InspectorTool`, `SceneDocument`, … (привязка инструмента к панели).
- Сделай файл `editor/avalonia/Views/PlaceholderView.axaml.cs` — `PlaceholderView` — пустая панель-заглушка.
**На выходе должно получиться:** `DockFactory.cs`, `Tools.cs`, `PlaceholderView.axaml.cs`; перетаскиваемая компоновка.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `dotnet build` = 0 ошибок; есть сброс компоновки.

### feature/engine-bridge
**Цель фичи:** первичная связь редактора с движком через C-интерфейс (P/Invoke).
**Описание фичи (для чего):** загрузка нативного моста и создание/уничтожение сессии движка из .NET — фундамент всех дальнейших вызовов.
**Пошаговое описание действий:**
- Сделай файл `editor/avalonia/Engine/EngineInterop.cs` — `static class EngineInterop` с `[DllImport] sky_editor_create()`, `sky_editor_destroy(IntPtr)`, `Resolve(...)`, `Candidates()`.
- В методах должна быть реализована логика: объявления P/Invoke; поиск `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в дереве сборки.
- Сделай файл `editor/avalonia/Engine/EditorSession.cs` — `class EditorSession : IDisposable` со свойством `Native` и `Dispose()`.
- В методах должна быть реализована логика: конструктор вызывает `sky_editor_create` с проверкой на не-null; `Dispose` вызывает `sky_editor_destroy`.
**На выходе должно получиться:** `EngineInterop.cs`, `EditorSession.cs`; сессия создаётся из .NET и корректно освобождается.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `dotnet build` = 0 ошибок; сессия движка создаётся из .NET (не-null).

---

## Контур E4 — Рантайм и физика

### feature/physics-world
**Цель фичи:** физический мир — тела, коллайдеры, гравитация, столкновения, heightfield, луч.
**Описание фичи (для чего):** ядро симуляции; контракты управления и запросов + конкретная реализация. Без неё нет физических взаимодействий.
**Пошаговое описание действий:**
- Сделай файл `engine/physics/include/sky/physics/physics.hpp`.
- В файле должны быть типы `RigidBodyDesc`, `ColliderShape`, `ColliderDesc`, `HeightfieldDesc`, `RaycastHit`, `CollisionEvent`; `class IPhysicsWorld` (`createBody`, `destroyBody`, `attachCollider`, `step`, `drainCollisionEvents`); `class IPhysicsQueryService` (`raycast`, `bodyTransform`).
- В методах должна быть реализована логика: создание/удаление тела; навешивание коллайдера; шаг симуляции; выдача событий за кадр; луч возвращает попадание или `nullopt`; `bodyTransform` — трансформ тела.
- Сделай файлы `engine/physics/include/sky/physics/physics_world.hpp`, `engine/physics/src/physics_world.cpp`.
- В файле должен быть `class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService` (`setGravity`, `setBodyVelocity`, `bodyVelocity`, `setBodyTransform`) + фабрика `createPhysicsWorld()`.
- В `.cpp` должна быть реализована логика: интегрирование гравитации в `step`; `detectAndResolve` (расталкивание AABB, сбор `CollisionEvent`); `sampleHeightfield`/`resolveHeightfields`; `raycast` по коллайдерам и высотной поверхности.
**На выходе должно получиться:** `physics.hpp`, `physics_world.hpp`, `physics_world.cpp` (`sky::physics`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности; привязанный объект синхронно опускается.

### feature/physics-object-sync
**Цель фичи:** синхронизация физических тел с объектами объектного мира до/после шага.
**Описание фичи (для чего):** переносит трансформы объектов в тела перед шагом и результаты обратно после — согласованное движение; + тест этапа.
**Пошаговое описание действий:**
- Дополни `engine/physics/include/sky/physics/physics_world.hpp` (+ реализация).
- В файле должен быть `class ObjectPhysicsSync : IPhysicsSyncContract` (`bind`, `unbind`, `pushKinematicState`, `pullSimulationResults`) + фабрика `createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)`.
- В методах должна быть реализована логика: `bind`/`unbind` связывают тело с объектом; `pushKinematicState` до шага переносит трансформы объектов в тела; `pullSimulationResults` после шага — обратно.
- Сделай файл `tests/physics_tests.cpp` — проверки падения, куба на полу, тела на heightfield, синхронизации объекта.
**На выходе должно получиться:** дополненный `physics_world.hpp` с `ObjectPhysicsSync`; `tests/physics_tests.cpp`; библиотека `sky_physics` собрана, тест зелёный.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** падение ≈4.9 м; куб на полу; тело на heightfield; объект синхронно опускается.

---

## Контур E5 — Пайплайн и QA

### feature/build-system
**Цель фичи:** система сборки CMake для движка, редактора, плеера и тестов.
**Описание фичи (для чего):** единый механизм сборки модулей на C++20; регистрация модулей и связей одной функцией.
**Пошаговое описание действий:**
- Сделай файл `engine/CMakeLists.txt` — функция `sky_add_module(NAME DIR sources…)`.
- В функции должна быть реализована логика: статическая библиотека с public-include-путями и C++20; регистрация модулей и связей.
- Сделай файл `CMakeLists.txt` (корневой) — объявление проекта, опции, `add_subdirectory` для движка/редактора/плеера/тестов.
- Сделай файл `tests/CMakeLists.txt`.
**На выходе должно получиться:** `CMakeLists.txt`, `engine/CMakeLists.txt`, `tests/CMakeLists.txt`; проект собирается через CMake.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** красный CI блокирует слияние; сломанный тест краснеет; редактор E3 вызывает `sky_editor_create`.

### feature/test-harness
**Цель фичи:** лёгкий тест-фреймворк и CI на GitHub Actions.
**Описание фичи (для чего):** макросы для юнит-тестов без падения процесса; CI ловит поломки сборки и нестабильные тесты, красный статус блокирует слияние.
**Пошаговое описание действий:**
- Сделай файл `tests/sky_test.hpp` — макрос `CHECK(condition)` и функция `inline int summary(const char* suite)`.
- В них должна быть реализована логика: `CHECK` фиксирует провал с файлом/строкой, не роняя процесс; `summary` печатает «N проверок, M провалов» и возвращает код выхода.
- Сделай файл `.github/workflows/ci.yml` — CI на Ubuntu: установка зависимостей (ninja, lavapipe, xvfb), `cmake --build`, `xvfb-run ctest`.
**На выходе должно получиться:** `tests/sky_test.hpp`, `.github/workflows/ci.yml`; работающий CI.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** красный CI блокирует слияние; сломанный тест краснеет.

### feature/c-abi-seed
**Цель фичи:** первичный плоский C-интерфейс движка для .NET-редактора (совместно с E1).
**Описание фичи (для чего):** набор `sky_editor_*`, через который редактор общается с движком; минимум — сессия и перечисление корней.
**Пошаговое описание действий:**
- Сделай файлы `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`, `editor/native_bridge/src/editor_bridge.cpp`.
- В файлах должны быть `sky_editor_create`, `sky_editor_destroy`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_object_name`.
- В функциях должна быть реализована логика: `create` собирает движок и демо-сцену и возвращает сессию; `destroy` уничтожает; `root_count`/`root_at` перечисляют корни; `object_name` пишет имя в буфер и возвращает длину.
**На выходе должно получиться:** `editor_bridge.h`, `editor_bridge.cpp`; рабочий C-интерфейс create/destroy/enumerate.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ (этап):** редактор E3 вызывает `sky_editor_create`; CI зелёный.

---

## Контур E6 — Data-oriented (ECS)

### feature/ecs-core
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

### feature/ecs-tests
**Цель фичи:** покрыть ECS-ядро тестами и подтвердить сборку `sky_ecs`.
**Описание фичи (для чего):** зафиксировать, что система обновляет только подходящие сущности, а результат читается через хранилище.
**Пошаговое описание действий:**
- Дополни `tests/runtime_tests.cpp` и `tests/world_tests.cpp` (ECS-часть).
- В файлах должна быть система, обновляющая только сущности с нужным компонентом; чтение результата через `storeFor<T>().get(...)`.
**На выходе должно получиться:** библиотека `sky_ecs` собрана; ECS-часть тестов зелёная.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** система обновляет только сущности с нужным компонентом; запрос по типам корректен.

---

## Замечания по сверке с исходником (для приёмки)

`role-*` авторитетны, но при реализации студенты сверяются с реальными
заголовками. Места, где текст ролей расходится с фактическим кодом эталона:

1. **E6 `EntityId`:** в роли `{index, generation}`, в реальном `engine/ecs/include/sky/core/ecs.hpp` — одно поле `std::uint64_t value`.
2. **E6 `entitiesWith`:** в роли `std::set<std::type_index>`, в коде — `const std::vector<std::type_index>&`.
3. **E1 `rotate`/критерий:** пример в роли (`{0,0,1}→{1,0,0}`) корректен, но фактический тест `core_tests.cpp` проверяет `{1,0,0}→{0,0,-1}`.
4. Контракт E2 (`rendering.hpp`) реально подключает `asset::AssetId`, а модуль E4 (`physics_world.hpp`) — `object` (для `ObjectPhysicsSync`).
