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

## feature/sky-backdrop

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/render-contract` (`RenderCommandType::SetSky`); Vulkan-бэкенд

**Цель фичи:** отрисовка неба (купол горизонт→зенит с диском солнца).

**Описание фичи:** часть Этапа 3 (выбор объекта, проекция, небо) — обработка команды `SetSky` в Vulkan-рендерере.

**Общий порядок реализации фичи:**
1. Дополнить `vulkan_renderer.cpp` обработкой команды `SetSky`.

**Файлы фичи:**
1. `engine/rendering_vulkan/src/vulkan_renderer.cpp`

### Файл: `engine/rendering_vulkan/src/vulkan_renderer.cpp`

**Назначение файла:** отрисовка неба в Vulkan-рендерере.

**Пошаговое описание действий:**
1. Обработать команду `RenderCommandType::SetSky`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение рендерера).

*Функции / методы:*
- обработка команды `RenderCommandType::SetSky`.

*Логика функций / методов:*
- обработка команды `RenderCommandType::SetSky` — купол-небо: горизонт (`color`) → зенит (`emissive`), диск солнца от направленного света. Реализуется как большой куб, приклеенный к камере, с флагом «небо» в push-константах.

**Результат по файлу:** небо отрисовано.

**Критерий правильности по файлу:**
1. Команда `SetSky` рисует купол-небо с диском солнца.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/rendering_vulkan/src/vulkan_renderer.cpp`

**Общий критерий правильности:**
1. небо отрисовано; центральный луч кадрированной камеры попадает в объект; проекция origin объекта близка к центру экрана.

---

## feature/inspector

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** C-интерфейс `sky_editor_*` (мост контура E5); `EditorSession` из Этапа 2

**Цель фичи:** инспектор полей компонентов (чтение/запись, добавление/удаление компонентов).

**Описание фичи:** часть Этапа 3 (гизмо, инспектор, панели данных) — дополнение `EditorSession` и панель инспектора с шаблонами полей.

**Общий порядок реализации фичи:**
1. Дополнить `EditorSession.cs` чтением/записью компонентов и классами полей.
2. Реализовать `InspectorView.axaml.cs`.

**Файлы фичи:**
1. `editor/avalonia/Engine/EditorSession.cs`
2. `editor/avalonia/Views/InspectorView.axaml.cs`

### Файл: `editor/avalonia/Engine/EditorSession.cs`

**Назначение файла:** доступ к компонентам и их полям через C-интерфейс.

**Пошаговое описание действий:**
1. Добавить чтение компонентов и типов.
2. Добавить запись поля и операции с компонентами.
3. Завести классы `ComponentView`, `ComponentField`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class ComponentView`
- `class ComponentField : INotifyPropertyChanged` (свойства `IsScalar/IsBool/IsVec3/IsMeshRef`, `Value`, `X/Y/Z`, `BoolValue`)

*Функции / методы:*
- `public List<ComponentView> ReadComponents(ulong id)`
- `public void SetComponentField(ulong id, int component, int field, string value)`
- `public List<ComponentType> AvailableTypes()`
- `public void AddComponent(ulong id, string typeId)`
- `public void RemoveComponent(ulong id, int component)`

*Логика функций / методов:*
- `ReadComponents(id)` — Возвращает: компоненты объекта с их полями.
- `SetComponentField(id, component, field, value)` — записывает поле через C-интерфейс.
- `AvailableTypes()` — Возвращает: типы для меню Add Component.
- `AddComponent(id, typeId)`/`RemoveComponent(id, component)` — добавляет/снимает компонент.

**Результат по файлу:** данные компонентов доступны инспектору.

**Критерий правильности по файлу:**
1. Изменение поля через `SetComponentField` применяется к движку.

### Файл: `editor/avalonia/Views/InspectorView.axaml.cs`

**Назначение файла:** панель инспектора.

**Пошаговое описание действий:**
1. Собрать карточки компонентов.
2. Задать шаблоны полей.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class InspectorView`

*Функции / методы:* обработчики полей и добавления/удаления компонентов.

*Логика функций / методов:*
- карточки компонентов, шаблоны полей.

**Результат по файлу:** панель инспектора на живых данных.

**Критерий правильности по файлу:**
1. Инспектор редактирует поля компонентов.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Engine/EditorSession.cs`
2. `editor/avalonia/Views/InspectorView.axaml.cs`

**Общий критерий правильности:**
1. изменение поля в инспекторе применяется к движку и отменяемо.

---

## feature/managed-runtime

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** `feature/dotnet-host` (точки входа вызываются хостом, обратный API)

**Цель фичи:** управляемый рантайм скриптинга на C# — точки входа, базовый класс скрипта и вспомогательные службы.

**Описание фичи:** вторая фича Этапа 4 контура E4 — managed-сборка `SkyEngine.Managed`: `UnmanagedCallersOnly`-точки входа, базовый класс `ScriptComponent`, обёртка над id объекта, таблица обратного API и службы Debug/Time/Input.

**Общий порядок реализации фичи:**
1. Реализовать точки входа `Bootstrap.cs`.
2. Реализовать базовый класс `ScriptComponent`.
3. Реализовать `NativeHandle`, `Engine`, `Debug`, `Time`, `Input`.

**Файлы фичи:**
1. `managed/SkyEngine.Managed/Bootstrap.cs`
2. `managed/SkyEngine.Managed/ScriptComponent.cs`
3. `managed/SkyEngine.Managed/NativeHandle.cs`
4. `managed/SkyEngine.Managed/Engine.cs`
5. `managed/SkyEngine.Managed/Debug.cs`
6. `managed/SkyEngine.Managed/Time.cs`
7. `managed/SkyEngine.Managed/Input.cs`

### Файл: `managed/SkyEngine.Managed/Bootstrap.cs`

**Назначение файла:** точки входа, вызываемые из C++.

**Пошаговое описание действий:**
1. Объявить `[UnmanagedCallersOnly]` точки входа.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `static class Bootstrap`

*Функции / методы:*
- `LoadAssembly`, `CreateInstance`, `DestroyInstance`, `InvokeLifecycle`, `Initialize`, `SetObjectId`, `TickFrame` (все `[UnmanagedCallersOnly]`).

*Логика функций / методов:*
- `[UnmanagedCallersOnly]` точки входа, вызываемые из C++: `LoadAssembly`, `CreateInstance`, `DestroyInstance`, `InvokeLifecycle`, `Initialize` (ставит обратный API), `SetObjectId`, `TickFrame`.

**Результат по файлу:** managed-точки входа доступны из C++.

**Критерий правильности по файлу:**
1. Точки входа вызываются хостом напрямую (без обёрток).

### Файл: `managed/SkyEngine.Managed/ScriptComponent.cs`

**Назначение файла:** базовый класс скрипта (аналог MonoBehaviour).

**Пошаговое описание действий:**
1. Объявить события жизненного цикла и защищённые методы трансформа.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class ScriptComponent`

*Функции / методы:*
- `OnCreate/OnStart/OnUpdate/OnFixedUpdate/OnDestroy`, защищённые `SetLocalPosition/SetLocalEuler/SetLocalScale`, свойство `Handle`.

*Логика функций / методов:*
- базовый класс скрипта: события жизненного цикла; защищённые `SetLocalPosition/SetLocalEuler/SetLocalScale`; свойство `Handle`.

**Результат по файлу:** базовый класс скрипта доступен пользователям.

**Критерий правильности по файлу:**
1. Класс-наследник переопределяет события жизненного цикла и меняет трансформ.

### Файл: `managed/SkyEngine.Managed/NativeHandle.cs`

**Назначение файла:** обёртка над id объекта.

**Пошаговое описание действий:**
1. Объявить обёртку над id объекта.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct NativeHandle`

*Функции / методы:* доступ к id объекта.

*Логика функций / методов:*
- обёртка над id объекта.

**Результат по файлу:** id объекта представлен типобезопасно.

**Критерий правильности по файлу:**
1. Файл компилируется.

### Файл: `managed/SkyEngine.Managed/Engine.cs`

**Назначение файла:** таблица делегатов обратного API.

**Пошаговое описание действий:**
1. Объявить таблицу делегатов нативных функций движка.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `static class Engine`

*Функции / методы:* делегаты обратного API.

*Логика функций / методов:*
- таблица делегатов обратного API (нативные функции движка).

**Результат по файлу:** managed-сторона вызывает нативные функции движка.

**Критерий правильности по файлу:**
1. Таблица делегатов ставится через `Initialize`.

### Файл: `managed/SkyEngine.Managed/Debug.cs`

**Назначение файла:** логирование в консоль редактора.

**Пошаговое описание действий:**
1. Реализовать `Debug.Log/LogWarning/LogError`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `static class Debug`

*Функции / методы:*
- `Debug.Log/LogWarning/LogError`

*Логика функций / методов:*
- `Debug.Log/LogWarning/LogError` — вывод в консоль редактора.

**Результат по файлу:** скрипты пишут в консоль редактора.

**Критерий правильности по файлу:**
1. `Debug.Log` попадает в консоль редактора.

### Файл: `managed/SkyEngine.Managed/Time.cs`

**Назначение файла:** время кадра.

**Пошаговое описание действий:**
1. Объявить `Time.TotalTime/DeltaTime`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `static class Time`

*Функции / методы:*
- `Time.TotalTime`, `Time.DeltaTime`

*Логика функций / методов:*
- `Time.TotalTime/DeltaTime` — время кадра, публикуемое хостом.

**Результат по файлу:** время кадра доступно скриптам.

**Критерий правильности по файлу:**
1. `Time.DeltaTime` отражает шаг кадра.

### Файл: `managed/SkyEngine.Managed/Input.cs`

**Назначение файла:** ввод.

**Пошаговое описание действий:**
1. Объявить `Input.GetKey(KeyCode)`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `static class Input`

*Функции / методы:*
- `Input.GetKey(KeyCode)`

*Логика функций / методов:*
- `Input.GetKey(KeyCode)` — состояние клавиши, читается скриптами.

**Результат по файлу:** ввод доступен скриптам.

**Критерий правильности по файлу:**
1. `Input.GetKey` отражает нажатую клавишу.

### На выходе должно получиться

**Список артефактов фичи:**
1. `managed/SkyEngine.Managed/Bootstrap.cs`
2. `managed/SkyEngine.Managed/ScriptComponent.cs`
3. `managed/SkyEngine.Managed/NativeHandle.cs`
4. `managed/SkyEngine.Managed/Engine.cs`
5. `managed/SkyEngine.Managed/Debug.cs`
6. `managed/SkyEngine.Managed/Time.cs`
7. `managed/SkyEngine.Managed/Input.cs`

**Общий критерий правильности:**
1. Базовый класс `ScriptComponent` со всеми событиями жизненного цикла доступен.
2. `Debug.Log` пишет в консоль редактора; `Time`/`Input` доступны скриптам.

---

## feature/vfs-and-bridge-tests

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** `feature/asset-database`, `feature/importers-obj-png`; C-интерфейс `sky_editor_*` (`feature/c-abi-seed`)

**Цель фичи:** файловая система, виртуальная ФС (`assets://…`) и тесты C-интерфейса, импорта и VFS.

**Описание фичи:** часть Этапа 3 (импортёры, виртуальная ФС, тесты интерфейса) — `IFileSystem`, `IVirtualFileSystem` и тесты на каждую группу.

**Общий порядок реализации фичи:**
1. Объявить `IFileSystem` и `IVirtualFileSystem`.
2. Реализовать стандартную и виртуальную ФС.
3. Написать тесты C-интерфейса, импорта и VFS.

**Файлы фичи:**
1. `engine/platform/include/sky/platform/file_system.hpp`
2. `engine/platform/include/sky/platform/virtual_file_system.hpp`
3. `engine/platform/src/std_file_system.cpp`
4. `engine/platform/src/virtual_file_system.cpp`
5. `tests/editor_bridge_tests.cpp`
6. `tests/asset_project_tests.cpp`
7. `tests/vfs_tests.cpp`

### Файл: `engine/platform/include/sky/platform/file_system.hpp`

**Назначение файла:** контракт файловой системы.

**Пошаговое описание действий:**
1. Объявить `IFileSystem`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class IFileSystem`

*Функции / методы:*
- `exists`, `isDirectory`, `readAll`, `writeAll`, `list`.

*Логика функций / методов:*
- `IFileSystem` — файловая система (exists/isDirectory/readAll/writeAll/list).

**Результат по файлу:** контракт файловой системы зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/platform/include/sky/platform/virtual_file_system.hpp`

**Назначение файла:** контракт виртуальной ФС.

**Пошаговое описание действий:**
1. Объявить `IVirtualFileSystem`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class IVirtualFileSystem`

*Функции / методы:*
- `void mount(const std::string& scheme, std::unique_ptr<IVfsMount> mount, int priority)`
- `std::optional<std::vector<std::byte>> readAll(const std::string& ref)`
- `std::vector<...> list(const std::string& dir)`

*Логика функций / методов:*
- `mount(scheme, mount, priority)` — монтирует схему (напр. `assets`).
- `readAll(ref)` — читает по ссылке `assets://…`. Возвращает: байты или `nullopt`.
- `list(dir)` — Возвращает: содержимое каталога.

**Результат по файлу:** контракт VFS зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/platform/src/std_file_system.cpp`

**Назначение файла:** реализация стандартной файловой системы.

**Пошаговое описание действий:**
1. Реализовать `IFileSystem` над реальной ФС.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `IFileSystem`.

*Функции / методы:*
- `exists`, `isDirectory`, `readAll`, `writeAll`, `list`.

*Логика функций / методов:*
- стандартная файловая система: exists/isDirectory/readAll/writeAll/list над реальными путями.

**Результат по файлу:** рабочая стандартная ФС.

**Критерий правильности по файлу:**
1. `writeAll` + `readAll` дают одинаковые байты.

### Файл: `engine/platform/src/virtual_file_system.cpp`

**Назначение файла:** реализация виртуальной ФС.

**Пошаговое описание действий:**
1. Реализовать монтирование схем и разрешение ссылок.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `IVirtualFileSystem`.

*Функции / методы:*
- `mount`, `readAll`, `list`.

*Логика функций / методов:*
- `mount(scheme, mount, priority)` — монтирует схему; `readAll(ref)` — разрешает ссылку `assets://…` в байты или `nullopt`; `list(dir)` — содержимое каталога.

**Результат по файлу:** рабочая VFS.

**Критерий правильности по файлу:**
1. Ссылка `assets://…` разрешается через смонтированную схему.

### Файл: `tests/editor_bridge_tests.cpp`

**Назначение файла:** тесты C-интерфейса движка.

**Пошаговое описание действий:**
1. Проверить группы C-интерфейса.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:* тест на каждую группу C-интерфейса.

**Результат по файлу:** зелёный тест `editor_bridge_tests`.

**Критерий правильности по файлу:**
1. `editor_bridge_tests` зелёный.

### Файл: `tests/asset_project_tests.cpp`

**Назначение файла:** тесты импорта ассетов.

**Пошаговое описание действий:**
1. Проверить импорт OBJ/PNG и базу ассетов.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:* тест на импорт (OBJ/PNG, база ассетов).

**Результат по файлу:** зелёный тест `asset_project_tests`.

**Критерий правильности по файлу:**
1. `asset_project_tests` зелёный.

### Файл: `tests/vfs_tests.cpp`

**Назначение файла:** тесты виртуальной ФС.

**Пошаговое описание действий:**
1. Проверить разрешение ссылок `assets://…` и `list`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:* тест на VFS (разрешение `assets://…`).

**Результат по файлу:** зелёный тест `vfs_tests`.

**Критерий правильности по файлу:**
1. `vfs_tests` зелёный.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/platform/include/sky/platform/file_system.hpp`
2. `engine/platform/include/sky/platform/virtual_file_system.hpp`
3. `engine/platform/src/std_file_system.cpp`
4. `engine/platform/src/virtual_file_system.cpp`
5. `tests/editor_bridge_tests.cpp`
6. `tests/asset_project_tests.cpp`
7. `tests/vfs_tests.cpp`

**Общий критерий правильности:**
1. ссылки `assets://` разрешаются; `asset_project_tests`, `vfs_tests`, `editor_bridge_tests` зелёные.
