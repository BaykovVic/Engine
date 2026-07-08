# Спринт 1 · День 7 — выдача фич (по одной на контур)

День 7: каждый контур берёт свою **7-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/scene-authoring` | Этап 2 (Неделя 2, веха M1). Сцена и сборочная точка |
| **E2** Рендеринг | `feature/sky-backdrop` | Этап 3 (Недели 3–4, веха M2). Выбор объекта, проекция, небо |
| **E3** Редактор(.NET) | `feature/inspector` | Этап 3 (Недели 3–4, веха M2). Гизмо, инспектор, панели данных |
| **E4** Рантайм/скриптинг | `feature/managed-runtime` | Этап 4 (Недели 5–6, веха M3). Подсистема скриптинга |
| **E5** Пайплайн/пакеты | `feature/vfs-and-bridge-tests` | Этап 3 (Недели 3–4, веха M2). Импортёры, виртуальная ФС, тесты интерфейса |
| **E6** Data-oriented(ECS) | — (фичи этого контура закончились) | — |

---

## E1 · `feature/scene-authoring`

*Этап: Этап 2 (Неделя 2, веха M1). Сцена и сборочная точка.*

#### Файлы `engine/scene/include/sky/scene/scene_authoring.hpp`, `engine/scene/src/scene_authoring.cpp`
`enum class PrimitiveKind {Cube, Plane, Sphere}`.
- `object::ObjectHandle createPrimitive(const AuthoringServices& services, PrimitiveKind kind, const std::string& name)`
  Что делает: создаёт объект-примитив с компонентом Mesh Renderer.
  Параметры: `services` — ссылки на миры объектов/компонентов, `kind` — вид, `name` — имя. Возвращает: хэндл объекта.

---

## E2 · `feature/sky-backdrop`

*Этап: Этап 3 (Недели 3–4, веха M2). Выбор объекта, проекция, небо.*

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- обработка команды `RenderCommandType::SetSky` — купол-небо: горизонт (`color`)
  → зенит (`emissive`), диск солнца от направленного света. Реализуется как
  большой куб, приклеенный к камере, с флагом «небо» в push-константах.

---

## E3 · `feature/inspector`

*Этап: Этап 3 (Недели 3–4, веха M2). Гизмо, инспектор, панели данных.*

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public List<ComponentView> ReadComponents(ulong id)` — Возвращает: компоненты объекта с их полями.
- `public void SetComponentField(ulong id, int component, int field, string value)` — записывает поле через C-интерфейс.
- `public List<ComponentType> AvailableTypes()` — Возвращает: типы для меню Add Component.
- `public void AddComponent(ulong id, string typeId)` / `public void RemoveComponent(ulong id, int component)`.
- классы `ComponentView`, `ComponentField : INotifyPropertyChanged` (свойства `IsScalar/IsBool/IsVec3/IsMeshRef`, `Value`, `X/Y/Z`, `BoolValue`).
- `Views/InspectorView.axaml.cs` — карточки компонентов, шаблоны полей.

---

## E4 · `feature/managed-runtime`

*Этап: Этап 4 (Недели 5–6, веха M3). Подсистема скриптинга.*

#### Файлы `managed/SkyEngine.Managed/*.cs`
- `Bootstrap.cs` — `[UnmanagedCallersOnly]` точки входа, вызываемые из C++:
  `LoadAssembly`, `CreateInstance`, `DestroyInstance`, `InvokeLifecycle`,
  `Initialize` (ставит обратный API), `SetObjectId`, `TickFrame`.
- `ScriptComponent.cs` — базовый класс скрипта (аналог MonoBehaviour):
  `OnCreate/OnStart/OnUpdate/OnFixedUpdate/OnDestroy`, защищённые
  `SetLocalPosition/SetLocalEuler/SetLocalScale`, свойство `Handle`.
- `NativeHandle.cs` — обёртка над id объекта.
- `Engine.cs` — таблица делегатов обратного API (нативные функции движка).
- `Debug.cs` — `Debug.Log/LogWarning/LogError` в консоль редактора.
- `Time.cs` — `Time.TotalTime/DeltaTime`. `Input.cs` — `Input.GetKey(KeyCode)`.

---

## E5 · `feature/vfs-and-bridge-tests`

*Этап: Этап 3 (Недели 3–4, веха M2). Импортёры, виртуальная ФС, тесты интерфейса.*

#### Файлы `engine/platform/include/sky/platform/{file_system,virtual_file_system}.hpp` + `src/{std_file_system,virtual_file_system}.cpp`
`class IFileSystem` (exists/isDirectory/readAll/writeAll/list) и
`class IVirtualFileSystem`:
- `bool mount(const std::string& alias, std::shared_ptr<IVfsMount> mountPoint, int priority)` — монтирует схему (напр. `assets`). Возвращает: успех.
- `std::optional<std::vector<std::byte>> readAll(const std::string& ref)` — читает по ссылке `assets://…`. Возвращает: байты или `nullopt`.
- `std::vector<...> list(const std::string& dir)` — Возвращает: содержимое каталога.

#### Файлы `tests/{editor_bridge_tests,asset_project_tests,vfs_tests}.cpp`
Тест на каждую группу C-интерфейса и на импорт/VFS.

---
