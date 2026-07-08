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
  Реализация: одна строка `return {a.x + b.x, a.y + b.y, a.z + b.z};` — агрегатный литерал `Vec3` из трёх покомпонентных сумм.
- `constexpr Vec3 operator*(const Vec3& a, float s)`
  Что делает: масштабирование вектора числом.
  Параметры: `a` — вектор, `s` — множитель. Возвращает: `{a.x*s, …}`.
  Реализация: `return {a.x * s, a.y * s, a.z * s};` — каждая компонента домножается на скаляр `s`.
- `constexpr Quat operator*(const Quat& a, const Quat& b)`
  Что делает: композиция двух поворотов (сначала `b`, потом `a`).
  Параметры: `a`, `b` — кватернионы. Возвращает: результирующий поворот.
  Реализация: возвращает четыре компоненты по формуле произведения кватернионов Гамильтона (`w*x + x*w + y*z − z*y` и т.д. для x,y,z и `w*w − x*x − y*y − z*z` для w).
- `constexpr Vec3 rotate(const Quat& q, const Vec3& v)`
  Что делает: поворачивает вектор кватернионом.
  Параметры: `q` — поворот, `v` — исходный вектор. Возвращает: повёрнутый вектор.
  Реализация: быстрая форма `q * v * q^-1` без промежуточного кватерниона: берётся `u = {q.x,q.y,q.z}`, считается `t = 2·cross(u,v)`, затем `v + q.w·t + cross(u,t)`.
- `constexpr Transform compose(const Transform& parent, const Transform& child)`
  Что делает: переводит локальный трансформ ребёнка в систему координат родителя (нужно для мирового трансформа).
  Параметры: `parent` — трансформ родителя, `child` — локальный трансформ ребёнка. Возвращает: трансформ ребёнка в системе родителя.
  Реализация: позиция = `parent.position + rotate(parent.rotation, child.position * parent.scale)`, поворот = `parent.rotation * child.rotation`, масштаб = покомпонентное произведение `parent.scale * child.scale`.
- `constexpr Quat conjugate(const Quat& q)`
  Что делает: сопряжённый кватернион (обратный поворот для единичного).
  Параметры: `q` — поворот. Возвращает: `{-q.x,-q.y,-q.z,q.w}`.
  Реализация: одна строка `return {-q.x, -q.y, -q.z, q.w};` — инвертируются знаки мнимой части, скалярная `w` сохраняется.
- `constexpr Transform invCompose(const Transform& parent, const Transform& world)`
  Что делает: обратная к `compose` — переводит мировой трансформ в локальный относительно родителя (нужно при смене родителя).
  Параметры: `parent` — трансформ родителя, `world` — мировой трансформ объекта. Возвращает: локальный трансформ относительно родителя.
  Реализация: `inverse = conjugate(parent.rotation)`; позиция = `divide(rotate(inverse, world.position − parent.position), parent.scale)`, поворот = `inverse * world.rotation`, масштаб = покомпонентное `divide(world.scale, parent.scale)`.

#### Файл `engine/core/include/sky/core/handle.hpp`
- `template <typename Tag> struct Handle { std::uint64_t value; … }`
  Что делает: типобезопасный идентификатор. Разные теги (`ObjectTag`,
  `ComponentTag`) дают несовместимые типы — нельзя перепутать хэндл объекта с
  хэндлом компонента. Содержит `isValid()`, статический `invalid()`, `operator==`.
  Реализация: шаблон-структура с единственным полем `std::uint64_t value = 0`; `isValid()` возвращает `value != 0`, `invalid()` — `constexpr Handle{0}`, сравнения генерируются через `operator<=>(const Handle&) const = default`. Тег в теле не используется — служит лишь для разделения типов на этапе компиляции.

**Проверка фичи:** `rotate(поворот 90° вокруг Y, {0,0,1})` ≈ `{1,0,0}` (±1e-5);
`invCompose(parent, compose(parent, child)) == child`.

### feature/core-services

Единый журнал и служба конфигурации — используются всеми подсистемами.

#### Файл `engine/core/include/sky/core/logger.hpp`
`enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}` объявляется здесь.
- `virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0`
  Что делает: записывает строку журнала.
  Параметры: `level` — важность, `category` — подсистема-источник, `message` — текст. Возвращает: ничего.
  Реализация: чистый контракт; тело в `ConsoleLogger::log` (`console_logger.cpp`) — отсекает уровни ниже `minimumLevel_`, выбирает поток (`stderr` при `level >= Error`, иначе `stdout`), под `std::scoped_lock` печатает `[УРОВЕНЬ] [категория] сообщение` через `std::fprintf` с `%.*s` и делает `fflush`.
- `void info/warning/error(std::string_view category, std::string_view message)`
  Что делает: сокращения для частых уровней (вызывают `log`). Возвращает: ничего.
  Реализация: невиртуальные однострочники прямо в интерфейсе — каждый вызывает `log(LogLevel::Info/Warning/Error, category, message)` с зафиксированным уровнем.

#### Файл `engine/core/include/sky/core/config_service.hpp`
- `virtual std::optional<std::string> getString(std::string_view key) const = 0`
  Что делает: читает строковое значение. Параметры: `key` — имя настройки. Возвращает: значение или `nullopt`.
  Реализация: контракт; тело в `InMemoryConfigService` — под `std::scoped_lock` ищет ключ в `std::unordered_map<std::string,std::string> values_`, возвращает `it->second` или `std::nullopt`.
- `virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0` — то же для целого. Возвращает: число или `nullopt`.
  Реализация: берёт `getString(key)`; при наличии парсит через `std::from_chars` по всей длине строки, возвращает число или `nullopt`, если разбор не удался или строка разобрана не полностью.
- `virtual std::optional<bool> getBool(std::string_view key) const = 0` — то же для логического. Возвращает: булево или `nullopt`.
  Реализация: берёт `getString(key)`; возвращает `true` для `"true"`/`"1"`, `false` для `"false"`/`"0"`, иначе `nullopt`.
- `virtual void set(std::string_view key, std::string value) = 0`
  Что делает: устанавливает значение. Параметры: `key`, `value`. Возвращает: ничего.
  Реализация: под `std::scoped_lock` записывает `values_[std::string(key)] = std::move(value)` — все значения хранятся строками, а `getInt`/`getBool` разбирают их при чтении.

#### Файлы `engine/core/src/console_logger.cpp`, `engine/core/src/memory_config_service.cpp`
Реализации интерфейсов выше плюс фабрики (объявлены в `runtime_services.hpp`):
- `std::unique_ptr<ILogger> createConsoleLogger()` — журнал в stdout/stderr.
  Реализация: `return std::make_unique<ConsoleLogger>(minimumLevel);` — создаёт скрытый в анонимном namespace `ConsoleLogger` с порогом уровня, отдаёт его через указатель на интерфейс `ILogger`.
- `std::unique_ptr<IConfigService> createInMemoryConfigService()` — конфиг на `map` ключ→значение.
  Реализация: `return std::make_unique<InMemoryConfigService>();` — создаёт скрытую реализацию с пустой `unordered_map` и мьютексом, отдаёт как `IConfigService`.

### feature/object-model

Единственный владелец иерархии сцены и трансформов.

#### Файл `engine/object/include/sky/object/object_model.hpp`
Три контракта. `using ObjectHandle = core::Handle<ObjectTag>`.

**`class IObjectFactory`** — создание и удаление объектов.
- `virtual ObjectHandle createObject(const std::string& name) = 0`
  Что делает: создаёт объект. Параметры: `name`. Возвращает: хэндл нового объекта.
  Реализация: контракт; тело в `ObjectWorldImpl` — берёт `ObjectHandle{nextId_++}` (счётчик стартует с 1, 0 зарезервирован под invalid), кладёт в `objects_` запись `ObjectRecord{name, {}}` (имя + пустой `TransformNode`), возвращает хэндл.
- `virtual void destroyObject(ObjectHandle object) = 0`
  Что делает: удаляет объект и его поддерево. Параметры: `object`. Возвращает: ничего.
  Реализация: находит запись; копирует список детей (разрушение их мутирует) и рекурсивно вызывает `destroyObject` для каждого; затем `detachFromParent(object)` (убирает себя из детей родителя) и `objects_.erase(object.value)`.

**`class IObjectHierarchyAccess`** — иерархия и трансформы.
- `virtual void setParent(ObjectHandle child, ObjectHandle parent) = 0` — перевешивает `child` под `parent` (invalid = корень).
  Реализация: находит запись ребёнка; если её нет или новый родитель создал бы цикл (`wouldCreateCycle` идёт вверх от `parent` и ищет `child`) — выходит; иначе `detachFromParent(child)`, проставляет `node.parent = parent` и добавляет `child` в `node.children` записи родителя (если родитель валиден).
- `virtual ObjectHandle parentOf(ObjectHandle object) const = 0` — Возвращает: родителя (или invalid).
  Реализация: `find(object)`; при наличии возвращает `record->node.parent`, иначе `ObjectHandle::invalid()`.
- `virtual std::vector<ObjectHandle> childrenOf(ObjectHandle object) const = 0` — Возвращает: прямых детей.
  Реализация: `find(object)`; возвращает копию вектора `record->node.children` либо пустой вектор.
- `virtual void setLocalTransform(ObjectHandle object, const core::Transform& transform) = 0` — задаёт локальный трансформ.
  Реализация: `find(object)`; если запись есть — присваивает `record->node.local = transform`, иначе тихо ничего не делает.
- `virtual core::Transform localTransform(ObjectHandle object) const = 0` — Возвращает: локальный трансформ.
  Реализация: `find(object)`; возвращает `record->node.local` либо единичный `core::Transform{}`.
- `virtual core::Transform worldTransform(ObjectHandle object) const = 0` — Возвращает: мировой трансформ (композиция локальных вверх по цепочке).
  Реализация: `find(object)`; для корня (родитель невалиден) возвращает сам локальный трансформ, иначе рекурсивно `core::compose(worldTransform(parent), local)` — поднимается по цепочке до корня.

**`class IObjectQueryService`** — запросы для чтения.
- `virtual bool exists(ObjectHandle object) const = 0` — Возвращает: жив ли объект.
  Реализация: `return objects_.contains(object.value);` — проверка наличия ключа в хранилище.
- `virtual std::string nameOf(ObjectHandle object) const = 0` — Возвращает: имя.
  Реализация: `find(object)`; возвращает `record->name` либо пустую строку.
- `virtual std::vector<ObjectHandle> findByName(const std::string& name) const = 0` — Возвращает: объекты с таким именем.
  Реализация: линейный проход по `objects_`; в результат добавляет `ObjectHandle{id}` для каждой записи, чьё `record.name == name`.

**Свободная функция:**
- `inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)`
  Что делает: задаёт мировой трансформ, пересчитывая локальный через `invCompose`. Параметры: `access`, `object`, `world`.
  Реализация: свободная inline-функция в заголовке; спрашивает `access.parentOf(object)` — для корня пишет `world` как локальный напрямую, иначе `access.setLocalTransform(object, core::invCompose(access.worldTransform(parent), world))`.

#### Файлы `engine/object/include/sky/object/object_world.hpp`, `engine/object/src/object_world.cpp`
- `class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService` — единый владелец, добавляет:
  - `virtual void renameObject(ObjectHandle object, const std::string& name) = 0` — переименовывает объект.
    Реализация: `find(object)`; если запись есть — присваивает `record->name = name`, иначе ничего не делает.
- `std::unique_ptr<ObjectWorld> createObjectWorld()` — Возвращает: реализацию мира объектов.
  Внутри `.cpp`: хранилище `id → {локальный трансформ, родитель, дети, имя}`; `worldTransform` = `compose` вверх; `destroyObject` рекурсивно удаляет поддерево.
  Реализация: `return std::make_unique<ObjectWorldImpl>();` — отдаёт скрытую реализацию, где состояние — `std::uint64_t nextId_ = 1` и `std::unordered_map<std::uint64_t, ObjectRecord> objects_` (`ObjectRecord = {name, TransformNode}`), а приватные хелперы `find`, `detachFromParent`, `wouldCreateCycle` обслуживают публичные методы.

### feature/component-model

Компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.

#### Файл `engine/component/include/sky/component/component_model.hpp`
`using FieldValue = std::variant<float, std::int64_t, bool, std::string,
core::Vec3>`. `ComponentDescriptor{typeId, displayName, fields, category}`.

**`class IComponentRegistry`** — реестр типов.
- `virtual void registerComponentType(const ComponentDescriptor& descriptor) = 0` — регистрирует тип.
  Реализация: контракт; тело в `ComponentWorldImpl` — `types_[descriptor.typeId] = descriptor;`, где `types_` — `unordered_map<std::string, ComponentDescriptor>` (повторная регистрация того же `typeId` перезаписывает описание).
- `virtual std::vector<ComponentDescriptor> availableTypes() const = 0` — Возвращает: типы для меню Add Component.
  Реализация: заводит вектор с `reserve(types_.size())` и складывает в него все значения `descriptor` из `types_`.

**`class IComponentAttachmentService`** — навешивание.
- `virtual ComponentHandle attach(object::ObjectHandle object, const std::string& typeId) = 0` — Возвращает: хэндл компонента.
  Реализация: если тип не зарегистрирован в `types_` или объект невалиден — возвращает `ComponentHandle::invalid()`; иначе берёт `ComponentHandle{nextId_++}`, кладёт в `instances_` запись `ComponentInstance{typeId, object}` (пустая карта полей) и добавляет хэндл в список `byObject_[object.value]`.
- `virtual void detach(ComponentHandle component) = 0` — снимает компонент.
  Реализация: находит экземпляр в `instances_`; убирает его хэндл из `byObject_[owner]` через `std::erase` и стирает запись из `instances_`.

**`class IComponentQueryService`** — запросы.
- `virtual std::vector<ComponentHandle> componentsOf(object::ObjectHandle object) const = 0` — Возвращает: компоненты объекта.
  Реализация: ищет объект в `byObject_`; возвращает копию его списка хэндлов либо пустой вектор.
- `virtual const ComponentDescriptor& descriptorOf(ComponentHandle component) const = 0` — Возвращает: описание типа.
  Реализация: по хэндлу берёт `typeId` экземпляра из `instances_`, по нему ищет описание в `types_`; при отсутствии экземпляра или типа возвращает ссылку на статический пустой `kEmpty` (ссылка остаётся валидной).
- `virtual object::ObjectHandle ownerOf(ComponentHandle component) const = 0` — Возвращает: объект-владелец.
  Реализация: находит экземпляр в `instances_`, возвращает `it->second.owner` либо `object::ObjectHandle::invalid()`.

#### Файлы `engine/component/include/sky/component/component_world.hpp`, `engine/component/src/component_world.cpp`
`class ComponentWorld` наследует интерфейсы выше и добавляет доступ к данным:
- `virtual void setField(ComponentHandle component, const std::string& name, FieldValue value) = 0` — записывает поле по имени.
  Реализация: находит экземпляр в `instances_`; при наличии `it->second.fields[name] = std::move(value)` — карта `std::map<std::string, FieldValue>` хранит значение любого из пяти типов `variant` и заводит поле при первой записи.
- `virtual std::optional<FieldValue> field(ComponentHandle component, const std::string& name) const = 0` — Возвращает: значение или `nullopt`.
  Реализация: находит экземпляр, затем поле по имени в его `fields`; возвращает копию значения либо `std::nullopt`, если нет экземпляра или поля.
- `virtual std::map<std::string, FieldValue> fields(ComponentHandle component) const = 0` — Возвращает: всю карту полей.
  Реализация: находит экземпляр; возвращает копию `it->second.fields` либо пустую карту.
- `virtual void detachAllFrom(object::ObjectHandle object) = 0` — снимает все компоненты объекта.
  Реализация: ищет объект в `byObject_`; стирает из `instances_` каждый его компонент, затем удаляет запись объекта из `byObject_`.
- `std::unique_ptr<ComponentWorld> createComponentWorld()` — фабрика.
  Реализация: `return std::make_unique<ComponentWorldImpl>();` — реализация держит `nextId_`, `types_`, `instances_` и индекс `byObject_` (объект → его компоненты) для быстрого `componentsOf`/`detachAllFrom`.

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
  Реализация: контракт; тело в `SceneWorldImpl` — `find(scene)`, добавляет объект в `record->rootObjects`; если сцена активна, синхронно обновляет `context_.rootObjects`.
- `virtual bool saveSceneAs(SceneHandle scene, const std::filesystem::path& path, …) = 0` — сохраняет сцену (полный SKYB — на этапе 3). Возвращает: успех.
  Реализация: третий параметр — `object::ObjectHandle excludeRoot` (пропускаемое поддерево). Проверяет сцену и непустой путь, записывает `path` в дескриптор и делегирует приватному `writeScene(record, excludeRoot)` (обход и запись в SKYB — см. фичу skyb-serialization ниже).
- `virtual std::vector<object::ObjectHandle> rootObjectsOf(SceneHandle scene) const = 0` — Возвращает: корневые объекты.
  Реализация: `find(scene)`; возвращает копию `record->rootObjects` либо пустой вектор.
- `std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps)` — фабрика.
  Реализация: `return std::make_unique<SceneWorldImpl>(deps);` — конструктор сохраняет `deps_` и, если задан `deps.migrations`, регистрирует миграцию схемы `sky.scene` с 1.0 на 1.1 (`migrateSceneV10ToV11`). Внутри держит `scenes_` (id → `SceneRecord`), активную сцену и накопитель шага физики.

### feature/scene-authoring

#### Файлы `engine/scene/include/sky/scene/scene_authoring.hpp`, `engine/scene/src/scene_authoring.cpp`
`enum class PrimitiveKind {Cube, Plane, Sphere}`.
- `object::ObjectHandle createPrimitive(const AuthoringServices& services, PrimitiveKind kind, const std::string& name)`
  Что делает: создаёт объект-примитив с компонентом Mesh Renderer.
  Параметры: `services` — ссылки на миры объектов/компонентов, `kind` — вид, `name` — имя. Возвращает: хэндл объекта.
  Реализация: создаёт объект `services.factory.createObject(name)`, навешивает компонент `"sky.mesh"`, пишет в него поля `material = "Default"` и `mesh = primitiveMeshName(kind)` (хелпер отображает `Cube/Plane/Sphere` в строки `"cube"/"plane"/"sphere"`); возвращает объект. Физику не создаёт.

### feature/editor-context

#### Файлы `editor/shell/src/editor_context.hpp`, `editor/shell/src/editor_context.cpp`
`class EditorContext` — собирает движок в один объект и строит демо-сцену.
- `EditorContext()` — конструктор: создаёт подсистемы, вызывает `buildDemoScene()`.
  Реализация: последовательно создаёт все подсистемы через их фабрики (файловая система, VFS, конфиг с `engine.renderer=opengl`, реестр рендереров с OpenGL-бэкендом, хранилище, миры объектов/компонентов/ECS/физики и их синхронизаторы, службу миграций, `SceneWorld` через `SceneWorldDeps`, контроллер play-режима), монтирует VFS (project/assets/packages) и восстанавливает состояние пакетов из `sky.lock`, регистрирует встроенные типы компонентов (`sky.mesh`, `sky.camera`, `sky.collider.box`, `sky.rigidbody`, `sky.script`, `sky.light`), поднимает пайплайн ассетов и стартовые материалы, затем `initScripting()` и `buildDemoScene()`.
- `std::vector<object::ObjectHandle> rootObjects() const` — Возвращает: корневые объекты (нужно E3 для дерева, E2 для обхода).
  Реализация: инлайн-геттер `return roots_;` — отдаёт копию приватного вектора `roots_`, который ведут `createEmpty`/`createPrimitive`/`destroyObject` и загрузка сцены.
- `object::ObjectHandle createEmpty(const std::string& name)` — Возвращает: хэндл пустого объекта.
  Реализация: `objects->createObject(name)`, затем `scenes->addRootObject(activeScene, object)` и `roots_.push_back(object)`; компонентов и физики не добавляет.
- `object::ObjectHandle createPrimitive(scene::PrimitiveKind kind, const std::string& name)` — Возвращает: хэндл примитива.
  Реализация: собирает `scene::AuthoringServices` из своих миров объектов/компонентов, зовёт свободную `scene::createPrimitive(services, kind, name)`, регистрирует результат как корень (`addRootObject` + `roots_.push_back`).
- приватный `void buildDemoScene()` — наполняет сцену примитивами, светом, камерой.
  Реализация: создаёт сцену `"SampleScene"` и террейн (`initTerrain`), затем три ящика `createCrate` (третьему меняет материал на `Gold`), направленный и точечный свет с полями компонента `sky.light`, импортирует OBJ-пирамиду через пайплайн ассетов и вешает на неё скрипт `Rotator`, добавляет `Main Camera` с компонентом `sky.camera` и скриптом `WasdMover`; в конце сохраняет префабы `crate.skyprefab` и `player.skyprefab` под `assets://Prefabs`.

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
  Реализация: чистый интерфейс (все три метода `= 0`, тел нет; фактические сигнатуры — `undo(EditorContext&)`/`redo(EditorContext&)`). Конвенция: действие уже применено к моменту `push`, `undo` его откатывает, `redo` применяет заново. Конкретные команды — классы в анонимном namespace `.cpp`, каждый хранит хэндлы/значения «до» и «после» и правит `EditorContext` через его публичные методы.
- `class UndoStack`:
  - `explicit UndoStack(EditorContext&)` — привязка к контексту.
    Реализация: конструктор в заголовке инициализирует `context_` ссылкой и `capacity_` (по умолчанию 100); два вектора `unique_ptr`-команд `undoList_`/`redoList_` стартуют пустыми.
  - `void push(std::unique_ptr<IEditorCommand> command)` — добавляет и применяет команду.
    Реализация: команда считается уже применённой — метод чистит `redoList_`, кладёт команду в конец `undoList_`, а при превышении `capacity_` сбрасывает самую старую (`erase(begin())`); затем `notify()` дёргает колбэк `onChanged_`.
  - `bool undo()` — Возвращает: было ли что отменять.
    Реализация: при пустом `undoList_` возвращает `false`; иначе снимает команду с конца, вызывает `command->undo(context_)`, переносит её в `redoList_`, `notify()`, возвращает `true`.
  - `bool redo()` — Возвращает: было ли что повторять.
    Реализация: зеркально `undo`: при пустом `redoList_` — `false`; иначе снимает команду с его конца, `command->redo(context_)`, переносит обратно в `undoList_`, `notify()`, `true`.
  - `bool canUndo() const` / `bool canRedo() const` — Возвращает: доступность.
    Реализация: инлайн-однострочники `return !undoList_.empty();` и `return !redoList_.empty();`.
- Фабрики команд (каждая возвращает `std::unique_ptr<IEditorCommand>`), по одной на операцию:
  `makeTransformCommand(object, before, after)`, `makeFieldCommand(component, name, before, after)`,
  `makeCreateCommand`, `makeDeleteCommand(context, object)`, `makeDuplicateCommand`,
  `makeReparentCommand`, `makeRenameCommand`, `makeAddComponentCommand`,
  `makeRemoveComponentCommand`, `makeMaterialCreateCommand`, `makeMaterialEditCommand`.
  Реализация: каждая — `std::make_unique<КонкретнаяCommand>(…)` над скрытым в анонимном namespace классом. `makeTransformCommand`: хранит `before`/`after` трансформы, `apply` пишет `objects->setLocalTransform`. `makeFieldCommand`: хранит имя поля и значения «до/после», зовёт `components->setField`. `makeCreateCommand`: по флагу `isCrate` на redo пересоздаёт через `createCrate`/`createEmpty`, undo — `destroyObject`. `makeDeleteCommand`: снимает `snapshotObject(object)` и родителя; undo — `restoreObject`, redo — `destroyObject`. `makeDuplicateCommand`: undo удаляет копию, redo зовёт `duplicateObject(source)`. `makeReparentCommand`: хранит старого/нового родителя и старый локальный трансформ; undo делает `reparent` назад и восстанавливает точный `oldLocal`. `makeRenameCommand`: `renameObject` на before/after. `makeAddComponentCommand`/`makeRemoveComponentCommand`: attach/detach компонента (remove предварительно сохраняет карту полей и восстанавливает её при undo). `makeMaterialCreateCommand`/`makeMaterialEditCommand`: создание/обновление материала в `materials` (`createMaterial`/`updateMaterial`, edit хранит `MaterialDesc` до и после).

### feature/object-snapshot

#### Дополнение файла `editor/shell/src/editor_context.cpp`
Структуры снимка в `.hpp`: `ObjectSnapshot{name, local, hasPhysicsBody,
components, children}`, `ComponentSnapshot{typeId, fields}`.
- `ObjectSnapshot snapshotObject(object::ObjectHandle object) const`
  Что делает: полный снимок поддерева (имя, трансформ, компоненты со всеми полями, дети). Возвращает: снимок.
  Реализация: заполняет `ObjectSnapshot`: `name = objects->nameOf`, `local = objects->localTransform`, `hasPhysicsBody = hasPhysicsBody(object)` (есть ли тело в `bodies_`); для каждого компонента из `components->componentsOf` кладёт `{typeId, components->fields(component)}`; рекурсивно снимает каждого ребёнка в `children`.
- `object::ObjectHandle restoreObject(const ObjectSnapshot& snapshot, object::ObjectHandle parent)`
  Что делает: восстанавливает поддерево из снимка под родителем (invalid = корень). Возвращает: хэндл корня.
  Реализация: `objects->createObject(snapshot.name)`; при валидном `parent` — `setParent`, иначе регистрирует как корень (`addRootObject` + `roots_.push_back`); ставит локальный трансформ; навешивает компоненты снимка и записывает все их поля; если `hasPhysicsBody` — `attachCrateBody`; рекурсивно восстанавливает детей. Обратна `snapshotObject`.
- `void newScene()` — очищает сцену.
  Реализация: `resetScene()` (сносит физику, террейн-фикстуру и все корни), затем `scenes->createScene({"Untitled", {}})` в `activeScene` и `initTerrain()` — новый пустой документ с террейн-фикстурой.
- `bool saveScene(const std::filesystem::path& path)` — сохраняет в SKYB. Возвращает: успех.
  Реализация: `return scenes->saveSceneAs(activeScene, path, terrainObject);` — делегирует записи SKYB, исключая корень террейна (его heightfield вне схемы сцены).
- `bool openScene(const std::filesystem::path& path)` — открывает из файла. Возвращает: успех.
  Реализация: `resetScene()`, затем `scenes->loadScene(path)`; при невалидном хэндле откатывается на новую пустую сцену с террейном и возвращает `false`; при успехе берёт `roots_ = scenes->rootObjectsOf(activeScene)`, добавляет террейн-фикстуру (`initTerrain`) и `reattachPhysics()`, возвращает `true`.
- `object::ObjectHandle duplicateObject(object::ObjectHandle object)` — глубокая копия с детьми. Возвращает: копию.
  Реализация: проверяет `objects->exists`; берёт родителя оригинала и делает `cloneSubtree(object, parent)` (рекурсивно копирует объект, его компоненты с полями, при наличии тела — `attachCrateBody`, и детей); если родитель невалиден — регистрирует копию как корень. Возвращает копию.
- `void reparent(object::ObjectHandle child, object::ObjectHandle newParent)` — меняет родителя, сохраняя мировое положение.
  Реализация: запоминает мировой трансформ ребёнка, зовёт `objects->setParent`; при валидном новом родителе убирает ребёнка из `roots_` и пересчитывает локальный трансформ из мирового относительно `worldTransform(newParent)` (обратный поворот, деление на масштаб родителя); при переносе в корень возвращает ребёнка в `roots_`/`addRootObject` и пишет мировой трансформ как локальный.
- `void destroyObject(object::ObjectHandle object)` — удаляет объект.
  Реализация: если у объекта есть тело в `bodies_` — отвязывает и уничтожает его в физике и стирает из `bodies_`; затем `components->detachAllFrom(object)`, `objects->destroyObject(object)` (сносит и поддерево) и `std::erase(roots_, object)`.

### feature/skyb-serialization

#### Дополнение файла `engine/scene/src/scene_world.cpp`
- `saveSceneAs` реализуется как настоящий SKYB: магия "SKYB", схема `sky.scene`,
  обход корней → имя, трансформ, компоненты с полями (через `ByteWriter`); чтение
  обратно с прямой миграцией версий.
  Реализация: приватный `writeScene` через `flatten` разворачивает корни (кроме `excludeRoot`) в pre-order массив с картой `indexOf`; `ByteWriter` пишет имя сцены, число объектов, а на каждый объект — индекс родителя (`kNoParent` для корня), имя, 10 float трансформа (`writeTransform`), затем число компонентов и на каждый — `typeId`, число полей и сами поля (`writeField` кладёт тег типа из `FieldTag` и значение). Итог отдаётся `deps_.storage.write` под схемой `sky.scene` версии 1.1. Чтение (`loadScene`) симметрично, прогоняя старые файлы через зарегистрированную миграцию `migrateSceneV10ToV11` (после каждого `typeId` вставляет нулевой счётчик полей).

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
  Реализация: обходит все поддеревья от `roots_` через явный стек; пропускает террейн-объект и объекты, у которых тело уже есть в `bodies_`; если среди компонентов объекта есть `sky.rigidbody` — зовёт `attachCrateBody(object)` (создаёт динамическое тело по мировому трансформу, вешает box-коллайдер, `physicsSync->bind` и регистрирует в `bodies_`) и переходит к следующему объекту.

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
  Реализация: проверяет `objects->exists`; `ByteWriter` пишет магию `kPrefabMagic` ("SKYP") и `kPrefabVersion`, затем свободный `writeSnapshot` рекурсивно сериализует `snapshotObject(object)` (имя, 10 float трансформа, флаг тела, компоненты с полями по тегу типа, дети); путь разворачивается `resolveAssetPath` (префикс `assets://` → корень ассетов), создаются каталоги и буфер пишется через `fileSystem->writeAll`.
- `object::ObjectHandle instantiatePrefab(const std::string& path)`
  Что делает: создаёт объект из файла префаба в корне сцены. Параметры: `path`. Возвращает: хэндл (invalid при ошибке).
  Реализация: читает файл (`readAll` по `resolveAssetPath`); `ByteReader` проверяет магию и версию (не выше текущей); свободный `readSnapshot` разбирает дерево в `ObjectSnapshot`; при успехе возвращает `restoreObject(snapshot, invalid)` (в корень сцены), иначе `ObjectHandle::invalid()`.
- `object::ObjectHandle spawnPrefabAt(const std::string& path, core::Vec3 position)`
  Что делает: то же, но ставит объект в мировую точку и пересаживает его физику (используется скриптами контура E4). Параметры: `path`, `position`. Возвращает: хэндл объекта.
  Реализация: зовёт `instantiatePrefab(path)`; при валидном объекте переписывает `position` в его локальный трансформ (в корне local = world) и обходит поддерево, пере-сажая каждое тело из `bodies_` на мировой трансформ с обнулением скорости (`setBodyTransform`/`setBodyVelocity`); если идёт play-режим — стартует скрипты поддерева `startScriptsFor(object)`.

**На выходе:** префаб сохраняется в `.skyprefab` и инстанцируется идентично.
**Критерий правильности этапа:** `savePrefab → instantiatePrefab` даёт объект с
теми же компонентами, полями, физикой и детьми.
