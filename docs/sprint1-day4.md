# Спринт 1. День 4

## Контур E1 (Ядро и данные)

feature/component-model

Цель фичи: компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.
Описание фичи (для чего): поля, описываемые данными (variant-map), позже бесплатно дают сериализацию, отмену и параметры скриптов.
Пошаговое описание действий:

Сделай файл engine/component/include/sky/component/component_model.hpp
using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>. ComponentDescriptor{typeId, displayName, fields, category}. В файле должны быть функции/методы:
class IComponentRegistry — реестр типов:
- `virtual void registerComponentType(const ComponentDescriptor& descriptor) = 0` — регистрирует тип.
- `virtual std::vector<ComponentDescriptor> availableTypes() const = 0` — Возвращает: типы для меню Add Component.
class IComponentAttachmentService — навешивание:
- `virtual ComponentHandle attach(object::ObjectHandle object, const std::string& typeId) = 0` — Возвращает: хэндл компонента.
- `virtual void detach(ComponentHandle component) = 0` — снимает компонент.
class IComponentQueryService — запросы:
- `virtual std::vector<ComponentHandle> componentsOf(object::ObjectHandle object) const = 0` — Возвращает: компоненты объекта.
- `virtual const ComponentDescriptor& descriptorOf(ComponentHandle component) const = 0` — Возвращает: описание типа.
- `virtual object::ObjectHandle ownerOf(ComponentHandle component) const = 0` — Возвращает: объект-владелец.

Сделай файл engine/component/include/sky/component/component_world.hpp
class ComponentWorld наследует интерфейсы выше и добавляет доступ к данным. В файле должны быть функции/методы:
- `virtual void setField(ComponentHandle component, const std::string& name, FieldValue value) = 0` — записывает поле по имени.
- `virtual std::optional<FieldValue> field(ComponentHandle component, const std::string& name) const = 0` — Возвращает: значение или nullopt.
- `virtual std::map<std::string, FieldValue> fields(ComponentHandle component) const = 0` — Возвращает: всю карту полей.
- `virtual void detachAllFrom(object::ObjectHandle object) = 0` — снимает все компоненты объекта.
- `std::unique_ptr<ComponentWorld> createComponentWorld()` — фабрика.

Сделай файл engine/component/src/component_world.cpp
В методах должна быть реализована логика: хранение компонентов и их полей, привязанных к объекту-владельцу.

На выходе должно получиться:
- engine/component/include/sky/component/component_model.hpp
- engine/component/include/sky/component/component_world.hpp
- engine/component/src/component_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: поле каждого из 5 типов записывается и читается без потерь.
