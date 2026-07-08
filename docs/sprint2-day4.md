# Спринт 2. День 4

## feature/undo-stack

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `EditorContext` из `feature/editor-context` (Этап 2)

**Цель фичи:** стек команд отмены операций редактора.

**Описание фичи:** базовая команда `IEditorCommand`, стек `UndoStack` и фабрики команд (по одной на операцию редактирования графа объектов). Часть Этапа 3: отмена операций и формат сцены SKYB.

**Общий порядок реализации фичи:**
1. Объявить `IEditorCommand`, `UndoStack` и фабрики команд в `editor_commands.hpp`.
2. Реализовать `UndoStack` и фабрики команд в `editor_commands.cpp`.

**Файлы фичи:**
1. `editor/shell/src/editor_commands.hpp`
2. `editor/shell/src/editor_commands.cpp`

### Файл: `editor/shell/src/editor_commands.hpp`

**Назначение файла:** контракт команды отмены, стек команд и фабрики команд.

**Пошаговое описание действий:**
1. Объявить `IEditorCommand`.
2. Объявить `UndoStack`.
3. Объявить фабрики команд по одной на операцию.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class IEditorCommand { virtual void redo()=0; virtual void undo()=0; virtual std::string label() const=0; }` — базовая команда.
- `class UndoStack`

*Функции / методы:*
- `UndoStack`: `explicit UndoStack(EditorContext&)`, `void push(std::unique_ptr<IEditorCommand> command)`, `bool undo()`, `bool redo()`, `bool canUndo() const`, `bool canRedo() const`
- Фабрики команд (каждая возвращает `std::unique_ptr<IEditorCommand>`): `makeTransformCommand(object, before, after)`, `makeFieldCommand(component, name, before, after)`, `makeCreateCommand`, `makeDeleteCommand(context, object)`, `makeDuplicateCommand`, `makeReparentCommand`, `makeRenameCommand`, `makeAddComponentCommand`, `makeRemoveComponentCommand`, `makeMaterialCreateCommand`, `makeMaterialEditCommand`

*Логика функций / методов:*
- `IEditorCommand` — базовая команда: `redo()`/`undo()` применяют и откатывают операцию, `label()` — подпись.
- `UndoStack(EditorContext&)` — привязка к контексту; `push` — добавляет и применяет команду; `undo()` — Возвращает: было ли что отменять; `redo()` — Возвращает: было ли что повторять; `canUndo()`/`canRedo()` — Возвращает: доступность.
- фабрики команд — по одной на операцию (transform/field/create/delete/duplicate/reparent/rename/add-component/remove-component/material-create/material-edit), каждая возвращает `std::unique_ptr<IEditorCommand>`.

**Результат по файлу:** контракт команд и стека зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `editor/shell/src/editor_commands.cpp`

**Назначение файла:** реализация стека команд и фабрик.

**Пошаговое описание действий:**
1. Реализовать `UndoStack` (`push`/`undo`/`redo`/`canUndo`/`canRedo`).
2. Реализовать фабрики команд.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытые классы-реализации команд.

*Функции / методы:*
- `UndoStack::push`, `UndoStack::undo`, `UndoStack::redo`, `UndoStack::canUndo`, `UndoStack::canRedo`; фабрики `makeTransformCommand`, `makeFieldCommand`, `makeCreateCommand`, `makeDeleteCommand`, `makeDuplicateCommand`, `makeReparentCommand`, `makeRenameCommand`, `makeAddComponentCommand`, `makeRemoveComponentCommand`, `makeMaterialCreateCommand`, `makeMaterialEditCommand`.

*Логика функций / методов:*
- `push` — добавляет команду в стек и применяет её (`redo`); `undo`/`redo` — переносят вершину между стеками отмены/повтора; `canUndo`/`canRedo` — наличие элементов.
- фабрики — конструируют конкретную команду для операции и возвращают `std::unique_ptr<IEditorCommand>`.

**Результат по файлу:** рабочий стек отмены с командами по всем операциям.

**Критерий правильности по файлу:**
1. `push` + `undo` откатывают операцию, `redo` повторяет её.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/editor_commands.hpp`
2. `editor/shell/src/editor_commands.cpp`

**Общий критерий правильности:**
1. undo/redo работают для transform/field/create/delete/duplicate/reparent/rename.

---

## feature/procedural-primitives

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/scene-authoring` (`createPrimitive`, `PrimitiveKind`)

**Цель фичи:** процедурные примитивы — сфера и плоскость строятся геометрически, а не куб-заглушкой.

**Описание фичи:** вторая фича Этапа 4 контура E2 — `createPrimitive` для `Sphere`/`Plane` строит вершины процедурно.

**Общий порядок реализации фичи:**
1. Дополнить `createPrimitive` процедурным построением сферы.
2. Дополнить `createPrimitive` процедурным построением плоскости.

**Файлы фичи:**
1. `engine/scene/src/scene_authoring.cpp`

### Файл: `engine/scene/src/scene_authoring.cpp`

**Назначение файла:** дополнение авторинга сцены — процедурные примитивы.

**Пошаговое описание действий:**
1. Для `PrimitiveKind::Sphere` построить вершины процедурно.
2. Для `PrimitiveKind::Plane` построить вершины процедурно.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение авторинга).

*Функции / методы:*
- `createPrimitive(...)`

*Логика функций / методов:*
- `createPrimitive(...)` для `PrimitiveKind::Sphere`/`Plane` строит вершины процедурно (а не куб-заглушку).

**Результат по файлу:** сфера и плоскость строятся процедурно.

**Критерий правильности по файлу:**
1. Примитивы `Sphere`/`Plane` — не куб-заглушки.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/scene/src/scene_authoring.cpp` (процедурные `Sphere`/`Plane`)

**Общий критерий правильности:**
1. Сфера/плоскость строятся процедурно.
2. Примитивы — не куб-заглушки.

---

## feature/mesh-picker

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/engine-bridge` (сессия и C-интерфейс), `feature/asset-database`/`feature/vfs-and-bridge-tests` (модели из `assets://Models`)

**Цель фичи:** выбор меша ссылкой из списка ассетов.

**Описание фичи:** первая фича Этапа 4 контура E3 — сбор списка мешей (примитивы + модели из `assets://Models`) и создание объекта-модели.

**Общий порядок реализации фичи:**
1. Реализовать `AvailableMeshes` в `EditorSession`.
2. Реализовать `CreateModel`.
3. Объявить вспомогательный класс `MeshOption`.

**Файлы фичи:**
1. `editor/avalonia/Engine/EditorSession.cs`

### Файл: `editor/avalonia/Engine/EditorSession.cs`

**Назначение файла:** дополнение обёртки сессии редактора — выбор меша.

**Пошаговое описание действий:**
1. Реализовать `AvailableMeshes(current)`.
2. Реализовать `CreateModel(name, meshRef)`.
3. Объявить `class MeshOption`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class MeshOption { public string Display; public string Value; }`

*Функции / методы:*
- `public List<MeshOption> AvailableMeshes(string current)`
- `public ulong CreateModel(string name, string meshRef)`

*Логика функций / методов:*
- `AvailableMeshes(current)` — собирает список мешей (примитивы + модели из `assets://Models`). Параметры: `current` — текущее значение. Возвращает: варианты для выпадающего списка.
- `CreateModel(name, meshRef)` — создаёт объект-модель. Возвращает: id объекта.
- `MeshOption` — пара «отображаемое имя / значение» для выпадающего списка.

**Результат по файлу:** поле меша выбирается из списка ассетов.

**Критерий правильности по файлу:**
1. `AvailableMeshes` возвращает примитивы и модели из `assets://Models`.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Engine/EditorSession.cs` (методы `AvailableMeshes`, `CreateModel`, класс `MeshOption`)

**Общий критерий правильности:**
1. Поле меша выбирается из списка ассетов.

---

## feature/user-assemblies

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** подсистема скриптинга (хост .NET, `SkyEngine.Managed`) из Этапа 4 контура E4

**Цель фичи:** компиляция пользовательских скриптов из `Assets/Scripts` и их загрузка в выгружаемый контекст.

**Описание фичи:** генерация csproj над пользовательскими скриптами, сборка через `dotnet build` и загрузка сборки в collectible ALC с возможностью выгрузки.

**Общий порядок реализации фичи:**
1. Реализовать `reloadUserScripts` и `scriptSourceDirs` в сборочной точке.
2. Добавить в `Bootstrap.cs` загрузку/выгрузку пользовательской сборки.

**Файлы фичи:**
1. `editor/shell/src/editor_context.cpp`
2. `managed/SkyEngine.Managed/Bootstrap.cs`

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** дополнение сборочной точки компиляцией и перезагрузкой пользовательских скриптов.

**Пошаговое описание действий:**
1. Реализовать `reloadUserScripts()` — сгенерировать csproj, собрать `dotnet build`, загрузить сборку.
2. Реализовать `scriptSourceDirs()` — вернуть каталоги-источники скриптов.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `EditorContext`).

*Функции / методы:*
- `bool reloadUserScripts()`
- `std::vector<std::filesystem::path> scriptSourceDirs() const`

*Логика функций / методов:*
- `reloadUserScripts()` — генерирует csproj над `Assets/Scripts/*.cs` (+ Runtime/*.cs пакетов), собирает через `dotnet build`, загружает сборку. Возвращает: успех.
- `scriptSourceDirs()` — Возвращает: каталоги-источники скриптов.

**Результат по файлу:** пользовательские скрипты собираются и загружаются в рантайм.

**Критерий правильности по файлу:**
1. Правка `.cs` подхватывается при следующем Play.

### Файл: `managed/SkyEngine.Managed/Bootstrap.cs`

**Назначение файла:** дополнение управляемых точек входа загрузкой/выгрузкой пользовательской сборки.

**Пошаговое описание действий:**
1. Добавить `LoadUserAssembly` — загрузка сборки в collectible ALC.
2. Добавить `UnloadUserAssembly` — выгрузка.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `Bootstrap`).

*Функции / методы:*
- `[UnmanagedCallersOnly] int LoadUserAssembly(IntPtr path)`
- `[UnmanagedCallersOnly] void UnloadUserAssembly()`

*Логика функций / методов:*
- `LoadUserAssembly(path)` — загружает пользовательскую сборку в collectible ALC. Возвращает: 1/0.
- `UnloadUserAssembly()` — выгружает её (когда исходников не осталось).

**Результат по файлу:** пользовательская сборка загружается и может быть выгружена.

**Критерий правильности по файлу:**
1. Сборка загружается в collectible ALC и выгружается при отсутствии исходников.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/editor_context.cpp` (дополнение: `reloadUserScripts`, `scriptSourceDirs`)
2. `managed/SkyEngine.Managed/Bootstrap.cs` (дополнение: `LoadUserAssembly`, `UnloadUserAssembly`)

**Общий критерий правильности:**
1. Скрипты из `Assets/Scripts` компилируются и работают в Play.
2. Правка `.cs` подхватывается при следующем Play.

---

## feature/package-resolver

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** нет

**Цель фичи:** менеджер пакетов — манифесты и разрешение версий (semver, MVS).

**Описание фичи:** разбор семантических версий и констрейнтов, сканирование каталога пакетов и разрешение графа зависимостей методом minimal version selection.

**Общий порядок реализации фичи:**
1. Объявить `Version`, `parseVersion`, `satisfies` в `semver.hpp`.
2. Объявить манифест и контракты пакетов в `package_system.hpp`.
3. Объявить `PackageWorld` в `package_world.hpp`.
4. Реализовать обнаружение и разрешение пакетов в `package_world.cpp`.

**Файлы фичи:**
1. `engine/package/include/sky/package/semver.hpp`
2. `engine/package/include/sky/package/package_system.hpp`
3. `engine/package/include/sky/package/package_world.hpp`
4. `engine/package/src/package_world.cpp`

### Файл: `engine/package/include/sky/package/semver.hpp`

**Назначение файла:** семантические версии и проверка констрейнтов.

**Пошаговое описание действий:**
1. Объявить `struct Version {int major, minor, patch;}`.
2. Объявить `parseVersion` и `satisfies`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct Version {int major, minor, patch;}`

*Функции / методы:*
- `std::optional<Version> parseVersion(const std::string& text)`
- `bool satisfies(const Version& version, const std::string& requirement)`

*Логика функций / методов:*
- `parseVersion(text)` — разбирает версию. Возвращает: версию или `nullopt`.
- `satisfies(version, requirement)` — проверяет соответствие констрейнту (`*`, `>=`, `^`, точная). Возвращает: да/нет.

**Результат по файлу:** разбор версий и проверка констрейнтов.

**Критерий правильности по файлу:**
1. `parseVersion`/`satisfies` корректны для `*`, `>=`, `^` и точной версии.

### Файл: `engine/package/include/sky/package/package_system.hpp`

**Назначение файла:** типы манифеста и контракты пакетной системы.

**Пошаговое описание действий:**
1. Объявить `PackageManifest` и сопутствующие типы пакетов.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct PackageManifest` (манифест пакета — используется `PackageWorld::resolve`/`discoveredPackages`).

*Функции / методы:* нет (объявления типов).

*Логика функций / методов:*
- `PackageManifest` — манифест пакета; служит единицей обнаружения и активации в резолвере.

**Результат по файлу:** типы манифеста зафиксированы.

**Критерий правильности по файлу:**
1. Заголовок компилируется; `PackageManifest` используется резолвером.

### Файл: `engine/package/include/sky/package/package_world.hpp`

**Назначение файла:** мир пакетов — обнаружение и разрешение.

**Пошаговое описание действий:**
1. Объявить `class PackageWorld` с методами обнаружения и разрешения.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class PackageWorld`

*Функции / методы:*
- `std::size_t discoverPackages(const std::filesystem::path& packagesRoot)`
- `std::vector<PackageManifest> resolve(const std::vector<std::string>& refs)`
- `std::vector<PackageManifest> discoveredPackages() const`

*Логика функций / методов:*
- `discoverPackages(packagesRoot)` — сканирует каталог. Возвращает: число найденных пакетов.
- `resolve(refs)` — разрешает граф зависимостей (MVS). Возвращает: пакеты в порядке активации (пусто при конфликте).
- `discoveredPackages()` — Возвращает: все обнаруженные.

**Результат по файлу:** интерфейс мира пакетов.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/package/src/package_world.cpp`

**Назначение файла:** реализация обнаружения и разрешения пакетов.

**Пошаговое описание действий:**
1. Реализовать сканирование каталога пакетов.
2. Реализовать разрешение графа зависимостей (MVS) с обнаружением конфликтов.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- реализация `PackageWorld`.

*Функции / методы:*
- `discoverPackages`, `resolve`, `discoveredPackages`.

*Логика функций / методов:*
- `discoverPackages` — читает манифесты из каталога и запоминает обнаруженные пакеты.
- `resolve` — строит граф зависимостей и выбирает минимальные подходящие версии (MVS); при конфликте возвращает пусто.
- `discoveredPackages` — отдаёт список обнаруженных пакетов.

**Результат по файлу:** рабочий резолвер пакетов.

**Критерий правильности по файлу:**
1. Версии резолвятся; конфликт даёт пустой результат.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/package/include/sky/package/semver.hpp`
2. `engine/package/include/sky/package/package_system.hpp`
3. `engine/package/include/sky/package/package_world.hpp`
4. `engine/package/src/package_world.cpp`

**Общий критерий правильности:**
1. Версии резолвятся (semver, MVS).
2. Конфликт версий выявляется (пустой результат `resolve`).
