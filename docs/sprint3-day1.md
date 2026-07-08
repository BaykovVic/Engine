# Спринт 3. День 1

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

## feature/editor-camera

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `core::Vec3`/`core::Transform` из `feature/math-and-handles`

**Цель фичи:** камера редактора (орбита/зум/панорамирование/снап к оси, поза и орто-высота).

**Описание фичи:** часть Этапа 3 (выбор объекта, проекция, небо) — камера редактора. Выбор объекта лучом (`pick`) и проекция мира в экран (`project`) реализуются в мосте контура E5 поверх этой камеры (совместная фича).

**Общий порядок реализации фичи:**
1. Объявить `EditorCamera` и его методы в `editor_camera.hpp`.
2. Реализовать управление камерой и вычисление позы.

**Файлы фичи:**
1. `editor/shell/src/editor_camera.hpp`

### Файл: `editor/shell/src/editor_camera.hpp`

**Назначение файла:** камера редактора.

**Пошаговое описание действий:**
1. Объявить структуру `EditorCamera`.
2. Объявить методы управления и запросов камеры.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct EditorCamera { float yawDegrees; float pitchDegrees; float distance; core::Vec3 target; float fovDegrees; bool orthographic; … }`

*Функции / методы:*
- `void orbit(float deltaYawDegrees, float deltaPitchDegrees)`
- `void zoom(float factor)`
- `void pan(float deltaRight, float deltaUp)`
- `void lookAlong(int axis)`
- `core::Transform pose() const`
- `float orthoHeight() const`

*Логика функций / методов:*
- `orbit(deltaYawDegrees, deltaPitchDegrees)` — вращает камеру вокруг цели. Параметры: приращения углов в градусах. Возвращает: ничего.
- `zoom(factor)` — приближает/отдаляет. Параметры: `factor` — коэффициент. Возвращает: ничего.
- `pan(deltaRight, deltaUp)` — сдвигает цель. Параметры: смещения. Возвращает: ничего.
- `lookAlong(axis)` — снапит вид к оси. Параметры: `axis` (0=+X,1=−X,2=+Y,3=−Y,…). Возвращает: ничего.
- `pose()` — Возвращает: мировую позу камеры.
- `orthoHeight()` — Возвращает: высоту орто-проекции (0 = перспектива).

**Результат по файлу:** управляемая камера редактора.

**Критерий правильности по файлу:**
1. Заголовок компилируется; `orbit`/`zoom`/`pan`/`lookAlong` меняют позу камеры.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/editor_camera.hpp`

**Общий критерий правильности:**
1. центральный луч кадрированной камеры попадает в объект; проекция origin объекта близка к центру экрана.

---

## feature/gizmos

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** C-интерфейс `sky_editor_*` (мост контура E5); `VulkanViewport`/`MainWindow` из Этапа 2

**Цель фичи:** манипуляторы (гизмо) перемещения/поворота/масштаба и операции модели представления.

**Описание фичи:** часть Этапа 3 (гизмо, инспектор, панели данных) — манипуляторы поверх кадра и связанные операции `MainViewModel`.

**Общий порядок реализации фичи:**
1. Дополнить `VulkanViewport.cs` свойствами и рисованием гизмо, обработкой драга.
2. Реализовать `MainViewModel.cs` с инструментом, операциями и свойствами трансформа.

**Файлы фичи:**
1. `editor/avalonia/Controls/VulkanViewport.cs`
2. `editor/avalonia/MainViewModel.cs`

### Файл: `editor/avalonia/Controls/VulkanViewport.cs`

**Назначение файла:** рисование и обработка манипуляторов поверх кадра.

**Пошаговое описание действий:**
1. Добавить свойства выделения и инструмента.
2. Добавить рисование гизмо.
3. Добавить драг осей.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение класса `VulkanViewport`).

*Функции / методы:*
- свойства `public ulong SelectedId`, `public bool LocalSpace`, `public GizmoTool Tool`.
- `DrawMoveGizmo`, `DrawRotateGizmo`, `DrawScaleGizmo`, `DrawSceneGizmo`.
- `OnPointerPressed/Moved/Released`.

*Логика функций / методов:*
- `DrawMoveGizmo`/`DrawRotateGizmo`/`DrawScaleGizmo`/`DrawSceneGizmo` — рисование манипуляторов поверх кадра.
- `OnPointerPressed/Moved/Released` — драг осей (перемещение/поворот/масштаб через C-интерфейс).

**Результат по файлу:** вьюпорт с гизмо.

**Критерий правильности по файлу:**
1. Гизмо move/rotate/scale отрисовываются и реагируют на драг.

### Файл: `editor/avalonia/MainViewModel.cs`

**Назначение файла:** модель представления с инструментом, операциями и свойствами трансформа.

**Пошаговое описание действий:**
1. Объявить `GizmoTool` и активный инструмент.
2. Реализовать операции создания/отмены/повтора/дублирования/удаления.
3. Завести двусторонние свойства трансформа.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `enum GizmoTool { Hand, Move, Rotate, Scale }`

*Функции / методы:*
- свойство `public GizmoTool Tool` (хоткеи Q/W/E/R).
- `public void CreateCube()`, `public void Undo()`, `public void Redo()`, `DuplicateSelected()`, `DeleteSelected()`.
- свойства трансформа `PositionX/Y/Z`, `RotationX/Y/Z`, `ScaleX/Y/Z` (двусторонние).

*Логика функций / методов:*
- `Tool` — активный инструмент, переключается хоткеями Q/W/E/R.
- `CreateCube`/`Undo`/`Redo`/`DuplicateSelected`/`DeleteSelected` — операции над сценой через C-интерфейс.
- `PositionX/Y/Z`, `RotationX/Y/Z`, `ScaleX/Y/Z` — двусторонние свойства трансформа выделенного объекта.

**Результат по файлу:** модель представления гизмо и операций.

**Критерий правильности по файлу:**
1. Активный инструмент синхронизирован; операции применяются к движку.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Controls/VulkanViewport.cs`
2. `editor/avalonia/MainViewModel.cs`

**Общий критерий правильности:**
1. гизмо move/rotate/scale работают; активный инструмент синхронизирован.

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
