# Спринт 1. День 4

## feature/component-model

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/object-model` (`ObjectHandle`); по эталонным связям модуля — `sky_serialization`, `sky_scripting`

**Цель фичи:** компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.

**Описание фичи:** поля, описываемые данными (variant-map), позже бесплатно дают сериализацию, отмену и параметры скриптов.

**Общий порядок реализации фичи:**
1. Объявить `FieldValue`, `ComponentDescriptor` и три контракта в `component_model.hpp`.
2. Объявить `ComponentWorld` и фабрику в `component_world.hpp`.
3. Реализовать `ComponentWorld` в `component_world.cpp`.

**Файлы фичи:**
1. `engine/component/include/sky/component/component_model.hpp`
2. `engine/component/include/sky/component/component_world.hpp`
3. `engine/component/src/component_world.cpp`

### Файл: `engine/component/include/sky/component/component_model.hpp`

**Назначение файла:** контракты компонентной модели и тип поля.

**Пошаговое описание действий:**
1. Объявить `FieldValue` и `ComponentDescriptor`.
2. Объявить `IComponentRegistry`, `IComponentAttachmentService`, `IComponentQueryService`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>`
- `ComponentDescriptor{typeId, displayName, fields, category}`
- `class IComponentRegistry`
- `class IComponentAttachmentService`
- `class IComponentQueryService`

*Функции / методы:*
- `IComponentRegistry`: `registerComponentType(const ComponentDescriptor&)`, `availableTypes()`
- `IComponentAttachmentService`: `attach(object::ObjectHandle, const std::string& typeId)`, `detach(ComponentHandle)`
- `IComponentQueryService`: `componentsOf(object::ObjectHandle)`, `descriptorOf(ComponentHandle)`, `ownerOf(ComponentHandle)`

*Логика функций / методов:*
- `registerComponentType` — регистрирует тип; `availableTypes` — типы для меню Add Component.
- `attach` — навешивает компонент, возвращает хэндл; `detach` — снимает компонент.
- `componentsOf` — компоненты объекта; `descriptorOf` — описание типа; `ownerOf` — объект-владелец.

**Результат по файлу:** контракты компонентной модели зафиксированы.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/component/include/sky/component/component_world.hpp`

**Назначение файла:** мир компонентов с доступом к полям.

**Пошаговое описание действий:**
1. Объявить `ComponentWorld` с доступом к полям.
2. Добавить фабрику `createComponentWorld`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class ComponentWorld` (наследует три контракта)

*Функции / методы:*
- `virtual void setField(ComponentHandle, const std::string& name, FieldValue value) = 0`
- `virtual std::optional<FieldValue> field(ComponentHandle, const std::string& name) const = 0`
- `virtual std::map<std::string, FieldValue> fields(ComponentHandle) const = 0`
- `virtual void detachAllFrom(object::ObjectHandle) = 0`
- `std::unique_ptr<ComponentWorld> createComponentWorld()`

*Логика функций / методов:*
- `setField` — записывает поле по имени; `field` — значение или `nullopt`; `fields` — вся карта полей; `detachAllFrom` — снимает все компоненты объекта.
- `createComponentWorld` — фабрика.

**Результат по файлу:** интерфейс мира компонентов с фабрикой.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/component/src/component_world.cpp`

**Назначение файла:** реализация мира компонентов.

**Пошаговое описание действий:**
1. Завести хранилище компонентов и их полей.
2. Реализовать доступ к полям и снятие компонентов.

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация `ComponentWorld`.

*Функции / методы:*
- `registerComponentType`, `availableTypes`, `attach`, `detach`, `componentsOf`, `descriptorOf`, `ownerOf`, `setField`, `field`, `fields`, `detachAllFrom`, `createComponentWorld`.

*Логика функций / методов:*
- хранение компонентов и их полей, привязанных к объекту-владельцу.
- `setField`/`field`/`fields` — запись/чтение поля по имени и выдача всей карты; `detachAllFrom` — снять все компоненты объекта.

**Результат по файлу:** рабочий мир компонентов.

**Критерий правильности по файлу:**
1. Поле каждого из 5 типов пишется и читается без потерь.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/component/include/sky/component/component_model.hpp`
2. `engine/component/include/sky/component/component_world.hpp`
3. `engine/component/src/component_world.cpp`

**Общий критерий правильности:**
1. Поле каждого из 5 типов записывается и читается без потерь.

---

## feature/vulkan-mesh-lighting

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/render-contract`, `feature/vulkan-offscreen` (Спринт 1); `core::Transform`/`core::Vec3`

**Цель фичи:** отрисовка мешей с матрицами, камера и освещение в Vulkan-рендерере.

**Описание фичи:** дополнение Vulkan-рендерера — загрузка мешей, накопление команд кадра, разбор потока `RenderCommand` (камера, свет, меши) и матричные помощники. Начало Этапа 2 контура E2.

**Общий порядок реализации фичи:**
1. Реализовать `createMeshFromData` и `submit`.
2. Реализовать разбор потока в `renderFrame` (`SetCamera`, `AddLight`, `DrawMesh`).
3. Реализовать матричные помощники.

**Файлы фичи:**
1. `engine/rendering_vulkan/src/vulkan_renderer.cpp`

### Файл: `engine/rendering_vulkan/src/vulkan_renderer.cpp`

**Назначение файла:** дополнение реализации Vulkan-рендерера мешами, камерой и светом.

**Пошаговое описание действий:**
1. Реализовать `createMeshFromData(std::span<const float>)` — загрузку меша.
2. Реализовать `submit(std::span<const RenderCommand>)` — накопление команд кадра.
3. В `renderFrame` разобрать поток: `SetCamera`, `AddLight`, `DrawMesh`.
4. Реализовать матричные помощники `perspective`, `orthographic`, `fromTransform`, `viewFromCameraPose`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `FrameUbo` (до 4 источников света).

*Функции / методы:*
- `createMeshFromData(std::span<const float>)`
- `submit(std::span<const RenderCommand>)`
- `renderFrame` (разбор потока)
- `perspective(fov,aspect,near,far)`, `orthographic(...)`, `fromTransform(Transform)`, `viewFromCameraPose(...)`

*Логика функций / методов:*
- `createMeshFromData(std::span<const float>)` — загрузка меша (реализация уже объявленного контракта). Возвращает: хэндл.
- `submit(std::span<const RenderCommand>)` — накопление команд кадра.
- в `renderFrame` — разбор потока: `SetCamera` (матрицы вида/проекции: `perspective`/`orthographic`), `AddLight` (до 4 источников в `FrameUbo`), `DrawMesh` (модельная матрица и материал в push-константах).
- матричные помощники `perspective(fov,aspect,near,far)`, `orthographic(...)`, `fromTransform(Transform)`, `viewFromCameraPose(...)`.

**Результат по файлу:** рендерер рисует освещённые меши с камерой.

**Критерий правильности по файлу:**
1. Кадр демо-сцены содержит освещённые меши.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/rendering_vulkan/src/vulkan_renderer.cpp` (дополнено мешами, камерой, светом)

**Общий критерий правильности:**
1. Кадр демо-сцены содержит освещённые меши.

---

## feature/vulkan-viewport

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/engine-bridge` (`EditorSession`, `sky_editor_create`) из Этапа 1 контура E3

**Цель фичи:** панель вьюпорта, показывающая кадр движка.

**Описание фичи:** `VulkanViewport : Control` держит `WriteableBitmap` и таймер кадров, дёргает нативный offscreen-рендер и блитит пиксели; `SceneView` хостит вьюпорт. Начало Этапа 2 контура E3.

**Общий порядок реализации фичи:**
1. Реализовать `VulkanViewport : Control` с `WriteableBitmap` и таймером кадров.
2. Реализовать `SetContext` и `RenderOnce`.
3. Собрать `SceneView`, хостящий вьюпорт.

**Файлы фичи:**
1. `editor/avalonia/Controls/VulkanViewport.cs`
2. `editor/avalonia/Views/SceneView.axaml.cs`

### Файл: `editor/avalonia/Controls/VulkanViewport.cs`

**Назначение файла:** элемент вьюпорта, рисующий кадр движка.

**Пошаговое описание действий:**
1. Объявить `class VulkanViewport : Control` с `WriteableBitmap` и таймером кадров.
2. Реализовать `SetContext(IntPtr context)`.
3. Реализовать `RenderOnce()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class VulkanViewport : Control` (держит `WriteableBitmap` и таймер кадров)

*Функции / методы:*
- `public void SetContext(IntPtr context)`
- `public void RenderOnce()`

*Логика функций / методов:*
- `SetContext(context)` — привязывает сессию движка. Параметры: `context`.
- `RenderOnce()` — рисует один кадр (дёргает нативный offscreen-рендер и блитит пиксели).

**Результат по файлу:** элемент вьюпорта, отображающий кадр движка.

**Критерий правильности по файлу:**
1. `RenderOnce()` выводит непустой кадр в `WriteableBitmap`.

### Файл: `editor/avalonia/Views/SceneView.axaml.cs`

**Назначение файла:** панель сцены, хостящая вьюпорт.

**Пошаговое описание действий:**
1. Разместить `VulkanViewport` внутри `SceneView`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class SceneView` (хостит `VulkanViewport`)

*Функции / методы:* нет (композиция вью).

*Логика функций / методов:*
- `SceneView` — хостит `VulkanViewport`.

**Результат по файлу:** панель сцены с вьюпортом.

**Критерий правильности по файлу:**
1. Панель вьюпорта показывает демо-сцену.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Controls/VulkanViewport.cs`
2. `editor/avalonia/Views/SceneView.axaml.cs`

**Общий критерий правильности:**
1. Панель вьюпорта показывает демо-сцену.

---

## feature/play-mode

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** `scene::SceneHandle` (`feature/scene-world`), `EditorContext` (`feature/editor-context`), `PhysicsWorld` (`feature/physics-world`)

**Цель фичи:** управление режимом воспроизведения со снимком/восстановлением сцены.

**Описание фичи:** часть Этапа 3 (режим воспроизведения и ввод) — контроллер воспроизведения и вход/выход из play в контексте редактора.

**Общий порядок реализации фичи:**
1. Объявить `PlayModeState` и `PlayModeController` в `play_mode_controller.hpp`.
2. Реализовать контроллер в `play_mode_controller.cpp`.
3. Дополнить `editor_context.cpp` методами `beginPlay`/`endPlay`.

**Файлы фичи:**
1. `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`
2. `editor/viewport_bridge/src/play_mode_controller.cpp`
3. `editor/shell/src/editor_context.cpp`

### Файл: `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`

**Назначение файла:** контракт контроллера воспроизведения.

**Пошаговое описание действий:**
1. Объявить `PlayModeState`.
2. Объявить `PlayModeController`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `enum class PlayModeState { Editing, Playing, Paused }`
- `class PlayModeController`

*Функции / методы:*
- `virtual void setScene(scene::SceneHandle scene) = 0`
- `virtual bool play() = 0`
- `virtual void pause() = 0`
- `virtual void stop() = 0`
- `virtual void tickFrame(double deltaSeconds) = 0`
- `virtual PlayModeState state() const = 0`

*Логика функций / методов:*
- `setScene(scene)` — задаёт сцену для воспроизведения.
- `play()` — запускает. Возвращает: успех (false = сцена не задана).
- `pause()`/`stop()` — пауза/остановка.
- `tickFrame(deltaSeconds)` — продвигает симуляцию на кадр.
- `state()` — Возвращает: текущее состояние.

**Результат по файлу:** контракт воспроизведения зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `editor/viewport_bridge/src/play_mode_controller.cpp`

**Назначение файла:** реализация контроллера воспроизведения.

**Пошаговое описание действий:**
1. Реализовать переходы состояний.
2. Реализовать `tickFrame`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `PlayModeController`.

*Функции / методы:*
- `setScene`, `play`, `pause`, `stop`, `tickFrame`, `state`.

*Логика функций / методов:*
- `setScene`/`play`/`pause`/`stop` — управляют состоянием (`Editing`/`Playing`/`Paused`); `play` возвращает false, если сцена не задана.
- `tickFrame(deltaSeconds)` — продвигает симуляцию на кадр в состоянии `Playing`.

**Результат по файлу:** рабочий контроллер воспроизведения.

**Критерий правильности по файлу:**
1. `play` без заданной сцены возвращает false; `tickFrame` продвигает симуляцию только в `Playing`.

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** вход/выход из play со снимком трансформов.

**Пошаговое описание действий:**
1. Реализовать `beginPlay`.
2. Реализовать `endPlay`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `EditorContext`).

*Функции / методы:*
- `void beginPlay()`
- `void endPlay()`

*Логика функций / методов:*
- `beginPlay()` — снимает локальные трансформы всех объектов (для восстановления).
- `endPlay()` — восстанавливает снимок и пересаживает тела с нулевой скоростью.

**Результат по файлу:** обратимый вход в режим воспроизведения.

**Критерий правильности по файлу:**
1. После `play → stop` сцена в исходном состоянии, тела без остаточной скорости.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`
2. `editor/viewport_bridge/src/play_mode_controller.cpp`
3. `editor/shell/src/editor_context.cpp`

**Общий критерий правильности:**
1. после `play → stop` сцена в исходном состоянии, тела без остаточной скорости.

---

## feature/ci-screenshot

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** `feature/build-system`, `feature/test-harness`, `feature/c-abi-seed` (CI), `feature/editor-shell` (`Program.cs`)

**Цель фичи:** сборка редактора и скриншот-артефакт в CI; первые модульные тесты.

**Описание фичи:** ветка `--screenshot` в headless-Avalonia, шаг сборки редактора в CI с публикацией PNG-артефакта и тест продвижения рантайма. Этап 2 контура E5.

**Общий порядок реализации фичи:**
1. Добавить ветку `--screenshot path` в `Program.cs`.
2. Добавить шаг сборки редактора и публикации PNG в `ci.yml`.
3. Написать `runtime_tests.cpp` (play-режим продвигает рантайм).

**Файлы фичи:**
1. `editor/avalonia/Program.cs`
2. `.github/workflows/ci.yml`
3. `tests/runtime_tests.cpp`

### Файл: `editor/avalonia/Program.cs`

**Назначение файла:** дополнение точки входа режимом скриншота.

**Пошаговое описание действий:**
1. Добавить ветку `--screenshot path` — headless Avalonia, рендер нескольких кадров, `window.CaptureRenderedFrame().Save(path)`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- ветка `--screenshot path` в `Main`.

*Логика функций / методов:*
- ветка `--screenshot path` — headless Avalonia, рендер нескольких кадров, `window.CaptureRenderedFrame().Save(path)`.

**Результат по файлу:** редактор умеет снимать скриншот в headless-режиме.

**Критерий правильности по файлу:**
1. `--screenshot path` сохраняет PNG.

### Файл: `.github/workflows/ci.yml`

**Назначение файла:** дополнение CI сборкой редактора и артефактом-скриншотом.

**Пошаговое описание действий:**
1. Добавить шаг `dotnet build editor/avalonia` (ошибка C# валит задачу).
2. Добавить публикацию PNG-артефакта.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* нет (декларативный CI).

*Логика функций / методов:*
- шаг `dotnet build editor/avalonia` (ошибка C# валит задачу) + публикация PNG-артефакта.

**Результат по файлу:** CI собирает редактор и публикует скриншот.

**Критерий правильности по файлу:**
1. Артефакт-PNG доступен из прогона CI; ошибка C# останавливает сборку.

### Файл: `tests/runtime_tests.cpp`

**Назначение файла:** тест продвижения рантайма в play-режиме.

**Пошаговое описание действий:**
1. Проверить, что play-режим действительно продвигает рантайм (ECS-система крутит объект).

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:*
- проверяет, что play-режим действительно продвигает рантайм (ECS-система крутит объект).

**Результат по файлу:** зелёный тест `runtime_tests`.

**Критерий правильности по файлу:**
1. Play-режим продвигает рантайм (объект вращается ECS-системой).

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Program.cs` (ветка `--screenshot`)
2. `.github/workflows/ci.yml` (сборка редактора + PNG-артефакт)
3. `tests/runtime_tests.cpp`

**Общий критерий правильности:**
1. Артефакт-PNG доступен из прогона CI.
2. Ошибка C# останавливает сборку.

---

## feature/ecs-systems

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** `feature/ecs-core` (ECS-ядро); `EcsTransform` из `feature/ecs-object-sync`

**Цель фичи:** первые прикладные системы поверх ECS (частицы и пакетная обработка трансформов).

**Описание фичи:** часть Этапа 3 (прикладные системы поверх ECS). Файлы этой фичи в текущем репозитории отсутствуют — создаются на этапе.

**Общий порядок реализации фичи:**
1. Реализовать систему частиц в `particle_system.cpp`.
2. Реализовать систему пакетной обработки трансформов в `transform_batch_system.cpp`.

**Файлы фичи:**
1. `engine/ecs/src/systems/particle_system.cpp`
2. `engine/ecs/src/systems/transform_batch_system.cpp`

### Файл: `engine/ecs/src/systems/particle_system.cpp`

**Назначение файла:** система частиц.

**Пошаговое описание действий:**
1. Объявить компонент `Particle`.
2. Реализовать `update(double)`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- компонент `Particle`.

*Функции / методы:*
- `update(double)`

*Логика функций / методов:*
- `update(double)` — продвигает позиции и время жизни частиц; мёртвые частицы удаляются.
- источник сущностей — рассыпка генерации карты (`materializeGenerationResult`).

**Результат по файлу:** визуально наблюдаемая система частиц.

**Критерий правильности по файлу:**
1. Частицы продвигаются, мёртвые удаляются.

### Файл: `engine/ecs/src/systems/transform_batch_system.cpp`

**Назначение файла:** система пакетной обработки трансформов.

**Пошаговое описание действий:**
1. Реализовать один `update` над всеми `EcsTransform`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `update(double)`

*Логика функций / методов:*
- система пакетной обработки трансформов — один `update` над всеми `EcsTransform`.

**Результат по файлу:** пакетная обработка трансформов за такт.

**Критерий правильности по файлу:**
1. Пакетная система применяется ко всем сущностям с компонентом за такт.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/ecs/src/systems/particle_system.cpp`
2. `engine/ecs/src/systems/transform_batch_system.cpp`

**Общий критерий правильности:**
1. пакетная система применяется ко всем сущностям с компонентом за такт; частицы видны.
