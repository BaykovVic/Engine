# Спринт 1 · День 4 — выдача фич (по одной на контур)

День 4: каждый контур берёт свою **4-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/component-model` | Этап 1 (Неделя 1). Математика, объектная и компонентная модели |
| **E2** Рендеринг | `feature/vulkan-mesh-lighting` | Этап 2 (Неделя 2, веха M1). Меши, камера, освещение, построитель кадра |
| **E3** Редактор(.NET) | `feature/vulkan-viewport` | Этап 2 (Неделя 2, веха M1). Панель вьюпорта и живое дерево объектов |
| **E4** Рантайм/скриптинг | `feature/play-mode` | Этап 3 (Недели 3–4, веха M2). Режим воспроизведения и ввод |
| **E5** Пайплайн/пакеты | `feature/ci-screenshot` | Этап 2 (Неделя 2, веха M1). Режим скриншотов и первые тесты |
| **E6** Data-oriented(ECS) | `feature/ecs-systems` | Этап 3 (Недели 3–4 · перспектива). Прикладные системы поверх ECS |

---

## E1 · `feature/component-model`

*Этап: Этап 1 (Неделя 1). Математика, объектная и компонентная модели.*

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

---

## E2 · `feature/vulkan-mesh-lighting`

*Этап: Этап 2 (Неделя 2, веха M1). Меши, камера, освещение, построитель кадра.*

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- `createMeshFromData(std::span<const float>)` — загрузка меша (реализация уже объявленного контракта). Возвращает: хэндл.
- `submit(std::span<const RenderCommand>)` — накопление команд кадра.
- в `renderFrame` — разбор потока: `SetCamera` (матрицы вида/проекции: `perspective`/`orthographic`), `AddLight` (до 4 источников в `FrameUbo`), `DrawMesh` (модельная матрица и материал в push-константах).
- матричные помощники `perspective(fov,aspect,near,far)`, `orthographic(...)`, `fromTransform(Transform)`, `viewFromCameraPose(...)`.

---

## E3 · `feature/vulkan-viewport`

*Этап: Этап 2 (Неделя 2, веха M1). Панель вьюпорта и живое дерево объектов.*

#### Файл `editor/avalonia/Controls/VulkanViewport.cs`
- `class VulkanViewport : Control` — держит `WriteableBitmap` и таймер кадров.
  - `public void SetContext(IntPtr context)` — привязывает сессию движка. Параметры: `context`.
  - `public void RenderOnce()` — рисует один кадр (дёргает нативный offscreen-рендер и блитит пиксели).
- `Views/SceneView.axaml.cs` — хостит `VulkanViewport`.

---

## E4 · `feature/play-mode`

*Этап: Этап 3 (Недели 3–4, веха M2). Режим воспроизведения и ввод.*

#### Файлы `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`, `editor/viewport_bridge/src/play_mode_controller.cpp`
`enum class PlayModeState { Editing, Playing, Paused }`.
- `class PlayModeController`:
  - `virtual void setScene(scene::SceneHandle scene) = 0` — задаёт сцену для воспроизведения.
  - `virtual bool play() = 0` — запускает. Возвращает: успех (false = сцена не задана).
  - `virtual void pause() = 0` / `virtual void stop() = 0` — пауза/остановка.
  - `virtual void tickFrame(double deltaSeconds) = 0` — продвигает симуляцию на кадр.
  - `virtual PlayModeState state() const = 0` — Возвращает: текущее состояние.

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `void beginPlay()` — снимает локальные трансформы всех объектов (для восстановления).
- `void endPlay()` — восстанавливает снимок и пересаживает тела с нулевой скоростью.

---

## E5 · `feature/ci-screenshot`

*Этап: Этап 2 (Неделя 2, веха M1). Режим скриншотов и первые тесты.*

#### Дополнение файла `editor/avalonia/Program.cs`
- ветка `--screenshot path` — headless Avalonia, рендер нескольких кадров,
  `window.CaptureRenderedFrame().Save(path)`.

#### Дополнение файла `.github/workflows/ci.yml`
- шаг `dotnet build editor/avalonia` (ошибка C# валит задачу) + публикация PNG-артефакта.

#### Файл `tests/runtime_tests.cpp`
Проверяет, что play-режим действительно продвигает рантайм (ECS-система крутит объект).

---

## E6 · `feature/ecs-systems`

*Этап: Этап 3 (Недели 3–4 · перспектива). Прикладные системы поверх ECS.*

#### Новые файлы `engine/ecs/src/systems/particle_system.cpp`, `.../transform_batch_system.cpp` (создаются)
- система частиц: компонент `Particle`, метод `update(double)` — продвигает
  позиции и время жизни; мёртвые частицы удаляются.
- система пакетной обработки трансформов — один `update` над всеми `EcsTransform`.
- источник сущностей — рассыпка генерации карты (`materializeGenerationResult`).

---
