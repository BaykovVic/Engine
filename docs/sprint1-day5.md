# Спринт 1. День 5

## Общие требования ко всем фичам

1. **Один PR — одна фича.** Не смешивать фичи и не менять файлы чужих контуров (например, `engine/core/include/sky/core/math.hpp` принадлежит E1).
2. **Никаких артефактов сборки в git**: `.exe`, `.o`, `.obj`, `.spv`, каталоги `build/` — запрещены. Временные файлы (`tests/tmp/`) в PR не включать.
3. **Namespace модуля обязателен** (`sky::core`, `sky::rendering`, `sky::object`, `sky::physics`, `sky::ecs`, ...). Код в глобальном namespace не принимается.
4. **Интерфейсы**: секция `public:`, виртуальный деструктор `virtual ~IИмя() = default;`, чисто виртуальные методы (`= 0`).
5. **Сигнатуры из задания копируются символ в символ** — включая `const`, `[[nodiscard]]`, типы возврата и параметры по умолчанию.
6. **Include-стиль**: `#include "sky/<модуль>/<файл>.hpp"` при `-Iengine/<модуль>/include`; пути от корня репозитория запрещены. `<bits/stdc++.h>` запрещён, `#pragma once` обязателен в каждом заголовке.
7. **Хэндлы** — только `core::Handle<Tag>`; в контейнерах ключ — `handle.value`. Собственные `std::hash<Handle>` и операторы в чужие заголовки не добавлять.
8. **Критерий приёмки каждой фичи — её приёмочный тест** (указан в конце блока фичи). PR без зелёного теста не рассматривается.

---

## feature/core-tests

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/math-and-handles`, `feature/object-model`, `feature/component-model`

**Цель фичи:** проверить математику, объектный и компонентный миры.

**Описание фичи:** зафиксировать корректность фундамента тестами; закрывает Этап 1 контура E1.

**Обязательные требования:**

1. **Тесты пишутся самостоятельно.** Это мета-фича: продукт дня — сами тесты. Копирование чужих готовых тестов не принимается; приёмка идёт по чек-листу покрытия (см. «Приёмочный тест» ниже).
2. **Оформление.** Оба файла используют харнесс `tests/sky_test.hpp` (`CHECK`, `sky::test::summary`); `main()` возвращает результат `summary(...)` (0 — успех, 1 — есть провалы). Тесты зарегистрированы в `tests/CMakeLists.txt`, видны в `ctest` и линкуются с `sky_core`, `sky_object`, `sky_component`. Инклюды — в стиле `#include "sky/core/math.hpp"` (от include-корня модуля).
3. **Обязательное покрытие `core_tests.cpp` (математика):**
   - `rotate`: поворот на 90° вокруг Y переводит (0,0,1) в (1,0,0) с точностью ±1e-5 (кватернион `{0, sin45, 0, cos45}`); сравнения `float` — с допуском (`std::fabs(...) < eps`), не через `==`;
   - `compose`: родитель с переносом И масштабом ≠ 1 — позиция ребёнка масштабируется и складывается; масштаб перемножается по компонентам;
   - `invCompose`: круговая проверка `invCompose(parent, compose(parent, child)) ≈ child`, где `parent` имеет одновременно перенос, поворот и масштаб.
4. **Обязательное покрытие `world_tests.cpp` (объектный мир):** `createObject`/`exists`/`nameOf`/`renameObject`; согласованность `setParent`/`parentOf`/`childrenOf` (у корня `parentOf` — невалидный хендл); отказ от цикла (перевесить предка под потомка нельзя); композиция трансформов по цепочке; ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире ≈ (0,0,-1); `setWorldTransform` читается обратно через `worldTransform`; `findByName` возвращает ВСЕ совпадения и пустой вектор для неизвестного имени; `destroyObject` удаляет всё поддерево.
5. **Обязательное покрытие `world_tests.cpp` (компонентный мир):** `registerComponentType` + `availableTypes`; `attach` — валидный хендл для зарегистрированного типа, невалидный для незарегистрированного; согласованность `componentsOf`/`ownerOf`/`descriptorOf`; `detach` и `detachAllFrom`; поле КАЖДОГО из 5 типов `FieldValue` (`float`, `std::int64_t`, `bool`, `std::string`, `core::Vec3`) пишется через `setField` и читается через `field` без потерь; `fields(...)` возвращает все 5; отсутствующее имя поля → `nullopt`.
6. **Детерминизм.** Без потоков, без чтения внешних файлов, без зависимости от порядка запусков.
7. **Вне объёма дня:** event bus, job scheduler, диагностика, конфиг-сервис — покрывать не требуется.

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

- **Приёмочный тест:** `tests/day5/core_tests_checklist.md` — покрытие сдаваемых тестов сверяется проверяющим по этому чек-листу (оформление + math + объектный/компонентный миры); `ctest -R "core_tests|world_tests"` зелёный, а при искусственной поломке любой проверки возвращает код 1.

---

## feature/frame-builder

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/render-contract`, `feature/vulkan-mesh-lighting`. Полный объём (обход живой сцены) дополнительно требует `feature/scene-world` и `EditorContext` (`feature/editor-context`), которые появляются позже, — в этот день строится ядро построителя против этих абстракций; приёмка идёт на дублёре фабрики ресурсов (NullRenderer), GPU не нужен.

**Цель фичи:** построитель кадра, обходящий сцену и формирующий поток команд отрисовки.

**Описание фичи:** `FrameBuilder` привязан к `EditorContext` и фабрике ресурсов, обходит сцену и даёт непустой поток `RenderCommand`; плюс Qt-free фасад вьюпорта. Вторая фича Этапа 2 контура E2.

**Обязательные требования:**

Публичный контракт построителя (namespace `sky::editor`; файл header-only — методы реализуются inline), объявления — символ в символ:

```cpp
// editor/shell/src/frame_builder.hpp
class FrameBuilder {
public:
    FrameBuilder(EditorContext& context, rendering::IRenderResourceFactory& factory);
    void setCamera(std::optional<core::Transform> pose, float orthoHeight = 0.0f);
    std::vector<rendering::RenderCommand> build(std::uint32_t width,
                                                std::uint32_t height);
};
```

- **Порядок команд кадра фиксирован:** `build` начинается с `BeginFrame` (ровно один, первая команда) → ровно один `SetViewport` с переданными `width`/`height` → ровно один `SetCamera` (раньше любого `DrawMesh`) → `AddLight` для каждого включённого источника света → `DrawMesh` для каждого включённого Mesh Renderer → `EndFrame` (ровно один, последняя команда).
- **`setCamera`:** заданная поза попадает в команду `SetCamera` без изменений; `nullopt` — возврат к камере сцены (объект `Main Camera`, при его отсутствии — запасная поза); `orthoHeight > 0` — ортографическая проекция, `0` — перспектива.
- **Детерминированность:** повторный `build` на неизменённой сцене даёт тот же поток команд; объекты обходятся в порядке иерархии (корни и их дети по порядку).
- **Никакого GPU в ядре:** построитель работает только через `rendering::IRenderResourceFactory` и абстракции контекста (объекты/компоненты/трансформы) — проверяется дублёром фабрики (NullRenderer).
- **Qt-free фасад:** `viewport_bridge.hpp` не включает Qt-заголовков; контракты `IPlayModeController` (`play`/`pause`/`stop`/`state`/`onStateChanged`) и `IRuntimePreviewHost` (`attachSurface`/`detachSurface`/`context`) — чисто виртуальные, в namespace `sky::editor`.

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

- **Приёмочный тест:** `tests/day5/frame_builder_tests.cpp` — собирается с NullRenderer в роли фабрики ресурсов (GPU/Vulkan не нужен); проверяет обрамление кадра `BeginFrame`/`EndFrame`, единственный `SetCamera` раньше любого `DrawMesh`, размеры кадра в `SetViewport` и передачу позы/`orthoHeight` из `setCamera`; код выхода 0. Полная сборка возможна после появления `EditorContext` (спринт 2) — см. шапку теста.

---

## feature/hierarchy-tree

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/engine-bridge`, `feature/vulkan-viewport`; C-интерфейс (реализация — в мосте контура E5)

**Цель фичи:** дерево объектов на живых данных через C-интерфейс.

**Описание фичи:** дополнение P/Invoke иерархии и трансформа, перечитывание иерархии в `ObservableCollection<SkyObject>` и `TreeView` по корням. Вторая фича Этапа 2 контура E3.

**Обязательные требования:**

1. **Обязательные файлы вью — оба:** `HierarchyView.axaml` (разметка; именно в ней объявляется дерево) и `HierarchyView.axaml.cs` (code-behind).
2. **Именно `TreeView`, не `ListBox`:** `<TreeView ItemsSource="{Binding Main.Roots}">` с иерархическим шаблоном `<TreeDataTemplate ItemsSource="{Binding Children}">`; узел показывает имя объекта (`{Binding Name}`).
3. **P/Invoke иерархии и трансформа — символ в символ** (реализация — в мосте контура E5):

```csharp
// editor/avalonia/Engine/EngineInterop.cs
[DllImport(Lib)] public static extern int sky_editor_root_count(IntPtr ctx);
[DllImport(Lib)] public static extern ulong sky_editor_root_at(IntPtr ctx, int index);
[DllImport(Lib)] public static extern int sky_editor_child_count(IntPtr ctx, ulong obj);
[DllImport(Lib)] public static extern ulong sky_editor_child_at(IntPtr ctx, ulong obj, int index);
[DllImport(Lib)] public static extern int sky_editor_object_name(IntPtr ctx, ulong obj, byte[] buffer, int capacity);
[DllImport(Lib)] public static extern void sky_editor_get_transform(IntPtr ctx, ulong obj, float[]? position, float[]? rotation, float[]? scale);
[DllImport(Lib)] public static extern int sky_editor_render_offscreen(IntPtr ctx, uint width, uint height, byte[] outRgba, int outLength);
[DllImport(Lib)] public static extern void sky_editor_viewport_orbit(IntPtr ctx, float deltaYawDegrees, float deltaPitchDegrees);
[DllImport(Lib)] public static extern void sky_editor_viewport_zoom(IntPtr ctx, float factor);
```

4. **Сессия:** класс `SkyObject` (`Id`, `Name`, `ObservableCollection<SkyObject> Children`) и `ObservableCollection<SkyObject> Roots`; `public void Reload()` перечитывает корни через `sky_editor_root_count`/`sky_editor_root_at`; `Load(id)` строит узел рекурсивно (`sky_editor_object_name` + `sky_editor_child_count`/`sky_editor_child_at`); `Transform(id)` возвращает позицию (3), кватернион (4), масштаб (3).
5. **Живые данные:** имена и вложенность в дереве приходят из движка через C-интерфейс — хардкод-заглушки не принимаются.

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

- **Приёмочный тест:** `tests/day5/hierarchy_tree_check.py` — `python3 tests/day5/hierarchy_tree_check.py <корень_репозитория>`, код выхода 0 (структурная проверка: все 9 P/Invoke, `Roots`/`Reload`/`Load`/`Transform`, `TreeView` с `TreeDataTemplate ItemsSource="{Binding Children}"`); плюс ручной smoke-тест из шапки скрипта, когда доступны dotnet и мост E5.

---

## feature/input-state

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** нет. Полный `EditorContext` (`feature/editor-context`) появляется только в спринте 2 — в этот день `editor/shell/src/editor_context.hpp` создаётся как минимальный самостоятельный каркас класса с состоянием ввода; позже его дорастит спринт 2.

**Цель фичи:** состояние клавиш, доступное движку и скриптам.

**Описание фичи:** часть Этапа 3 (режим воспроизведения и ввод) — inline-методы состояния ввода в `EditorContext`.

**Обязательные требования:**

Полного `EditorContext` в этот день ещё нет — файл создаётся как минимальный самостоятельный каркас класса. Сигнатуры методов — символ в символ:

```cpp
// editor/shell/src/editor_context.hpp (namespace sky::editor; минимальный каркас)
class EditorContext {
public:
    void setKeyDown(int key, bool down);        // реализуется inline
    [[nodiscard]] bool keyDown(int key) const;  // реализуется inline
private:
    std::unordered_set<int> keysDown_;
};
```

1. **Семантика множества, а не счётчика:** `setKeyDown(key, true)` заносит клавишу в `keysDown_`, `setKeyDown(key, false)` снимает; никакого счётчика вложенных нажатий и никакой пофреймовой очистки на этом уровне нет — состояние удержания живёт, пока клавишу не отпустили.
2. **Идемпотентность автоповтора:** клавиатура шлёт KeyDown многократно — повторное `setKeyDown(key, true)` ничего не «копит», одного отпускания достаточно; отпускание ненажатой (или уже отпущенной) клавиши безопасно и ничего не меняет.
3. **Аккорды:** несколько клавиш удерживаются одновременно (WASD + модификаторы); отпускание одной не трогает остальные.
4. **Const-чтение:** `keyDown` обязан быть `const` и возвращать `bool` — вызывается на const-контексте, чтение не мутирует состояние (скрипты только читают через `Input.GetKey`).
5. **Переносимые коды клавиш:** ASCII верхнего регистра для букв/цифр, пробел = 32, именованные клавиши с 256 (зеркалятся managed-перечислением `SkyEngine.KeyCode`).

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

- **Приёмочный тест:** `tests/day5/input_state_tests.cpp` — компилируется одним заголовком (`g++ -std=c++20 -O1 -I editor/shell/src tests/day5/input_state_tests.cpp`; линковать движок не нужно — методы inline) и проходит с кодом выхода 0: исходное состояние, нажатие/отпускание, аккорды, идемпотентность автоповтора, const-чтение.

---

## feature/asset-database

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** нет

**Цель фичи:** база ассетов и контракт импортёра.

**Описание фичи:** часть Этапа 3 (импортёры, виртуальная ФС, тесты интерфейса) — контракт `IAssetImporter` и `AssetDatabase` с импортом и разрешением ассетов.

**Обязательные требования:**

Контракт модуля (namespace `sky::asset`). Интерфейсы приводятся без секций `public:` и виртуальных деструкторов (`virtual ~IИмя() = default;` обязателен по общим требованиям, п. 4); объявления — символ в символ, при расхождении с сокращёнными сигнатурами в описаниях файлов ниже приоритет у этого блока:

```cpp
// engine/asset/include/sky/asset/asset_system.hpp
struct AssetId {
    std::uint64_t value = 0;
    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
    auto operator<=>(const AssetId&) const = default;
};
struct AssetDescriptor {
    AssetId id;
    std::string assetType;
    std::filesystem::path sourcePath;
    std::vector<AssetId> dependencies;
    std::uint64_t contentVersion = 0;
};
class IAssetResolver {
    [[nodiscard]] virtual std::optional<AssetDescriptor> resolve(AssetId id) const = 0;
    [[nodiscard]] virtual std::optional<AssetId> findBySourcePath(
        const std::filesystem::path& sourcePath) const = 0;
};
class IAssetRegistry {
    virtual AssetId registerAsset(const AssetDescriptor& descriptor) = 0;
    virtual void unregisterAsset(AssetId id) = 0;
    [[nodiscard]] virtual std::vector<AssetId> dependentsOf(AssetId id) const = 0;
    [[nodiscard]] virtual std::vector<AssetDescriptor> allAssets() const = 0;
};
class IAssetImporter {
    [[nodiscard]] virtual bool supports(const std::filesystem::path& sourcePath) const = 0;
    virtual std::optional<AssetDescriptor> import(const std::filesystem::path& sourcePath) = 0;
};
class IImportPipeline {
    virtual void registerImporter(IAssetImporter& importer) = 0;
    virtual std::optional<AssetId> importAsset(const std::filesystem::path& sourcePath) = 0;
    virtual bool reimport(AssetId id) = 0;
};
// engine/asset/include/sky/asset/asset_database.hpp
class AssetDatabase : public IAssetResolver, public IAssetRegistry, public IImportPipeline {
public:
    ~AssetDatabase() override = default;
};
std::unique_ptr<AssetDatabase> createAssetDatabase();
AssetId assetIdFromPath(const std::filesystem::path& sourcePath);
```

1. **`assetIdFromPath` — стабильная валидная идентичность** из нормализованного пути источника (FNV-1a): одинаковый путь → одинаковый id (переживает переоткрытие проекта и повторный импорт), разный путь → разный id; разделители и `.`-сегменты нормализуются; всё ссылается на `AssetId`, никогда на путь.
2. **Реестр согласован:** `registerAsset`/`resolve`/`findBySourcePath`/`unregisterAsset` работают в связке; `allAssets` возвращает все записи; после `unregisterAsset` ассет не разрешается.
3. **Конвейер импорта:** `importAsset` выбирает импортёр через `supports`; неподдерживаемый источник отклоняется (`nullopt`); `reimport` сохраняет id и повышает `contentVersion` (1 → 2).
4. **Граф зависимостей:** `dependentsOf` — обратный запрос («кто зависит от id»), корректный и напрямую, и после `unregisterAsset` зависимого.

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

- **Приёмочный тест:** `tests/day5/asset_database_tests.cpp` — `g++ -std=c++20 tests/day5/asset_database_tests.cpp engine/asset/src/asset_database.cpp -Iengine/asset/include -o asset_database_tests && ./asset_database_tests`, код выхода 0: идентичность (`assetIdFromPath`), реестр, `dependentsOf` и конвейер импорта (`reimport`: `contentVersion` 1 → 2) на локальном дублёре импортёра.

---

## feature/ecs-multithreading

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** `feature/ecs-core` (мир и планировщик), `feature/ecs-object-sync` (`pullEcsResults`)

**Цель фичи:** параллельный tick систем через планировщик задач ядра.

**Описание фичи:** единственная фича Этапа 4 контура E6 — раскладка систем по `IJobScheduler` и барьер перед синхронизацией.

**Обязательные требования:**

Контракт планировщика (namespace `sky::core`) — символ в символ; точка внедрения планировщика в мир ECS фиксируется этим заданием (без неё фича нереализуема):

```cpp
// engine/core/include/sky/core/job_scheduler.hpp
struct JobHandle {
    std::uint64_t value = 0;

    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
};

class IJobScheduler {
public:
    virtual ~IJobScheduler() = default;

    using Job = std::function<void()>;

    virtual JobHandle schedule(Job job) = 0;
    virtual JobHandle scheduleAfter(JobHandle dependency, Job job) = 0;
    virtual void wait(JobHandle job) = 0;
};

// engine/core/include/sky/core/runtime_services.hpp — объявление пула потоков
std::unique_ptr<IJobScheduler> createThreadPoolScheduler(unsigned threadCount = 0);

// engine/ecs/include/sky/ecs/ecs_world.hpp — точка внедрения (фиксируется заданием)
std::unique_ptr<EcsWorld> createEcsWorld(core::IJobScheduler* scheduler = nullptr);
```

1. **Точка внедрения:** перегрузка `createEcsWorld(core::IJobScheduler* scheduler = nullptr)` — при `nullptr` `tick` остаётся последовательным (существующие вызовы `createEcsWorld()` не меняются).
2. **Зависимость систем — по порядку регистрации:** независимые системы уходят в `schedule`; система, читающая данные ранее зарегистрированной, ставится через `scheduleAfter` от неё — порядок `update` относительно порядка регистрации сохраняется.
3. **Хранилища компонентов не потокобезопасны** — планировщик не пускает конфликтующие системы одновременно; параллелизм допустим только между системами без общих данных.
4. **`tick` — барьер:** до возврата из `tick` стоит `wait` по всем поставленным задачам, поэтому `pullEcsResults` видит финальные значения; каждая система выполняется ровно один раз за tick.
5. **Критерий приёмки — эквивалентность:** результат параллельного `tick` побитово совпадает с контрольным последовательным расчётом; повторные прогоны детерминированы.
6. **Запрет sleep и тайминговых проверок:** порядок наблюдается через `wait` и атомики, а не через задержки — результат одинаков на любом числе ядер. Пул потоков (`engine/core/src/job_scheduler.cpp`, `createThreadPoolScheduler`) — часть фичи: рабочая реализация контракта в движке; приёмочный тест проверяет сам контракт локальным многопоточным дублёром.

**Общий порядок реализации фичи:**
1. Использовать контракт `IJobScheduler` из заготовки `job_scheduler.hpp`.
2. В `tick` разложить независимые системы по `schedule`, зависимые — через `scheduleAfter`.
3. Поставить барьер `wait` перед `pullEcsResults`.

**Файлы фичи:**
1. `engine/core/include/sky/core/job_scheduler.hpp`
2. `engine/ecs/src/ecs_world.cpp`
3. `engine/core/src/job_scheduler.cpp` (пул потоков — `createThreadPoolScheduler`; объявление — в `runtime_services.hpp`)

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
3. `engine/core/src/job_scheduler.cpp` (пул потоков `createThreadPoolScheduler`)

**Общий критерий правильности:**
1. Параллельный tick независимых систем через `IJobScheduler`.
2. Результат детерминирован и совпадает с однопоточным (тест эквивалентности).

- **Приёмочный тест:** `tests/day5/ecs_multithreading_tests.cpp` — одна команда g++ из шапки теста (`tests/day5/ecs_multithreading_tests.cpp` + `engine/ecs/src/ecs_world.cpp` + `engine/ecs/src/object_sync.cpp`, `-pthread`), код выхода 0: контракт `schedule`/`scheduleAfter`/`wait` проверяется локальным многопоточным дублёром, эквивалентность `tick` контрольному последовательному расчёту — побитово (повторные прогоны детерминированы), каждая система выполняется ровно один раз за tick, барьер перед `pullEcsResults` — через цикл push → tick → pull; без sleep и тайминговых проверок.
