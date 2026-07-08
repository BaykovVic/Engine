# Спринт 1 · День 10 — выдача фич (по одной на контур)

День 10: каждый контур берёт свою **10-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/object-snapshot` | Этап 3 (Недели 3–4, веха M2). Отмена операций и формат сцены SKYB |
| **E2** Рендеринг | `feature/shadow-mapping` | Этап 5 (Недели 7–8, веха M4). Тени |
| **E3** Редактор(.NET) | `feature/dragdrop-2d` | Этап 4 (Недели 5–6, веха M3). Выбор меша, перетаскивание, режим 2D |
| **E4** Рантайм/скриптинг | `feature/gameplay-api` | Этап 5 (Недели 7–8, веха M4). Пользовательские сборки и игровой интерфейс |
| **E5** Пайплайн/пакеты | `feature/package-lock-and-install` | Этап 5 (Недели 7–8, веха M4). Менеджер пакетов |
| **E6** Data-oriented(ECS) | — (фичи этого контура закончились) | — |

---

## E1 · `feature/object-snapshot`

*Этап: Этап 3 (Недели 3–4, веха M2). Отмена операций и формат сцены SKYB.*

#### Дополнение файла `editor/shell/src/editor_context.cpp`
Структуры снимка в `.hpp`: `ObjectSnapshot{name, local, hasPhysicsBody,
components, children}`, `ComponentSnapshot{typeId, fields}`.
- `ObjectSnapshot snapshotObject(object::ObjectHandle object) const`
  Что делает: полный снимок поддерева (имя, трансформ, компоненты со всеми полями, дети). Возвращает: снимок.
- `object::ObjectHandle restoreObject(const ObjectSnapshot& snapshot, object::ObjectHandle parent)`
  Что делает: восстанавливает поддерево из снимка под родителем (invalid = корень). Возвращает: хэндл корня.
- `void newScene()` — очищает сцену.
- `bool saveScene(const std::filesystem::path& path)` — сохраняет в SKYB. Возвращает: успех.
- `bool openScene(const std::filesystem::path& path)` — открывает из файла. Возвращает: успех.
- `object::ObjectHandle duplicateObject(object::ObjectHandle object)` — глубокая копия с детьми. Возвращает: копию.
- `void reparent(object::ObjectHandle child, object::ObjectHandle newParent)` — меняет родителя, сохраняя мировое положение.
- `void destroyObject(object::ObjectHandle object)` — удаляет объект.

---

## E2 · `feature/shadow-mapping`

*Этап: Этап 5 (Недели 7–8, веха M4). Тени.*

#### Файл `engine/rendering_vulkan/shaders/shadow.vert` (+ `shadow.vert.spv.h`)
Depth-only вершинный шейдер: `lightViewProjection * model * position`; обе
матрицы в push-константах (`struct ShadowPush`).

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- `initShadowResources()` — создаёт сэмплируемую карту глубины 2048×2048 (D32),
  сэмплер с зажимом к границе, отдельный проход и конвейер (со slope-scaled
  bias), дескриптор set 7.
- в `renderFrame` — сначала depth-only проход теней по всем `DrawMesh`, затем
  основной проход с привязкой карты теней.
- `FrameUbo` растёт: `lightViewProjection[16]`, флаг теней и индекс источника.
- в `mesh.frag` — функция `shadowVisibility(worldPos, ndl)`: выборка 3×3 (PCF) с
  нормаль-зависимым смещением.

---

## E3 · `feature/dragdrop-2d`

*Этап: Этап 4 (Недели 5–6, веха M3). Выбор меша, перетаскивание, режим 2D.*

#### Файлы `editor/avalonia/Views/ProjectView.axaml.cs`, `Views/GameView.axaml.cs`
- перетаскивание модели из панели проекта в сцену (`DataObject` с ссылкой на ассет).
- ортографический режим 2D и угловой гизмо переключения проекций (в `VulkanViewport`).
- P/Invoke: `sky_editor_create_mesh_object`, `sky_editor_set_view_2d`, `sky_editor_look_along_axis`.

---

## E4 · `feature/gameplay-api`

*Этап: Этап 5 (Недели 7–8, веха M4). Пользовательские сборки и игровой интерфейс.*

#### Дополнение файлов `managed/SkyEngine.Managed/{Engine,ScriptComponent,Physics}.cs`
- таблица обратного API растёт до 12 указателей: `GetWorldPosition`,
  `Instantiate`, `DestroyObject`, `SetVelocity`, `GetVelocity`, `Raycast`.
- `ScriptComponent`: защищённые `Instantiate(prefab, x,y,z)`, `Destroy()`,
  `SetVelocity(x,y,z)`, `GetWorldPosition()` (в т.ч. по id чужого объекта).
- `Physics.cs`: `Physics.Raycast(...) → RaycastHit`.

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- нативные колбэки `scriptGetWorldPosition/Instantiate/DestroyObject/SetVelocity/
  GetVelocity/Raycast`; raycast физики по heightfield (марш + бисекция + нормаль).

---

## E5 · `feature/package-lock-and-install`

*Этап: Этап 5 (Недели 7–8, веха M4). Менеджер пакетов.*

#### Файлы `engine/package/include/sky/package/{package_lock,package_installer}.hpp`, `engine/package/src/package_installer.cpp`
- `std::uint64_t manifestChecksum(const PackageManifest& manifest)` — Возвращает: контрольную сумму манифеста.
- `bool savePackageLock(...)` / `std::optional<std::vector<LockedPackage>> loadPackageLock(...)` — запись/чтение `sky.lock` (версии + чексуммы + active).
- `class PackageInstaller`:
  - `std::optional<PackageManifest> install(const std::string& source, const std::filesystem::path& projectPackages)`
    Что делает: устанавливает пакет из источника (каталог / tarball / git с `#tag`) через immutable-кэш. Параметры: `source`, `projectPackages` — куда установить. Возвращает: манифест или `nullopt`.

---
