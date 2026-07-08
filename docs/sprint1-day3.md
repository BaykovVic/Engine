# Спринт 1. День 3

## Контур E1 (Ядро и данные)

feature/object-model

Цель фичи: единственный владелец иерархии сцены и трансформов.
Описание фичи (для чего): объекты, их дерево и трансформы; остальные модули держат только хендлы.
Пошаговое описание действий:
Сделай файл engine/object/include/sky/object/object_model.hpp
В файле engine/object/include/sky/object/object_model.hpp должны быть using ObjectHandle = core::Handle<ObjectTag>; class IObjectFactory (createObject(const std::string& name), destroyObject(ObjectHandle object)); class IObjectHierarchyAccess (setParent, parentOf, childrenOf, setLocalTransform, localTransform, worldTransform); class IObjectQueryService (exists, nameOf, findByName); свободная inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)
В методах должна быть реализована логика: createObject создаёт объект, destroyObject удаляет объект и его поддерево; setParent перевешивает child под parent (invalid = корень); worldTransform — композиция локальных вверх по цепочке; setWorldTransform задаёт мировой трансформ, пересчитывая локальный через invCompose.
Сделай файл engine/object/include/sky/object/object_world.hpp
В файле engine/object/include/sky/object/object_world.hpp должны быть class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService с методом renameObject(ObjectHandle object, const std::string& name) и std::unique_ptr<ObjectWorld> createObjectWorld()
В методах должна быть реализована логика: единый владелец мира объектов; renameObject переименовывает объект.
Сделай файл engine/object/src/object_world.cpp
В файле engine/object/src/object_world.cpp должна быть реализация ObjectWorld
В методах должна быть реализована логика: хранилище id → {локальный трансформ, родитель, дети, имя}; worldTransform = compose вверх; destroyObject рекурсивно удаляет поддерево.
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
В файле tests/vulkan_tests.cpp должна быть проверка на lavapipe
В тесте должна быть реализована логика: ready() истинно, кадр рендерится, readbackFrame() непустой, центральный пиксель отличается от углового.
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
В файле editor/avalonia/Engine/EngineInterop.cs должны быть static class EngineInterop с [DllImport] static extern IntPtr sky_editor_create(), [DllImport] static extern void sky_editor_destroy(IntPtr ctx), static IntPtr Resolve(...), static string[] Candidates()
В методах должна быть реализована логика: sky_editor_create возвращает указатель на сессию движка; sky_editor_destroy уничтожает сессию; Resolve/Candidates находят libsky_editor_bridge.so по SKY_BRIDGE_PATH и в дереве сборки.
Сделай файл editor/avalonia/Engine/EditorSession.cs
В файле editor/avalonia/Engine/EditorSession.cs должен быть class EditorSession : IDisposable со свойством IntPtr Native и методом Dispose()
В методах должна быть реализована логика: конструктор вызывает sky_editor_create и проверяет не-null; Dispose() вызывает sky_editor_destroy; свойство Native отдаёт нативный указатель сессии.
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
В файле editor/native_bridge/include/sky/editor/bridge/editor_bridge.h должны быть SkyEditorContext* sky_editor_create(void), void sky_editor_destroy(SkyEditorContext* ctx), int32_t sky_editor_root_count(SkyEditorContext* ctx), SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index), int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)
В функциях должна быть реализована логика (объявления): create возвращает указатель на сессию (собирает движок и демо-сцену); destroy уничтожает; root_count — число корней; root_at — id корня; object_name пишет имя в буфер и возвращает длину.
Сделай файл editor/native_bridge/src/editor_bridge.cpp
В файле editor/native_bridge/src/editor_bridge.cpp должны быть реализации sky_editor_create, sky_editor_destroy, sky_editor_root_count, sky_editor_root_at, sky_editor_object_name
В функциях должна быть реализована логика: сборка движка и демо-сцены в сессии; перечисление корней; чтение имени объекта в буфер.
На выходе должно получиться:
- editor/native_bridge/include/sky/editor/bridge/editor_bridge.h
- editor/native_bridge/src/editor_bridge.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: красный CI блокирует слияние; сломанный тест краснеет; редактор E3 вызывает sky_editor_create.
