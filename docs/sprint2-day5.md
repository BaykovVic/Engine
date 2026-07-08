# Спринт 2. День 5

## feature/object-snapshot

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `EditorContext` из `feature/editor-context`; `feature/undo-stack`

**Цель фичи:** снимок/восстановление поддерева объекта и операции редактирования графа (дублирование, смена родителя, удаление, новая/сохранение/открытие сцены).

**Описание фичи:** часть Этапа 3 (отмена операций и формат сцены SKYB) — структуры снимка и операции над графом объектов в `EditorContext`.

**Общий порядок реализации фичи:**
1. Объявить структуры снимка в `editor_context.hpp`.
2. Реализовать снимок/восстановление и операции графа в `editor_context.cpp`.

**Файлы фичи:**
1. `editor/shell/src/editor_context.hpp`
2. `editor/shell/src/editor_context.cpp`

### Файл: `editor/shell/src/editor_context.hpp`

**Назначение файла:** структуры снимка объекта.

**Пошаговое описание действий:**
1. Объявить `ObjectSnapshot` и `ComponentSnapshot`.
2. Объявить методы снимка/восстановления и операций графа.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `ObjectSnapshot{name, local, hasPhysicsBody, components, children}`
- `ComponentSnapshot{typeId, fields}`

*Функции / методы:*
- `ObjectSnapshot snapshotObject(object::ObjectHandle object) const`
- `object::ObjectHandle restoreObject(const ObjectSnapshot& snapshot, object::ObjectHandle parent)`
- `void newScene()`
- `bool saveScene(const std::filesystem::path& path)`
- `bool openScene(const std::filesystem::path& path)`
- `object::ObjectHandle duplicateObject(object::ObjectHandle object)`
- `void reparent(object::ObjectHandle child, object::ObjectHandle newParent)`
- `void destroyObject(object::ObjectHandle object)`

*Логика функций / методов:*
- `ObjectSnapshot`/`ComponentSnapshot` — структуры снимка поддерева (имя, трансформ, флаг физического тела, компоненты, дети / typeId и поля).

**Результат по файлу:** структуры снимка и объявления операций.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** снимок/восстановление и операции над графом объектов.

**Пошаговое описание действий:**
1. Реализовать `snapshotObject`/`restoreObject`.
2. Реализовать `newScene`/`saveScene`/`openScene`.
3. Реализовать `duplicateObject`/`reparent`/`destroyObject`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `EditorContext`).

*Функции / методы:*
- `snapshotObject`, `restoreObject`, `newScene`, `saveScene`, `openScene`, `duplicateObject`, `reparent`, `destroyObject`.

*Логика функций / методов:*
- `snapshotObject(object)` — полный снимок поддерева (имя, трансформ, компоненты со всеми полями, дети). Возвращает: снимок.
- `restoreObject(snapshot, parent)` — восстанавливает поддерево из снимка под родителем (invalid = корень). Возвращает: хэндл корня.
- `newScene()` — очищает сцену.
- `saveScene(path)` — сохраняет в SKYB. Возвращает: успех.
- `openScene(path)` — открывает из файла. Возвращает: успех.
- `duplicateObject(object)` — глубокая копия с детьми. Возвращает: копию.
- `reparent(child, newParent)` — меняет родителя, сохраняя мировое положение.
- `destroyObject(object)` — удаляет объект.

**Результат по файлу:** снимок/восстановление и операции графа работают.

**Критерий правильности по файлу:**
1. `restoreObject(snapshotObject(x), parent)` воспроизводит поддерево `x`.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/editor_context.hpp`
2. `editor/shell/src/editor_context.cpp`

**Общий критерий правильности:**
1. `save → open` восстанавливает граф объектов, трансформы и поля компонентов.

---

## feature/shadow-mapping

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** Vulkan-рендерер (`vulkan_renderer.cpp`, `FrameUbo`, `mesh.frag`) и построитель кадра из предыдущих этапов контура E2

**Цель фичи:** теневое картирование от направленного источника.

**Описание фичи:** depth-only проход теней и основной проход с привязкой карты теней; объекты отбрасывают тени в редакторе и плеере.

**Общий порядок реализации фичи:**
1. Написать depth-only вершинный шейдер `shadow.vert` и встроить `shadow.vert.spv.h`.
2. Создать теневые ресурсы (`initShadowResources`), провести теневой и основной проходы в `renderFrame`, расширить `FrameUbo`, добавить `shadowVisibility` в `mesh.frag`.

**Файлы фичи:**
1. `engine/rendering_vulkan/shaders/shadow.vert`
2. `engine/rendering_vulkan/src/vulkan_renderer.cpp`

### Файл: `engine/rendering_vulkan/shaders/shadow.vert`

**Назначение файла:** depth-only вершинный шейдер для прохода теней (+ встроенный `shadow.vert.spv.h`).

**Пошаговое описание действий:**
1. Реализовать вычисление позиции `lightViewProjection * model * position`.
2. Передать обе матрицы в push-константах через `struct ShadowPush`.
3. Скомпилировать в SPIR-V и встроить массив в `shadow.vert.spv.h`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct ShadowPush` (в push-константах: `lightViewProjection`, `model`)

*Функции / методы:* вершинная точка входа шейдера.

*Логика функций / методов:*
- depth-only вершинный шейдер: `lightViewProjection * model * position`; обе матрицы в push-константах (`struct ShadowPush`).

**Результат по файлу:** глубинный шейдер теней, готовый к использованию в теневом проходе.

**Критерий правильности по файлу:**
1. Шейдер компилируется в SPIR-V; `shadow.vert.spv.h` встраивается.

### Файл: `engine/rendering_vulkan/src/vulkan_renderer.cpp`

**Назначение файла:** дополнение Vulkan-рендерера теневым проходом и выборкой карты теней.

**Пошаговое описание действий:**
1. Реализовать `initShadowResources()` — карта глубины, сэмплер, проход и конвейер теней.
2. В `renderFrame` провести сначала depth-only проход теней, затем основной проход с привязкой карты теней.
3. Расширить `FrameUbo` полями теней.
4. Добавить в `mesh.frag` функцию `shadowVisibility`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- расширение `FrameUbo`: `lightViewProjection[16]`, флаг теней и индекс источника.

*Функции / методы:*
- `initShadowResources()`
- дополнение `renderFrame()`
- `shadowVisibility(worldPos, ndl)` (в `mesh.frag`)

*Логика функций / методов:*
- `initShadowResources()` — создаёт сэмплируемую карту глубины 2048×2048 (D32), сэмплер с зажимом к границе, отдельный проход и конвейер (со slope-scaled bias), дескриптор set 7.
- в `renderFrame` — сначала depth-only проход теней по всем `DrawMesh`, затем основной проход с привязкой карты теней.
- `FrameUbo` растёт: `lightViewProjection[16]`, флаг теней и индекс источника.
- в `mesh.frag` — функция `shadowVisibility(worldPos, ndl)`: выборка 3×3 (PCF) с нормаль-зависимым смещением.

**Результат по файлу:** рендерер выполняет теневой проход и затеняет основной кадр.

**Критерий правильности по файлу:**
1. На кадре видна тень под объектом; тень движется вместе с падающим телом.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/rendering_vulkan/shaders/shadow.vert` (+ `shadow.vert.spv.h`)
2. `engine/rendering_vulkan/src/vulkan_renderer.cpp` (дополнение: `initShadowResources`, теневой проход в `renderFrame`, рост `FrameUbo`, `shadowVisibility` в `mesh.frag`)

**Общий критерий правильности:**
1. Объекты отбрасывают тени в редакторе и плеере (подтверждается скриншотом).
2. На кадре видна тень под объектом; тень движется вместе с падающим телом.

---

## feature/dragdrop-2d

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/mesh-picker` (создание модели), `feature/vulkan-viewport` (`VulkanViewport`)

**Цель фичи:** перетаскивание модели в сцену и ортографический режим 2D.

**Описание фичи:** вторая фича Этапа 4 контура E3 — drag-n-drop модели из панели проекта, режим 2D и угловой гизмо переключения проекций.

**Общий порядок реализации фичи:**
1. Реализовать перетаскивание модели из панели проекта в сцену.
2. Реализовать ортографический режим 2D и угловой гизмо переключения проекций.
3. Объявить P/Invoke для создания меша, режима 2D и снапа к оси.

**Файлы фичи:**
1. `editor/avalonia/Views/ProjectView.axaml.cs`
2. `editor/avalonia/Views/GameView.axaml.cs`

### Файл: `editor/avalonia/Views/ProjectView.axaml.cs`

**Назначение файла:** панель проекта — источник перетаскивания модели.

**Пошаговое описание действий:**
1. Оформить `DataObject` со ссылкой на ассет для перетаскивания.
2. Инициировать drag-n-drop модели в сцену.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class ProjectView`

*Функции / методы:*
- обработчики перетаскивания модели.

*Логика функций / методов:*
- перетаскивание модели из панели проекта в сцену (`DataObject` с ссылкой на ассет).
- задействует P/Invoke `sky_editor_create_mesh_object` при сбросе модели в сцену.

**Результат по файлу:** модель перетаскивается из панели проекта.

**Критерий правильности по файлу:**
1. Модель из панели проекта появляется в сцене.

### Файл: `editor/avalonia/Views/GameView.axaml.cs`

**Назначение файла:** режим 2D и переключение проекций.

**Пошаговое описание действий:**
1. Реализовать ортографический режим 2D.
2. Реализовать угловой гизмо переключения проекций (в `VulkanViewport`).

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class GameView`

*Функции / методы:*
- обработчики переключения 3D/2D; P/Invoke `sky_editor_set_view_2d`, `sky_editor_look_along_axis`.

*Логика функций / методов:*
- ортографический режим 2D и угловой гизмо переключения проекций (в `VulkanViewport`).
- P/Invoke: `sky_editor_create_mesh_object`, `sky_editor_set_view_2d`, `sky_editor_look_along_axis`.

**Результат по файлу:** доступен режим 2D и переключение проекций.

**Критерий правильности по файлу:**
1. Переключение 3D/2D работает.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Views/ProjectView.axaml.cs`
2. `editor/avalonia/Views/GameView.axaml.cs`

**Общий критерий правильности:**
1. Модель добавляется в сцену перетаскиванием (появляется из панели проекта).
2. Переключение 3D/2D работает.

---

## feature/gameplay-api

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** `feature/user-assemblies` (загрузка пользовательских сборок); подсистема скриптинга и обратный API движка из Этапа 4 контура E4

**Цель фичи:** расширение API движка функциями геймплея (позиция, спавн, уничтожение, скорость, луч).

**Описание фичи:** таблица обратного API растёт до 12 указателей; скриптам становятся доступны `Instantiate`/`Destroy`/velocity/raycast, а нативная сторона реализует соответствующие колбэки и физический луч по heightfield.

**Общий порядок реализации фичи:**
1. Расширить таблицу обратного API в `Engine.cs`.
2. Добавить геймплейные методы в `ScriptComponent.cs`.
3. Добавить `Physics.Raycast` в `Physics.cs`.
4. Реализовать нативные колбэки и raycast по heightfield в `editor_context.cpp`.

**Файлы фичи:**
1. `managed/SkyEngine.Managed/Engine.cs`
2. `managed/SkyEngine.Managed/ScriptComponent.cs`
3. `managed/SkyEngine.Managed/Physics.cs`
4. `editor/shell/src/editor_context.cpp`

### Файл: `managed/SkyEngine.Managed/Engine.cs`

**Назначение файла:** дополнение таблицы делегатов обратного API геймплейными функциями.

**Пошаговое описание действий:**
1. Расширить таблицу обратного API до 12 указателей.

**Что должно быть в файле:**

*Структуры / классы / enum:* дополнение таблицы делегатов обратного API.

*Функции / методы:*
- делегаты `GetWorldPosition`, `Instantiate`, `DestroyObject`, `SetVelocity`, `GetVelocity`, `Raycast`.

*Логика функций / методов:*
- таблица обратного API растёт до 12 указателей: `GetWorldPosition`, `Instantiate`, `DestroyObject`, `SetVelocity`, `GetVelocity`, `Raycast`.

**Результат по файлу:** managed-сторона знает геймплейные нативные функции.

**Критерий правильности по файлу:**
1. Таблица обратного API содержит 12 указателей и связывается при инициализации.

### Файл: `managed/SkyEngine.Managed/ScriptComponent.cs`

**Назначение файла:** дополнение базового класса скрипта геймплейными методами.

**Пошаговое описание действий:**
1. Добавить защищённые `Instantiate`, `Destroy`, `SetVelocity`, `GetWorldPosition`.

**Что должно быть в файле:**

*Структуры / классы / enum:* дополнение `ScriptComponent`.

*Функции / методы:*
- защищённые `Instantiate(prefab, x, y, z)`, `Destroy()`, `SetVelocity(x, y, z)`, `GetWorldPosition()`.

*Логика функций / методов:*
- `ScriptComponent`: защищённые `Instantiate(prefab, x,y,z)`, `Destroy()`, `SetVelocity(x,y,z)`, `GetWorldPosition()` (в т.ч. по id чужого объекта).

**Результат по файлу:** скрипт может спавнить/уничтожать объекты и управлять скоростью.

**Критерий правильности по файлу:**
1. Скрипт спавнит/уничтожает объекты и читает мировую позицию (в т.ч. чужого объекта).

### Файл: `managed/SkyEngine.Managed/Physics.cs`

**Назначение файла:** доступ к физическому лучу из скриптов.

**Пошаговое описание действий:**
1. Добавить `Physics.Raycast`.

**Что должно быть в файле:**

*Структуры / классы / enum:* `Physics`, `RaycastHit`.

*Функции / методы:*
- `Physics.Raycast(...) → RaycastHit`

*Логика функций / методов:*
- `Physics.cs`: `Physics.Raycast(...) → RaycastHit`.

**Результат по файлу:** скрипт читает физический луч.

**Критерий правильности по файлу:**
1. `Physics.Raycast` возвращает попадание `RaycastHit`.

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** дополнение сборочной точки нативными колбэками геймплейного API.

**Пошаговое описание действий:**
1. Реализовать нативные колбэки геймплея.
2. Реализовать raycast физики по heightfield.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `EditorContext`).

*Функции / методы:*
- нативные колбэки `scriptGetWorldPosition`, `scriptInstantiate`, `scriptDestroyObject`, `scriptSetVelocity`, `scriptGetVelocity`, `scriptRaycast`.

*Логика функций / методов:*
- нативные колбэки `scriptGetWorldPosition/Instantiate/DestroyObject/SetVelocity/GetVelocity/Raycast`; raycast физики по heightfield (марш + бисекция + нормаль).

**Результат по файлу:** нативная сторона обслуживает геймплейный API скриптов.

**Критерий правильности по файлу:**
1. Скрипт спавнит/уничтожает объекты и читает физический луч.

### На выходе должно получиться

**Список артефактов фичи:**
1. `managed/SkyEngine.Managed/Engine.cs` (дополнение: таблица обратного API до 12 указателей)
2. `managed/SkyEngine.Managed/ScriptComponent.cs` (дополнение: `Instantiate`, `Destroy`, `SetVelocity`, `GetWorldPosition`)
3. `managed/SkyEngine.Managed/Physics.cs` (дополнение: `Physics.Raycast`)
4. `editor/shell/src/editor_context.cpp` (дополнение: нативные колбэки геймплея, raycast по heightfield)

**Общий критерий правильности:**
1. Доступны `Instantiate`/`Destroy`/velocity/raycast.
2. Скрипт спавнит/уничтожает объекты и читает физический луч.

---

## feature/package-lock-and-install

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** `feature/package-resolver` (`PackageManifest`, разрешение версий)

**Цель фичи:** файл блокировки `sky.lock` и установка пакетов из источников.

**Описание фичи:** контрольная сумма манифеста, запись/чтение `sky.lock` (версии + чексуммы + active) и установка пакета из каталога / tarball / git через immutable-кэш.

**Общий порядок реализации фичи:**
1. Объявить `manifestChecksum`, `savePackageLock`, `loadPackageLock` в `package_lock.hpp`.
2. Объявить `PackageInstaller` в `package_installer.hpp`.
3. Реализовать установку из источников в `package_installer.cpp`.

**Файлы фичи:**
1. `engine/package/include/sky/package/package_lock.hpp`
2. `engine/package/include/sky/package/package_installer.hpp`
3. `engine/package/src/package_installer.cpp`

### Файл: `engine/package/include/sky/package/package_lock.hpp`

**Назначение файла:** контрольная сумма манифеста и файл блокировки.

**Пошаговое описание действий:**
1. Объявить `manifestChecksum`.
2. Объявить `savePackageLock`/`loadPackageLock` и тип `LockedPackage`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct LockedPackage` (версия + чексумма + active).

*Функции / методы:*
- `std::uint64_t manifestChecksum(const PackageManifest& manifest)`
- `bool savePackageLock(...)`
- `std::optional<std::vector<LockedPackage>> loadPackageLock(...)`

*Логика функций / методов:*
- `manifestChecksum(manifest)` — Возвращает: контрольную сумму манифеста.
- `savePackageLock(...)` / `loadPackageLock(...)` — запись/чтение `sky.lock` (версии + чексуммы + active).

**Результат по файлу:** формат `sky.lock` и контрольная сумма манифеста.

**Критерий правильности по файлу:**
1. `sky.lock` записывается и читается без потерь (round-trip).

### Файл: `engine/package/include/sky/package/package_installer.hpp`

**Назначение файла:** установщик пакетов из источников.

**Пошаговое описание действий:**
1. Объявить `class PackageInstaller` с методом `install`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class PackageInstaller`

*Функции / методы:*
- `std::optional<PackageManifest> install(const std::string& source, const std::filesystem::path& projectPackages)`

*Логика функций / методов:*
- `install(source, projectPackages)` — устанавливает пакет из источника (каталог / tarball / git с `#tag`) через immutable-кэш. Параметры: `source`, `projectPackages` — куда установить. Возвращает: манифест или `nullopt`.

**Результат по файлу:** интерфейс установщика пакетов.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/package/src/package_installer.cpp`

**Назначение файла:** реализация установки из источников и immutable-кэша.

**Пошаговое описание действий:**
1. Реализовать установку из каталога, tarball и git с `#tag`.
2. Провести установку через immutable-кэш.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- реализация `PackageInstaller`.

*Функции / методы:*
- `install`.

*Логика функций / методов:*
- `install` — распознаёт источник (каталог / tarball / git с `#tag`), помещает пакет в immutable-кэш и устанавливает в `projectPackages`; возвращает манифест или `nullopt`.

**Результат по файлу:** рабочий установщик из трёх источников.

**Критерий правильности по файлу:**
1. Пакет ставится из каталога / архива / git.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/package/include/sky/package/package_lock.hpp`
2. `engine/package/include/sky/package/package_installer.hpp`
3. `engine/package/src/package_installer.cpp`

**Общий критерий правильности:**
1. `sky.lock` персистит (round-trip версий, чексумм и active).
2. Пакет ставится из каталога / архива / git.
