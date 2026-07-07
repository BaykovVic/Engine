# ТЗ · E3 — Редактор (.NET) (весь срок)

**Роль.** Владелец всего UI: окно, докинг, панели, гизмо, инспектор,
скриншот-режим. Ты общаешься с движком **только через C ABI** (P/Invoke) —
это делает тебя независимым от C++-команды: пока ABI-функции нет, ты пишешь
её заглушку и ждёшь реализацию снизу (правило «сустав двигает верхний»).

**Стек.** C#/.NET 8, Avalonia, Dock.Avalonia. **Каталог:** `editor/avalonia`.

## Твои суставы
- **Потребляешь:** C ABI `sky_editor_*` (владелец — E5/E1), кадры вьюпорта (E2).
- **Предоставляешь:** ничего нативного — ты верхний слой. Но твои ожидания к ABI (какие функции нужны) — это ТЗ для нижних.

Правило: нужна функция ABI — заводишь `[DllImport]` + ожидание и просишь E5/E1 реализовать под тест.

---

## Неделя 1 — каркас, докинг, hello-bridge
- `[B] SkyEditor.csproj` (net8.0 + Avalonia пакеты).
- `[S] Program.cs`, `App.axaml{,.cs}`, `MainWindow.axaml{,.cs}` — окно «Sky Engine».
- `[S] Docking/DockFactory.cs`, `Docking/Tools.cs`; `[S] Views/*.axaml` — 5–6 панелей-заглушек.
- `[S] Engine/EngineInterop.cs` — DllImport-резолвер (`SKY_BRIDGE_PATH` + build-дерево), `sky_editor_create/destroy`.
- `[S] Engine/EditorSession.cs` — `IDisposable`, в ctor create + проверка, `Native`.
**Зависишь:** E5/E1 (заглушка C ABI). **Готово:** `dotnet build` 0 ошибок; окно; вкладки таскаются; в статус-баре «bridge OK».

## Неделя 2 (M1) — панель-вьюпорт и живая Hierarchy
- `[S] Controls/VulkanViewport.cs` — `WriteableBitmap`, таймер, `SetContext(IntPtr)`, `RenderOnce()`, блит offscreen-кадра.
- `[S] Views/SceneView.axaml{,.cs}`, `HierarchyView.axaml{,.cs}` — вьюпорт + `TreeView` по `Roots`.
- `[S] EngineInterop.cs` (дополнить): `root_count/at`, `child_count/at`, `object_name/exists`, `get_transform`, `render_offscreen`, орбита/зум.
- `[S] EditorSession.cs` — `SkyObject{Id,Name,Children}`, `Reload()`, `Load(id)`, `Transform(id)`.
**Зависишь:** E2 (кадр), E1 (иерархия через ABI). **Готово:** демо-сцена во вьюпорте, Hierarchy живая.

## Недели 3–4 (M2) — гизмо, Inspector, панели
- `[S] Controls/VulkanViewport.cs` (дополнить): гизмо move/rotate/scale поверх кадра, drag-состояния, scene-гизмо в углу; свойства `SelectedId/LocalSpace/Tool`.
- `[S] MainViewModel.cs` — `GizmoTool`, хоткеи Q/W/E/R, `PositionX/Y/Z` и т.д., `SelectedComponents`, `CreateCube/Duplicate/Delete/Undo/Redo`.
- `[S] EditorSession.cs` (дополнить): `ComponentView`, `ComponentField` (IsScalar/IsBool/IsVec3/IsMeshRef, `Value` через ABI), `ReadComponents`, `SetComponentField`, `AvailableTypes`, `AddComponent/RemoveComponent`.
- `[S] Views/{InspectorView,ConsoleView,MaterialsView,PackagesView}.axaml{,.cs}` — на живых данных.
**Зависишь:** E2 (pick/project для гизмо), E1 (команды undo через ABI). **Готово:** гизмо во всех режимах, Inspector редактирует поля, панели живые.

## Недели 5–6 (M3) — пикер мешей, drag-drop, scene-гизмо, 2D
- `[S] EditorSession.cs` (дополнить): `MeshOption`, `AvailableMeshes`, `CreateModel`, `MeshDisplayName`.
- `[S] Views/ProjectView.axaml.cs`, `HierarchyView.axaml.cs` — drag модели из Project, drop в Hierarchy.
- `[S] Controls/VulkanViewport.cs` (дополнить): scene-гизмо (клик по конусу → `look_along_axis`, Persp/Iso), 2D ортографический режим.
**Зависишь:** E5 (импортеры моделей).

## Недели 7–8 (M4) — пикер классов, поля скриптов
- `[S] EditorSession.cs` (дополнить): `ComponentField.IsScriptClass` → ComboBox, `ScriptClasses(current)`; поля скрипта (`scriptParam`) у `sky.script`, сеттер через `SetScriptField`.
- `[S] Views/InspectorView.axaml` (дополнить): ComboBox класса + рендер полей скрипта.
**Зависишь:** E4 (ABI классов и полей скриптов). **Готово:** класс скрипта выбирается дропдауном, его поля редактируются в Inspector.

## Твои личные ворота
- M1: демо-сцена в панели, Hierarchy живая.
- M2: гизмо + Inspector.
- M4: пикер классов и сериализуемые поля.
