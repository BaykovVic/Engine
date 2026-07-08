# Спринт 2. День 2

## feature/scene-authoring

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/object-model`, `feature/component-model` (Спринт 1), `feature/scene-world`

**Цель фичи:** авторинг сцены — создание объектов-примитивов с компонентом Mesh Renderer.

**Описание фичи:** свободная функция `createPrimitive` над мирами объектов и компонентов создаёт примитив (Cube/Plane/Sphere) с рендер-компонентом. Вторая фича Этапа 2 контура E1.

**Общий порядок реализации фичи:**
1. Объявить `enum class PrimitiveKind {Cube, Plane, Sphere}` и `AuthoringServices` в `scene_authoring.hpp`.
2. Объявить `createPrimitive`.
3. Реализовать создание примитива с компонентом Mesh Renderer в `scene_authoring.cpp`.

**Файлы фичи:**
1. `engine/scene/include/sky/scene/scene_authoring.hpp`
2. `engine/scene/src/scene_authoring.cpp`

### Файл: `engine/scene/include/sky/scene/scene_authoring.hpp`

**Назначение файла:** контракт авторинга примитивов сцены.

**Пошаговое описание действий:**
1. Объявить `enum class PrimitiveKind {Cube, Plane, Sphere}`.
2. Объявить `AuthoringServices` (ссылки на миры объектов/компонентов).
3. Объявить `createPrimitive`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `enum class PrimitiveKind {Cube, Plane, Sphere}`
- `AuthoringServices` (ссылки на миры объектов/компонентов)

*Функции / методы:*
- `object::ObjectHandle createPrimitive(const AuthoringServices& services, PrimitiveKind kind, const std::string& name)`

*Логика функций / методов:*
- `createPrimitive` — создаёт объект-примитив с компонентом Mesh Renderer. Параметры: `services` — ссылки на миры объектов/компонентов, `kind` — вид, `name` — имя. Возвращает: хэндл объекта.

**Результат по файлу:** контракт авторинга примитивов зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется; `createPrimitive` использует `object::ObjectHandle`.

### Файл: `engine/scene/src/scene_authoring.cpp`

**Назначение файла:** реализация авторинга примитивов.

**Пошаговое описание действий:**
1. Реализовать `createPrimitive` для `PrimitiveKind`.
2. Создать объект в мире объектов и навесить компонент Mesh Renderer через мир компонентов.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `createPrimitive`.

*Логика функций / методов:*
- `createPrimitive(services, kind, name)` — создаёт объект-примитив в мире объектов и навешивает на него компонент Mesh Renderer через мир компонентов; возвращает хэндл объекта.

**Результат по файлу:** рабочее создание примитивов с рендер-компонентом.

**Критерий правильности по файлу:**
1. Созданный примитив имеет компонент Mesh Renderer и корректное имя.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/scene/include/sky/scene/scene_authoring.hpp`
2. `engine/scene/src/scene_authoring.cpp`

**Общий критерий правильности:**
1. `createPrimitive` создаёт объект-примитив с компонентом Mesh Renderer.

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
