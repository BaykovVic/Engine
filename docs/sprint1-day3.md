# Спринт 1. День 3

## Контур E1 (Ядро и данные)

feature/object-model

Цель фичи: единственный владелец иерархии сцены и трансформов.
Описание фичи (для чего): объекты, их дерево и трансформы; остальные модули держат только хендлы.
Пошаговое описание действий:

Сделай файл engine/object/include/sky/object/object_model.hpp
Три контракта. using ObjectHandle = core::Handle<ObjectTag>. В файле должны быть функции/методы:
class IObjectFactory — создание и удаление объектов:
- `virtual ObjectHandle createObject(const std::string& name) = 0`
  Что делает: создаёт объект. Параметры: name. Возвращает: хэндл нового объекта.
- `virtual void destroyObject(ObjectHandle object) = 0`
  Что делает: удаляет объект и его поддерево. Параметры: object. Возвращает: ничего.
class IObjectHierarchyAccess — иерархия и трансформы:
- `virtual void setParent(ObjectHandle child, ObjectHandle parent) = 0` — перевешивает child под parent (invalid = корень).
- `virtual ObjectHandle parentOf(ObjectHandle object) const = 0` — Возвращает: родителя (или invalid).
- `virtual std::vector<ObjectHandle> childrenOf(ObjectHandle object) const = 0` — Возвращает: прямых детей.
- `virtual void setLocalTransform(ObjectHandle object, const core::Transform& transform) = 0` — задаёт локальный трансформ.
- `virtual core::Transform localTransform(ObjectHandle object) const = 0` — Возвращает: локальный трансформ.
- `virtual core::Transform worldTransform(ObjectHandle object) const = 0` — Возвращает: мировой трансформ (композиция локальных вверх по цепочке).
class IObjectQueryService — запросы для чтения:
- `virtual bool exists(ObjectHandle object) const = 0` — Возвращает: жив ли объект.
- `virtual std::string nameOf(ObjectHandle object) const = 0` — Возвращает: имя.
- `virtual std::vector<ObjectHandle> findByName(const std::string& name) const = 0` — Возвращает: объекты с таким именем.
Свободная функция:
- `inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)`
  Что делает: задаёт мировой трансформ, пересчитывая локальный через invCompose. Параметры: access, object, world.

Сделай файл engine/object/include/sky/object/object_world.hpp
class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService — единый владелец, добавляет функции/методы:
- `virtual void renameObject(ObjectHandle object, const std::string& name) = 0` — переименовывает объект.
- `std::unique_ptr<ObjectWorld> createObjectWorld()` — Возвращает: реализацию мира объектов.

Сделай файл engine/object/src/object_world.cpp
Реализация ObjectWorld (скрытый класс) и фабрика createObjectWorld(). В методах должна быть реализована логика:
- хранилище id → {локальный трансформ, родитель, дети, имя}.
- createObject — завести объект; destroyObject — рекурсивно удалить объект и всё его поддерево.
- setParent — сменить родителя (обновить списки детей); worldTransform(object) — свернуть локальные трансформы вверх по цепочке родителей через compose.
- setLocalTransform/localTransform, renameObject, exists/nameOf/findByName — доступ к состоянию объекта.

На выходе должно получиться:
- engine/object/include/sky/object/object_model.hpp
- engine/object/include/sky/object/object_world.hpp
- engine/object/src/object_world.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире = (0,0,-1).

## Контур E2 (Рендеринг)

feature/vulkan-tests

Цель фичи: проверить Vulkan-рендерер на программном драйвере lavapipe.
Описание фичи (для чего): автотест закадрового рендера без видеокарты (в CI).
Пошаговое описание действий:

Сделай файл tests/vulkan_tests.cpp
В файле должна быть проверка на lavapipe. В методах должна быть реализована логика: ready() истинно, кадр рендерится, readbackFrame() непустой, центральный пиксель отличается от углового.

На выходе должно получиться:
- tests/vulkan_tests.cpp
- библиотека sky_rendering_vulkan собрана; формируется triangle.png; тест vulkan_tests зелёный на lavapipe
КРИТЕРИЙ ПРАВИЛЬНОСТИ: на triangle.png центральный пиксель отличается от углового; readbackFrame() возвращает непустой массив.

## Контур E3 (Редактор .NET)

feature/engine-bridge

Цель фичи: первичная связь редактора с движком через C-интерфейс (P/Invoke).
Описание фичи (для чего): загрузка нативного моста и создание/уничтожение сессии движка из .NET — фундамент всех дальнейших вызовов.
Пошаговое описание действий:

Сделай файл editor/avalonia/Engine/EngineInterop.cs
static class EngineInterop — резолвер нативной библиотеки и P/Invoke-объявления. В файле должны быть функции/методы:
- `[DllImport] static extern IntPtr sky_editor_create()` — Возвращает: указатель на сессию движка.
- `[DllImport] static extern void sky_editor_destroy(IntPtr ctx)` — уничтожает сессию.
- `static IntPtr Resolve(...)`, `static string[] Candidates()` — находят libsky_editor_bridge.so по SKY_BRIDGE_PATH и в дереве сборки.

Сделай файл editor/avalonia/Engine/EditorSession.cs
class EditorSession : IDisposable — обёртка над сессией. В файле должны быть функции/методы:
- конструктор вызывает sky_editor_create и проверяет не-null.
- `Dispose()` → destroy.
- свойство `IntPtr Native`.

На выходе должно получиться:
- editor/avalonia/Engine/EngineInterop.cs
- editor/avalonia/Engine/EditorSession.cs
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build — 0 ошибок; сессия движка создаётся из .NET (не-null).

## Контур E5 (Пайплайн и QA)

feature/c-abi-seed

Цель фичи: первичный плоский C-интерфейс движка sky_editor_* (совместно с E1).
Описание фичи (для чего): плоский набор C-функций, через который .NET-редактор общается с C++-движком; на этом этапе минимум — сессия и перечисление корней.
Пошаговое описание действий:

Сделай файл editor/native_bridge/include/sky/editor/bridge/editor_bridge.h
В файле должны быть функции/методы:
- `SkyEditorContext* sky_editor_create(void)` — Возвращает: указатель на сессию (собирает движок и демо-сцену).
- `void sky_editor_destroy(SkyEditorContext* ctx)` — уничтожает сессию.
- `int32_t sky_editor_root_count(SkyEditorContext* ctx)` — Возвращает: число корневых объектов.
- `SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index)` — Возвращает: id корневого объекта.
- `int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)` — пишет имя в буфер. Возвращает: длину.

Сделай файл editor/native_bridge/src/editor_bridge.cpp
В методах должна быть реализована логика: create собирает движок и демо-сцену в сессии; destroy уничтожает; root_count/root_at перечисляют корни; object_name пишет имя в буфер и возвращает длину.

На выходе должно получиться:
- editor/native_bridge/include/sky/editor/bridge/editor_bridge.h
- editor/native_bridge/src/editor_bridge.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: красный CI блокирует слияние; сломанный тест краснеет; редактор E3 вызывает sky_editor_create.
