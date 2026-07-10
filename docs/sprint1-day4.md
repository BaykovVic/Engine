# Спринт 1. День 4

## Общие требования ко всем фичам

1. **Один PR — одна фича.** Не смешивать фичи и не менять файлы чужих контуров (например, `engine/core/include/sky/core/math.hpp` принадлежит E1).
2. **Никаких артефактов сборки в git**: `.exe`, `.o`, `.obj`, `.spv`, каталоги `build/` — запрещены. Временные файлы (`tests/tmp/`) в PR не включать.
3. **Namespace модуля обязателен** (`sky::core`, `sky::rendering`, `sky::object`, `sky::physics`, `sky::ecs`, ...). Код в глобальном namespace не принимается.
4. **Интерфейсы**: секция `public:`, виртуальный деструктор `virtual ~IИмя() = default;`, чисто виртуальные методы (`= 0`).
5. **Сигнатуры из задания копируются символ в символ** — включая `const`, `[[nodiscard]]`, типы возврата и параметры по умолчанию.
6. **Include-стиль**: `#include "sky/<модуль>/<файл>.hpp"` при `-Iengine/<модуль>/include`; пути от корня репозитория запрещены. `<bits/stdc++.h>` запрещён, `#pragma once` обязателен в каждом заголовке.
7. **Хэндлы** — только `core::Handle<Tag>`; в контейнерах ключ — `handle.value`. Собственные `std::hash<Handle>` и операторы в чужие заголовки не добавлять.
8. **Критерий приёмки каждой фичи — её приёмочный тест** (указан в конце блока фичи). PR без зелёного теста не рассматривается.

---

## feature/component-model

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/object-model` (`ObjectHandle`); по связям модуля в сборке — `sky_serialization`, `sky_scripting` (это связи уровня CMake: сами файлы дня 4 включают только `sky_core` и `sky_object`)

**Цель фичи:** компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.

**Описание фичи:** поля, описываемые данными (variant-map), позже бесплатно дают сериализацию, отмену и параметры скриптов.

**Обязательные требования:**

- Публичный контракт модуля копируется символ в символ. `component_model.hpp` (namespace `sky::component`):

```cpp
struct ComponentTag {};
using ComponentHandle = core::Handle<ComponentTag>;

struct InspectableField {
    std::string name;
    std::string typeName;
};

struct ComponentDescriptor {
    std::string typeId;
    std::string displayName;
    bool isScriptComponent = false;
    std::string managedTypeName;
    std::vector<InspectableField> fields;
    std::string category;
};

// class IComponentRegistry
virtual void registerComponentType(const ComponentDescriptor& descriptor) = 0;
[[nodiscard]] virtual std::vector<ComponentDescriptor> availableTypes() const = 0;
// class IComponentAttachmentService
virtual ComponentHandle attach(object::ObjectHandle object, const std::string& typeId) = 0;
virtual void detach(ComponentHandle component) = 0;
// class IComponentQueryService
[[nodiscard]] virtual std::vector<ComponentHandle> componentsOf(
    object::ObjectHandle object) const = 0;
[[nodiscard]] virtual const ComponentDescriptor& descriptorOf(
    ComponentHandle component) const = 0;
[[nodiscard]] virtual object::ObjectHandle ownerOf(ComponentHandle component) const = 0;
```

- `component_world.hpp` — `FieldValue` объявляется именно здесь (не в `component_model.hpp`), состав варианта ровно из 5 типов:

```cpp
using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>;

class ComponentWorld : public IComponentRegistry,
                       public IComponentAttachmentService,
                       public IComponentQueryService {
    virtual void detachAllFrom(object::ObjectHandle object) = 0;
    virtual void setField(ComponentHandle component, const std::string& name,
                          FieldValue value) = 0;
    [[nodiscard]] virtual std::optional<FieldValue> field(
        ComponentHandle component, const std::string& name) const = 0;
    [[nodiscard]] virtual std::map<std::string, FieldValue> fields(
        ComponentHandle component) const = 0;
};

std::unique_ptr<ComponentWorld> createComponentWorld();
```

- У `ComponentWorld` — `~ComponentWorld() override = default;` в секции `public:`; перечисленные методы тоже публичные.
- `ComponentDescriptor` содержит 6 полей, включая `isScriptComponent` и `managedTypeName` (скрипт-компоненты); дескрипторы заполняются по именам полей, а не по порядку членов.
- `attach` возвращает `ComponentHandle`; `field` возвращает `std::nullopt` для незаписанного поля; ключ в контейнерах — `handle.value`.

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

- **Приёмочный тест:** `tests/day4/component_model_tests.cpp` — автономный (встроенный `CHECK`), собирается из корня репозитория одной командой: `g++ -std=c++20 -Iengine/core/include -Iengine/object/include -Iengine/component/include tests/day4/component_model_tests.cpp engine/object/src/object_world.cpp engine/component/src/component_world.cpp -o component_model_tests && ./component_model_tests`. Ожидаемый вывод: `component_model_tests: 32 checks, 0 failures`, код возврата 0. Собирается без `serialization`/`scripting` — тест-дубли не нужны.

---

## feature/vulkan-mesh-lighting

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/render-contract`, `feature/vulkan-offscreen` (Спринт 1); `core::Transform`/`core::Vec3`

**Цель фичи:** отрисовка мешей с матрицами, камера и освещение в Vulkan-рендерере.

**Описание фичи:** дополнение Vulkan-рендерера — загрузка мешей, накопление команд кадра, разбор потока `RenderCommand` (камера, свет, меши) и матричные помощники. Начало Этапа 2 контура E2.

**Обязательные требования:**

- После фичи `renderFrame` обязан обрабатывать команды `SetViewport` (аспект кадра), `SetCamera`, `AddLight`, `SetSky` (небесный купол — приклеенный к камере куб c зенитным цветом в `emissive`) и `DrawMesh`. Типы команд и света — из контракта `sky/rendering/rendering.hpp`, символ в символ:

```cpp
enum class RenderCommandType : std::uint8_t {
    BeginFrame,
    SetViewport,
    SetCamera,
    AddLight,
    SetSky,
    BindPipeline,
    DrawMesh,
    EndFrame,
};

enum class LightType : std::uint32_t {
    Directional = 0,
    Point = 1,
};
```

- Реализуемые методы (объявленный ранее контракт, сигнатуры символ в символ):

```cpp
void submit(std::span<const rendering::RenderCommand> commands) override;

rendering::RenderResourceHandle createMeshFromData(
    std::span<const float> interleavedPosNormalUv) override;
```

- Валидация `createMeshFromData`: если рендерер не готов, span пуст или `interleavedPosNormalUv.size() % 24 != 0` — вернуть `rendering::RenderResourceHandle::invalid()`. 24 float — это один треугольник: 3 вершины по 8 float (позиция 3 + нормаль 3 + UV 2).
- Освещение (фактическое поведение реализации): в `FrameUbo` укладываются максимум `kMaxLights = 4` источника, лишние `AddLight` игнорируются; для `LightType::Directional` вектор — `core::rotate(command.transform.rotation, {0.0f, 0.0f, -1.0f})`, для `LightType::Point` — `command.transform.position`; цвет — `command.color`, помноженный на `command.lightIntensity`; в метаданные света пишутся признак точечного источника и `command.lightRange`; первый `Directional` задаёт направление теней.
- `SetCamera`: при `command.orthoHeight > 0.0f` — `orthographic(...)`, иначе `perspective(command.fovDegrees, aspect, ...)`; `viewProjection = proj * viewFromCameraPose(command.transform)`.
- `DrawMesh`: модельная матрица `fromTransform(command.transform)` и параметры материала — в push-константах; неизвестный `resource` рисуется встроенным кубом. `submit` копит команды кадра, `renderFrame` разбирает накопленный поток и очищает его.

**Общий порядок реализации фичи:**
1. Реализовать `createMeshFromData` и `submit`.
2. Реализовать разбор потока в `renderFrame` (`SetCamera`, `AddLight`, `DrawMesh`).
3. Реализовать матричные помощники.

**Файлы фичи:**
1. `engine/rendering_vulkan/src/vulkan_renderer.cpp`

### Файл: `engine/rendering_vulkan/src/vulkan_renderer.cpp`

**Назначение файла:** дополнение реализации Vulkan-рендерера мешами, камерой и светом.

**Пошаговое описание действий:**
1. Реализовать `createMeshFromData(std::span<const float>)` — загрузку меша.
2. Реализовать `submit(std::span<const RenderCommand>)` — накопление команд кадра.
3. В `renderFrame` разобрать поток: `SetCamera`, `AddLight`, `DrawMesh`.
4. Реализовать матричные помощники `perspective`, `orthographic`, `fromTransform`, `viewFromCameraPose`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `FrameUbo` (до 4 источников света).

*Функции / методы:*
- `createMeshFromData(std::span<const float>)`
- `submit(std::span<const RenderCommand>)`
- `renderFrame` (разбор потока)
- `perspective(fov,aspect,near,far)`, `orthographic(...)`, `fromTransform(Transform)`, `viewFromCameraPose(...)`

*Логика функций / методов:*
- `createMeshFromData(std::span<const float>)` — загрузка меша (реализация уже объявленного контракта). Возвращает: хэндл.
- `submit(std::span<const RenderCommand>)` — накопление команд кадра.
- в `renderFrame` — разбор потока: `SetCamera` (матрицы вида/проекции: `perspective`/`orthographic`), `AddLight` (до 4 источников в `FrameUbo`), `DrawMesh` (модельная матрица и материал в push-константах).
- матричные помощники `perspective(fov,aspect,near,far)`, `orthographic(...)`, `fromTransform(Transform)`, `viewFromCameraPose(...)`.

**Результат по файлу:** рендерер рисует освещённые меши с камерой.

**Критерий правильности по файлу:**
1. Кадр демо-сцены содержит освещённые меши.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/rendering_vulkan/src/vulkan_renderer.cpp` (дополнено мешами, камерой, светом)

**Общий критерий правильности:**
1. Кадр демо-сцены содержит освещённые меши.

- **Приёмочный тест:** `tests/day4/vulkan_mesh_lighting_tests.cpp` — собирается из корня: `g++ -std=c++20 -Wall -Wextra -Iengine/core/include -Iengine/rendering/include -Iengine/rendering_vulkan/include tests/day4/vulkan_mesh_lighting_tests.cpp engine/rendering_vulkan/src/vulkan_renderer.cpp -lvulkan -o vulkan_mesh_lighting_tests && ./vulkan_mesh_lighting_tests`. Запускается при наличии Vulkan-драйвера (GPU или программный lavapipe); без него `createVulkanRenderer` возвращает `nullptr`, тест печатает `SKIPPED (Vulkan недоступен)` и выходит с кодом 0 (в CI выставляется `SKY_REQUIRE_VULKAN=1` — тогда отсутствие Vulkan считается провалом). Ожидаемый вывод на машине с Vulkan: `vulkan_mesh_lighting_tests: 14 checks, 0 failures`.

---

## feature/vulkan-viewport

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/engine-bridge` (`EditorSession`, `sky_editor_create`) из Этапа 1 контура E3

**Цель фичи:** панель вьюпорта, показывающая кадр движка.

**Описание фичи:** `VulkanViewport : Control` держит `WriteableBitmap` и таймер кадров, дёргает нативный offscreen-рендер и блитит пиксели; `SceneView` хостит вьюпорт. Начало Этапа 2 контура E3.

**Обязательные требования:**

- Обязательные файлы вьюпорта: `editor/avalonia/Controls/VulkanViewport.cs`, `editor/avalonia/Views/SceneView.axaml` **и** его code-behind `editor/avalonia/Views/SceneView.axaml.cs` (`x:Class="SkyEditor.Views.SceneView"`, `public partial class SceneView : UserControl`).
- Объявления из фактических исходников, символ в символ:

```csharp
// editor/avalonia/Controls/VulkanViewport.cs
public sealed class VulkanViewport : Control

public void SetContext(IntPtr context) => _context = context;

/// Renders a single frame synchronously (used for headless capture).
public void RenderOnce() => RenderFrame();
```

```xml
<!-- editor/avalonia/Views/SceneView.axaml -->
<controls:VulkanViewport Name="Viewport"/>
```

- Имя элемента — ровно `Name="Viewport"`: его ищут `FindControl<VulkanViewport>("Viewport")` в code-behind и скриншот-режим `Program.cs`.
- P/Invoke-связка объявлена в `editor/avalonia/Engine/EngineInterop.cs` (зависимость `feature/engine-bridge`), пиксели маршалятся как `byte[]` (RGBA-буфер на вызывающей стороне), возврат `1` — кадр отрисован:

```csharp
[DllImport(Lib)] public static extern int sky_editor_render_offscreen(IntPtr ctx, uint width, uint height, byte[] outRgba, int outLength);
```

- `VulkanViewport` держит `WriteableBitmap` (`Rgba8888`, пересоздаётся при смене размера) и `DispatcherTimer` с интервалом 16 мс; при недоступном Vulkan-устройстве после 3 неудачных кадров выводит читаемое сообщение вместо тихого чёрного экрана.
- `SceneView.axaml.cs` передаёт нативную сессию во вьюпорт: `_viewport.SetContext(vm.NativeContext)`.

**Общий порядок реализации фичи:**
1. Реализовать `VulkanViewport : Control` с `WriteableBitmap` и таймером кадров.
2. Реализовать `SetContext` и `RenderOnce`.
3. Собрать `SceneView`, хостящий вьюпорт.

**Файлы фичи:**
1. `editor/avalonia/Controls/VulkanViewport.cs`
2. `editor/avalonia/Views/SceneView.axaml.cs`

### Файл: `editor/avalonia/Controls/VulkanViewport.cs`

**Назначение файла:** элемент вьюпорта, рисующий кадр движка.

**Пошаговое описание действий:**
1. Объявить `class VulkanViewport : Control` с `WriteableBitmap` и таймером кадров.
2. Реализовать `SetContext(IntPtr context)`.
3. Реализовать `RenderOnce()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class VulkanViewport : Control` (держит `WriteableBitmap` и таймер кадров)

*Функции / методы:*
- `public void SetContext(IntPtr context)`
- `public void RenderOnce()`

*Логика функций / методов:*
- `SetContext(context)` — привязывает сессию движка. Параметры: `context`.
- `RenderOnce()` — рисует один кадр (дёргает нативный offscreen-рендер и блитит пиксели).

**Результат по файлу:** элемент вьюпорта, отображающий кадр движка.

**Критерий правильности по файлу:**
1. `RenderOnce()` выводит непустой кадр в `WriteableBitmap`.

### Файл: `editor/avalonia/Views/SceneView.axaml.cs`

**Назначение файла:** панель сцены, хостящая вьюпорт.

**Пошаговое описание действий:**
1. Разместить `VulkanViewport` внутри `SceneView`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class SceneView` (хостит `VulkanViewport`)

*Функции / методы:* нет (композиция вью).

*Логика функций / методов:*
- `SceneView` — хостит `VulkanViewport`.

**Результат по файлу:** панель сцены с вьюпортом.

**Критерий правильности по файлу:**
1. Панель вьюпорта показывает демо-сцену.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Controls/VulkanViewport.cs`
2. `editor/avalonia/Views/SceneView.axaml.cs`

**Общий критерий правильности:**
1. Панель вьюпорта показывает демо-сцену.

- **Приёмочный тест:** `tests/day4/vulkan_viewport_check.py` — структурная проверка: `python3 tests/day4/vulkan_viewport_check.py <корень-репозитория>` (по умолчанию `.`). Проверяет: `VulkanViewport` наследует `Control`, держит `WriteableBitmap` и таймер кадров, имеет `SetContext(IntPtr)` и `RenderOnce()`, дёргает `sky_editor_render_offscreen`; P/Invoke объявлен в `EngineInterop.cs` с `byte[]`-буфером; `SceneView.axaml` — корректный XML с корнем `UserControl`, хостит `<controls:VulkanViewport Name="Viewport"/>`; `SceneView.axaml.cs` наследует `UserControl` и вызывает `SetContext`. Код выхода 0 — все проверки пройдены. Ручной smoke-тест (dotnet + мост + Vulkan) перечислен в шапке скрипта.

---

## feature/play-mode

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** контрактные заголовки `sky/scene/scene_system.hpp` (`scene::SceneHandle`, `scene::ISceneRuntime` — только объявления) и `sky/editor/viewport/viewport_bridge.hpp`. Реализация `feature/scene-world` и `EditorContext` (`feature/editor-context`) появляются позже — контроллер реализуется против абстракции сценового рантайма (приёмочный тест даёт дублёр). `PhysicsWorld` (`feature/physics-world`) нужен только части `beginPlay`/`endPlay` в `editor_context.cpp`.

**Цель фичи:** управление режимом воспроизведения со снимком/восстановлением сцены.

**Описание фичи:** часть Этапа 3 (режим воспроизведения и ввод) — контроллер воспроизведения и вход/выход из play в контексте редактора.

**Обязательные требования:**

- `PlayModeState` и контракт `IPlayModeController` объявлены **не здесь**, а в уже опубликованном `sky/editor/viewport/viewport_bridge.hpp` — этот заголовок в данной фиче не меняется (не в этот день). Фактические объявления, символ в символ:

```cpp
enum class PlayModeState {
    Editing,
    Playing,
    Paused,
};

class IPlayModeController {
public:
    virtual ~IPlayModeController() = default;

    using StateChanged = std::function<void(PlayModeState)>;

    virtual bool play() = 0;
    virtual bool pause() = 0;
    virtual bool stop() = 0;
    [[nodiscard]] virtual PlayModeState state() const = 0;
    virtual void onStateChanged(StateChanged callback) = 0;
};
```

- `play_mode_controller.hpp` добавляет только наследника и фабрику (namespace `sky::editor`):

```cpp
class PlayModeController : public IPlayModeController {
public:
    ~PlayModeController() override = default;

    virtual void setScene(scene::SceneHandle scene) = 0;
    virtual void tickFrame(double deltaSeconds) = 0;
};

std::unique_ptr<PlayModeController> createPlayModeController(
    scene::ISceneRuntime& sceneRuntime);
```

- `pause()` и `stop()` возвращают `bool` (успех перехода), **не** `void` — сигнатуры выше обязательны символ в символ.
- Сценовый мир (`feature/scene-world`) появляется позже: контроллер реализуется против абстракции сценового рантайма `scene::ISceneRuntime` (из заголовка `scene_system.hpp`, только объявления); приёмочный тест подставляет записывающий дублёр.
- `onStateChanged` регистрирует колбэк, вызываемый на каждом переходе состояния; `tickFrame` пробрасывает кадр в сценовый рантайм только в состоянии `Playing`.

**Общий порядок реализации фичи:**
1. Объявить `PlayModeController` и фабрику в `play_mode_controller.hpp` (`PlayModeState` и `IPlayModeController` уже объявлены в `viewport_bridge.hpp`).
2. Реализовать контроллер в `play_mode_controller.cpp`.
3. Дополнить `editor_context.cpp` методами `beginPlay`/`endPlay`.

**Файлы фичи:**
1. `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`
2. `editor/viewport_bridge/src/play_mode_controller.cpp`
3. `editor/shell/src/editor_context.cpp`

### Файл: `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`

**Назначение файла:** контракт контроллера воспроизведения.

**Пошаговое описание действий:**
1. Включить `viewport_bridge.hpp` (`PlayModeState`, `IPlayModeController` уже объявлены там).
2. Объявить `PlayModeController` и фабрику `createPlayModeController`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class PlayModeController : public IPlayModeController` (`enum class PlayModeState { Editing, Playing, Paused }` — из `viewport_bridge.hpp`)

*Функции / методы:*
- `virtual void setScene(scene::SceneHandle scene) = 0`
- `virtual bool play() = 0` (унаследовано)
- `virtual bool pause() = 0` (унаследовано)
- `virtual bool stop() = 0` (унаследовано)
- `virtual void tickFrame(double deltaSeconds) = 0`
- `[[nodiscard]] virtual PlayModeState state() const = 0` (унаследовано)
- `virtual void onStateChanged(StateChanged callback) = 0` (унаследовано)
- `std::unique_ptr<PlayModeController> createPlayModeController(scene::ISceneRuntime& sceneRuntime)`

*Логика функций / методов:*
- `setScene(scene)` — задаёт сцену для воспроизведения.
- `play()` — запускает. Возвращает: успех (false = сцена не задана).
- `pause()`/`stop()` — пауза/остановка. Возвращают: успех перехода.
- `tickFrame(deltaSeconds)` — продвигает симуляцию на кадр.
- `state()` — Возвращает: текущее состояние; `onStateChanged(callback)` — уведомление о переходах.

**Результат по файлу:** контракт воспроизведения зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `editor/viewport_bridge/src/play_mode_controller.cpp`

**Назначение файла:** реализация контроллера воспроизведения.

**Пошаговое описание действий:**
1. Реализовать переходы состояний.
2. Реализовать `tickFrame`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `PlayModeController`.

*Функции / методы:*
- `setScene`, `play`, `pause`, `stop`, `tickFrame`, `state`.

*Логика функций / методов:*
- `setScene`/`play`/`pause`/`stop` — управляют состоянием (`Editing`/`Playing`/`Paused`); `play` возвращает false, если сцена не задана.
- `tickFrame(deltaSeconds)` — продвигает симуляцию на кадр в состоянии `Playing`.

**Результат по файлу:** рабочий контроллер воспроизведения.

**Критерий правильности по файлу:**
1. `play` без заданной сцены возвращает false; `tickFrame` продвигает симуляцию только в `Playing`.

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** вход/выход из play со снимком трансформов.

**Пошаговое описание действий:**
1. Реализовать `beginPlay`.
2. Реализовать `endPlay`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `EditorContext`).

*Функции / методы:*
- `void beginPlay()`
- `void endPlay()`

*Логика функций / методов:*
- `beginPlay()` — снимает локальные трансформы всех объектов (для восстановления).
- `endPlay()` — восстанавливает снимок и пересаживает тела с нулевой скоростью.

**Результат по файлу:** обратимый вход в режим воспроизведения.

**Критерий правильности по файлу:**
1. После `play → stop` сцена в исходном состоянии, тела без остаточной скорости.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`
2. `editor/viewport_bridge/src/play_mode_controller.cpp`
3. `editor/shell/src/editor_context.cpp`

**Общий критерий правильности:**
1. после `play → stop` сцена в исходном состоянии, тела без остаточной скорости.

- **Приёмочный тест:** `tests/day4/play_mode_tests.cpp` — собирается из корня с `editor/viewport_bridge/src/play_mode_controller.cpp` (реализация сценового мира не нужна): `g++ -std=c++20 -O1 -I editor/viewport_bridge/include -I editor/tools/include -I engine/scene/include -I engine/rendering/include -I engine/asset/include -I engine/object/include -I engine/core/include -I engine/platform/include tests/day4/play_mode_tests.cpp editor/viewport_bridge/src/play_mode_controller.cpp -o play_mode_tests && ./play_mode_tests`. Контроллер проверяется через записывающий дублёр сцены (`RecordingSceneRuntime`). Ожидаемый вывод: `play_mode_tests (day4): 35 checks, 0 failures`, код выхода 0. Часть «`beginPlay`/`endPlay` в `editor_context.cpp`» в день 4 этим тестом не покрывается.

---

## feature/ci-screenshot

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** `feature/build-system`, `feature/test-harness`, `feature/c-abi-seed` (CI), `feature/editor-shell` (`Program.cs`)

**Цель фичи:** сборка редактора и скриншот-артефакт в CI; первые модульные тесты.

**Описание фичи:** ветка `--screenshot` в headless-Avalonia, шаг сборки редактора в CI и публикация PNG-артефакта headless-прогона плеера (`player-smoke.png`), плюс тест продвижения рантайма. Этап 2 контура E5.

**Обязательные требования:**

- Публикуемый в CI PNG-артефакт — **скриншот плеера, а не редактора**: шаг `Player smoke` запускает `./build/player/sky_player --headless player-smoke.png --frames 60`, затем `actions/upload-artifact@v4` публикует артефакт `player-smoke` с `path: player-smoke.png`.
- Точный состав шагов workflow (`.github/workflows/ci.yml`, job `linux`, `ubuntu-24.04`): `actions/checkout@v4` → установка системных зависимостей (`ninja-build libx11-dev libvulkan-dev mesa-vulkan-drivers vulkan-tools xvfb`) → `actions/setup-dotnet@v4` (.NET `8.0.x`) → `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` → `cmake --build build -j"$(nproc)"` → `dotnet build editor/avalonia/SkyEditor.csproj -c Release --nologo` (ошибка C# валит job) → `xvfb-run -a ctest --test-dir build --output-on-failure -j"$(nproc)"` → player smoke → upload-artifact `*.png`. Vulkan в CI — программный драйвер lavapipe (Mesa), X11-тесты под Xvfb.
- Точное поведение ветки `--screenshot` в `editor/avalonia/Program.cs`: `Main` находит `--screenshot` через `Array.IndexOf(args, "--screenshot")` и при наличии пути вызывает `Screenshot(path, demo)`; headless-Avalonia поднимается через `UseSkia().UseHeadless(new AvaloniaHeadlessPlatformOptions { UseHeadlessDrawing = false })`; открывается `MainWindow`, кадры вьюпорта прогоняются через `Viewport.RenderOnce()` (4 кадра, с `--play` — 60), затем `window.CaptureRenderedFrame()`.
- Коды выхода `Screenshot`: `CaptureRenderedFrame()` вернул `null` → `capture failed` в stderr и **код 1**; иначе `frame.Save(path)`, `wrote {path}` в stdout и **код 0**. Дополнительные флаги: `--size WxH`, `--select <имя>`, `--demo`, `--play`.
- `tests/runtime_tests.cpp` подключается в общий CTest-набор (шаг `ctest` того же workflow) и проверяет, что play-режим продвигает рантайм.

**Общий порядок реализации фичи:**
1. Добавить ветку `--screenshot path` в `Program.cs`.
2. Добавить шаг сборки редактора (`dotnet build`) и публикацию PNG-артефакта плеера в `ci.yml`.
3. Написать `runtime_tests.cpp` (play-режим продвигает рантайм).

**Файлы фичи:**
1. `editor/avalonia/Program.cs`
2. `.github/workflows/ci.yml`
3. `tests/runtime_tests.cpp`

### Файл: `editor/avalonia/Program.cs`

**Назначение файла:** дополнение точки входа режимом скриншота.

**Пошаговое описание действий:**
1. Добавить ветку `--screenshot path` — headless Avalonia, рендер нескольких кадров, `window.CaptureRenderedFrame().Save(path)`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- ветка `--screenshot path` в `Main`.

*Логика функций / методов:*
- ветка `--screenshot path` — headless Avalonia, рендер нескольких кадров, `window.CaptureRenderedFrame().Save(path)`.

**Результат по файлу:** редактор умеет снимать скриншот в headless-режиме.

**Критерий правильности по файлу:**
1. `--screenshot path` сохраняет PNG.

### Файл: `.github/workflows/ci.yml`

**Назначение файла:** дополнение CI сборкой редактора и артефактом-скриншотом.

**Пошаговое описание действий:**
1. Добавить шаг `dotnet build editor/avalonia/SkyEditor.csproj -c Release --nologo` (ошибка C# валит задачу).
2. Добавить шаг headless-прогона плеера (`sky_player --headless player-smoke.png --frames 60`) и публикацию PNG-артефакта (`actions/upload-artifact@v4`).

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* нет (декларативный CI).

*Логика функций / методов:*
- шаги: checkout → зависимости → setup-dotnet → cmake configure → cmake build → `dotnet build editor/avalonia/SkyEditor.csproj` (ошибка C# валит задачу) → ctest под Xvfb → player smoke → публикация PNG-артефакта `player-smoke.png`.

**Результат по файлу:** CI собирает редактор и публикует PNG-скриншот headless-прогона плеера.

**Критерий правильности по файлу:**
1. Артефакт-PNG доступен из прогона CI; ошибка C# останавливает сборку.

### Файл: `tests/runtime_tests.cpp`

**Назначение файла:** тест продвижения рантайма в play-режиме.

**Пошаговое описание действий:**
1. Проверить, что play-режим действительно продвигает рантайм (ECS-система крутит объект).

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:*
- проверяет, что play-режим действительно продвигает рантайм (ECS-система крутит объект).

**Результат по файлу:** зелёный тест `runtime_tests`.

**Критерий правильности по файлу:**
1. Play-режим продвигает рантайм (объект вращается ECS-системой).

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Program.cs` (ветка `--screenshot`)
2. `.github/workflows/ci.yml` (сборка редактора + PNG-артефакт `player-smoke.png`)
3. `tests/runtime_tests.cpp`

**Общий критерий правильности:**
1. Артефакт-PNG (скриншот headless-прогона плеера) доступен из прогона CI.
2. Ошибка C# останавливает сборку.

- **Приёмочный тест:** `tests/day4/ci_screenshot_check.py` — `python3 tests/day4/ci_screenshot_check.py <корень_репозитория>`. Проверяет: workflow в `.github/workflows/*.yml` запускается на push/pull_request и содержит шаги checkout, установку зависимостей, конфигурацию cmake, сборку (`cmake --build`), сборку редактора `dotnet build` (ошибка C# валит задачу), запуск тестов `ctest` и публикацию PNG-артефакта (`actions/upload-artifact`); в `editor/avalonia/Program.cs` — ветку `--screenshot <path>` (headless Avalonia, рендер кадров, `window.CaptureRenderedFrame().Save(path)`, `Main` возвращает код выхода); наличие `tests/runtime_tests.cpp`. Код выхода: 0 — все проверки пройдены, 1 — есть провалы.

---

## feature/ecs-systems

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** `feature/ecs-core` (ECS-ядро); `EcsTransform` из `feature/ecs-object-sync`

**Цель фичи:** первые прикладные системы поверх ECS (частицы и пакетная обработка трансформов).

**Описание фичи:** часть Этапа 3 (прикладные системы поверх ECS). Файлы этой фичи в текущем репозитории отсутствуют — создаются на этапе.

**Обязательные требования:**

- В репозитории у этой фичи нет опубликованного заголовка — **API фиксируется этим заданием**. Завести общий заголовок `engine/ecs/include/sky/ecs/systems.hpp`:

```cpp
#pragma once

#include <memory>

#include "sky/core/math.hpp"
#include "sky/ecs/ecs_world.hpp"
#include "sky/ecs/object_sync.hpp"

namespace sky::ecs {

/// Компонент частицы: позиция, скорость, оставшееся время жизни.
struct Particle {
    core::Vec3 position{};
    core::Vec3 velocity{};
    float lifetimeSeconds = 0.0f;
};

std::unique_ptr<IEcsSystem> createParticleSystem(EcsWorld& world);
std::unique_ptr<IEcsSystem> createTransformBatchSystem(EcsWorld& world);

} // namespace sky::ecs
```

- Обе системы реализуют контракт `sky::ecs::IEcsSystem` из `ecs.hpp` (`[[nodiscard]] virtual std::string name() const = 0;`, `virtual void update(double deltaSeconds) = 0;`) и работают через `registerSystem`/`tick` мира; удаление мёртвых частиц — после обхода, чтобы не инвалидировать результат запроса `entitiesWith` на ходу.
- Числовые критерии приёмки системы частиц (согласованы с приёмочным тестом): частица `{position (0,0,0), velocity (2,0,0), lifetimeSeconds 1.0}` после `tick(0.5)` имеет `position.x == 1.0f` и `lifetimeSeconds == 0.5f`; частица с `lifetimeSeconds 0.25` после того же такта уничтожена **вместе с сущностью** (`isAlive == false`, компонента нет); сущность без `Particle` не тронута; после второго `tick(0.5)` частиц не осталось (`count() == 0`, `entitiesWith({typeid(Particle)})` пуст).
- Числовые критерии приёмки пакетной системы: из 8 сущностей с `EcsTransform` (позиции `x = 0..7`) ОДИН `tick` обрабатывает все 8 (каждая сдвинута на `+1.0f` по X, охват такта == 8); девятая сущность без компонента в пакет не попадает; после `destroyEntity` одной из них охват следующего такта == 7.
- Совместная работа: у сущности с `Particle` и `EcsTransform` обе зарегистрированные системы видят свои данные за один `tick`; после `unregisterSystem` системы частиц пакетная продолжает работать одна.

**Общий порядок реализации фичи:**
1. Реализовать систему частиц в `particle_system.cpp`.
2. Реализовать систему пакетной обработки трансформов в `transform_batch_system.cpp`.

**Файлы фичи:**
1. `engine/ecs/src/systems/particle_system.cpp`
2. `engine/ecs/src/systems/transform_batch_system.cpp`

### Файл: `engine/ecs/src/systems/particle_system.cpp`

**Назначение файла:** система частиц.

**Пошаговое описание действий:**
1. Объявить компонент `Particle`.
2. Реализовать `update(double)`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- компонент `Particle`.

*Функции / методы:*
- `update(double)`

*Логика функций / методов:*
- `update(double)` — продвигает позиции и время жизни частиц; мёртвые частицы удаляются.
- источник сущностей — любой код, создающий сущности с компонентом `Particle`; приёмочный тест создаёт частицы напрямую в ECS-мире (связь с генерацией карты в эту фичу не входит).

**Результат по файлу:** визуально наблюдаемая система частиц.

**Критерий правильности по файлу:**
1. Частицы продвигаются, мёртвые удаляются.

### Файл: `engine/ecs/src/systems/transform_batch_system.cpp`

**Назначение файла:** система пакетной обработки трансформов.

**Пошаговое описание действий:**
1. Реализовать один `update` над всеми `EcsTransform`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `update(double)`

*Логика функций / методов:*
- система пакетной обработки трансформов — один `update` над всеми `EcsTransform`.

**Результат по файлу:** пакетная обработка трансформов за такт.

**Критерий правильности по файлу:**
1. Пакетная система применяется ко всем сущностям с компонентом за такт.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/ecs/src/systems/particle_system.cpp`
2. `engine/ecs/src/systems/transform_batch_system.cpp`

**Общий критерий правильности:**
1. пакетная система применяется ко всем сущностям с компонентом за такт; частицы видны.

- **Приёмочный тест:** `tests/day4/ecs_systems_tests.cpp` — фиксирует поведенческий контракт этапа на ECS-подложке; собирается из корня: `g++ -std=c++20 -Iengine/core/include -Iengine/ecs/include -Iengine/object/include tests/day4/ecs_systems_tests.cpp engine/ecs/src/ecs_world.cpp -o ecs_systems_tests && ./ecs_systems_tests`. Требования к PR: оба файла фичи существуют, реализованные в них системы удовлетворяют тем же проверкам (частицы продвигаются, мёртвые удаляются; пакетная система обходит все `EcsTransform` за один `tick`). Ожидаемый вывод: `ecs_systems_tests: 21 checks, 0 failures`, код возврата 0.
