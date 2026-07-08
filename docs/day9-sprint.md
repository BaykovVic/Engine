# Спринт 1 · День 9 — выдача фич (по одной на контур)

День 9: каждый контур берёт свою **9-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/undo-stack` | Этап 3 (Недели 3–4, веха M2). Отмена операций и формат сцены SKYB |
| **E2** Рендеринг | `feature/procedural-primitives` | Этап 4 (Недели 5–6, веха M3). Текстуры и слоты PBR |
| **E3** Редактор(.NET) | `feature/mesh-picker` | Этап 4 (Недели 5–6, веха M3). Выбор меша, перетаскивание, режим 2D |
| **E4** Рантайм/скриптинг | `feature/user-assemblies` | Этап 5 (Недели 7–8, веха M4). Пользовательские сборки и игровой интерфейс |
| **E5** Пайплайн/пакеты | `feature/package-resolver` | Этап 5 (Недели 7–8, веха M4). Менеджер пакетов |
| **E6** Data-oriented(ECS) | — (фичи этого контура закончились) | — |

---

## E1 · `feature/undo-stack`

*Этап: Этап 3 (Недели 3–4, веха M2). Отмена операций и формат сцены SKYB.*

#### Файлы `editor/shell/src/editor_commands.hpp`, `editor/shell/src/editor_commands.cpp`
- `class IEditorCommand { virtual void redo()=0; virtual void undo()=0; virtual std::string label() const=0; }` — базовая команда.
- `class UndoStack`:
  - `explicit UndoStack(EditorContext&)` — привязка к контексту.
  - `void push(std::unique_ptr<IEditorCommand> command)` — добавляет и применяет команду.
  - `bool undo()` — Возвращает: было ли что отменять.
  - `bool redo()` — Возвращает: было ли что повторять.
  - `bool canUndo() const` / `bool canRedo() const` — Возвращает: доступность.
- Фабрики команд (каждая возвращает `std::unique_ptr<IEditorCommand>`), по одной на операцию:
  `makeTransformCommand(object, before, after)`, `makeFieldCommand(component, name, before, after)`,
  `makeCreateCommand`, `makeDeleteCommand(context, object)`, `makeDuplicateCommand`,
  `makeReparentCommand`, `makeRenameCommand`, `makeAddComponentCommand`,
  `makeRemoveComponentCommand`, `makeMaterialCreateCommand`, `makeMaterialEditCommand`.

---

## E2 · `feature/procedural-primitives`

*Этап: Этап 4 (Недели 5–6, веха M3). Текстуры и слоты PBR.*

#### Дополнение файла `engine/scene/src/scene_authoring.cpp`
- `createPrimitive(...)` для `PrimitiveKind::Sphere`/`Plane` строит вершины
  процедурно (а не куб-заглушку).

---

## E3 · `feature/mesh-picker`

*Этап: Этап 4 (Недели 5–6, веха M3). Выбор меша, перетаскивание, режим 2D.*

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public List<MeshOption> AvailableMeshes(string current)`
  Что делает: собирает список мешей (примитивы + модели из `assets://Models`). Параметры: `current` — текущее значение. Возвращает: варианты для выпадающего списка.
- `public ulong CreateModel(string name, string meshRef)` — создаёт объект-модель. Возвращает: id объекта.
- `class MeshOption { public string Display; public string Value; }`.

---

## E4 · `feature/user-assemblies`

*Этап: Этап 5 (Недели 7–8, веха M4). Пользовательские сборки и игровой интерфейс.*

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `bool reloadUserScripts()`
  Что делает: генерирует csproj над `Assets/Scripts/*.cs` (+ Runtime/*.cs пакетов), собирает через `dotnet build`, загружает сборку. Возвращает: успех.
- `std::vector<std::filesystem::path> scriptSourceDirs() const` — Возвращает: каталоги-источники скриптов.

#### Дополнение файла `managed/SkyEngine.Managed/Bootstrap.cs`
- `[UnmanagedCallersOnly] int LoadUserAssembly(IntPtr path)` — загружает пользовательскую сборку в collectible ALC. Возвращает: 1/0.
- `[UnmanagedCallersOnly] void UnloadUserAssembly()` — выгружает её (когда исходников не осталось).

---

## E5 · `feature/package-resolver`

*Этап: Этап 5 (Недели 7–8, веха M4). Менеджер пакетов.*

#### Файлы `engine/package/include/sky/package/{package_system,package_world}.hpp`, `semver.hpp`, `engine/package/src/package_world.cpp`
- `struct Version {int major, minor, patch;}`; `std::optional<Version> parseVersion(const std::string& text)` — разбирает версию. Возвращает: версию или `nullopt`.
- `bool satisfies(const Version& version, const std::string& requirement)` — проверяет соответствие констрейнту (`*`, `>=`, `^`, точная). Возвращает: да/нет.
- `class PackageWorld`:
  - `std::size_t discoverPackages(const std::filesystem::path& packagesRoot)` — сканирует каталог. Возвращает: число найденных пакетов.
  - `std::vector<PackageManifest> resolve(const std::vector<std::string>& refs)` — разрешает граф зависимостей (MVS). Возвращает: пакеты в порядке активации (пусто при конфликте).
  - `std::vector<PackageManifest> discoveredPackages() const` — Возвращает: все обнаруженные.

---
