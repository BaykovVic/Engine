# Техническое задание · Контур E3 «Редактор (.NET)»

**Область ответственности.** Пользовательский интерфейс редактора на Avalonia: окно, компоновка панелей, вьюпорт, гизмо, инспектор. Связь с движком через P/Invoke.

Все пути и имена методов взяты из фактического репозитория и совпадают с проектом 1:1. «Сделай файл» — файл создаётся на этом этапе; «Дополни файл» — в существующий файл добавляются перечисленные методы.


---

## Этап 1 (Неделя 1). Каркас, компоновка панелей, связь с движком

**Общее описание задач контура.**

Создать проект редактора, компоновку панелей-заглушек и первичный вызов движка через P/Invoke.

- **Сделай файл** `editor/avalonia/SkyEditor.csproj`
  (данные/разметка — функций нет)
- **Сделай файл** `editor/avalonia/Program.cs`
  В файле должны быть: `BuildAvaloniaApp`, `Main`, `Screenshot`, `MainWindow`, `FindByName`, `Program`, `picker`
- **Сделай файл** `editor/avalonia/App.axaml.cs`
  В файле должны быть: `Initialize`, `OnFrameworkInitializationCompleted`, `MainWindow`, `App`
- **Сделай файл** `editor/avalonia/MainWindow.axaml.cs`
  В файле должны быть: `MainWindow`, `DockFactory`, `UpdateTransport`, `SetClass`, `ResetLayout`, `OnResetLayout`, `OnNewScene`, `OnOpenScene`, `OnSaveScene`, `OnSaveSceneAs`, `SaveAs`, `OnQuit`, `OnUndo`, `OnRedo`, `OnCreateCube`, `OnDuplicate`, `OnDelete`, `OnSaveAsPrefab`, `OnPlay`, `OnPause`, `OnStop`, `OnToolHand`, `OnToolMove`, `OnToolRotate`, `OnToolScale`, `SelectTool`, `OnKeyDown`
- **Сделай файл** `editor/avalonia/Docking/DockFactory.cs`
  В файле должны быть: `DockFactory`, `CreateLayout`, `ProportionalDockSplitter`, `InitLayout`, `HostWindow`
- **Сделай файл** `editor/avalonia/Docking/Tools.cs`
  В файле должны быть: `EditorTool`, `EditorDocument`, `HierarchyTool`, `InspectorTool`, `MaterialsTool`, `TerrainTool`, `ProjectTool`, `ConsoleTool`, `PackagesTool`, `SceneDocument`, `GameDocument`, `PlaceholderTool`
- **Сделай файл** `editor/avalonia/Views/PlaceholderView.axaml.cs`
  В файле должны быть: `PlaceholderView`
- **Дополни файл** `editor/avalonia/Engine/EngineInterop.cs`
  В файле должны быть: `sky_editor_create`, `sky_editor_destroy`, `Resolve`, `Candidates`
- **Дополни файл** `editor/avalonia/Engine/EditorSession.cs`
  В файле должны быть: `EditorSession`, `Dispose`

**На выходе должно получиться (список артефактов):**
- `dotnet build editor/avalonia` без ошибок; окно открывается; панели перетаскиваются.

**Критерий правильности:** `dotnet build` = 0 ошибок; сессия движка создаётся из .NET (не-null); есть сброс компоновки.


---

## Этап 2 (Неделя 2). Панель вьюпорта и живое дерево объектов

**Общее описание задач контура.**

Реализовать панель вьюпорта (кадр движка) и дерево объектов на живых данных.

- **Сделай файл** `editor/avalonia/Controls/VulkanViewport.cs`
  В файле должны быть: `VulkanViewport`, `SetContext`, `RenderOnce`, `OnAttachedToVisualTree`, `OnDetachedFromVisualTree`, `RenderFrame`, `PixelSize`, `WriteableBitmap`, `Vector`, `Render`, `Rect`, `FormattedText`, `Point`, `SceneGizmoGeometry`, `DrawSceneGizmo`, `Pen`, `SceneGizmoClick`, `DrawGizmo`, `GizmoAxes`, `DrawMoveGizmo`, `DrawScaleGizmo`, `RingPoints`, `DrawRotateGizmo`, `DistanceToSegment`, `BeginGizmoDrag`, `OnPointerPressed`, `OnPointerMoved`, `OnPointerReleased`, `OnPointerWheelChanged`
- **Сделай файл** `editor/avalonia/Views/SceneView.axaml.cs`
  В файле должны быть: `SceneView`, `UpdatePlayBadge`, `Bind`, `OnSpaceToggle`, `OnView3D`, `OnView2D`, `SetView2D`, `SyncViewToggles`
- **Сделай файл** `editor/avalonia/Views/HierarchyView.axaml.cs`
  В файле должны быть: `HierarchyView`, `OnCreate`, `OnDragOver`, `OnDrop`
- **Дополни файл** `editor/avalonia/Engine/EngineInterop.cs`
  В файле должны быть: `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`, `sky_editor_child_at`, `sky_editor_object_name`, `sky_editor_get_transform`, `sky_editor_render_offscreen`, `sky_editor_viewport_orbit`, `sky_editor_viewport_zoom`
- **Дополни файл** `editor/avalonia/Engine/EditorSession.cs`
  В файле должны быть: `Reload`, `Load`, `Transform`

**На выходе должно получиться (список артефактов):**
- Демо-сцена видна в панели вьюпорта; дерево объектов отражает сцену.

**Критерий правильности:** Панель вьюпорта показывает демо-сцену; в дереве — имена и вложенность объектов движка.


---

## Этап 3 (Недели 3–4). Гизмо, инспектор, панели данных

**Общее описание задач контура.**

Реализовать манипуляторы, инспектор полей компонентов и панели консоли/материалов/пакетов.

- **Сделай файл** `editor/avalonia/MainViewModel.cs`
  В файле должны быть: `MainViewModel`, `EditorSession`, `RefreshMaterials`, `GenerateTerrain`, `RefreshConsole`, `ClearConsole`, `RefreshPackages`, `InstallPackage`, `TogglePackage`, `RediscoverPackages`, `IsNullOrEmpty`, `NewScene`, `SaveScene`, `OpenScene`, `LoadSelection`, `SelectById`, `ReloadTransform`, `CreateCube`, `DuplicateSelected`, `DeleteSelected`, `AvailableTypes`, `AddComponent`, `RemoveComponent`, `RefreshComponents`, `SetPos`, `SetRot`, `SetScale`, `CommitEdit`, `Undo`, `Redo`, `ReselectAfterHistory`, `sky_editor_play_state`, `Play`, `Pause`, `Stop`, `SetView2D`, `CurrentVfsDir`, `RefreshProject`, `ProjectEntry`, `IsModelAsset`, `IsPrefabAsset`, `AssetRefFor`, `CreateModelFromAsset`, `InstantiatePrefabFromAsset`, `SaveSelectedAsPrefab`, `OpenProjectEntry`, `Find`, `FirstWithComponents`, `Fmt`, `OnPropertyChanged`, `GizmoTool`
- **Сделай файл** `editor/avalonia/Views/InspectorView.axaml.cs`
  В файле должны быть: `InspectorView`, `OnRemoveComponent`, `OnAddComponentSelected`
- **Сделай файл** `editor/avalonia/Views/ConsoleView.axaml.cs`
  В файле должны быть: `ConsoleView`, `Poll`, `OnClear`
- **Сделай файл** `editor/avalonia/Views/MaterialsView.axaml.cs`
  В файле должны быть: `MaterialsView`
- **Сделай файл** `editor/avalonia/Views/PackagesView.axaml.cs`
  В файле должны быть: `PackagesView`, `OnRefresh`, `OnToggle`, `OnInstall`
- **Дополни файл** `editor/avalonia/Engine/EditorSession.cs`
  В файле должны быть: `ReadComponents`, `SetComponentField`, `AvailableTypes`, `AddComponent`, `RemoveComponent`
- **Дополни файл** `editor/avalonia/Engine/EngineInterop.cs`
  В файле должны быть: `sky_editor_component_field_count`, `sky_editor_component_field_name`, `sky_editor_component_field_value`, `sky_editor_set_component_field`, `sky_editor_add_component`, `sky_editor_remove_component`, `sky_editor_undo`, `sky_editor_redo`

**На выходе должно получиться (список артефактов):**
- Гизмо move/rotate/scale работают; инспектор редактирует поля; панели на живых данных.

**Критерий правильности:** Изменение поля в инспекторе применяется к движку и отменяемо; активный инструмент синхронизирован.


---

## Этап 4 (Недели 5–6). Выбор меша, перетаскивание, режим 2D

**Общее описание задач контура.**

Реализовать выбор меша ссылкой, перетаскивание модели в сцену и ортографический режим.

- **Сделай файл** `editor/avalonia/Views/ProjectView.axaml.cs`
  В файле должны быть: `ProjectView`, `OnOpen`, `OnPointerPressed`, `OnPointerMoved`, `DataObject`
- **Сделай файл** `editor/avalonia/Views/GameView.axaml.cs`
  В файле должны быть: `GameView`, `OnKeyDown`, `OnKeyUp`, `OnLostFocus`, `SendKey`, `MapKey`
- **Дополни файл** `editor/avalonia/Engine/EditorSession.cs`
  В файле должны быть: `AvailableMeshes`, `CreateModel`, `MeshDisplayName`
- **Дополни файл** `editor/avalonia/Engine/EngineInterop.cs`
  В файле должны быть: `sky_editor_create_mesh_object`, `sky_editor_set_view_2d`, `sky_editor_look_along_axis`

**На выходе должно получиться (список артефактов):**
- Поле меша выбирается из списка ассетов; модель добавляется перетаскиванием; есть режим 2D.

**Критерий правильности:** Модель из панели проекта появляется в сцене; переключение 3D/2D работает.


---

## Этап 5 (Недели 7–8). Выбор класса скрипта и его поля

**Общее описание задач контура.**

Реализовать выбор класса скрипта из списка и редактирование его полей в инспекторе.

- **Дополни файл** `editor/avalonia/Engine/EditorSession.cs`
  В файле должны быть: `ScriptClasses`, `SetScriptField`
- **Дополни файл** `editor/avalonia/Engine/EngineInterop.cs`
  В файле должны быть: `sky_editor_script_class_count`, `sky_editor_script_class_name`, `sky_editor_script_field_count`, `sky_editor_script_field_name`, `sky_editor_script_field_value`, `sky_editor_set_script_field`

**На выходе должно получиться (список артефактов):**
- Класс скрипта выбирается дропдауном; поля скрипта редактируются в инспекторе.

**Критерий правильности:** Дропдаун показывает классы-наследники ScriptComponent; правка поля скрипта применяется к инстансу в Play.
