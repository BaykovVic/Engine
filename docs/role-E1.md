# Техническое задание · Контур E1 «Ядро и данные»

**Область ответственности.** Математический фундамент, объектная и
компонентная модели, модель сцены, механизм отмены операций и сборочная
точка редактора `EditorContext`. От этого контура зависят все остальные —
базовые интерфейсы предоставляются в первую очередь.

Каждый этап (неделя) разбит на **фичи** `feature/<название>` — логические
группы заданий недели. Одна фича = одна ветка в git и один запрос на слияние.
У каждого метода указаны: **сигнатура**, **что делает**, **параметры** и **что
возвращает**; методы сгруппированы по классам.

## Обозначения (C++)

Термины, встречающиеся в сигнатурах:
- `constexpr` — функция может вычисляться на этапе компиляции; пишется как обычная функция.
- `virtual … = 0` — чисто виртуальный метод: объявление без реализации, его обязана реализовать наследующая реализация (это контракт).
- `class IИмя` — интерфейс: набор методов-контрактов, реализуемых отдельным классом.
- `template <typename Tag>` — шаблон: обобщённый тип, параметризуемый другим типом.
- `std::optional<T>` — «значение типа T или ничего» (используется, когда данных может не быть).
- `std::unique_ptr<T>` — владеющий указатель: автоматически освобождает объект.
- `std::vector<T>` — динамический массив значений типа T.
- `std::variant<A,B,…>` — значение одного из перечисленных типов (у нас — поле любого из пяти типов).
- `std::string_view` — «взгляд» на строку без копирования (только для чтения).
- `const` в конце метода — метод не меняет объект (только читает).
- `[[nodiscard]]` — результат метода нельзя игнорировать.

---

## Этап 1 (Неделя 1). Математика, объектная и компонентная модели

**Общее описание задач этапа.** Математический слой, службы логирования и
конфигурации, объектная модель (иерархия и трансформы) и компонентная модель
с полями, описываемыми данными. Разбито на пять фич.

### feature/math-and-handles

Математика и типобезопасные идентификаторы — фундамент, от которого зависят все.

#### Файл `engine/core/include/sky/core/math.hpp`
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

#### Файл `engine/core/include/sky/core/handle.hpp`
- `template <typename Tag> struct Handle { std::uint64_t value; … }`
  Что делает: типобезопасный идентификатор. Разные теги (`ObjectTag`,
  `ComponentTag`) дают несовместимые типы — нельзя перепутать хэндл объекта с
  хэндлом компонента. Содержит `isValid()`, статический `invalid()`, `operator==`.

**Проверка фичи:** `rotate(поворот 90° вокруг Y, {0,0,1})` ≈ `{1,0,0}` (±1e-5);
`invCompose(parent, compose(parent, child)) == child`.

### feature/core-services

Единый журнал и служба конфигурации — используются всеми подсистемами.

#### Файл `engine/core/include/sky/core/logger.hpp`
`enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}` объявляется здесь.
- `virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0`
  Что делает: записывает строку журнала.
  Параметры: `level` — важность, `category` — подсистема-источник, `message` — текст. Возвращает: ничего.
- `void info/warning/error(std::string_view category, std::string_view message)`
  Что делает: сокращения для частых уровней (вызывают `log`). Возвращает: ничего.

#### Файл `engine/core/include/sky/core/config_service.hpp`
- `virtual std::optional<std::string> getString(std::string_view key) const = 0`
  Что делает: читает строковое значение. Параметры: `key` — имя настройки. Возвращает: значение или `nullopt`.
- `virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0` — то же для целого. Возвращает: число или `nullopt`.
- `virtual std::optional<bool> getBool(std::string_view key) const = 0` — то же для логического. Возвращает: булево или `nullopt`.
- `virtual void set(std::string_view key, std::string value) = 0`
  Что делает: устанавливает значение. Параметры: `key`, `value`. Возвращает: ничего.

#### Файлы `engine/core/src/console_logger.cpp`, `engine/core/src/memory_config_service.cpp`
Реализации интерфейсов выше плюс фабрики (объявлены в `runtime_services.hpp`):
- `std::unique_ptr<ILogger> createConsoleLogger()` — журнал в stdout/stderr.
- `std::unique_ptr<IConfigService> createInMemoryConfigService()` — конфиг на `map` ключ→значение.

### feature/object-model

Единственный владелец иерархии сцены и трансформов.

#### Файл `engine/object/include/sky/object/object_model.hpp`
Три контракта. `using ObjectHandle = core::Handle<ObjectTag>`.

**`class IObjectFactory`** — создание и удаление объектов.
- `virtual ObjectHandle createObject(const std::string& name) = 0`
  Что делает: создаёт объект. Параметры: `name`. Возвращает: хэндл нового объекта.
- `virtual void destroyObject(ObjectHandle object) = 0`
  Что делает: удаляет объект и его поддерево. Параметры: `object`. Возвращает: ничего.

**`class IObjectHierarchyAccess`** — иерархия и трансформы.
- `virtual void setParent(ObjectHandle child, ObjectHandle parent) = 0` — перевешивает `child` под `parent` (invalid = корень).
- `virtual ObjectHandle parentOf(ObjectHandle object) const = 0` — Возвращает: родителя (или invalid).
- `virtual std::vector<ObjectHandle> childrenOf(ObjectHandle object) const = 0` — Возвращает: прямых детей.
- `virtual void setLocalTransform(ObjectHandle object, const core::Transform& transform) = 0` — задаёт локальный трансформ.
- `virtual core::Transform localTransform(ObjectHandle object) const = 0` — Возвращает: локальный трансформ.
- `virtual core::Transform worldTransform(ObjectHandle object) const = 0` — Возвращает: мировой трансформ (композиция локальных вверх по цепочке).

**`class IObjectQueryService`** — запросы для чтения.
- `virtual bool exists(ObjectHandle object) const = 0` — Возвращает: жив ли объект.
- `virtual std::string nameOf(ObjectHandle object) const = 0` — Возвращает: имя.
- `virtual std::vector<ObjectHandle> findByName(const std::string& name) const = 0` — Возвращает: объекты с таким именем.

**Свободная функция:**
- `inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)`
  Что делает: задаёт мировой трансформ, пересчитывая локальный через `invCompose`. Параметры: `access`, `object`, `world`.

#### Файлы `engine/object/include/sky/object/object_world.hpp`, `engine/object/src/object_world.cpp`
- `class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService` — единый владелец, добавляет:
  - `virtual void renameObject(ObjectHandle object, const std::string& name) = 0` — переименовывает объект.
- `std::unique_ptr<ObjectWorld> createObjectWorld()` — Возвращает: реализацию мира объектов.
  Внутри `.cpp`: хранилище `id → {локальный трансформ, родитель, дети, имя}`; `worldTransform` = `compose` вверх; `destroyObject` рекурсивно удаляет поддерево.

### feature/component-model

Компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.

#### Файл `engine/component/include/sky/component/component_model.hpp`
`using FieldValue = std::variant<float, std::int64_t, bool, std::string,
core::Vec3>`. `ComponentDescriptor{typeId, displayName, fields, category}`.

**`class IComponentRegistry`** — реестр типов.
- `virtual void registerComponentType(const ComponentDescriptor& descriptor) = 0` — регистрирует тип.
- `virtual std::vector<ComponentDescriptor> availableTypes() const = 0` — Возвращает: типы для меню Add Component.

**`class IComponentAttachmentService`** — навешивание.
- `virtual ComponentHandle attach(object::ObjectHandle object, const std::string& typeId) = 0` — Возвращает: хэндл компонента.
- `virtual void detach(ComponentHandle component) = 0` — снимает компонент.

**`class IComponentQueryService`** — запросы.
- `virtual std::vector<ComponentHandle> componentsOf(object::ObjectHandle object) const = 0` — Возвращает: компоненты объекта.
- `virtual const ComponentDescriptor& descriptorOf(ComponentHandle component) const = 0` — Возвращает: описание типа.
- `virtual object::ObjectHandle ownerOf(ComponentHandle component) const = 0` — Возвращает: объект-владелец.

#### Файлы `engine/component/include/sky/component/component_world.hpp`, `engine/component/src/component_world.cpp`
`class ComponentWorld` наследует интерфейсы выше и добавляет доступ к данным:
- `virtual void setField(ComponentHandle component, const std::string& name, FieldValue value) = 0` — записывает поле по имени.
- `virtual std::optional<FieldValue> field(ComponentHandle component, const std::string& name) const = 0` — Возвращает: значение или `nullopt`.
- `virtual std::map<std::string, FieldValue> fields(ComponentHandle component) const = 0` — Возвращает: всю карту полей.
- `virtual void detachAllFrom(object::ObjectHandle object) = 0` — снимает все компоненты объекта.
- `std::unique_ptr<ComponentWorld> createComponentWorld()` — фабрика.

### feature/core-tests

#### Файлы `tests/core_tests.cpp`, `tests/world_tests.cpp`
Проверяют математику, объектный и компонентный миры.

**На выходе должно получиться (список артефактов):**
- Библиотеки `sky_core`, `sky_object`, `sky_component` собраны и линкуются.
- Тесты `core_tests`, `world_tests` зелёные.

**Критерий правильности этапа:** поворот `(0,0,1)` на 90° вокруг Y = `(1,0,0)`±1e-5;
ребёнок `(1,0,0)` под родителем, повёрнутым на 90° вокруг Y, в мире = `(0,0,-1)`;
поле каждого из 5 типов записывается и читается без потерь.

---

## Этап 2 (Неделя 2, веха M1). Сцена и сборочная точка

**Общее описание задач этапа.** Модель сцены и сборка всех подсистем в единый
`EditorContext` с демонстрационной сценой. Три фичи.

### feature/scene-world

#### Файлы `engine/scene/include/sky/scene/scene_world.hpp`, `engine/scene/src/scene_world.cpp`
`class SceneWorld` (наследует `ISceneRepository`, `ISceneRuntime`,
`ISceneQueryService`), создаётся через `SceneWorldDeps`.
- `virtual void addRootObject(SceneHandle scene, object::ObjectHandle object) = 0` — добавляет объект в корень сцены.
- `virtual bool saveSceneAs(SceneHandle scene, const std::filesystem::path& path, …) = 0` — сохраняет сцену (полный SKYB — на этапе 3). Возвращает: успех.
- `virtual std::vector<object::ObjectHandle> rootObjectsOf(SceneHandle scene) const = 0` — Возвращает: корневые объекты.
- `std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps)` — фабрика.

### feature/scene-authoring

#### Файлы `engine/scene/include/sky/scene/scene_authoring.hpp`, `engine/scene/src/scene_authoring.cpp`
`enum class PrimitiveKind {Cube, Plane, Sphere}`.
- `object::ObjectHandle createPrimitive(const AuthoringServices& services, PrimitiveKind kind, const std::string& name)`
  Что делает: создаёт объект-примитив с компонентом Mesh Renderer.
  Параметры: `services` — ссылки на миры объектов/компонентов, `kind` — вид, `name` — имя. Возвращает: хэндл объекта.

### feature/editor-context

#### Файлы `editor/shell/src/editor_context.hpp`, `editor/shell/src/editor_context.cpp`
`class EditorContext` — собирает движок в один объект и строит демо-сцену.
- `EditorContext()` — конструктор: создаёт подсистемы, вызывает `buildDemoScene()`.
- `std::vector<object::ObjectHandle> rootObjects() const` — Возвращает: корневые объекты (нужно E3 для дерева, E2 для обхода).
- `object::ObjectHandle createEmpty(const std::string& name)` — Возвращает: хэндл пустого объекта.
- `object::ObjectHandle createPrimitive(scene::PrimitiveKind kind, const std::string& name)` — Возвращает: хэндл примитива.
- приватный `void buildDemoScene()` — наполняет сцену примитивами, светом, камерой.

**На выходе:** библиотека `sky_scene` собрана; `EditorContext` строит демо-сцену.
**Критерий правильности этапа:** `rootObjects()` возвращает именованные объекты
демо-сцены; сцена пригодна для обхода рендером контура E2.

---

## Этап 3 (Недели 3–4, веха M2). Отмена операций и формат сцены SKYB

**Общее описание задач этапа.** Стек команд отмены, бинарный формат сцены SKYB
и операции редактирования графа объектов. Четыре фичи.

### feature/undo-stack

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

### feature/object-snapshot

#### Дополнение файла `editor/shell/src/editor_context.cpp`
Структуры снимка в `.hpp`: `ObjectSnapshot{name, local, hasPhysicsBody,
components, children}`, `ComponentSnapshot{typeId, fields}`.
- `ObjectSnapshot snapshotObject(object::ObjectHandle object) const`
  Что делает: полный снимок поддерева (имя, трансформ, компоненты со всеми полями, дети). Возвращает: снимок.
- `object::ObjectHandle restoreObject(const ObjectSnapshot& snapshot, object::ObjectHandle parent)`
  Что делает: восстанавливает поддерево из снимка под родителем (invalid = корень). Возвращает: хэндл корня.
- `void newScene()` — очищает сцену.
- `bool saveScene(const std::filesystem::path& path)` — сохраняет в SKYB. Возвращает: успех.
- `bool openScene(const std::filesystem::path& path)` — открывает из файла. Возвращает: успех.
- `object::ObjectHandle duplicateObject(object::ObjectHandle object)` — глубокая копия с детьми. Возвращает: копию.
- `void reparent(object::ObjectHandle child, object::ObjectHandle newParent)` — меняет родителя, сохраняя мировое положение.
- `void destroyObject(object::ObjectHandle object)` — удаляет объект.

### feature/skyb-serialization

#### Дополнение файла `engine/scene/src/scene_world.cpp`
- `saveSceneAs` реализуется как настоящий SKYB: магия "SKYB", схема `sky.scene`,
  обход корней → имя, трансформ, компоненты с полями (через `ByteWriter`); чтение
  обратно с прямой миграцией версий.

### feature/undo-scene-tests

#### Файлы `tests/undo_tests.cpp`, `tests/scene_tests.cpp`

**На выходе:** отмена/повтор всех операций; сцена сохраняется в SKYB и открывается идентично.
**Критерий правильности этапа:** `save → open` восстанавливает граф объектов,
трансформы и поля компонентов; undo/redo работают для transform/field/create/
delete/duplicate/reparent/rename.

---

## Этап 4 (Недели 5–6, веха M3). Восстановление физики из сцены

**Общее описание задач этапа.** Воссоздание физических тел из компонентов при
открытии сцены. Одна фича.

### feature/physics-reattach

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- приватный `void reattachPhysics()`
  Что делает: при `openScene` создаёт физические тела и коллайдеры из
  компонентов `sky.rigidbody`/`sky.collider.box`, связывает их с объектами.
  Возвращает: ничего.

**На выходе:** при открытии сцены физические тела воссоздаются из компонентов.
**Критерий правильности этапа:** сохранённая сцена с физикой после открытия
падает так же, как до сохранения.

---

## Этап 5 (Недели 7–8, веха M4). Префабы

**Общее описание задач этапа.** Префабы SKYP поверх снимка объекта. Одна фича.

### feature/prefabs

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `bool savePrefab(object::ObjectHandle object, const std::string& path)`
  Что делает: сериализует поддерево в файл `.skyprefab` (SKYP поверх `snapshotObject`).
  Параметры: `object` — что сохранить, `path` — путь (принимает `assets://…`). Возвращает: успех.
- `object::ObjectHandle instantiatePrefab(const std::string& path)`
  Что делает: создаёт объект из файла префаба в корне сцены. Параметры: `path`. Возвращает: хэндл (invalid при ошибке).
- `object::ObjectHandle spawnPrefabAt(const std::string& path, core::Vec3 position)`
  Что делает: то же, но ставит объект в мировую точку и пересаживает его физику (используется скриптами контура E4). Параметры: `path`, `position`. Возвращает: хэндл объекта.

**На выходе:** префаб сохраняется в `.skyprefab` и инстанцируется идентично.
**Критерий правильности этапа:** `savePrefab → instantiatePrefab` даёт объект с
теми же компонентами, полями, физикой и детьми.
