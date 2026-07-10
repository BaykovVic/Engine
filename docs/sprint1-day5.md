# Спринт 1. День 5

## feature/core-tests

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/math-and-handles`, `feature/object-model`, `feature/component-model`

**Цель фичи:** проверить математику, объектный и компонентный миры.

**Описание фичи:** зафиксировать корректность фундамента тестами; закрывает Этап 1 контура E1.

**Общий порядок реализации фичи:**
1. Написать проверки математики в `core_tests.cpp`.
2. Написать проверки объектного и компонентного миров в `world_tests.cpp`.

**Файлы фичи:**
1. `tests/core_tests.cpp`
2. `tests/world_tests.cpp`

### Файл: `tests/core_tests.cpp`

**Назначение файла:** тесты математики.

**Пошаговое описание действий:**
1. Проверить `rotate`.
2. Проверить `compose`/`invCompose`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:* повороты и композиция трансформов дают ожидаемые значения (`rotate`/`compose`/`invCompose`).

**Результат по файлу:** зелёный тест `core_tests`.

**Критерий правильности по файлу:**
1. `rotate`/`compose`/`invCompose` проходят проверки.

### Файл: `tests/world_tests.cpp`

**Назначение файла:** тесты объектного и компонентного миров.

**Пошаговое описание действий:**
1. Проверить иерархию и мировые трансформы.
2. Проверить запись/чтение полей всех 5 типов.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:* иерархия и мировые трансформы корректны; поле каждого из 5 типов пишется и читается без потерь.

**Результат по файлу:** зелёный тест `world_tests`.

**Критерий правильности по файлу:**
1. Мировой трансформ ребёнка корректен; поля 5 типов без потерь.

### На выходе должно получиться

**Список артефактов фичи:**
1. `tests/core_tests.cpp`
2. `tests/world_tests.cpp`
3. библиотеки `sky_core`, `sky_object`, `sky_component` собраны и линкуются; тесты `core_tests`, `world_tests` зелёные

**Общий критерий правильности:**
1. Поворот (0,0,1) на 90° вокруг Y = (1,0,0)±1e-5.
2. Ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире = (0,0,-1).
3. Поле каждого из 5 типов записывается и читается без потерь.

---

## feature/frame-builder

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/render-contract`, `feature/vulkan-mesh-lighting`, `feature/scene-world`; `EditorContext`

**Цель фичи:** построитель кадра, обходящий сцену и формирующий поток команд отрисовки.

**Описание фичи:** `FrameBuilder` привязан к `EditorContext` и фабрике ресурсов, обходит сцену и даёт непустой поток `RenderCommand`; плюс Qt-free фасад вьюпорта. Вторая фича Этапа 2 контура E2.

**Общий порядок реализации фичи:**
1. Объявить `FrameBuilder` с конструктором, `setCamera` и `build` в `frame_builder.hpp`.
2. Реализовать обход сцены и формирование потока команд.
3. Объявить Qt-free фасад вьюпорта в `viewport_bridge.hpp`.

**Файлы фичи:**
1. `editor/shell/src/frame_builder.hpp`
2. `editor/viewport_bridge/include/sky/editor/viewport/viewport_bridge.hpp`

### Файл: `editor/shell/src/frame_builder.hpp`

**Назначение файла:** построитель потока команд отрисовки из сцены.

**Пошаговое описание действий:**
1. Объявить `FrameBuilder(EditorContext&, rendering::IRenderResourceFactory&)`.
2. Объявить `setCamera(std::optional<core::Transform> pose, float orthoHeight = 0.0f)`.
3. Объявить `build(std::uint32_t width, std::uint32_t height)`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class FrameBuilder`

*Функции / методы:*
- `FrameBuilder(EditorContext& context, rendering::IRenderResourceFactory& factory)`
- `void setCamera(std::optional<core::Transform> pose, float orthoHeight = 0.0f)`
- `std::vector<rendering::RenderCommand> build(std::uint32_t width, std::uint32_t height)`

*Логика функций / методов:*
- `FrameBuilder(context, factory)` — конструктор — привязывает построитель к контексту и фабрике ресурсов. Параметры: `context`, `factory`.
- `setCamera(pose, orthoHeight)` — задаёт камеру кадра. Параметры: `pose` — поза камеры (nullopt = камера сцены), `orthoHeight` — высота орто-проекции (0 = перспектива).
- `build(width, height)` — обходит сцену и формирует поток команд отрисовки. Параметры: `width`, `height` — размер кадра. Возвращает: список команд.

**Результат по файлу:** построитель кадра, дающий поток `RenderCommand`.

**Критерий правильности по файлу:**
1. `build` даёт непустой поток команд для демо-сцены.

### Файл: `editor/viewport_bridge/include/sky/editor/viewport/viewport_bridge.hpp`

**Назначение файла:** Qt-free фасад вьюпорта, общий для редактора и плеера.

**Пошаговое описание действий:**
1. Объявить `IPlayModeController` и `IRuntimePreviewHost`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `IPlayModeController`
- `IRuntimePreviewHost`

*Функции / методы:* контракты фасада вьюпорта.

*Логика функций / методов:*
- Qt-free фасад вьюпорта (`IPlayModeController`, `IRuntimePreviewHost`), общий для редактора и плеера.

**Результат по файлу:** общий фасад вьюпорта.

**Критерий правильности по файлу:**
1. Заголовок компилируется без зависимости от Qt.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/frame_builder.hpp`
2. `editor/viewport_bridge/include/sky/editor/viewport/viewport_bridge.hpp`

**Общий критерий правильности:**
1. Демо-сцена рендерится с освещением.
2. `FrameBuilder` даёт непустой поток команд (`RenderCommand` не пуст).

---

## feature/hierarchy-tree

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/engine-bridge`, `feature/vulkan-viewport`; C-интерфейс (реализация — в мосте контура E5)

**Цель фичи:** дерево объектов на живых данных через C-интерфейс.

**Описание фичи:** дополнение P/Invoke иерархии и трансформа, перечитывание иерархии в `ObservableCollection<SkyObject>` и `TreeView` по корням. Вторая фича Этапа 2 контура E3.

**Общий порядок реализации фичи:**
1. Добавить P/Invoke иерархии и трансформа в `EngineInterop.cs`.
2. Реализовать `Reload`, `Load`, `Transform` в `EditorSession.cs`.
3. Собрать `HierarchyView` (`TreeView` по `Roots`).

**Файлы фичи:**
1. `editor/avalonia/Engine/EngineInterop.cs`
2. `editor/avalonia/Engine/EditorSession.cs`
3. `editor/avalonia/Views/HierarchyView.axaml.cs`

### Файл: `editor/avalonia/Engine/EngineInterop.cs`

**Назначение файла:** дополнение P/Invoke иерархии и трансформа.

**Пошаговое описание действий:**
1. Объявить P/Invoke иерархии и трансформа (реализация — в мосте контура E5).

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- P/Invoke: `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`, `sky_editor_child_at`, `sky_editor_object_name`, `sky_editor_get_transform`, `sky_editor_render_offscreen`, `sky_editor_viewport_orbit`, `sky_editor_viewport_zoom`.

*Логика функций / методов:*
- P/Invoke иерархии и трансформа (реализация — в мосте контура E5): `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`, `sky_editor_child_at`, `sky_editor_object_name`, `sky_editor_get_transform`, `sky_editor_render_offscreen`, `sky_editor_viewport_orbit`, `sky_editor_viewport_zoom`.

**Результат по файлу:** объявлены вызовы C-интерфейса иерархии/трансформа/вьюпорта.

**Критерий правильности по файлу:**
1. P/Invoke-объявления компилируются и резолвятся в мосте.

### Файл: `editor/avalonia/Engine/EditorSession.cs`

**Назначение файла:** дополнение сессии перечитыванием иерархии и трансформов.

**Пошаговое описание действий:**
1. Реализовать `Reload()` — перечитывание иерархии в `ObservableCollection<SkyObject> Roots`.
2. Реализовать `Load(ulong id)` — рекурсивное построение узла.
3. Реализовать `Transform(ulong id)`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `ObservableCollection<SkyObject> Roots`

*Функции / методы:*
- `public void Reload()`
- `private SkyObject Load(ulong id)`
- `public (float[] position, float[] rotation, float[] scale) Transform(ulong id)`

*Логика функций / методов:*
- `Reload()` — перечитывает иерархию через C-интерфейс в `ObservableCollection<SkyObject> Roots`.
- `Load(id)` — рекурсивно строит узел дерева.
- `Transform(id)` — читает трансформ объекта. Параметры: `id`. Возвращает: позицию (3), кватернион (4), масштаб (3).

**Результат по файлу:** сессия отдаёт живое дерево объектов и трансформы.

**Критерий правильности по файлу:**
1. `Reload()` наполняет `Roots` именами и вложенностью объектов движка.

### Файл: `editor/avalonia/Views/HierarchyView.axaml.cs`

**Назначение файла:** панель дерева объектов.

**Пошаговое описание действий:**
1. Собрать `TreeView` по `Roots`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class HierarchyView` (`TreeView` по `Roots`)

*Функции / методы:* нет (композиция вью).

*Логика функций / методов:*
- `HierarchyView` — `TreeView` по `Roots`.

**Результат по файлу:** панель с деревом объектов сцены.

**Критерий правильности по файлу:**
1. В дереве — имена и вложенность объектов движка.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Engine/EngineInterop.cs` (P/Invoke иерархии/трансформа)
2. `editor/avalonia/Engine/EditorSession.cs` (`Reload`/`Load`/`Transform`)
3. `editor/avalonia/Views/HierarchyView.axaml.cs`

**Общий критерий правильности:**
1. Дерево объектов отражает сцену.
2. В дереве — имена и вложенность объектов движка.

---

## feature/input-state

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** `EditorContext` (`feature/editor-context`)

**Цель фичи:** состояние клавиш, доступное движку и скриптам.

**Описание фичи:** часть Этапа 3 (режим воспроизведения и ввод) — inline-методы состояния ввода в `EditorContext`.

**Общий порядок реализации фичи:**
1. Дополнить `editor_context.hpp` inline-методами состояния ввода.

**Файлы фичи:**
1. `editor/shell/src/editor_context.hpp`

### Файл: `editor/shell/src/editor_context.hpp`

**Назначение файла:** состояние клавиш в контексте редактора.

**Пошаговое описание действий:**
1. Объявить inline-методы `setKeyDown` и `keyDown`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `EditorContext`, методы объявлены inline).

*Функции / методы:*
- `void setKeyDown(int key, bool down)`
- `bool keyDown(int key) const`

*Логика функций / методов:*
- `setKeyDown(key, down)` — заносит/снимает клавишу. Параметры: `key` — переносимый код, `down` — нажата ли.
- `keyDown(key)` — Возвращает: нажата ли клавиша (читается скриптами через `Input`).

**Результат по файлу:** состояние клавиш доступно движку.

**Критерий правильности по файлу:**
1. `keyDown(key)` отражает предыдущий `setKeyDown(key, ...)`.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/editor_context.hpp`

**Общий критерий правильности:**
1. состояние клавиш доступно движку; после `play → stop` сцена в исходном состоянии, тела без остаточной скорости.

---

## feature/asset-database

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** нет

**Цель фичи:** база ассетов и контракт импортёра.

**Описание фичи:** часть Этапа 3 (импортёры, виртуальная ФС, тесты интерфейса) — контракт `IAssetImporter` и `AssetDatabase` с импортом и разрешением ассетов.

**Общий порядок реализации фичи:**
1. Объявить `IAssetImporter` в `asset_system.hpp`.
2. Объявить `AssetDatabase` и фабрику в `asset_database.hpp`.
3. Реализовать базу ассетов в `asset_database.cpp`.

**Файлы фичи:**
1. `engine/asset/include/sky/asset/asset_system.hpp`
2. `engine/asset/include/sky/asset/asset_database.hpp`
3. `engine/asset/src/asset_database.cpp`

### Файл: `engine/asset/include/sky/asset/asset_system.hpp`

**Назначение файла:** контракт импортёра ассетов.

**Пошаговое описание действий:**
1. Объявить `IAssetImporter`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class IAssetImporter`

*Функции / методы:*
- `virtual bool supports(...) const = 0`
- `virtual ImportResult import(...) = 0`

*Логика функций / методов:*
- `supports(...)` — Возвращает: поддерживает ли формат.
- `import(...)` — Возвращает: результат импорта.

**Результат по файлу:** контракт импортёра зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/asset/include/sky/asset/asset_database.hpp`

**Назначение файла:** база ассетов.

**Пошаговое описание действий:**
1. Объявить `AssetDatabase` и фабрику.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class AssetDatabase`

*Функции / методы:*
- `AssetId importAsset(const std::filesystem::path& path)`
- `std::optional<AssetDescriptor> resolve(AssetId id)`
- `std::unique_ptr<AssetDatabase> createAssetDatabase(...)`

*Логика функций / методов:*
- `importAsset(path)` — импортирует ассет. Возвращает: id.
- `resolve(id)` — Возвращает: описание ассета или `nullopt`.
- `createAssetDatabase(...)` — фабрика.

**Результат по файлу:** контракт базы ассетов зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/asset/src/asset_database.cpp`

**Назначение файла:** реализация базы ассетов.

**Пошаговое описание действий:**
1. Реализовать импорт и разрешение ассетов.
2. Дать фабрику `createAssetDatabase`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `AssetDatabase`.

*Функции / методы:*
- `importAsset`, `resolve`, `createAssetDatabase`.

*Логика функций / методов:*
- `importAsset(path)` — выбирает подходящий импортёр (`supports`), импортирует и заводит запись, возвращает id.
- `resolve(id)` — возвращает описание ассета по id или `nullopt`.
- `createAssetDatabase(...)` — фабрика.

**Результат по файлу:** рабочая база ассетов.

**Критерий правильности по файлу:**
1. Импортированный ассет разрешается через `resolve` по своему id.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/asset/include/sky/asset/asset_system.hpp`
2. `engine/asset/include/sky/asset/asset_database.hpp`
3. `engine/asset/src/asset_database.cpp`

**Общий критерий правильности:**
1. `asset_project_tests` зелёный (импорт и разрешение ассетов).

---

## feature/ecs-multithreading

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** `feature/ecs-core` (мир и планировщик), `feature/ecs-object-sync` (`pullEcsResults`)

**Цель фичи:** параллельный tick систем через планировщик задач ядра.

**Описание фичи:** единственная фича Этапа 4 контура E6 — раскладка систем по `IJobScheduler` и барьер перед синхронизацией.

**Общий порядок реализации фичи:**
1. Использовать контракт `IJobScheduler` из заготовки `job_scheduler.hpp`.
2. В `tick` разложить независимые системы по `schedule`, зависимые — через `scheduleAfter`.
3. Поставить барьер `wait` перед `pullEcsResults`.

**Файлы фичи:**
1. `engine/core/include/sky/core/job_scheduler.hpp`
2. `engine/ecs/src/ecs_world.cpp`

### Файл: `engine/core/include/sky/core/job_scheduler.hpp`

**Назначение файла:** контракт планировщика задач (использовать существующую заготовку).

**Пошаговое описание действий:**
1. Объявить `IJobScheduler` с постановкой задач и ожиданием.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class IJobScheduler`

*Функции / методы:*
- `virtual JobHandle schedule(Job job) = 0`
- `virtual JobHandle scheduleAfter(JobHandle dependency, Job job) = 0`
- `virtual void wait(JobHandle job) = 0`

*Логика функций / методов:*
- `schedule(job)` — ставит задачу в очередь. Возвращает: хэндл задачи.
- `scheduleAfter(dependency, job)` — задача после зависимости.
- `wait(job)` — ждёт завершения.

**Результат по файлу:** контракт планировщика задач зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/ecs/src/ecs_world.cpp`

**Назначение файла:** дополнение мира ECS — параллельный tick.

**Пошаговое описание действий:**
1. Разложить независимые системы по `schedule`.
2. Зависимые системы поставить через `scheduleAfter`.
3. Поставить барьер `wait` перед `pullEcsResults`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение реализации `EcsWorld`).

*Функции / методы:* `tick`.

*Логика функций / методов:*
- `tick` раскладывает независимые системы по `schedule`, зависимые — через `scheduleAfter`; барьер `wait` перед `pullEcsResults`.

**Результат по файлу:** параллельный tick систем.

**Критерий правильности по файлу:**
1. Результат детерминирован и совпадает с однопоточным.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/core/include/sky/core/job_scheduler.hpp`
2. `engine/ecs/src/ecs_world.cpp` (параллельный `tick`)

**Общий критерий правильности:**
1. Параллельный tick независимых систем через `IJobScheduler`.
2. Результат детерминирован и совпадает с однопоточным (тест эквивалентности).
