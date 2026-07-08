# Спринт 1. День 4

## Контур E1 (Ядро и данные)

feature/component-model

Цель фичи: компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.
Описание фичи (для чего): поля, описываемые данными (variant-map), позже бесплатно дают сериализацию, отмену и параметры скриптов.
Пошаговое описание действий:
Сделай файл engine/component/include/sky/component/component_model.hpp
В файле engine/component/include/sky/component/component_model.hpp должны быть using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>; ComponentDescriptor{typeId, displayName, fields, category}; class IComponentRegistry (registerComponentType, availableTypes); class IComponentAttachmentService (attach, detach); class IComponentQueryService (componentsOf, descriptorOf, ownerOf)
В методах должна быть реализована логика: registerComponentType регистрирует тип, availableTypes — типы для меню Add Component; attach навешивает компонент (возвращает хэндл), detach снимает; componentsOf — компоненты объекта, descriptorOf — описание типа, ownerOf — объект-владелец.
Сделай файл engine/component/include/sky/component/component_world.hpp
В файле engine/component/include/sky/component/component_world.hpp должны быть class ComponentWorld с методами setField, field, fields, detachAllFrom и std::unique_ptr<ComponentWorld> createComponentWorld()
В методах должна быть реализована логика: setField записывает поле по имени; field возвращает значение или nullopt; fields возвращает всю карту полей; detachAllFrom снимает все компоненты объекта.
Сделай файл engine/component/src/component_world.cpp
В файле engine/component/src/component_world.cpp должна быть реализация ComponentWorld
В методах должна быть реализована логика: хранение компонентов и их полей, привязанных к объекту-владельцу.
На выходе должно получиться:
- engine/component/include/sky/component/component_model.hpp
- engine/component/include/sky/component/component_world.hpp
- engine/component/src/component_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: поле каждого из 5 типов записывается и читается без потерь.
