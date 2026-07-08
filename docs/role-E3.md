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
  Реализация: `<OutputType>Exe`, `TargetFramework net8.0`, `Nullable enable`, `AvaloniaUseCompiledBindingsByDefault true`, `AllowUnsafeBlocks true`; `PackageReference` на Avalonia 11.2.1 (Avalonia, .Desktop, .Themes.Fluent, .Fonts.Inter, .Headless) и Dock.Avalonia / Dock.Model.Mvvm 11.2.0.
- `static int Main(string[] args)` — точка входа; ветка `--screenshot` (headless). Возвращает: код выхода.
  Реализация: `Array.IndexOf(args, "--screenshot")`; если флаг и путь есть — вызывает приватный `Screenshot(path, есть ли "--demo")` (конфигурирует `AppBuilder<App>().UseSkia().UseHeadless(...)`, создаёт `MainWindow`, показывает, гоняет `Dispatcher.UIThread.RunJobs()`, при `--demo` создаёт куб и двигает его, крутит кадры `viewport.RenderOnce()`, сохраняет `window.CaptureRenderedFrame()`); иначе `BuildAvaloniaApp().StartWithClassicDesktopLifetime(args)`. `BuildAvaloniaApp` = `AppBuilder.Configure<App>().UsePlatformDetect().WithInterFont().LogToTrace()`.
- `App` — приложение Avalonia; `OnFrameworkInitializationCompleted()` открывает `MainWindow`.
  Реализация: `Initialize()` грузит XAML через `AvaloniaXamlLoader.Load(this)`; `OnFrameworkInitializationCompleted()` при `IClassicDesktopStyleApplicationLifetime desktop` присваивает `desktop.MainWindow = new MainWindow()` и вызывает `base`.
- `MainWindow` — окно «Sky Engine», меню (File/Edit/GameObject), обработчики пунктов.
  Реализация: конструктор создаёт `MainViewModel _vm`, ставит `DataContext`, находит `DockControl`, создаёт `DockFactory(_vm)` и `ResetLayout()`; вешает `OnKeyDown` (хоткеи Delete/Ctrl+D/N/O/S/Z/Y и Q/W/E/R, но не при фокусе в `TextBox`) и `DispatcherTimer` 200 мс → `UpdateTransport()` (подсветка `playButton`/`pauseButton` классом `on` по `sky_editor_play_state`). Пункты меню — обёртки над `_vm` (`NewScene`/`OpenScene`/`SaveScene`/`Undo`/`Redo`/`CreateCube`/`DuplicateSelected`/`DeleteSelected`/`Play`/`Pause`/`Stop`) и `SelectTool` (держит четыре тогла взаимоисключающими).

### feature/docking-layout

#### Файлы `editor/avalonia/Docking/DockFactory.cs`, `Docking/Tools.cs`, `Views/PlaceholderView.axaml.cs`
- `class DockFactory` — `CreateLayout()` собирает раскладку из панелей (Hierarchy/Scene/Inspector/…).
  Реализация: наследник `Dock.Avalonia Factory`; `CreateLayout()` создаёт инструменты (`HierarchyTool`, `SceneDocument`, `GameDocument`, `InspectorTool`, `TerrainTool`, `MaterialsTool`, `ProjectTool`, `ConsoleTool`, `PackagesTool`) с общим `Main = _main`, раскладывает их в `ToolDock` (Left 0.2, Right 0.26, Bottom 0.28) и `DocumentDock` (Scene+Game), собирает горизонтальный `ProportionalDock` centre и вертикальный workspace со `ProportionalDockSplitter`, оборачивает в `CreateRootDock()`. `InitLayout()` заполняет `DockableLocator` (Root) и `HostWindowLocator` (`new HostWindow()`), чтобы вкладки отрывались в плавающие окна.
- `Tools.cs` — классы-инструменты (по одному на панель): `HierarchyTool`, `InspectorTool`, `SceneDocument`, …
  Реализация: базовые `EditorTool : Tool` и `EditorDocument : Document` с полем `MainViewModel? Main`; конкретные пустые `sealed`-подклассы (`HierarchyTool`, `InspectorTool`, `MaterialsTool`, `TerrainTool`, `ProjectTool`, `ConsoleTool`, `PackagesTool`, `SceneDocument`); `GameDocument` с `Caption`; `PlaceholderTool` с `Caption`.
- `PlaceholderView` — пустая панель-заглушка.
  Реализация: `UserControl`, конструктор только `AvaloniaXamlLoader.Load(this)`.

### feature/engine-bridge

#### Файлы `editor/avalonia/Engine/EngineInterop.cs`, `Engine/EditorSession.cs`
- `static class EngineInterop` — резолвер нативной библиотеки и P/Invoke-объявления. На этом этапе:
  - `[DllImport] static extern IntPtr sky_editor_create()` — Возвращает: указатель на сессию движка.
    Реализация: `[DllImport(Lib)] public static extern`, где `Lib = "sky_editor_bridge"`; вызывается из конструктора `EditorSession`.
  - `[DllImport] static extern void sky_editor_destroy(IntPtr ctx)` — уничтожает сессию.
    Реализация: `[DllImport(Lib)] public static extern`; вызывается из `EditorSession.Dispose()`.
  - `static IntPtr Resolve(...)`, `static string[] Candidates()` — находят `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в дереве сборки.
    Реализация: статический конструктор регистрирует `NativeLibrary.SetDllImportResolver`; `Resolve` для имени `Lib` перебирает `Candidates()`, проверяя `File.Exists` + `NativeLibrary.TryLoad`; `Candidates()` подбирает имя по ОС (`.so`/`.dll`/`.dylib`), берёт `SKY_BRIDGE_PATH`, путь рядом со сборкой и относительный `build/editor/native_bridge/<file>`.
- `class EditorSession : IDisposable` — обёртка над сессией: конструктор вызывает `sky_editor_create` и проверяет не-null; `Dispose()` → `destroy`; свойство `IntPtr Native`.
  Реализация: поле `IntPtr _ctx`; конструктор `_ctx = EngineInterop.sky_editor_create()`, при `IntPtr.Zero` бросает `InvalidOperationException`, затем `Reload()`; `Native => _ctx`; `Dispose()` при не-нулевом `_ctx` вызывает `sky_editor_destroy(_ctx)` и обнуляет `_ctx`.

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
  Реализация: поля `IntPtr _context`, `WriteableBitmap? _bitmap`, `byte[]? _buffer`, `PixelSize _size`, `DispatcherTimer? _timer`; в `OnAttachedToVisualTree` создаёт таймер 16 мс с `Tick => RenderFrame()` и `Start()`, в `OnDetachedFromVisualTree` — `Stop()`; конструктор ставит `ClipToBounds = true`.
  - `public void SetContext(IntPtr context)` — привязывает сессию движка. Параметры: `context`.
    Реализация: `=> _context = context;` — сохраняет нативный хэндл сессии в поле `_context`, откуда его берут `RenderFrame` и обработчики ввода.
  - `public void RenderOnce()` — рисует один кадр (дёргает нативный offscreen-рендер и блитит пиксели).
    Реализация: `=> RenderFrame();`. `RenderFrame` считает размер с учётом `RenderScaling`, при изменении пересоздаёт `WriteableBitmap` (`Rgba8888`, `Opaque`) и `_buffer`; вызывает `sky_editor_render_offscreen` (или `sky_editor_render_game_offscreen` при `GameView`); при коде 1 через `_bitmap.Lock()` копирует `_buffer` `Marshal.Copy` (построчно, если `RowBytes` не совпал) и `InvalidateVisual()`; при провале после 3 кадров рисует сообщение об отсутствии Vulkan.
- `Views/SceneView.axaml.cs` — хостит `VulkanViewport`.
  Реализация: находит `VulkanViewport "Viewport"`; `Bind()` берёт `Main` из `EditorDocument`, вызывает `SetContext(vm.NativeContext)`, подписывает `ObjectPicked → vm.SelectById`, `TransformChanged → vm.ReloadTransform`, `ProjectionToggled → SyncViewToggles`, синхронизирует `SelectedId`/`Tool` (в т.ч. по `vm.PropertyChanged`); `DispatcherTimer` 200 мс → `UpdatePlayBadge()` по `sky_editor_play_state`; `OnSpaceToggle`/`OnView2D`/`OnView3D` переключают `LocalSpace` и проекцию.

### feature/hierarchy-tree

#### Дополнение файла `editor/avalonia/Engine/EngineInterop.cs`
P/Invoke иерархии и трансформа (реализация — в мосте контура E5):
`sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`,
`sky_editor_child_at`, `sky_editor_object_name`, `sky_editor_get_transform`,
`sky_editor_render_offscreen`, `sky_editor_viewport_orbit`, `sky_editor_viewport_zoom`.
  Реализация: в `EngineInterop.cs` это `[DllImport(Lib)] public static extern`-объявления (строковые буферы читаются через хелпер `ReadString`, принимающий `(byte[] buffer, int capacity)` и декодирующий UTF-8); сама нативная реализация функций живёт в мосте контура E5 — в managed-коде только сигнатуры.

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public void Reload()` — перечитывает иерархию через C-интерфейс в `ObservableCollection<SkyObject> Roots`.
  Реализация: `Roots.Clear()`; `count = sky_editor_root_count(_ctx)`; цикл `Roots.Add(Load(sky_editor_root_at(_ctx, i)))`. Вызывается после каждой мутации сцены (create/duplicate/delete/rename/undo/redo/open/new).
- `private SkyObject Load(ulong id)` — рекурсивно строит узел дерева.
  Реализация: имя через `ReadString(sky_editor_object_name)`, создаёт `SkyObject(id, name)`; цикл `sky_editor_component_count`/`_component_type` добавляет `SkyComponent(typeId)`; цикл `sky_editor_child_count`/`_child_at` рекурсивно `Load` детей в `node.Children`.
- `public (float[] position, float[] rotation, float[] scale) Transform(ulong id)`
  Что делает: читает трансформ объекта. Параметры: `id`. Возвращает: позицию (3), кватернион (4), масштаб (3).
  Реализация: выделяет `float[3]`/`float[4]`/`float[3]`, вызывает `sky_editor_get_transform(_ctx, id, position, rotation, scale)` и возвращает кортеж.
- `Views/HierarchyView.axaml.cs` — `TreeView` по `Roots`.
  Реализация: `UserControl` (`TreeView` в XAML привязан к `Roots`); включает `DragDrop.SetAllowDrop`, обработчики `OnDragOver` (эффект Copy для `ProjectView.AssetRefFormat`) и `OnDrop` (модель → `Vm.CreateModelFromAsset`, `.skyprefab` → `Vm.InstantiatePrefabFromAsset`); `OnCreate → Vm.CreateCube`.

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
  Реализация: авто-свойства `{ get; set; }`: `SelectedId` (0 = нет выбора) — объект, для которого рисуется гизмо; `LocalSpace` — оси по ориентации объекта (иначе мировые); `Tool` типа `GizmoTool` (по умолчанию `Move`). Ставятся из `SceneView.Bind()` по состоянию `MainViewModel`.
- `DrawMoveGizmo`, `DrawRotateGizmo`, `DrawScaleGizmo`, `DrawSceneGizmo` — рисование манипуляторов поверх кадра.
  Реализация: `Render()` вызывает `DrawGizmo` (`switch` по `Tool`) и `DrawSceneGizmo`. `GizmoAxes` проецирует начало и концы осей через `sky_editor_get_world_transform` + `sky_editor_project`, оси мировые или повёрнутые кватернионом (`RotateByQuat`) при `LocalSpace`/`Scale`. `DrawMoveGizmo` — 3 линии+точки цветами осей (`#F0626E`/`#62C76E`/`#5A93F8`); `DrawScaleGizmo` — линии+квадраты по концам и центральный бокс равномерного масштаба; `DrawRotateGizmo` — три спроецированных кольца (`RingPoints`); `DrawSceneGizmo` — угловой кубик из 6 конусов по базису камеры (`sky_editor_camera_basis`) с подписью проекции (`sky_editor_view_2d`).
- `OnPointerPressed/Moved/Released` — драг осей (перемещение/поворот/масштаб через C-интерфейс).
  Реализация: `OnPointerPressed` сначала пробует `SceneGizmoClick` (снап камеры `sky_editor_look_along_axis` / переключение `sky_editor_set_view_2d`), затем `BeginGizmoDrag` (захват ближней оси перемещения/масштаба или кольца поворота), иначе орбита/пан. `OnPointerMoved`: при активном драге Move → `sky_editor_set_world_position`, Scale → `sky_editor_set_scale`, Rotate → `sky_editor_rotate_world_axis`, каждый раз `TransformChanged?.Invoke()`; иначе камера `sky_editor_viewport_orbit`/`sky_editor_viewport_pan`. `OnPointerReleased` завершает драг и вызывает `sky_editor_commit_edit` (один undo на драг); одиночный клик без движения → `sky_editor_pick` и `ObjectPicked?.Invoke(id)`. `OnPointerWheelChanged → sky_editor_viewport_zoom`.

#### Файл `editor/avalonia/MainViewModel.cs`
- `enum GizmoTool { Hand, Move, Rotate, Scale }`; свойство `public GizmoTool Tool` (хоткеи Q/W/E/R).
  Реализация: `enum GizmoTool { Hand, Move, Rotate, Scale }`; поле `_tool = GizmoTool.Move`, свойство `Tool` с проверкой на изменение и `OnPropertyChanged()`; хоткеи Q/W/E/R обрабатываются в `MainWindow.OnKeyDown` → `SelectTool`, который пишет `_vm.Tool` и синхронизирует тоглы тулбара.
- `public void CreateCube()`, `public void Undo()`, `public void Redo()`, `DuplicateSelected()`, `DeleteSelected()`.
  Реализация: `CreateCube → SelectById(_session.CreateCube("Cube"))`; `DuplicateSelected` при выборе → `SelectById(_session.Duplicate(id))`; `DeleteSelected → _session.Delete(id)` и выбор `Roots[0]`; `Undo`/`Redo → _session.Undo()/Redo()` (те вызывают `sky_editor_undo`/`redo` и `Reload`) и `ReselectAfterHistory()` (восстанавливает выбор по id).
- свойства трансформа `PositionX/Y/Z`, `RotationX/Y/Z`, `ScaleX/Y/Z` (двусторонние).
  Реализация: геттеры возвращают `Fmt(_pos/_rot/_scale[axis])` (инвариантный формат `0.###`); сеттеры парсят инвариантно (`TryParse`), пишут `_session.SetPosition`/`SetLocalEuler`/`SetScale`, затем `_session.CommitEdit()` (один undo на правку поля) и `OnPropertyChanged`. `_rot` хранится в углах Эйлера — при выборе объекта `LoadSelection`/`ReloadTransform` читают `sky_editor_get_transform` и конвертируют кватернион `QuatToEuler`.

### feature/inspector

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public List<ComponentView> ReadComponents(ulong id)` — Возвращает: компоненты объекта с их полями.
  Реализация: цикл по `sky_editor_component_count`; тип и отображаемое имя через `ReadString(sky_editor_component_type / _component_display_name)`; поля через `sky_editor_component_field_count`/`_name`/`_type`/`_value` → `ComponentField`; для `sky.script` дополнительно добавляет сериализуемые поля скрипта (`sky_editor_script_field_*`) с `scriptParam: true`.
- `public void SetComponentField(ulong id, int component, int field, string value)` — записывает поле через C-интерфейс.
  Реализация: `=> EngineInterop.sky_editor_set_component_field(_ctx, id, component, field, value);` (ABI с `CharSet.Ansi`).
- `public List<ComponentType> AvailableTypes()` — Возвращает: типы для меню Add Component.
  Реализация: цикл по `sky_editor_available_type_count`; для каждого через `ReadString` читает id/name/category (`sky_editor_available_type_id`/`_name`/`_category`) и, если id не пуст, добавляет `ComponentType(id, name, cat)`.
- `public void AddComponent(ulong id, string typeId)` / `public void RemoveComponent(ulong id, int component)`.
  Реализация: обёртки — `AddComponent → sky_editor_add_component(_ctx, id, typeId)`; `RemoveComponent → sky_editor_remove_component(_ctx, id, component)`.
- классы `ComponentView`, `ComponentField : INotifyPropertyChanged` (свойства `IsScalar/IsBool/IsVec3/IsMeshRef`, `Value`, `X/Y/Z`, `BoolValue`).
  Реализация: `ComponentView` держит `Index/TypeId/DisplayName/Category`, `Glyph` (по TypeId) и `List<ComponentField> Fields`. `ComponentField` хранит `_session/_object/_component/_field/_value` и презентационные флаги, выведенные из `Type`/`Name`: `IsVec3 (Type=="Vec3")`, `IsBool (Type=="bool")`, `IsMeshRef (Name=="mesh")`, `IsScalar` (остальное); сеттер `Value` при изменении пишет через `_session.SetComponentField` (или `SetScriptField` при `_scriptParam`) и `Raise`; `X/Y/Z` разбирают/собирают строку `"x, y, z"`; `BoolValue` — `"true"/"false"`.
- `Views/InspectorView.axaml.cs` — карточки компонентов, шаблоны полей.
  Реализация: `UserControl`; `OnRemoveComponent` берёт `ComponentView` из `DataContext` и вызывает `Vm.RemoveComponent(view.Index)`; `OnAddComponentSelected` берёт `ComponentType` из `ListBox`, сбрасывает `SelectedItem`, скрывает `Flyout` кнопки `AddComponentButton` и вызывает `Vm.AddComponent(type.TypeId)`. Карточки и шаблоны полей (скаляр/Vec3/bool/mesh/class) задаются в `InspectorView.axaml` по флагам `ComponentField`.

### feature/data-panels

#### Файлы `editor/avalonia/Views/{ConsoleView,MaterialsView,PackagesView}.axaml.cs`
Панели консоли (лог с цветом по уровню), материалов и пакетов — на живых данных
через соответствующие P/Invoke.
  Реализация: `ConsoleView` — `DispatcherTimer` 400 мс → `Poll()` вызывает `Vm.RefreshConsole()` (тот через `sky_editor_log_count`/`_level`/`_text` перечитывает лог в `ObservableCollection<LogLine> ConsoleLog`, цвет строки — `LogLine.Color` по уровню), автоскролл `ScrollToEnd`; `OnClear → Vm.ClearConsole()`. `MaterialsView` — `UserControl.Load`, данные из `Vm.Materials` (`ReadMaterials` читает `sky_editor_material_*`, `RefreshSwatch` строит цвет по `baseColor`), поля `MaterialField.Value` пишут `sky_editor_set_material_field`. `PackagesView` — `OnRefresh → RediscoverPackages` (`sky_editor_package_refresh`), `OnToggle → TogglePackage` (`sky_editor_package_set_active`), `OnInstall → InstallPackage` (`sky_editor_package_install`); таблица из `ReadPackages` (`sky_editor_package_count`/`_info`/`_active`).

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
  Реализация: стартует с `None/Cube/Plane/Sphere`; затем `ListProject("assets://Models")` (через `sky_editor_vfs_count`/`_vfs_entry`) фильтрует файлы с расширениями `.obj/.fbx/.gltf/.glb` → `MeshOption(MeshDisplayName(value), value="assets://Models/"+name)`; если `current` непуст и его нет в списке — добавляет его, чтобы поле не потеряло значение.
- `public ulong CreateModel(string name, string meshRef)` — создаёт объект-модель. Возвращает: id объекта.
  Реализация: `id = sky_editor_create_mesh_object(_ctx, name, meshRef)`; затем `Reload()`; возвращает `id`.
- `class MeshOption { public string Display; public string Value; }`.
  Реализация: `sealed`-класс с `Display`/`Value` (конструктор `(display, value)`), `ToString() => Display` — так `ComboBox` показывает дружелюбное имя, а хранит ссылку.

### feature/dragdrop-2d

#### Файлы `editor/avalonia/Views/ProjectView.axaml.cs`, `Views/GameView.axaml.cs`
- перетаскивание модели из панели проекта в сцену (`DataObject` с ссылкой на ассет).
  Реализация: `ProjectView.OnPointerPressed` запоминает `_pressPoint` и `_pressEntry`; `OnPointerMoved` при зажатой ЛКМ и сдвиге >5 px, если это модель/prefab (`Main.IsModelAsset`/`IsPrefabAsset`), формирует `DataObject`, кладёт `AssetRefFormat` = `Main.AssetRefFor(entry)` и запускает `DragDrop.DoDragDrop(e, data, Copy)`. Приём — в `HierarchyView.OnDrop` (модель → `CreateModelFromAsset`, `.skyprefab` → `InstantiatePrefabFromAsset`). `GameView.axaml.cs` при этом хостит второй `VulkanViewport` (`GameView=true`) и прокидывает клавиши в play-режим через `sky_editor_set_key_state` (`MapKey`, освобождение зажатых клавиш при потере фокуса).
- ортографический режим 2D и угловой гизмо переключения проекций (в `VulkanViewport`).
  Реализация: `SceneView.OnView2D`/`OnView3D → SetView2D → Vm.SetView2D → sky_editor_set_view_2d`, тоглы `view3D`/`view2D` держатся взаимоисключающими; в `VulkanViewport.SceneGizmoClick` клик по подписи проекции переключает `sky_editor_set_view_2d` и поднимает `ProjectionToggled`, клик по конусу — `sky_editor_look_along_axis`; `SceneView.SyncViewToggles` читает `sky_editor_view_2d` и обновляет тоглы.
- P/Invoke: `sky_editor_create_mesh_object`, `sky_editor_set_view_2d`, `sky_editor_look_along_axis`.
  Реализация: `[DllImport(Lib)] public static extern`-объявления в `EngineInterop.cs` (у `create_mesh_object` — `CharSet.Ansi`); нативная реализация — в мосте (E5/E1), managed зовёт их из `EditorSession`/`VulkanViewport`/`MainViewModel`.

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
  Реализация: цикл по `sky_editor_script_class_count`; каждое имя через `ReadString(sky_editor_script_class_name)`, непустые добавляются в список; если `current` непуст и его нет — добавляется в конец (значение поля не теряется, даже если сборка скрипта пропала).
- `public void SetScriptField(ulong id, int component, int field, string value)` — записывает поле скрипта.
  Реализация: `=> EngineInterop.sky_editor_set_script_field(_ctx, id, component, field, value);` — вызывается из сеттера `ComponentField.Value`, когда `_scriptParam == true`.
- `ComponentField.IsScriptClass` — поле `class` рендерится выпадающим списком; сериализуемые поля скрипта добавляются к списку у компонента `sky.script`.
  Реализация: `IsScriptClass => !_scriptParam && Name == "class"`; при нём UI биндится на `ScriptClassOptions` (=`_session.ScriptClasses(_value)`) и `SelectedScriptClass` (пишет `Value` при выборе). Сами сериализуемые поля скрипта добавляет `ReadComponents`: для `sky.script` дочитывает `sky_editor_script_field_count`/`_name`/`_type`/`_value` и создаёт `ComponentField` с `scriptParam: true`.

#### Дополнение файла `editor/avalonia/Engine/EngineInterop.cs`
P/Invoke: `sky_editor_script_class_count`, `sky_editor_script_class_name`,
`sky_editor_script_field_count`, `sky_editor_script_field_name`,
`sky_editor_script_field_value`, `sky_editor_set_script_field`.
  Реализация: `[DllImport(Lib)] public static extern`-объявления в `EngineInterop.cs` (строки читаются через `ReadString`); нативная реализация — в мосте (E5), managed вызывает их из `EditorSession.ScriptClasses`/`ReadComponents`/`SetScriptField`.

**На выходе:** класс скрипта выбирается дропдауном; поля скрипта редактируются в инспекторе.
**Критерий правильности этапа:** дропдаун показывает классы-наследники
ScriptComponent; правка поля скрипта применяется к инстансу в Play.
