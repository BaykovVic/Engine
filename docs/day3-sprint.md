# Спринт 1 · День 3 — выдача фич (по одной на контур)

День 3: каждый контур берёт свою **3-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/object-model` | Этап 1 (Неделя 1). Математика, объектная и компонентная модели |
| **E2** Рендеринг | `feature/vulkan-tests` | Этап 1 (Неделя 1). Vulkan от инициализации до кадра в файле |
| **E3** Редактор(.NET) | `feature/engine-bridge` | Этап 1 (Неделя 1). Каркас, компоновка панелей, связь с движком |
| **E4** Рантайм/скриптинг | `feature/player-runtime` | Этап 2 (Неделя 2, веха M1). Автономный проигрыватель |
| **E5** Пайплайн/пакеты | `feature/c-abi-seed` | Этап 1 (Неделя 1). Сборка, интеграция, тесты, первичный C-интерфейс |
| **E6** Data-oriented(ECS) | `feature/ecs-object-sync` | Этап 2 (Неделя 2 · к 17 июля). Синхронизация с объектным миром |

---

## E1 · `feature/object-model`

*Этап: Этап 1 (Неделя 1). Математика, объектная и компонентная модели.*

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

---

## E2 · `feature/vulkan-tests`

*Этап: Этап 1 (Неделя 1). Vulkan от инициализации до кадра в файле.*

#### Файл `tests/vulkan_tests.cpp`
На lavapipe: `ready()` истинно, кадр рендерится, `readbackFrame()` непустой,
центральный пиксель отличается от углового.

---

## E3 · `feature/engine-bridge`

*Этап: Этап 1 (Неделя 1). Каркас, компоновка панелей, связь с движком.*

#### Файлы `editor/avalonia/Engine/EngineInterop.cs`, `Engine/EditorSession.cs`
- `static class EngineInterop` — резолвер нативной библиотеки и P/Invoke-объявления. На этом этапе:
  - `[DllImport] static extern IntPtr sky_editor_create()` — Возвращает: указатель на сессию движка.
  - `[DllImport] static extern void sky_editor_destroy(IntPtr ctx)` — уничтожает сессию.
  - `static IntPtr Resolve(...)`, `static string[] Candidates()` — находят `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в дереве сборки.
- `class EditorSession : IDisposable` — обёртка над сессией: конструктор вызывает `sky_editor_create` и проверяет не-null; `Dispose()` → `destroy`; свойство `IntPtr Native`.

---

## E4 · `feature/player-runtime`

*Этап: Этап 2 (Неделя 2, веха M1). Автономный проигрыватель.*

#### Файл `player/src/main.cpp`
- `int main(int argc, char** argv)`
  Что делает: точка входа; разбирает `--frames N`, `--headless out.png`, `--scene path`. Возвращает: код выхода.
- `int runHeadless(EditorContext& context, int frames, const char* screenshotPath)`
  Что делает: безоконный прогон — цикл `tickFrame → build → renderFrame`, сохранение PNG. Параметры: `context`, `frames` — число кадров, `screenshotPath` — файл. Возвращает: код выхода.
- `int runWindowed(EditorContext& context, int frameLimit)`
  Что делает: оконный прогон (окно X11/Cocoa + swapchain-рендерер). Параметры: `context`, `frameLimit`. Возвращает: код выхода.
- `int mapPlatformKey(std::int32_t keysym)` — переводит платформенный код клавиши в переносимый (общий с C#). Возвращает: переносимый код или 0.

---

## E5 · `feature/c-abi-seed`

*Этап: Этап 1 (Неделя 1). Сборка, интеграция, тесты, первичный C-интерфейс.*

#### Файлы `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`, `editor/native_bridge/src/editor_bridge.cpp`
Плоский C-интерфейс (совместно с E1). На этом этапе — минимум:
- `SkyEditorContext* sky_editor_create(void)` — Возвращает: указатель на сессию (собирает движок и демо-сцену).
- `void sky_editor_destroy(SkyEditorContext* ctx)` — уничтожает сессию.
- `int32_t sky_editor_root_count(SkyEditorContext* ctx)` — Возвращает: число корневых объектов.
- `SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index)` — Возвращает: id корневого объекта.
- `int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)` — пишет имя в буфер. Возвращает: длину.

---

## E6 · `feature/ecs-object-sync`

*Этап: Этап 2 (Неделя 2 · к 17 июля). Синхронизация с объектным миром.*

#### Файлы `engine/ecs/include/sky/ecs/object_sync.hpp`, `engine/ecs/src/object_sync.cpp`
`struct EcsTransform { core::Transform value; }` — базовый компонент трансформа.
- `class IEcsObjectSync`:
  - `virtual EntityId bind(object::ObjectHandle object) = 0` — связывает объект с сущностью. Возвращает: сущность.
  - `virtual void unbind(object::ObjectHandle object) = 0` — разрывает связь.
  - `virtual EntityId entityOf(object::ObjectHandle object) const = 0` — Возвращает: сущность по объекту.
  - `virtual object::ObjectHandle objectOf(EntityId entity) const = 0` — Возвращает: объект по сущности.
  - `virtual void pushAuthoringState() = 0` — до такта переносит трансформ объекта в `EcsTransform`.
  - `virtual void pullEcsResults() = 0` — после такта переносит результат обратно в объектный мир.
- `std::unique_ptr<IEcsObjectSync> createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)` — фабрика.

Точка интеграции — цикл такта в `engine/scene/src/scene_world.cpp` (совместно с
контуром E1): `pushAuthoringState()` → `tick(dt)` → `pullEcsResults()`.

---
