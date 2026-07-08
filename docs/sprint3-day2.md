# Спринт 3. День 2

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

## feature/input-state

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** `EditorContext` (`feature/editor-context`)

**Цель фичи:** состояние клавиш, доступное движку и скриптам.

**Описание фичи:** часть Этапа 3 (режим воспроизведения и ввод) — inline-методы состояния ввода в `EditorContext`.

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
