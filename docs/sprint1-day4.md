# Спринт 1. День 4

## feature/component-model

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/object-model` (`ObjectHandle`); по эталонным связям модуля — `sky_serialization`, `sky_scripting`

**Цель фичи:** компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.

**Описание фичи:** поля, описываемые данными (variant-map), позже бесплатно дают сериализацию, отмену и параметры скриптов.

**Общий порядок реализации фичи:**
1. Объявить `FieldValue`, `ComponentDescriptor` и три контракта в `component_model.hpp`.
2. Объявить `ComponentWorld` и фабрику в `component_world.hpp`.
3. Реализовать `ComponentWorld` в `component_world.cpp`.

**Файлы фичи:**
1. `engine/component/include/sky/component/component_model.hpp`
2. `engine/component/include/sky/component/component_world.hpp`
3. `engine/component/src/component_world.cpp`

### Файл: `engine/component/include/sky/component/component_model.hpp`

**Назначение файла:** контракты компонентной модели и тип поля.

**Пошаговое описание действий:**
1. Объявить `FieldValue` и `ComponentDescriptor`.
2. Объявить `IComponentRegistry`, `IComponentAttachmentService`, `IComponentQueryService`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>`
- `ComponentDescriptor{typeId, displayName, fields, category}`
- `class IComponentRegistry`
- `class IComponentAttachmentService`
- `class IComponentQueryService`

*Функции / методы:*
- `IComponentRegistry`: `registerComponentType(const ComponentDescriptor&)`, `availableTypes()`
- `IComponentAttachmentService`: `attach(object::ObjectHandle, const std::string& typeId)`, `detach(ComponentHandle)`
- `IComponentQueryService`: `componentsOf(object::ObjectHandle)`, `descriptorOf(ComponentHandle)`, `ownerOf(ComponentHandle)`

*Логика функций / методов:*
- `registerComponentType` — регистрирует тип; `availableTypes` — типы для меню Add Component.
- `attach` — навешивает компонент, возвращает хэндл; `detach` — снимает компонент.
- `componentsOf` — компоненты объекта; `descriptorOf` — описание типа; `ownerOf` — объект-владелец.

**Результат по файлу:** контракты компонентной модели зафиксированы.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/component/include/sky/component/component_world.hpp`

**Назначение файла:** мир компонентов с доступом к полям.

**Пошаговое описание действий:**
1. Объявить `ComponentWorld` с доступом к полям.
2. Добавить фабрику `createComponentWorld`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class ComponentWorld` (наследует три контракта)

*Функции / методы:*
- `virtual void setField(ComponentHandle, const std::string& name, FieldValue value) = 0`
- `virtual std::optional<FieldValue> field(ComponentHandle, const std::string& name) const = 0`
- `virtual std::map<std::string, FieldValue> fields(ComponentHandle) const = 0`
- `virtual void detachAllFrom(object::ObjectHandle) = 0`
- `std::unique_ptr<ComponentWorld> createComponentWorld()`

*Логика функций / методов:*
- `setField` — записывает поле по имени; `field` — значение или `nullopt`; `fields` — вся карта полей; `detachAllFrom` — снимает все компоненты объекта.
- `createComponentWorld` — фабрика.

**Результат по файлу:** интерфейс мира компонентов с фабрикой.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/component/src/component_world.cpp`

**Назначение файла:** реализация мира компонентов.

**Пошаговое описание действий:**
1. Завести хранилище компонентов и их полей.
2. Реализовать доступ к полям и снятие компонентов.

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация `ComponentWorld`.

*Функции / методы:*
- `registerComponentType`, `availableTypes`, `attach`, `detach`, `componentsOf`, `descriptorOf`, `ownerOf`, `setField`, `field`, `fields`, `detachAllFrom`, `createComponentWorld`.

*Логика функций / методов:*
- хранение компонентов и их полей, привязанных к объекту-владельцу.
- `setField`/`field`/`fields` — запись/чтение поля по имени и выдача всей карты; `detachAllFrom` — снять все компоненты объекта.

**Результат по файлу:** рабочий мир компонентов.

**Критерий правильности по файлу:**
1. Поле каждого из 5 типов пишется и читается без потерь.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/component/include/sky/component/component_model.hpp`
2. `engine/component/include/sky/component/component_world.hpp`
3. `engine/component/src/component_world.cpp`

**Общий критерий правильности:**
1. Поле каждого из 5 типов записывается и читается без потерь.
