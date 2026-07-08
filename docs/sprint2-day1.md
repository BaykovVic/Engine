# Спринт 2. День 1

## feature/scene-world

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/object-model`, `feature/component-model` (Спринт 1); `core::Transform`

**Цель фичи:** модель сцены — репозиторий сцен, рантайм и запросы над сценой.

**Описание фичи:** `SceneWorld` наследует `ISceneRepository`, `ISceneRuntime`, `ISceneQueryService` и создаётся через `SceneWorldDeps`; хранит корневые объекты сцены. Начало Этапа 2 контура E1 — модель сцены и сборка подсистем.

**Общий порядок реализации фичи:**
1. Объявить `SceneWorld` (наследует три контракта) и `SceneWorldDeps` в `scene_world.hpp`.
2. Объявить фабрику `createSceneWorld(const SceneWorldDeps& deps)`.
3. Реализовать хранение корней и операции сцены в `scene_world.cpp`.

**Файлы фичи:**
1. `engine/scene/include/sky/scene/scene_world.hpp`
2. `engine/scene/src/scene_world.cpp`

### Файл: `engine/scene/include/sky/scene/scene_world.hpp`

**Назначение файла:** контракт и объявление мира сцены.

**Пошаговое описание действий:**
1. Объявить `class SceneWorld`, наследующий `ISceneRepository`, `ISceneRuntime`, `ISceneQueryService`.
2. Объявить методы `addRootObject`, `saveSceneAs`, `rootObjectsOf`.
3. Объявить `SceneWorldDeps` и фабрику `createSceneWorld`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class SceneWorld : ISceneRepository, ISceneRuntime, ISceneQueryService`
- `SceneWorldDeps` (зависимости для создания мира сцены)

*Функции / методы:*
- `virtual void addRootObject(SceneHandle scene, object::ObjectHandle object) = 0`
- `virtual bool saveSceneAs(SceneHandle scene, const std::filesystem::path& path, …) = 0`
- `virtual std::vector<object::ObjectHandle> rootObjectsOf(SceneHandle scene) const = 0`
- `std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps)`

*Логика функций / методов:*
- `addRootObject` — добавляет объект в корень сцены.
- `saveSceneAs` — сохраняет сцену (полный SKYB — на этапе 3). Возвращает: успех.
- `rootObjectsOf` — Возвращает: корневые объекты.
- `createSceneWorld` — фабрика мира сцены из `SceneWorldDeps`.

**Результат по файлу:** контракт мира сцены зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется; методы используют `object::ObjectHandle`.

### Файл: `engine/scene/src/scene_world.cpp`

**Назначение файла:** реализация мира сцены.

**Пошаговое описание действий:**
1. Реализовать хранение корневых объектов сцены.
2. Реализовать `addRootObject`, `rootObjectsOf`.
3. Реализовать заглушку `saveSceneAs` (полный SKYB — на этапе 3) и фабрику.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `SceneWorld`.

*Функции / методы:*
- `addRootObject`, `saveSceneAs`, `rootObjectsOf`, `createSceneWorld`.

*Логика функций / методов:*
- `addRootObject(scene, object)` — заносит объект в список корней сцены.
- `rootObjectsOf(scene)` — возвращает корневые объекты сцены.
- `saveSceneAs(scene, path, …)` — сохраняет сцену; на этом этапе полный SKYB не требуется (реализуется на этапе 3), возвращает успех.
- `createSceneWorld(deps)` — создаёт реализацию мира сцены из зависимостей.

**Результат по файлу:** рабочий мир сцены с корневыми объектами.

**Критерий правильности по файлу:**
1. Добавленный корневой объект возвращается через `rootObjectsOf`.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/scene/include/sky/scene/scene_world.hpp`
2. `engine/scene/src/scene_world.cpp`
3. библиотека `sky_scene` собрана.

**Общий критерий правильности:**
1. `SceneWorld` создаётся через `createSceneWorld(deps)`.
2. `addRootObject` + `rootObjectsOf` дают согласованный список корней сцены.

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

## feature/player-runtime

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** `EditorContext` (`feature/editor-context`), рендерер (`feature/vulkan-offscreen`), построитель кадра (`feature/frame-builder`)

**Цель фичи:** автономный проигрыватель с собственным циклом и режимами запуска.

**Описание фичи:** проигрыватель с точкой входа, безоконным и оконным режимами, разбором аргументов и переносимым кодом клавиш. Этап 2 контура E4.

**Общий порядок реализации фичи:**
1. Реализовать `main` с разбором `--frames N`, `--headless out.png`, `--scene path`.
2. Реализовать `runHeadless` (цикл `tickFrame → build → renderFrame`, сохранение PNG).
3. Реализовать `runWindowed` и `mapPlatformKey`.

**Файлы фичи:**
1. `player/src/main.cpp`

### Файл: `player/src/main.cpp`

**Назначение файла:** точка входа и цикл автономного проигрывателя.

**Пошаговое описание действий:**
1. Реализовать `int main(int argc, char** argv)` — разбор `--frames N`, `--headless out.png`, `--scene path`.
2. Реализовать `int runHeadless(EditorContext&, int frames, const char* screenshotPath)`.
3. Реализовать `int runWindowed(EditorContext&, int frameLimit)`.
4. Реализовать `int mapPlatformKey(std::int32_t keysym)`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `int main(int argc, char** argv)`
- `int runHeadless(EditorContext& context, int frames, const char* screenshotPath)`
- `int runWindowed(EditorContext& context, int frameLimit)`
- `int mapPlatformKey(std::int32_t keysym)`

*Логика функций / методов:*
- `main` — точка входа; разбирает `--frames N`, `--headless out.png`, `--scene path`. Возвращает: код выхода.
- `runHeadless` — безоконный прогон — цикл `tickFrame → build → renderFrame`, сохранение PNG. Параметры: `context`, `frames` — число кадров, `screenshotPath` — файл. Возвращает: код выхода.
- `runWindowed` — оконный прогон (окно X11/Cocoa + swapchain-рендерер). Параметры: `context`, `frameLimit`. Возвращает: код выхода.
- `mapPlatformKey` — переводит платформенный код клавиши в переносимый (общий с C#). Возвращает: переносимый код или 0.

**Результат по файлу:** бинарь `sky_player`; безоконный режим пишет PNG.

**Критерий правильности по файлу:**
1. `sky_player --headless out.png` формирует изображение кадра.

### На выходе должно получиться

**Список артефактов фичи:**
1. `player/src/main.cpp`
2. бинарь `sky_player`; безоконный режим пишет PNG.

**Общий критерий правильности:**
1. `sky_player --headless out.png` формирует изображение кадра.

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

## feature/ecs-object-sync

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** `feature/ecs-core` (Спринт 1), `feature/object-model`; интеграция в `scene_world.cpp`

**Цель фичи:** явный контракт синхронизации ECS с объектным миром.

**Описание фичи:** `IEcsObjectSync` связывает объекты и сущности и переносит трансформ вокруг такта (push до, pull после), без неявного двойного владения. Этап 2 контура E6.

**Общий порядок реализации фичи:**
1. Объявить `EcsTransform` и `IEcsObjectSync` в `object_sync.hpp`.
2. Объявить фабрику `createEcsObjectSync`.
3. Реализовать привязку и двустороннюю синхронизацию в `object_sync.cpp`.

**Файлы фичи:**
1. `engine/ecs/include/sky/ecs/object_sync.hpp`
2. `engine/ecs/src/object_sync.cpp`

### Файл: `engine/ecs/include/sky/ecs/object_sync.hpp`

**Назначение файла:** контракт синхронизации ECS↔объектный мир.

**Пошаговое описание действий:**
1. Объявить `struct EcsTransform { core::Transform value; }`.
2. Объявить `class IEcsObjectSync` с методами связывания и синхронизации.
3. Объявить фабрику `createEcsObjectSync`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct EcsTransform { core::Transform value; }`
- `class IEcsObjectSync`

*Функции / методы:*
- `virtual EntityId bind(object::ObjectHandle object) = 0`
- `virtual void unbind(object::ObjectHandle object) = 0`
- `virtual EntityId entityOf(object::ObjectHandle object) const = 0`
- `virtual object::ObjectHandle objectOf(EntityId entity) const = 0`
- `virtual void pushAuthoringState() = 0`
- `virtual void pullEcsResults() = 0`
- `std::unique_ptr<IEcsObjectSync> createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)`

*Логика функций / методов:*
- `EcsTransform` — базовый компонент трансформа.
- `bind(object)` — связывает объект с сущностью. Возвращает: сущность.
- `unbind(object)` — разрывает связь.
- `entityOf(object)` — Возвращает: сущность по объекту.
- `objectOf(entity)` — Возвращает: объект по сущности.
- `pushAuthoringState()` — до такта переносит трансформ объекта в `EcsTransform`.
- `pullEcsResults()` — после такта переносит результат обратно в объектный мир.
- `createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)` — фабрика.

**Результат по файлу:** контракт синхронизации зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется; использует `EntityId` и `object::ObjectHandle`.

### Файл: `engine/ecs/src/object_sync.cpp`

**Назначение файла:** реализация синхронизации ECS↔объектный мир.

**Пошаговое описание действий:**
1. Реализовать привязку сущность↔объект (`bind`/`unbind`/`entityOf`/`objectOf`).
2. Реализовать `pushAuthoringState` и `pullEcsResults`.
3. Дать фабрику `createEcsObjectSync`; точка интеграции — цикл такта в `scene_world.cpp` (`pushAuthoringState()` → `tick(dt)` → `pullEcsResults()`).

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `IEcsObjectSync`.

*Функции / методы:*
- `bind`, `unbind`, `entityOf`, `objectOf`, `pushAuthoringState`, `pullEcsResults`, `createEcsObjectSync`.

*Логика функций / методов:*
- `bind(object)` — создаёт/находит сущность для объекта, ведёт двустороннее отображение; возвращает сущность.
- `unbind(object)` — убирает связь объекта и сущности.
- `entityOf`/`objectOf` — читают отображение в обе стороны.
- `pushAuthoringState()` — до такта переносит трансформ объекта в `EcsTransform`.
- `pullEcsResults()` — после такта переносит результат обратно в объектный мир.
- `createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)` — создаёт реализацию; интегрируется в цикл такта `scene_world.cpp` (совместно с E1): `pushAuthoringState()` → `tick(dt)` → `pullEcsResults()`.

**Результат по файлу:** рабочая двусторонняя синхронизация вокруг такта.

**Критерий правильности по файлу:**
1. Объект, обработанный ECS-системой, получает изменённый трансформ в объектном мире; при паузе не меняется.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/ecs/include/sky/ecs/object_sync.hpp`
2. `engine/ecs/src/object_sync.cpp`
3. привязка сущность↔объект; двусторонняя синхронизация вокруг такта; тест в `integrity_tests`.

**Общий критерий правильности:**
1. Объект, обработанный ECS-системой, получает изменённый трансформ в объектном мире.
2. При паузе трансформ не меняется.
