# Техническое задание · Контур E1 «Ядро и данные»

**Область ответственности.** Математический фундамент, объектная и
компонентная модели, модель сцены, механизм отмены операций и сборочная
точка редактора `EditorContext`. От этого контура зависят все остальные —
базовые интерфейсы предоставляются в первую очередь.

Все пути, сигнатуры и имена методов взяты из фактического репозитория. У
каждого метода указано: **сигнатура**, **что делает**, **параметры** и **что
возвращает**. Методы сгруппированы по классам; перед каждой группой — краткое
пояснение назначения.

---

## Этап 1 (Неделя 1). Математика, объектная и компонентная модели

**Общее описание задач контура.** Реализовать математический слой, службы
логирования и конфигурации, объектную модель (иерархия и трансформы) и
компонентную модель с полями, описываемыми данными.

### Сделай файл `engine/core/include/sky/core/math.hpp`

Свободные функции над векторами, кватернионами и трансформами (все
`constexpr`). Структуры `Vec3{x,y,z}`, `Quat{x,y,z,w}`, `Transform{position,
rotation, scale}` объявляются здесь же.

- `constexpr Vec3 operator+(const Vec3& a, const Vec3& b)`
  Что делает: покомпонентное сложение векторов.
  Параметры: `a`, `b` — слагаемые. Возвращает: сумму `{a.x+b.x, …}`.
- `constexpr Vec3 operator*(const Vec3& a, float s)`
  Что делает: масштабирование вектора числом.
  Параметры: `a` — вектор, `s` — множитель. Возвращает: `{a.x*s, …}`.
- `constexpr Quat operator*(const Quat& a, const Quat& b)`
  Что делает: композиция двух поворотов (сначала `b`, потом `a`).
  Параметры: `a`, `b` — кватернионы. Возвращает: результирующий поворот.
- `constexpr Vec3 rotate(const Quat& q, const Vec3& v)`
  Что делает: поворачивает вектор кватернионом.
  Параметры: `q` — поворот, `v` — исходный вектор. Возвращает: повёрнутый вектор.
- `constexpr Transform compose(const Transform& parent, const Transform& child)`
  Что делает: переводит локальный трансформ ребёнка в систему координат родителя (нужно для мирового трансформа).
  Параметры: `parent` — трансформ родителя, `child` — локальный трансформ ребёнка. Возвращает: трансформ ребёнка в системе родителя.
- `constexpr Quat conjugate(const Quat& q)`
  Что делает: сопряжённый кватернион (обратный поворот для единичного).
  Параметры: `q` — поворот. Возвращает: `{-q.x,-q.y,-q.z,q.w}`.
- `constexpr Transform invCompose(const Transform& parent, const Transform& world)`
  Что делает: обратная к `compose` — переводит мировой трансформ в локальный относительно родителя (нужно при смене родителя).
  Параметры: `parent` — трансформ родителя, `world` — мировой трансформ объекта. Возвращает: локальный трансформ относительно родителя.

**Проверка:** `rotate(поворот 90° вокруг Y, {0,0,1})` ≈ `{1,0,0}` (±1e-5);
`invCompose(parent, compose(parent, child)) == child`.

### Сделай файл `engine/core/include/sky/core/handle.hpp`

- `template <typename Tag> struct Handle { std::uint64_t value; … }`
  Что делает: типобезопасный идентификатор. Разные теги (`ObjectTag`,
  `ComponentTag`) дают несовместимые типы — нельзя перепутать хэндл объекта с
  хэндлом компонента. Содержит `isValid()`, статический `invalid()`,
  `operator==`.

### Сделай файл `engine/core/include/sky/core/logger.hpp`

Единый журнал для всех модулей. `enum class LogLevel {Trace,Debug,Info,
Warning,Error,Critical}` объявляется здесь.

- `virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0`
  Что делает: записывает строку журнала.
  Параметры: `level` — важность, `category` — подсистема-источник (напр. "Scene"), `message` — текст. Возвращает: ничего.
- `void info/warning/error(std::string_view category, std::string_view message)`
  Что делает: сокращения для частых уровней (вызывают `log` с нужным уровнем).
  Параметры: как выше без `level`. Возвращает: ничего.

### Сделай файл `engine/core/include/sky/core/config_service.hpp`

Иерархическая конфигурация уровня движка (без доменного состояния).

- `virtual std::optional<std::string> getString(std::string_view key) const = 0`
  Что делает: читает строковое значение по ключу.
  Параметры: `key` — имя настройки. Возвращает: значение или `nullopt`, если ключа нет.
- `virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0`
  То же для целого числа. Возвращает: число или `nullopt`.
- `virtual std::optional<bool> getBool(std::string_view key) const = 0`
  То же для логического. Возвращает: булево или `nullopt`.
- `virtual void set(std::string_view key, std::string value) = 0`
  Что делает: устанавливает значение.
  Параметры: `key` — имя, `value` — значение (строкой). Возвращает: ничего.

### Сделай файлы `engine/core/src/console_logger.cpp`, `engine/core/src/memory_config_service.cpp`

Реализации интерфейсов выше плюс фабрики (объявлены в `runtime_services.hpp`):
- `std::unique_ptr<ILogger> createConsoleLogger()` — журнал в stdout/stderr с именем уровня и категорией.
- `std::unique_ptr<IConfigService> createInMemoryConfigService()` — конфиг на основе `map` ключ→значение.

### Сделай файл `engine/object/include/sky/object/object_model.hpp`

Три контракта объектной модели. `using ObjectHandle = core::Handle<ObjectTag>`.

**`class IObjectFactory`** — создание и удаление объектов сцены.
- `virtual ObjectHandle createObject(const std::string& name) = 0`
  Создаёт объект с именем. Параметры: `name` — имя. Возвращает: хэндл нового объекта.
- `virtual void destroyObject(ObjectHandle object) = 0`
  Удаляет объект (и его поддерево). Параметры: `object` — что удалить. Возвращает: ничего.

**`class IObjectHierarchyAccess`** — иерархия и трансформы (единственный владелец).
- `virtual void setParent(ObjectHandle child, ObjectHandle parent) = 0`
  Перевешивает `child` под `parent` (недействительный parent = корень). Возвращает: ничего.
- `virtual ObjectHandle parentOf(ObjectHandle object) const = 0`
  Возвращает: хэндл родителя (или invalid для корня).
- `virtual std::vector<ObjectHandle> childrenOf(ObjectHandle object) const = 0`
  Возвращает: прямых детей объекта.
- `virtual void setLocalTransform(ObjectHandle object, const core::Transform& transform) = 0`
  Задаёт локальный трансформ (относительно родителя). Возвращает: ничего.
- `virtual core::Transform localTransform(ObjectHandle object) const = 0`
  Возвращает: локальный трансформ.
- `virtual core::Transform worldTransform(ObjectHandle object) const = 0`
  Возвращает: мировой трансформ (композиция локальных вверх по цепочке родителей).

**`class IObjectQueryService`** — запросы только для чтения.
- `virtual bool exists(ObjectHandle object) const = 0` — жив ли объект. Возвращает: да/нет.
- `virtual std::string nameOf(ObjectHandle object) const = 0` — Возвращает: имя объекта.
- `virtual std::vector<ObjectHandle> findByName(const std::string& name) const = 0` — Возвращает: все объекты с таким именем.

**Свободная функция:**
- `inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)`
  Что делает: задаёт мировой трансформ, пересчитывая локальный через `invCompose` (для корня — как есть). Параметры: `access` — иерархия, `object` — объект, `world` — желаемый мировой трансформ.

### Сделай файлы `engine/object/include/sky/object/object_world.hpp`, `engine/object/src/object_world.cpp`

- `class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService`
  Единый владелец иерархии. Добавляет:
  - `virtual void renameObject(ObjectHandle object, const std::string& name) = 0` — переименовывает объект.
- `std::unique_ptr<ObjectWorld> createObjectWorld()`
  Что делает: создаёт реализацию мира объектов. Возвращает: владеющий указатель.
  Внутри `.cpp`: хранилище `id → {локальный трансформ, родитель, дети, имя}`; `worldTransform` = `compose` локальных вверх; `destroyObject` рекурсивно удаляет поддерево.

### Сделай файл `engine/component/include/sky/component/component_model.hpp`

`using FieldValue = std::variant<float, std::int64_t, bool, std::string,
core::Vec3>` — значение поля любого из пяти типов. `ComponentDescriptor`
(typeId, displayName, список полей, категория) и `InspectableField{name,
typeName}` объявляются здесь.

**`class IComponentRegistry`** — реестр типов компонентов.
- `virtual void registerComponentType(const ComponentDescriptor& descriptor) = 0` — регистрирует тип с описанием полей.
- `virtual std::vector<ComponentDescriptor> availableTypes() const = 0` — Возвращает: список зарегистрированных типов (для меню Add Component).

**`class IComponentAttachmentService`** — навешивание компонентов.
- `virtual ComponentHandle attach(object::ObjectHandle object, const std::string& typeId) = 0`
  Вешает компонент типа `typeId` на объект. Возвращает: хэндл компонента.
- `virtual void detach(ComponentHandle component) = 0` — снимает компонент.

**`class IComponentQueryService`** — запросы.
- `virtual std::vector<ComponentHandle> componentsOf(object::ObjectHandle object) const = 0` — Возвращает: компоненты объекта.
- `virtual const ComponentDescriptor& descriptorOf(ComponentHandle component) const = 0` — Возвращает: описание типа компонента.
- `virtual object::ObjectHandle ownerOf(ComponentHandle component) const = 0` — Возвращает: объект-владелец компонента.

### Сделай файлы `engine/component/include/sky/component/component_world.hpp`, `engine/component/src/component_world.cpp`

`class ComponentWorld` наследует все интерфейсы выше и добавляет доступ к данным:
- `virtual void setField(ComponentHandle component, const std::string& name, FieldValue value) = 0`
  Записывает значение поля по имени. Параметры: `component`, `name` — имя поля, `value` — значение. Возвращает: ничего.
- `virtual std::optional<FieldValue> field(ComponentHandle component, const std::string& name) const = 0`
  Читает поле. Возвращает: значение или `nullopt`.
- `virtual std::map<std::string, FieldValue> fields(ComponentHandle component) const = 0`
  Возвращает: всю карту полей компонента (используется сериализацией, undo, скриптами).
- `virtual void detachAllFrom(object::ObjectHandle object) = 0` — снимает все компоненты объекта.
- `std::unique_ptr<ComponentWorld> createComponentWorld()` — фабрика.

### Сделай файлы `tests/core_tests.cpp`, `tests/world_tests.cpp`

Проверяют математику, объектный и компонентный миры (см. критерии этапа).

**На выходе должно получиться (список артефактов):**
- Библиотеки `sky_core`, `sky_object`, `sky_component` собраны и линкуются.
- Тесты `core_tests`, `world_tests` зелёные.

**Критерий правильности:** поворот `(0,0,1)` на 90° вокруг Y = `(1,0,0)`±1e-5;
ребёнок `(1,0,0)` под родителем, повёрнутым на 90° вокруг Y, в мире = `(0,0,-1)`;
поле каждого из 5 типов записывается и читается без потерь.

---

## Этап 2 (Неделя 2, веха M1). Сцена и сборочная точка

**Общее описание задач контура.** Реализовать модель сцены и собрать все
подсистемы движка в единый `EditorContext` с демонстрационной сценой.

### Сделай файлы `engine/scene/include/sky/scene/scene_world.hpp`, `engine/scene/src/scene_world.cpp`

`class SceneWorld` (наследует `ISceneRepository`, `ISceneRuntime`,
`ISceneQueryService`). Создаётся через `SceneWorldDeps` (ссылки на объектный,
компонентный, физический миры и бэкенд сериализации).
- `virtual void addRootObject(SceneHandle scene, object::ObjectHandle object) = 0`
  Добавляет объект в корень сцены. Параметры: `scene` — сцена, `object` — объект. Возвращает: ничего.
- `virtual bool saveSceneAs(SceneHandle scene, const std::filesystem::path& path, …) = 0`
  Сохраняет сцену в файл (на этапе 3 — полный SKYB). Возвращает: успех.
- `virtual std::vector<object::ObjectHandle> rootObjectsOf(SceneHandle scene) const = 0`
  Возвращает: корневые объекты сцены.
- `std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps)` — фабрика.

### Сделай файлы `engine/scene/include/sky/scene/scene_authoring.hpp`, `engine/scene/src/scene_authoring.cpp`

`enum class PrimitiveKind {Cube, Plane, Sphere}`.
- `object::ObjectHandle createPrimitive(const AuthoringServices& services, PrimitiveKind kind, const std::string& name)`
  Что делает: создаёт объект-примитив с компонентом Mesh Renderer.
  Параметры: `services` — набор ссылок на миры объектов/компонентов, `kind` — вид примитива, `name` — имя. Возвращает: хэндл созданного объекта.

### Сделай файлы `editor/shell/src/editor_context.hpp`, `editor/shell/src/editor_context.cpp`

`class EditorContext` — собирает движок в один объект (владеет всеми мирами
через `unique_ptr`) и строит демо-сцену. На этом этапе реализуй:
- `EditorContext()` — конструктор: создаёт все подсистемы, вызывает `buildDemoScene()`.
- `std::vector<object::ObjectHandle> rootObjects() const`
  Возвращает: корневые объекты активной сцены (нужно контуру E3 для дерева и E2 для обхода).
- `object::ObjectHandle createEmpty(const std::string& name)`
  Создаёт пустой объект в корне сцены. Возвращает: его хэндл.
- `object::ObjectHandle createPrimitive(scene::PrimitiveKind kind, const std::string& name)`
  Создаёт примитив с мешем. Возвращает: его хэндл.
- приватный `void buildDemoScene()` — наполняет сцену примитивами, светом, камерой.

**На выходе должно получиться:** библиотека `sky_scene` собрана; `EditorContext`
строит демо-сцену.
**Критерий правильности:** `rootObjects()` возвращает именованные объекты
демо-сцены; сцена пригодна для обхода рендером контура E2.

---

## Этап 3 (Недели 3–4, веха M2). Отмена операций и формат сцены SKYB

**Общее описание задач контура.** Реализовать стек команд отмены, бинарный
формат сцены SKYB и операции редактирования графа объектов. Снимок поддерева
делай без потерь — он используется отменой удаления, префабами и сценами.

### Сделай файлы `editor/shell/src/editor_commands.hpp`, `editor/shell/src/editor_commands.cpp`

- `class IEditorCommand { virtual void redo()=0; virtual void undo()=0; virtual std::string label() const=0; }`
  Базовый класс команды: применить, отменить, человекочитаемая метка.
- `class UndoStack`:
  - `explicit UndoStack(EditorContext&)` — привязка к контексту.
  - `void push(std::unique_ptr<IEditorCommand> command)` — добавляет и применяет команду. Параметры: `command` — команда.
  - `bool undo()` — отменяет верхнюю. Возвращает: было ли что отменять.
  - `bool redo()` — повторяет. Возвращает: было ли что повторять.
  - `bool canUndo() const` / `bool canRedo() const` — Возвращает: доступность.
- Фабрики команд (каждая возвращает `std::unique_ptr<IEditorCommand>`), реализуй по одной на операцию:
  - `makeTransformCommand(object, before, after)` — изменение трансформа (до/после).
  - `makeFieldCommand(component, fieldName, before, after)` — изменение поля компонента.
  - `makeCreateCommand(...)` / `makeDeleteCommand(context, object)` — создание/удаление.
  - `makeDuplicateCommand`, `makeReparentCommand`, `makeRenameCommand`, `makeAddComponentCommand`, `makeRemoveComponentCommand`, `makeMaterialCreateCommand`, `makeMaterialEditCommand` — аналогично, по смыслу имени.

### Дополни файл `editor/shell/src/editor_context.cpp`

Структуры снимка объявлены в `.hpp`: `ObjectSnapshot{name, local, hasPhysicsBody,
components, children}`, `ComponentSnapshot{typeId, fields}`.
- `ObjectSnapshot snapshotObject(object::ObjectHandle object) const`
  Что делает: делает полный снимок поддерева (имя, трансформ, компоненты со всеми полями, дети). Параметры: `object` — корень поддерева. Возвращает: снимок.
- `object::ObjectHandle restoreObject(const ObjectSnapshot& snapshot, object::ObjectHandle parent)`
  Что делает: восстанавливает поддерево из снимка под указанным родителем (invalid = корень). Возвращает: хэндл восстановленного корня.
- `void newScene()` — очищает сцену до пустой.
- `bool saveScene(const std::filesystem::path& path)` — сохраняет сцену в SKYB. Возвращает: успех.
- `bool openScene(const std::filesystem::path& path)` — открывает сцену из файла. Возвращает: успех (false = остаётся пустая сцена).
- `object::ObjectHandle duplicateObject(object::ObjectHandle object)` — глубоко копирует объект с детьми. Возвращает: копию.
- `void reparent(object::ObjectHandle child, object::ObjectHandle newParent)` — меняет родителя, сохраняя мировое положение.
- `void destroyObject(object::ObjectHandle object)` — удаляет объект из сцены.

### Дополни файл `engine/scene/src/scene_world.cpp`

- реализуй `saveSceneAs` как настоящий SKYB: магия "SKYB", схема `sky.scene`,
  обход корней → имя, трансформ, компоненты с полями (через `ByteWriter`);
  чтение обратно с прямой миграцией версий.

### Сделай файлы `tests/undo_tests.cpp`, `tests/scene_tests.cpp`

**На выходе должно получиться:** отмена/повтор всех операций; сцена сохраняется
в SKYB и открывается идентично.
**Критерий правильности:** `save → open` восстанавливает граф объектов,
трансформы и поля компонентов; undo/redo работают для transform/field/create/
delete/duplicate/reparent/rename.

---

## Этап 4 (Недели 5–6, веха M3). Восстановление физики из сцены

**Общее описание задач контура.** Реализовать воссоздание физических тел из
компонентов при открытии сцены.

### Дополни файл `editor/shell/src/editor_context.cpp`

- приватный `void reattachPhysics()`
  Что делает: при `openScene` создаёт физические тела и коллайдеры из
  компонентов `sky.rigidbody`/`sky.collider.box`, связывает их с объектами.
  Параметры: нет. Возвращает: ничего.

**На выходе должно получиться:** при открытии сцены физические тела
воссоздаются из компонентов.
**Критерий правильности:** сохранённая сцена с физикой после открытия падает
так же, как до сохранения.

---

## Этап 5 (Недели 7–8, веха M4). Префабы

**Общее описание задач контура.** Реализовать префабы SKYP поверх снимка
объекта.

### Дополни файл `editor/shell/src/editor_context.cpp`

- `bool savePrefab(object::ObjectHandle object, const std::string& path)`
  Что делает: сериализует поддерево объекта в файл `.skyprefab` (формат SKYP
  поверх `snapshotObject`). Параметры: `object` — что сохранить, `path` — путь
  (принимает `assets://…`). Возвращает: успех.
- `object::ObjectHandle instantiatePrefab(const std::string& path)`
  Что делает: создаёт объект из файла префаба в корне сцены. Параметры: `path`
  — путь к `.skyprefab`. Возвращает: хэндл созданного объекта (invalid при ошибке).
- `object::ObjectHandle spawnPrefabAt(const std::string& path, core::Vec3 position)`
  Что делает: то же, но ставит объект в мировую точку и пересаживает его
  физику (используется скриптами контура E4). Параметры: `path`, `position` —
  мировая позиция. Возвращает: хэндл объекта.

**На выходе должно получиться:** префаб сохраняется в `.skyprefab` и
инстанцируется идентично.
**Критерий правильности:** `savePrefab → instantiatePrefab` даёт объект с теми
же компонентами, полями, физикой и детьми.
