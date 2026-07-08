# Спринт 1 · День 5 — выдача фич (по одной на контур)

День 5: каждый контур берёт свою **5-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/core-tests` | Этап 1 (Неделя 1). Математика, объектная и компонентная модели |
| **E2** Рендеринг | `feature/frame-builder` | Этап 2 (Неделя 2, веха M1). Меши, камера, освещение, построитель кадра |
| **E3** Редактор(.NET) | `feature/hierarchy-tree` | Этап 2 (Неделя 2, веха M1). Панель вьюпорта и живое дерево объектов |
| **E4** Рантайм/скриптинг | `feature/input-state` | Этап 3 (Недели 3–4, веха M2). Режим воспроизведения и ввод |
| **E5** Пайплайн/пакеты | `feature/asset-database` | Этап 3 (Недели 3–4, веха M2). Импортёры, виртуальная ФС, тесты интерфейса |
| **E6** Data-oriented(ECS) | `feature/ecs-multithreading` | Этап 4 (Недели 5–6 · перспектива). Многопоточное исполнение систем |

---

## E1 · `feature/core-tests`

*Этап: Этап 1 (Неделя 1). Математика, объектная и компонентная модели.*

#### Файлы `tests/core_tests.cpp`, `tests/world_tests.cpp`
Проверяют математику, объектный и компонентный миры.

---

## E2 · `feature/frame-builder`

*Этап: Этап 2 (Неделя 2, веха M1). Меши, камера, освещение, построитель кадра.*

#### Файл `editor/shell/src/frame_builder.hpp`
- `FrameBuilder(EditorContext& context, rendering::IRenderResourceFactory& factory)`
  Что делает: конструктор — привязывает построитель к контексту и фабрике ресурсов. Параметры: `context`, `factory`.
- `void setCamera(std::optional<core::Transform> pose, float orthoHeight = 0.0f)`
  Что делает: задаёт камеру кадра. Параметры: `pose` — поза камеры (nullopt = камера сцены), `orthoHeight` — высота орто-проекции (0 = перспектива).
- `std::vector<rendering::RenderCommand> build(std::uint32_t width, std::uint32_t height)`
  Что делает: обходит сцену и формирует поток команд отрисовки. Параметры: `width`, `height` — размер кадра. Возвращает: список команд.

#### Файл `editor/viewport_bridge/include/sky/editor/viewport/viewport_bridge.hpp`
Qt-free фасад вьюпорта (`IPlayModeController`, `IRuntimePreviewHost`), общий для
редактора и плеера.

---

## E3 · `feature/hierarchy-tree`

*Этап: Этап 2 (Неделя 2, веха M1). Панель вьюпорта и живое дерево объектов.*

#### Дополнение файла `editor/avalonia/Engine/EngineInterop.cs`
P/Invoke иерархии и трансформа (реализация — в мосте контура E5):
`sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`,
`sky_editor_child_at`, `sky_editor_object_name`, `sky_editor_get_transform`,
`sky_editor_render_offscreen`, `sky_editor_viewport_orbit`, `sky_editor_viewport_zoom`.

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public void Reload()` — перечитывает иерархию через C-интерфейс в `ObservableCollection<SkyObject> Roots`.
- `private SkyObject Load(ulong id)` — рекурсивно строит узел дерева.
- `public (float[] position, float[] rotation, float[] scale) Transform(ulong id)`
  Что делает: читает трансформ объекта. Параметры: `id`. Возвращает: позицию (3), кватернион (4), масштаб (3).
- `Views/HierarchyView.axaml.cs` — `TreeView` по `Roots`.

---

## E4 · `feature/input-state`

*Этап: Этап 3 (Недели 3–4, веха M2). Режим воспроизведения и ввод.*

#### Дополнение файла `editor/shell/src/editor_context.hpp` (методы объявлены inline)
- `void setKeyDown(int key, bool down)` — заносит/снимает клавишу. Параметры: `key` — переносимый код, `down` — нажата ли.
- `bool keyDown(int key) const` — Возвращает: нажата ли клавиша (читается скриптами через `Input`).

---

## E5 · `feature/asset-database`

*Этап: Этап 3 (Недели 3–4, веха M2). Импортёры, виртуальная ФС, тесты интерфейса.*

#### Файлы `engine/asset/include/sky/asset/{asset_system,asset_database}.hpp`, `engine/asset/src/asset_database.cpp`
`class IAssetImporter` — контракт импортёра:
- `virtual bool supports(...) const = 0` — Возвращает: поддерживает ли формат.
- `virtual ImportResult import(...) = 0` — Возвращает: результат импорта.
`class AssetDatabase`:
- `std::optional<AssetId> importAsset(const std::filesystem::path& sourcePath)` — импортирует ассет. Возвращает: id или `nullopt`.
- `std::optional<AssetDescriptor> resolve(AssetId id) const` — Возвращает: описание ассета или `nullopt`.
- `std::unique_ptr<AssetDatabase> createAssetDatabase()` — фабрика.

---

## E6 · `feature/ecs-multithreading`

*Этап: Этап 4 (Недели 5–6 · перспектива). Многопоточное исполнение систем.*

#### Файл `engine/core/include/sky/core/job_scheduler.hpp` (использовать существующую заготовку)
- `class IJobScheduler`:
  - `virtual JobHandle schedule(Job job) = 0` — ставит задачу в очередь. Возвращает: хэндл задачи.
  - `virtual JobHandle scheduleAfter(JobHandle dependency, Job job) = 0` — задача после зависимости.
  - `virtual void wait(JobHandle job) = 0` — ждёт завершения.

#### Дополнение файла `engine/ecs/src/ecs_world.cpp`
- `tick` раскладывает независимые системы по `schedule`, зависимые — через
  `scheduleAfter`; барьер `wait` перед `pullEcsResults`.

---
