# Спринт 1. День 3

## feature/object-model

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/math-and-handles` (`core::Transform`, `Handle`)

**Цель фичи:** единственный владелец иерархии сцены и трансформов.

**Описание фичи:** объекты, их дерево и трансформы; остальные модули держат только хендлы.

**Общий порядок реализации фичи:**
1. Объявить три контракта в `object_model.hpp`.
2. Объявить `ObjectWorld` и фабрику в `object_world.hpp`.
3. Реализовать `ObjectWorld` в `object_world.cpp`.

**Файлы фичи:**
1. `engine/object/include/sky/object/object_model.hpp`
2. `engine/object/include/sky/object/object_world.hpp`
3. `engine/object/src/object_world.cpp`

### Файл: `engine/object/include/sky/object/object_model.hpp`

**Назначение файла:** контракты объектной модели.

**Пошаговое описание действий:**
1. Объявить `ObjectHandle`.
2. Объявить `IObjectFactory`, `IObjectHierarchyAccess`, `IObjectQueryService`.
3. Объявить свободную `setWorldTransform`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `using ObjectHandle = core::Handle<ObjectTag>`
- `class IObjectFactory`
- `class IObjectHierarchyAccess`
- `class IObjectQueryService`

*Функции / методы:*
- `IObjectFactory`: `createObject(const std::string& name)`, `destroyObject(ObjectHandle)`
- `IObjectHierarchyAccess`: `setParent`, `parentOf`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`
- `IObjectQueryService`: `exists`, `nameOf`, `findByName`
- `inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)`

*Логика функций / методов:*
- `createObject` — создаёт объект; `destroyObject` — удаляет объект и его поддерево.
- `setParent` — перевешивает `child` под `parent` (invalid = корень); `worldTransform` — композиция локальных вверх по цепочке.
- `exists`/`nameOf`/`findByName` — запросы для чтения.
- `setWorldTransform` — задаёт мировой трансформ, пересчитывая локальный через `invCompose`.

**Результат по файлу:** контракты объектной модели зафиксированы.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/object/include/sky/object/object_world.hpp`

**Назначение файла:** единый мир объектов.

**Пошаговое описание действий:**
1. Объявить `ObjectWorld`, наследующий три контракта.
2. Добавить `renameObject` и фабрику `createObjectWorld`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService`

*Функции / методы:*
- `virtual void renameObject(ObjectHandle object, const std::string& name) = 0`
- `std::unique_ptr<ObjectWorld> createObjectWorld()`

*Логика функций / методов:*
- `renameObject` — переименовывает объект.
- `createObjectWorld` — фабрика мира объектов.

**Результат по файлу:** интерфейс мира объектов с фабрикой.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/object/src/object_world.cpp`

**Назначение файла:** реализация мира объектов.

**Пошаговое описание действий:**
1. Завести хранилище объектов.
2. Реализовать иерархию и трансформы.
3. Реализовать рекурсивное удаление.

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация `ObjectWorld`.

*Функции / методы:*
- `createObject`, `destroyObject`, `setParent`, `parentOf`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`, `exists`, `nameOf`, `findByName`, `renameObject`, `createObjectWorld`.

*Логика функций / методов:*
- хранилище id → {локальный трансформ, родитель, дети, имя}.
- `worldTransform` = `compose` локальных вверх по цепочке родителей.
- `destroyObject` рекурсивно удаляет объект и всё поддерево.

**Результат по файлу:** рабочий мир объектов.

**Критерий правильности по файлу:**
1. `worldTransform` ребёнка корректен относительно повёрнутого родителя.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/object/include/sky/object/object_model.hpp`
2. `engine/object/include/sky/object/object_world.hpp`
3. `engine/object/src/object_world.cpp`

**Общий критерий правильности:**
1. Ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире = (0,0,-1).

---

## feature/vulkan-tests

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/vulkan-offscreen` (день 2)

**Цель фичи:** проверить Vulkan-рендерер на программном драйвере lavapipe.

**Описание фичи:** автотест закадрового рендера без видеокарты (в CI).

**Общий порядок реализации фичи:**
1. Написать тест на lavapipe.

**Файлы фичи:**
1. `tests/vulkan_tests.cpp`

### Файл: `tests/vulkan_tests.cpp`

**Назначение файла:** тест Vulkan-рендерера.

**Пошаговое описание действий:**
1. Создать рендерер, отрисовать кадр.
2. Прочитать кадр и сверить пиксели.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовая функция.

*Логика функций / методов:* `ready()` истинно, кадр рендерится, `readbackFrame()` непустой, центральный пиксель отличается от углового.

**Результат по файлу:** зелёный тест `vulkan_tests`.

**Критерий правильности по файлу:**
1. Тест проходит на lavapipe.

### На выходе должно получиться

**Список артефактов фичи:**
1. `tests/vulkan_tests.cpp`
2. библиотека `sky_rendering_vulkan` собрана; формируется `triangle.png`; тест `vulkan_tests` зелёный на lavapipe

**Общий критерий правильности:**
1. На `triangle.png` центральный пиксель отличается от углового.
2. `readbackFrame()` возвращает непустой массив.

---

## feature/engine-bridge

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/c-abi-seed` (`libsky_editor_bridge.so`, ниже в этом дне)

**Цель фичи:** первичная связь редактора с движком через C-интерфейс (P/Invoke).

**Описание фичи:** загрузка нативного моста и создание/уничтожение сессии движка из .NET — фундамент всех дальнейших вызовов.

**Общий порядок реализации фичи:**
1. Объявить P/Invoke и резолвер в `EngineInterop.cs`.
2. Обернуть сессию в `EditorSession.cs`.

**Файлы фичи:**
1. `editor/avalonia/Engine/EngineInterop.cs`
2. `editor/avalonia/Engine/EditorSession.cs`

### Файл: `editor/avalonia/Engine/EngineInterop.cs`

**Назначение файла:** P/Invoke-объявления и резолвер нативной библиотеки.

**Пошаговое описание действий:**
1. Объявить `sky_editor_create`/`destroy`.
2. Реализовать резолвер библиотеки.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `static class EngineInterop`

*Функции / методы:*
- `[DllImport] static extern IntPtr sky_editor_create()`
- `[DllImport] static extern void sky_editor_destroy(IntPtr ctx)`
- `static IntPtr Resolve(...)`
- `static string[] Candidates()`

*Логика функций / методов:*
- `sky_editor_create` — возвращает указатель на сессию движка; `sky_editor_destroy` — уничтожает сессию.
- `Resolve`/`Candidates` — находят `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в дереве сборки.

**Результат по файлу:** доступ к нативному мосту из .NET.

**Критерий правильности по файлу:**
1. Библиотека моста находится и загружается.

### Файл: `editor/avalonia/Engine/EditorSession.cs`

**Назначение файла:** обёртка над сессией движка.

**Пошаговое описание действий:**
1. В конструкторе создать сессию.
2. Реализовать `Dispose`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class EditorSession : IDisposable`

*Функции / методы:*
- конструктор `EditorSession()`
- `Dispose()`
- свойство `IntPtr Native`

*Логика функций / методов:*
- конструктор вызывает `sky_editor_create` и проверяет не-null.
- `Dispose()` вызывает `sky_editor_destroy`.
- `Native` — нативный указатель сессии.

**Результат по файлу:** управляемая обёртка сессии.

**Критерий правильности по файлу:**
1. Сессия создаётся (не-null) и освобождается в `Dispose`.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Engine/EngineInterop.cs`
2. `editor/avalonia/Engine/EditorSession.cs`

**Общий критерий правильности:**
1. `dotnet build` — 0 ошибок.
2. Сессия движка создаётся из .NET (не-null).

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

## feature/c-abi-seed

- **Исполнитель:** E5 (Пайплайн и QA, совместно с E1)
- **Порядок реализации:** 5
- **Зависимости:** модуль `object` (`feature/object-model`) для перечисления корней

**Цель фичи:** первичный плоский C-интерфейс движка `sky_editor_*`.

**Описание фичи:** плоский набор C-функций, через который .NET-редактор общается с C++-движком; на этом этапе минимум — сессия и перечисление корней.

**Общий порядок реализации фичи:**
1. Объявить C-функции в `editor_bridge.h`.
2. Реализовать их в `editor_bridge.cpp`.

**Файлы фичи:**
1. `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`
2. `editor/native_bridge/src/editor_bridge.cpp`

### Файл: `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`

**Назначение файла:** объявления плоского C-интерфейса.

**Пошаговое описание действий:**
1. Объявить `create`/`destroy` сессии.
2. Объявить перечисление корней и чтение имени.

**Что должно быть в файле:**

*Структуры / классы / enum:* непрозрачный тип `SkyEditorContext`; тип `SkyObjectId`.

*Функции / методы:*
- `SkyEditorContext* sky_editor_create(void)`
- `void sky_editor_destroy(SkyEditorContext* ctx)`
- `int32_t sky_editor_root_count(SkyEditorContext* ctx)`
- `SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index)`
- `int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)`

*Логика функций / методов:*
- `sky_editor_create` — возвращает указатель на сессию (собирает движок и демо-сцену).
- `sky_editor_destroy` — уничтожает сессию.
- `sky_editor_root_count` — число корневых объектов.
- `sky_editor_root_at` — id корневого объекта.
- `sky_editor_object_name` — пишет имя в буфер; возвращает длину.

**Результат по файлу:** заголовок C-интерфейса.

**Критерий правильности по файлу:**
1. Заголовок пригоден для P/Invoke из .NET.

### Файл: `editor/native_bridge/src/editor_bridge.cpp`

**Назначение файла:** реализация C-интерфейса.

**Пошаговое описание действий:**
1. Реализовать сборку движка и демо-сцены в сессии.
2. Реализовать перечисление корней и чтение имени.

**Что должно быть в файле:**

*Структуры / классы / enum:* определение `SkyEditorContext` (сессия).

*Функции / методы:*
- `sky_editor_create`, `sky_editor_destroy`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_object_name`.

*Логика функций / методов:*
- `create` собирает движок и демо-сцену в сессии; `destroy` уничтожает; `root_count`/`root_at` перечисляют корни; `object_name` пишет имя в буфер и возвращает длину.

**Результат по файлу:** библиотека `libsky_editor_bridge.so`.

**Критерий правильности по файлу:**
1. `sky_editor_create` возвращает не-null сессию.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`
2. `editor/native_bridge/src/editor_bridge.cpp`
3. библиотека `libsky_editor_bridge.so`

**Общий критерий правильности:**
1. Красный CI блокирует слияние; сломанный тест краснеет.
2. Редактор E3 вызывает `sky_editor_create` и получает не-null сессию.

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
