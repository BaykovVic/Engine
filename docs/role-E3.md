# Техническое задание · Контур E3 «Редактор (.NET)»

**Область ответственности.** Пользовательский интерфейс редактора на Avalonia:
окно, компоновка панелей, вьюпорт, гизмо, инспектор, режим снятия скриншотов.
Взаимодействие с движком — только через C-интерфейс (P/Invoke). Если нужной
функции C-интерфейса нет, контур объявляет её ожидание и заглушку, а реализацию
предоставляют контуры E5/E1.

Каждый этап (неделя) разбит на **фичи** `feature/<название>`. Одна фича = одна
ветка в git. У методов указаны: **сигнатура**, **что делает**, **параметры** и
**что возвращает**.

## Обозначения (C# / Avalonia)

- `[DllImport(...)]` — объявление функции из нативной библиотеки (P/Invoke); вызывается как обычный статический метод.
- `IntPtr` — нативный указатель (у нас — хэндл сессии движка); `ulong` — id объекта.
- `UserControl` — панель интерфейса; `Control` — элемент, рисующий себя сам (вьюпорт).
- `INotifyPropertyChanged` — объект уведомляет интерфейс об изменении свойств (двусторонняя привязка).
- `ObservableCollection<T>` — список, автоматически обновляющий интерфейс при изменении.

---

## Этап 1 (Неделя 1). Каркас, компоновка панелей, связь с движком

**Общее описание задач этапа.** Проект редактора, компоновка перетаскиваемых
панелей-заглушек и первичный вызов движка через P/Invoke. Три фичи.

### feature/editor-shell

#### Файлы `editor/avalonia/SkyEditor.csproj`, `Program.cs`, `App.axaml.cs`, `MainWindow.axaml.cs`
- `csproj` — проект .NET 8 с пакетами Avalonia.
- `static int Main(string[] args)` — точка входа; ветка `--screenshot` (headless). Возвращает: код выхода.
- `App` — приложение Avalonia; `OnFrameworkInitializationCompleted()` открывает `MainWindow`.
- `MainWindow` — окно «Sky Engine», меню (File/Edit/GameObject), обработчики пунктов.

### feature/docking-layout

#### Файлы `editor/avalonia/Docking/DockFactory.cs`, `Docking/Tools.cs`, `Views/PlaceholderView.axaml.cs`
- `class DockFactory` — `CreateLayout()` собирает раскладку из панелей (Hierarchy/Scene/Inspector/…).
- `Tools.cs` — классы-инструменты (по одному на панель): `HierarchyTool`, `InspectorTool`, `SceneDocument`, …
- `PlaceholderView` — пустая панель-заглушка.

### feature/engine-bridge

#### Файлы `editor/avalonia/Engine/EngineInterop.cs`, `Engine/EditorSession.cs`
- `static class EngineInterop` — резолвер нативной библиотеки и P/Invoke-объявления. На этом этапе:
  - `[DllImport] static extern IntPtr sky_editor_create()` — Возвращает: указатель на сессию движка.
  - `[DllImport] static extern void sky_editor_destroy(IntPtr ctx)` — уничтожает сессию.
  - `static IntPtr Resolve(...)`, `static string[] Candidates()` — находят `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в дереве сборки.
- `class EditorSession : IDisposable` — обёртка над сессией: конструктор вызывает `sky_editor_create` и проверяет не-null; `Dispose()` → `destroy`; свойство `IntPtr Native`.

**На выходе:** `dotnet build` без ошибок; окно открывается; панели перетаскиваются.
**Критерий правильности этапа:** `dotnet build` = 0 ошибок; сессия движка
создаётся из .NET (не-null); есть сброс компоновки.

---

## Этап 2 (Неделя 2, веха M1). Панель вьюпорта и живое дерево объектов

**Общее описание задач этапа.** Панель вьюпорта (кадр движка) и дерево объектов
на живых данных через C-интерфейс. Две фичи.

### feature/vulkan-viewport

#### Файл `editor/avalonia/Controls/VulkanViewport.cs`
- `class VulkanViewport : Control` — держит `WriteableBitmap` и таймер кадров.
  - `public void SetContext(IntPtr context)` — привязывает сессию движка. Параметры: `context`.
  - `public void RenderOnce()` — рисует один кадр (дёргает нативный offscreen-рендер и блитит пиксели).
- `Views/SceneView.axaml.cs` — хостит `VulkanViewport`.

### feature/hierarchy-tree

#### Дополнение файла `editor/avalonia/Engine/EngineInterop.cs`
P/Invoke иерархии и трансформа (реализация — в мосте контура E5):
`sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`,
`sky_editor_child_at`, `sky_editor_object_name`, `sky_editor_get_transform`,
`sky_editor_render_offscreen`, `sky_editor_viewport_orbit`, `sky_editor_viewport_zoom`.

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public void Reload()` — перечитывает иерархию через C-интерфейс в `ObservableCollection<SkyObject> Roots`.
- `private SkyObject Load(ulong id)` — рекурсивно строит узел дерева.
- `public (float[] position, float[] rotation, float[] scale) Transform(ulong id)`
  Что делает: читает трансформ объекта. Параметры: `id`. Возвращает: позицию (3), кватернион (4), масштаб (3).
- `Views/HierarchyView.axaml.cs` — `TreeView` по `Roots`.

**На выходе:** демо-сцена видна в панели вьюпорта; дерево объектов отражает сцену.
**Критерий правильности этапа:** панель вьюпорта показывает демо-сцену; в дереве —
имена и вложенность объектов движка.

---

## Этап 3 (Недели 3–4, веха M2). Гизмо, инспектор, панели данных

**Общее описание задач этапа.** Манипуляторы, инспектор полей компонентов и
панели консоли/материалов/пакетов. Три фичи.

### feature/gizmos

#### Дополнение файла `editor/avalonia/Controls/VulkanViewport.cs`
- свойства `public ulong SelectedId`, `public bool LocalSpace`, `public GizmoTool Tool`.
- `DrawMoveGizmo`, `DrawRotateGizmo`, `DrawScaleGizmo`, `DrawSceneGizmo` — рисование манипуляторов поверх кадра.
- `OnPointerPressed/Moved/Released` — драг осей (перемещение/поворот/масштаб через C-интерфейс).

#### Файл `editor/avalonia/MainViewModel.cs`
- `enum GizmoTool { Hand, Move, Rotate, Scale }`; свойство `public GizmoTool Tool` (хоткеи Q/W/E/R).
- `public void CreateCube()`, `public void Undo()`, `public void Redo()`, `DuplicateSelected()`, `DeleteSelected()`.
- свойства трансформа `PositionX/Y/Z`, `RotationX/Y/Z`, `ScaleX/Y/Z` (двусторонние).

### feature/inspector

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public List<ComponentView> ReadComponents(ulong id)` — Возвращает: компоненты объекта с их полями.
- `public void SetComponentField(ulong id, int component, int field, string value)` — записывает поле через C-интерфейс.
- `public List<ComponentType> AvailableTypes()` — Возвращает: типы для меню Add Component.
- `public void AddComponent(ulong id, string typeId)` / `public void RemoveComponent(ulong id, int component)`.
- классы `ComponentView`, `ComponentField : INotifyPropertyChanged` (свойства `IsScalar/IsBool/IsVec3/IsMeshRef`, `Value`, `X/Y/Z`, `BoolValue`).
- `Views/InspectorView.axaml.cs` — карточки компонентов, шаблоны полей.

### feature/data-panels

#### Файлы `editor/avalonia/Views/{ConsoleView,MaterialsView,PackagesView}.axaml.cs`
Панели консоли (лог с цветом по уровню), материалов и пакетов — на живых данных
через соответствующие P/Invoke.

**На выходе:** гизмо move/rotate/scale работают; инспектор редактирует поля; панели живые.
**Критерий правильности этапа:** изменение поля в инспекторе применяется к
движку и отменяемо; активный инструмент синхронизирован.

---

## Этап 4 (Недели 5–6, веха M3). Выбор меша, перетаскивание, режим 2D

**Общее описание задач этапа.** Выбор меша ссылкой, перетаскивание модели в
сцену и ортографический режим. Две фичи.

### feature/mesh-picker

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public List<MeshOption> AvailableMeshes(string current)`
  Что делает: собирает список мешей (примитивы + модели из `assets://Models`). Параметры: `current` — текущее значение. Возвращает: варианты для выпадающего списка.
- `public ulong CreateModel(string name, string meshRef)` — создаёт объект-модель. Возвращает: id объекта.
- `class MeshOption { public string Display; public string Value; }`.

### feature/dragdrop-2d

#### Файлы `editor/avalonia/Views/ProjectView.axaml.cs`, `Views/GameView.axaml.cs`
- перетаскивание модели из панели проекта в сцену (`DataObject` с ссылкой на ассет).
- ортографический режим 2D и угловой гизмо переключения проекций (в `VulkanViewport`).
- P/Invoke: `sky_editor_create_mesh_object`, `sky_editor_set_view_2d`, `sky_editor_look_along_axis`.

**На выходе:** поле меша выбирается из списка ассетов; модель добавляется перетаскиванием; есть режим 2D.
**Критерий правильности этапа:** модель из панели проекта появляется в сцене;
переключение 3D/2D работает.

---

## Этап 5 (Недели 7–8, веха M4). Выбор класса скрипта и его поля

**Общее описание задач этапа.** Выбор класса скрипта из списка и редактирование
его полей в инспекторе. Одна фича.

### feature/script-inspector

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public List<string> ScriptClasses(string current)`
  Что делает: получает через C-интерфейс имена классов-скриптов. Параметры: `current`. Возвращает: список классов для выпадающего списка.
- `public void SetScriptField(ulong id, int component, int field, string value)` — записывает поле скрипта.
- `ComponentField.IsScriptClass` — поле `class` рендерится выпадающим списком; сериализуемые поля скрипта добавляются к списку у компонента `sky.script`.

#### Дополнение файла `editor/avalonia/Engine/EngineInterop.cs`
P/Invoke: `sky_editor_script_class_count`, `sky_editor_script_class_name`,
`sky_editor_script_field_count`, `sky_editor_script_field_name`,
`sky_editor_script_field_value`, `sky_editor_set_script_field`.

**На выходе:** класс скрипта выбирается дропдауном; поля скрипта редактируются в инспекторе.
**Критерий правильности этапа:** дропдаун показывает классы-наследники
ScriptComponent; правка поля скрипта применяется к инстансу в Play.
