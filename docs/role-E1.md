# ТЗ · E1 — Ядро и данные (весь срок)

**Роль.** Владелец нижнего слоя движка: математика, объектный и компонентный
миры, сериализация, undo, сборочная точка `EditorContext`. Ты — фундамент:
почти все стартуют от твоих контрактов, поэтому первую неделю выдаёшь их
инкрементально и рано.

**Стек.** C++20. **Модули:** `core`, `object`, `component`, `scene`,
`serialization` (совместно), `editor/shell` (EditorContext, команды).

## Твои суставы (что ты отдаёшь другим)

| Интерфейс | Кому нужен |
|---|---|
| `Vec3/Quat/Transform`, `Handle<Tag>` | всем |
| `IObjectFactory / IObjectHierarchyAccess / IObjectQueryService` | E4 (sync), E3 (иерархия), E2 (трансформы) |
| `ComponentWorld`, `FieldValue`, `ComponentDescriptor` | E3 (Inspector), E4 (скрипты), E5 (сцены) |
| `SceneWorld`, `ISceneRepository` (SKYB) | E5, E3 |
| `EditorContext`, `UndoStack`, `ObjectSnapshot` | E3 (через ABI), E4 (play), E5 |

Правило: интерфейсы — append-only. Меняешь — только добавляя методы.

---

## Неделя 1 — фундамент
- `[H] engine/core/include/sky/core/math.hpp` — `Vec2/Vec3/Quat/Transform`, операторы, `rotate`, `compose`, `invCompose`, `conjugate`, `divide`.
- `[H] engine/core/include/sky/core/handle.hpp` — `template<Tag> Handle` (isValid/invalid/==).
- `[H] logger.hpp` + `[S] console_logger.cpp`; `[H] config_service.hpp` + `[S] memory_config_service.cpp`.
- `[H] object/object_model.hpp` (три контракта) · `[H] object_world.hpp` · `[S] object_world.cpp`.
- `[H] component/component_model.hpp` (`FieldValue`, дескриптор, 4 интерфейса) · `[H] component_world.hpp` · `[S] component_world.cpp`.
- `[T] tests/core_tests.cpp`, `tests/world_tests.cpp`.
**Разблокируешь:** E4 (E1-3 нужен для sync), E3 (иерархия), E2 (трансформы).
**Готово:** `ctest -R core_tests|world_tests` зелёный; поворот и композиция трансформов проверены численно.

## Неделя 2 (M1) — сцена и сборочная точка
- `[H+S] scene/scene_world.{hpp,cpp}` — `SceneWorld`, `addRootObject`, `rootObjectsOf`, каркас `saveSceneAs`.
- `[H+S] scene/scene_authoring.{hpp,cpp}` — `PrimitiveKind`, `createPrimitive` (вешает `sky.mesh`).
- `[H+S] editor/shell/src/editor_context.{hpp,cpp}` — собрать все миры в один объект; `buildDemoScene()` (примитивы, свет, камера); `rootObjects()`.
**Разблокируешь:** E2 (FrameBuilder обходит твою сцену), E3 (Hierarchy на `rootObjects`), E5 (мост поверх контекста).
**Готово:** демо-сцена наполнена; `rootObjects()` возвращает именованные рут-объекты.

## Недели 3–4 (M2) — undo, SKYB, операции
- `[H+S] editor/shell/src/editor_commands.{hpp,cpp}` — `IEditorCommand`, `UndoStack`, 12 фабрик: `makeTransform/Field/Create/CreateSnapshot/Delete/Duplicate/Reparent/Rename/AddComponent/RemoveComponent/MaterialCreate/MaterialEdit`.
- `[S] editor_context.cpp` (дополнить): `ObjectSnapshot`/`ComponentSnapshot`, `snapshotObject`, `restoreObject`, `newScene/saveScene/openScene`, `duplicateObject`, `reparent`, `destroyObject`.
- `[S] scene/scene_world.cpp` (дополнить): реальный SKYB (magic "SKYB", schema `sky.scene`, обход рутов → имя/трансформ/компоненты через `ByteWriter`), загрузка с миграцией.
**Ключевое решение:** `ObjectSnapshot` делай lossless сразу — он переиспользуется трижды (undo delete, префабы неделя 7, сцены).
**Готово:** save→open round-trip графа объектов; undo/redo всех операций.

## Недели 5–6 (M3) — граница, физика из компонентов
- `[S] editor_context.cpp` (дополнить): `reattachPhysics()` — восстановление тел из `sky.rigidbody`/`sky.collider.box` при `openScene`.
- Ревизия границы `EditorContext`: пометить, что позже уедет в `RuntimeContext` (реализация — неделя 8, задел твой).
**Зависишь:** E4 (контракт физики), E5 (SKYB round-trip физики).

## Недели 7–8 (M4) — префабы, терраин в SKYB
- `[H+S] editor_context.{hpp,cpp}` (дополнить): `savePrefab` (SKYP поверх `snapshotObject`), `instantiatePrefab`, `spawnPrefabAt`; `writeSnapshot`/`readSnapshot`, `resolveAssetPath`.
- `[S] scene/scene_world.cpp` (дополнить): heightfield террейна и `enabled` в SKYB.
**Разблокируешь:** E3 (drag-drop префабов), E4 (`spawnPrefabAt` для скриптов).

## Твои личные ворота
- M1: демо-сцена наполнена, `rootObjects()` работает.
- M2: save/open/undo/redo — зелёные bridge-тесты (совместно с E5).
- M4: префабы round-trip'ятся; терраин в SKYB.
