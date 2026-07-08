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

## feature/dotnet-host

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** нет (хостинг .NET; требует hostfxr в системе)

**Цель фичи:** хостинг .NET внутри нативного процесса — контракт хоста скриптов и его реализация через hostfxr.

**Описание фичи:** первая фича Этапа 4 контура E4 — контракт `IScriptHost` и реализация `DotNetScriptHost`, запускающая среду .NET, загружающая сборки и управляющая жизненным циклом инстансов скриптов.

**Общий порядок реализации фичи:**
1. Объявить граничные типы `ScriptLifecycleEvent`/`AssemblyRef`.
2. Объявить контракт `IScriptHost`.
3. Объявить `DotNetScriptHost` и фабрику `createDotNetScriptHost`.
4. Реализовать хост через hostfxr в `.cpp`.

**Файлы фичи:**
1. `engine/scripting/include/sky/scripting/scripting_boundary.hpp`
2. `engine/scripting/include/sky/scripting/script_host.hpp`
3. `engine/scripting/include/sky/scripting/dotnet_host.hpp`
4. `engine/scripting/src/dotnet_host.cpp`

### Файл: `engine/scripting/include/sky/scripting/scripting_boundary.hpp`

**Назначение файла:** граничные типы managed↔native.

**Пошаговое описание действий:**
1. Объявить `ScriptLifecycleEvent`.
2. Объявить `AssemblyRef`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `enum class ScriptLifecycleEvent { OnCreate, OnStart, OnUpdate, OnFixedUpdate, OnDestroy }`
- `struct AssemblyRef { name; path; }`

*Функции / методы:* нет.

*Логика функций / методов:*
- `ScriptLifecycleEvent` — событие жизненного цикла скрипта.
- `AssemblyRef` — ссылка на сборку (имя и путь).

**Результат по файлу:** граничные типы скриптинга зафиксированы.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/scripting/include/sky/scripting/script_host.hpp`

**Назначение файла:** контракт хоста скриптов.

**Пошаговое описание действий:**
1. Объявить `IScriptHost`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class IScriptHost`

*Функции / методы:*
- `virtual bool start() = 0`
- `virtual bool loadAssembly(const AssemblyRef& assembly) = 0`
- `virtual std::uint64_t createInstance(const std::string& managedTypeName) = 0`
- `virtual void destroyInstance(std::uint64_t managedInstanceId) = 0`
- `virtual bool invokeLifecycle(std::uint64_t id, ScriptLifecycleEvent event, double dt) = 0`

*Логика функций / методов:*
- `start()` — запускает среду .NET. Возвращает: успех.
- `loadAssembly(assembly)` — загружает сборку. Возвращает: успех.
- `createInstance(managedTypeName)` — создаёт managed-инстанс класса. Параметры: `managedTypeName` — полное имя класса. Возвращает: id инстанса (0 при ошибке).
- `destroyInstance(managedInstanceId)` — уничтожает инстанс.
- `invokeLifecycle(id, event, dt)` — вызывает событие жизненного цикла. Параметры: `id`, `event`, `dt` — шаг времени. Возвращает: успех.

**Результат по файлу:** контракт хоста скриптов зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/scripting/include/sky/scripting/dotnet_host.hpp`

**Назначение файла:** реализация-контракт хоста через hostfxr.

**Пошаговое описание действий:**
1. Объявить `DotNetScriptHost`, наследующий `IScriptHost`.
2. Объявить фабрику `createDotNetScriptHost`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class DotNetScriptHost : IScriptHost`

*Функции / методы:*
- `virtual void installEngineApi(const void* apiTable) = 0`
- `virtual void setInstanceObjectId(std::uint64_t id, std::uint64_t objectId) = 0`
- `virtual void beginFrame(double totalSeconds, double deltaSeconds) = 0`
- `virtual std::vector<std::string> scriptClassNames() = 0`
- `std::unique_ptr<DotNetScriptHost> createDotNetScriptHost(const DotNetHostConfig&)`

*Логика функций / методов:*
- `installEngineApi(apiTable)` — передаёт managed-стороне таблицу нативных функций (обратный API).
- `setInstanceObjectId(id, objectId)` — связывает инстанс скрипта с объектом.
- `beginFrame(totalSeconds, deltaSeconds)` — публикует время кадра managed-стороне.
- `scriptClassNames()` — Возвращает: имена классов-наследников ScriptComponent.
- `createDotNetScriptHost(config)` — фабрика (nullptr, если hostfxr не найден).

**Результат по файлу:** контракт реализации хоста зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/scripting/src/dotnet_host.cpp`

**Назначение файла:** реализация хоста через hostfxr.

**Пошаговое описание действий:**
1. Реализовать запуск среды .NET через hostfxr.
2. Реализовать загрузку сборок и управление инстансами.
3. Реализовать обратный API, время кадра и перечень классов.
4. Реализовать фабрику.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `DotNetScriptHost`.

*Функции / методы:*
- `start`, `loadAssembly`, `createInstance`, `destroyInstance`, `invokeLifecycle`, `installEngineApi`, `setInstanceObjectId`, `beginFrame`, `scriptClassNames`, `createDotNetScriptHost`.

*Логика функций / методов:*
- `start` — запускает среду .NET через hostfxr.
- `loadAssembly`/`createInstance`/`destroyInstance`/`invokeLifecycle` — загрузка сборки и управление жизненным циклом инстансов.
- `installEngineApi`/`setInstanceObjectId`/`beginFrame`/`scriptClassNames` — обратный API, связывание с объектом, время кадра, перечень классов-скриптов.
- `createDotNetScriptHost(config)` — фабрика; возвращает nullptr, если hostfxr не найден.

**Результат по файлу:** рабочий хост скриптов .NET.

**Критерий правильности по файлу:**
1. Хост запускает среду .NET, загружает сборку и создаёт инстанс.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/scripting/include/sky/scripting/scripting_boundary.hpp`
2. `engine/scripting/include/sky/scripting/script_host.hpp`
3. `engine/scripting/include/sky/scripting/dotnet_host.hpp`
4. `engine/scripting/src/dotnet_host.cpp`

**Общий критерий правильности:**
1. Хост запускает среду .NET, загружает сборку, создаёт инстанс и вызывает события жизненного цикла.
2. Тест `dotnet_host_tests` зелёный (проверяется в фиче `feature/scripting-integration`).

---

## feature/importers-obj-png

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** `IAssetImporter` из `feature/asset-database`; `platform::IFileSystem`

**Цель фичи:** импортёры OBJ и PNG (декодирование/кодирование).

**Описание фичи:** часть Этапа 3 (импортёры, виртуальная ФС, тесты интерфейса) — импортёр OBJ и декодер/кодер PNG.

**Общий порядок реализации фичи:**
1. Объявить и реализовать импортёр OBJ.
2. Объявить и реализовать декодер/кодер PNG и импортёр PNG.

**Файлы фичи:**
1. `engine/asset/include/sky/asset/obj_importer.hpp`
2. `engine/asset/src/obj_importer.cpp`
3. `engine/asset/include/sky/asset/png_decoder.hpp`
4. `engine/asset/src/png_decoder.cpp`

### Файл: `engine/asset/include/sky/asset/obj_importer.hpp`

**Назначение файла:** контракт импортёра OBJ.

**Пошаговое описание действий:**
1. Объявить `createObjImporter`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `std::unique_ptr<IAssetImporter> createObjImporter(...)`

*Логика функций / методов:*
- `createObjImporter(...)` — импортёр OBJ (парсинг v/vn/vt/f).

**Результат по файлу:** объявление импортёра OBJ.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/asset/src/obj_importer.cpp`

**Назначение файла:** реализация импортёра OBJ.

**Пошаговое описание действий:**
1. Реализовать парсинг v/vn/vt/f.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация импортёра OBJ.

*Функции / методы:*
- `createObjImporter`, `supports`, `import`.

*Логика функций / методов:*
- импортёр OBJ (парсинг v/vn/vt/f).

**Результат по файлу:** рабочий импортёр OBJ.

**Критерий правильности по файлу:**
1. OBJ импортируется (v/vn/vt/f разбираются).

### Файл: `engine/asset/include/sky/asset/png_decoder.hpp`

**Назначение файла:** декодер/кодер PNG и импортёр PNG.

**Пошаговое описание действий:**
1. Объявить `ImageData`.
2. Объявить `decodePng`, `encodePngRgba`, `createPngImporter`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct ImageData { std::uint32_t width, height; std::vector<std::uint8_t> pixels; }`

*Функции / методы:*
- `std::optional<ImageData> decodePng(const std::vector<std::byte>& bytes)`
- `std::vector<std::byte> encodePngRgba(const ImageData& image)`
- `std::unique_ptr<IAssetImporter> createPngImporter(platform::IFileSystem& fileSystem)`

*Логика функций / методов:*
- `decodePng(bytes)` — декодирует PNG. Возвращает: изображение или `nullopt`.
- `encodePngRgba(image)` — кодирует RGBA в PNG. Возвращает: байты файла.
- `createPngImporter(fileSystem)` — импортёр PNG.

**Результат по файлу:** контракт PNG зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/asset/src/png_decoder.cpp`

**Назначение файла:** реализация декодера/кодера PNG и импортёра PNG.

**Пошаговое описание действий:**
1. Реализовать `decodePng`/`encodePngRgba`.
2. Реализовать импортёр PNG.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация импортёра PNG.

*Функции / методы:*
- `decodePng`, `encodePngRgba`, `createPngImporter`.

*Логика функций / методов:*
- `decodePng` — декодирует байты PNG в `ImageData` или `nullopt`; `encodePngRgba` — кодирует RGBA в байты PNG; `createPngImporter(fileSystem)` — импортёр PNG.

**Результат по файлу:** рабочие декодер/кодер PNG и импортёр.

**Критерий правильности по файлу:**
1. `decodePng(encodePngRgba(image))` восстанавливает изображение.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/asset/include/sky/asset/obj_importer.hpp`
2. `engine/asset/src/obj_importer.cpp`
3. `engine/asset/include/sky/asset/png_decoder.hpp`
4. `engine/asset/src/png_decoder.cpp`

**Общий критерий правильности:**
1. OBJ/PNG импортируются; `asset_project_tests` зелёный.

---

## feature/ecs-profiling

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** планировщик систем ECS (`tick`) из этапов контура E6; C-интерфейс (E5) и панель редактора (E3) — совместно

**Цель фичи:** покадровые тайминги систем и вывод в редактор (совместно с E3/E5).

**Описание фичи:** измерение длительности `update` каждой системы за такт и накопление таймингов; совместно с E3/E5 — C-интерфейс выдачи таймингов и панель профилировщика в редакторе.

**Общий порядок реализации фичи:**
1. Добавить измерение и накопление таймингов систем в `ecs_world.cpp`.
2. Совместно с E3/E5 — C-интерфейс выдачи таймингов и панель профилировщика в редакторе.

**Файлы фичи:**
1. `engine/ecs/src/ecs_world.cpp`

### Файл: `engine/ecs/src/ecs_world.cpp`

**Назначение файла:** дополнение реализации ECS-мира измерением таймингов систем.

**Пошаговое описание действий:**
1. Измерять длительность `update` каждой системы за такт.
2. Накапливать тайминги для выдачи наружу.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение реализации `EcsWorld`).

*Функции / методы:*
- дополнение `tick` — измерение и накопление таймингов.

*Логика функций / методов:*
- измерение длительности `update` каждой системы за такт; накопление таймингов.

**Результат по файлу:** покадровые тайминги систем измеряются и доступны.

**Критерий правильности по файлу:**
1. Длительность такта каждой системы доступна редактору и отображается.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/ecs/src/ecs_world.cpp` (дополнение: измерение таймингов систем)
2. Совместно с E3/E5: C-интерфейс выдачи таймингов систем; панель профилировщика в редакторе.

**Общий критерий правильности:**
1. Тайминги систем измеряются; панель профилировщика в редакторе.
2. Длительность такта каждой системы доступна редактору и отображается.
