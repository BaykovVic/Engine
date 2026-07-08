# Спринт 4. День 1

## feature/physics-reattach

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/physics-world` (физический мир и коллайдеры), `feature/object-snapshot`/`feature/skyb-serialization` (открытие сцены)

**Цель фичи:** воссоздание физических тел из компонентов при открытии сцены.

**Описание фичи:** единственная фича Этапа 4 контура E1 — при `openScene` физические тела и коллайдеры восстанавливаются из компонентов сцены и связываются с объектами.

**Общий порядок реализации фичи:**
1. Добавить приватный `reattachPhysics()` в `EditorContext`.
2. Вызвать его из `openScene` после восстановления графа объектов.

**Файлы фичи:**
1. `editor/shell/src/editor_context.cpp`

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** дополнение сборочной точки редактора — восстановление физики из сцены.

**Пошаговое описание действий:**
1. Реализовать приватный `reattachPhysics()`.
2. Обойти объекты сцены и найти компоненты `sky.rigidbody`/`sky.collider.box`.
3. Создать по ним физические тела и коллайдеры и связать их с объектами.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `EditorContext`).

*Функции / методы:*
- приватный `void reattachPhysics()`

*Логика функций / методов:*
- `reattachPhysics()` — при `openScene` создаёт физические тела и коллайдеры из компонентов `sky.rigidbody`/`sky.collider.box`, связывает их с объектами. Возвращает: ничего.

**Результат по файлу:** открытие сцены восстанавливает физику.

**Критерий правильности по файлу:**
1. Сохранённая сцена с физикой после открытия падает так же, как до сохранения.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/editor_context.cpp` (метод `reattachPhysics`)

**Общий критерий правильности:**
1. При открытии сцены физические тела воссоздаются из компонентов.
2. Сохранённая сцена с физикой после открытия падает так же, как до сохранения.

---

## feature/pbr-textures

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/vulkan-mesh-lighting` (рендерер мешей и материалов), `feature/render-contract` (контракт ресурсов)

**Цель фичи:** загрузка текстур и привязка слотов PBR-материала.

**Описание фичи:** первая фича Этапа 4 контура E2 — реализация загрузки текстур на GPU и привязки шести дескрипторных слотов PBR.

**Общий порядок реализации фичи:**
1. Реализовать `createTextureFromData`.
2. Реализовать `uploadTexture`.
3. Реализовать `bindMaterial` с шестью слотами PBR и нейтральными заглушками.

**Файлы фичи:**
1. `engine/rendering_vulkan/src/vulkan_renderer.cpp`

### Файл: `engine/rendering_vulkan/src/vulkan_renderer.cpp`

**Назначение файла:** дополнение Vulkan-рендерера — текстуры и слоты PBR.

**Пошаговое описание действий:**
1. Реализовать загрузку текстуры из пикселей.
2. Создать изображение GPU, вид и дескриптор.
3. Привязать шесть дескрипторных сетов PBR, заменяя отсутствующие слоты нейтральными значениями.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение реализации рендерера).

*Функции / методы:*
- `createTextureFromData(std::uint32_t w, std::uint32_t h, std::span<const std::uint8_t> rgba)`
- `uploadTexture(...)`
- `bindMaterial(command)`

*Логика функций / методов:*
- `createTextureFromData(w, h, rgba)` — загрузка текстуры. Возвращает: хэндл.
- `uploadTexture(...)` — создаёт изображение GPU, вид и дескриптор.
- `bindMaterial(command)` — привязывает шесть дескрипторных сетов PBR (albedo/normal/roughness/metallic/occlusion/height); отсутствующий слот заменяется нейтральным значением (белый / плоская нормаль).

**Результат по файлу:** материалы используют текстурные слоты PBR.

**Критерий правильности по файлу:**
1. Отсутствующие текстурные слоты заменяются нейтральными значениями.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/rendering_vulkan/src/vulkan_renderer.cpp` (методы `createTextureFromData`, `uploadTexture`, `bindMaterial`)

**Общий критерий правильности:**
1. Материалы используют текстурные слоты.
2. Отсутствующие текстурные слоты заменяются нейтральными значениями (белый / плоская нормаль).

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

## feature/importers-fbx-gltf

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** `feature/asset-database` (контракт `IAssetImporter`), `feature/player-runtime` (`sky_player`), `feature/managed-runtime` (цель `sky_managed`)

**Цель фичи:** импортёры FBX/glTF и запуск сохранённой сцены проигрывателем.

**Описание фичи:** единственная фича Этапа 4 контура E5 — импортёры FBX и glTF плюс сборочная цель `sky_managed` и шаг CI «player smoke».

**Общий порядок реализации фичи:**
1. Реализовать импортёр FBX.
2. Реализовать импортёр glTF (через `mini_json.hpp`).
3. Добавить цель `sky_managed` и переменную `SKY_MANAGED_DIR` в CMake.
4. Добавить шаг CI «player smoke».

**Файлы фичи:**
1. `engine/asset/include/sky/asset/fbx_importer.hpp`
2. `engine/asset/src/fbx_importer.cpp`
3. `engine/asset/include/sky/asset/gltf_importer.hpp`
4. `engine/asset/src/gltf_importer.cpp`
5. `engine/asset/src/mini_json.hpp`
6. `CMakeLists.txt`
7. `.github/workflows/ci.yml`

### Файл: `engine/asset/include/sky/asset/fbx_importer.hpp`

**Назначение файла:** объявление импортёра FBX.

**Пошаговое описание действий:**
1. Объявить фабрику `createFbxImporter`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `std::unique_ptr<IAssetImporter> createFbxImporter(...)`

*Логика функций / методов:*
- `createFbxImporter(...)` — импортёр FBX.

**Результат по файлу:** объявлен импортёр FBX.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/asset/src/fbx_importer.cpp`

**Назначение файла:** реализация импортёра FBX.

**Пошаговое описание действий:**
1. Реализовать разбор FBX и построение меша.

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация импортёра FBX.

*Функции / методы:* `supports`, `import`, `createFbxImporter`.

*Логика функций / методов:*
- реализует контракт `IAssetImporter` для формата FBX.

**Результат по файлу:** FBX импортируется.

**Критерий правильности по файлу:**
1. FBX-файл импортируется в меш.

### Файл: `engine/asset/include/sky/asset/gltf_importer.hpp`

**Назначение файла:** объявление импортёра glTF.

**Пошаговое описание действий:**
1. Объявить фабрику `createGltfImporter`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `std::unique_ptr<IAssetImporter> createGltfImporter(...)`

*Логика функций / методов:*
- `createGltfImporter(...)` — импортёр glTF (использует `mini_json.hpp`).

**Результат по файлу:** объявлен импортёр glTF.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/asset/src/gltf_importer.cpp`

**Назначение файла:** реализация импортёра glTF.

**Пошаговое описание действий:**
1. Разобрать glTF через `mini_json.hpp` и построить меш.

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация импортёра glTF.

*Функции / методы:* `supports`, `import`, `createGltfImporter`.

*Логика функций / методов:*
- реализует контракт `IAssetImporter` для формата glTF, разбирая JSON через `mini_json.hpp`.

**Результат по файлу:** glTF импортируется.

**Критерий правильности по файлу:**
1. glTF-файл импортируется в меш.

### Файл: `engine/asset/src/mini_json.hpp`

**Назначение файла:** минимальный разбор JSON для glTF.

**Пошаговое описание действий:**
1. Реализовать разбор JSON, достаточный для glTF.

**Что должно быть в файле:**

*Структуры / классы / enum:* типы разбора JSON.

*Функции / методы:* функции разбора JSON.

*Логика функций / методов:*
- минимальный разбор JSON, используемый импортёром glTF.

**Результат по файлу:** доступен разбор JSON для glTF.

**Критерий правильности по файлу:**
1. Заголовок компилируется и разбирает валидный JSON.

### Файл: `CMakeLists.txt`

**Назначение файла:** дополнение сборки — цель `sky_managed` и переменная.

**Пошаговое описание действий:**
1. Объявить цель `sky_managed`.
2. Объявить переменную `SKY_MANAGED_DIR`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* нет (декларативный CMake).

*Логика функций / методов:*
- цель `sky_managed` и переменная `SKY_MANAGED_DIR` (совместно с E4).

**Результат по файлу:** managed-сборка подключена в конфигурации.

**Критерий правильности по файлу:**
1. `sky_managed` собирается, `SKY_MANAGED_DIR` доступна.

### Файл: `.github/workflows/ci.yml`

**Назначение файла:** дополнение CI — шаг «player smoke».

**Пошаговое описание действий:**
1. Добавить шаг «player smoke» с запуском проигрывателя и публикацией артефакта.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* нет (конфигурация CI).

*Логика функций / методов:*
- шаг CI «player smoke»: `sky_player --headless player-smoke.png --frames 60` + артефакт.

**Результат по файлу:** CI прогоняет smoke-запуск проигрывателя.

**Критерий правильности по файлу:**
1. Шаг smoke в CI зелёный.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/asset/include/sky/asset/fbx_importer.hpp`
2. `engine/asset/src/fbx_importer.cpp`
3. `engine/asset/include/sky/asset/gltf_importer.hpp`
4. `engine/asset/src/gltf_importer.cpp`
5. `engine/asset/src/mini_json.hpp`
6. `CMakeLists.txt` (цель `sky_managed`, `SKY_MANAGED_DIR`)
7. `.github/workflows/ci.yml` (шаг «player smoke»)

**Общий критерий правильности:**
1. FBX и glTF импортируются.
2. `sky_player --scene X.skybox` запускает сцену.
3. Шаг smoke в CI зелёный.

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
