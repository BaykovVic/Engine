# Манифест файлов Sky Engine (сгенерирован из репозитория)

Список файлов, которые нужно создать, и объявленные в них функции, методы
и типы. Сгенерирован автоматически из фактических исходников — совпадает с
проектом 1:1. Формат: **создай файл** → **в файле должны быть** перечисленные
функции/методы/типы.

Всего файлов: **231**.


## editor/avalonia

**Создай файл** `editor/avalonia/App.axaml.cs`
В файле должны быть: `Initialize`, `OnFrameworkInitializationCompleted`, `MainWindow`, `App`

**Создай файл** `editor/avalonia/Controls/VulkanViewport.cs`
В файле должны быть: `readonly`, `VulkanViewport`, `SetContext`, `RenderOnce`, `OnAttachedToVisualTree`, `OnDetachedFromVisualTree`, `RenderFrame`, `PixelSize`, `WriteableBitmap`, `Vector`, `Render`, `Rect`, `FormattedText`, `Point`, `SceneGizmoGeometry`, `DrawSceneGizmo`, `Pen`, `SceneGizmoClick`, `DrawGizmo`, `GizmoAxes`, `DrawMoveGizmo`, `DrawScaleGizmo`, `RingPoints`, `DrawRotateGizmo`, `DistanceToSegment`, `BeginGizmoDrag`, `OnPointerPressed`, `OnPointerMoved`, `OnPointerReleased`, `OnPointerWheelChanged`

**Создай файл** `editor/avalonia/Converters/GlyphConverter.cs`
В файле должны быть: `Convert`, `ConvertBack`, `NotSupportedException`, `GlyphConverter`

**Создай файл** `editor/avalonia/Converters/HexBrushConverter.cs`
В файле должны быть: `Convert`, `ConvertBack`, `NotSupportedException`, `HexBrushConverter`

**Создай файл** `editor/avalonia/Docking/DockFactory.cs`
В файле должны быть: `DockFactory`, `CreateLayout`, `ProportionalDockSplitter`, `InitLayout`, `HostWindow`

**Создай файл** `editor/avalonia/Docking/Tools.cs`
В файле должны быть: `EditorTool`, `EditorDocument`, `HierarchyTool`, `InspectorTool`, `MaterialsTool`, `TerrainTool`, `ProjectTool`, `ConsoleTool`, `PackagesTool`, `SceneDocument`, `GameDocument`, `PlaceholderTool`

**Создай файл** `editor/avalonia/Engine/EditorSession.cs`
В файле должны быть: `SkyComponent`, `CategoryOf`, `ComponentView`, `ComponentField`, `Vec`, `SetVec`, `Raise`, `MeshOption`, `ToString`, `LogLine`, `PackageInfo`, `ComponentType`, `MaterialView`, `RefreshSwatch`, `MaterialField`, `ProjectEntry`, `SkyObject`, `EditorSession`, `Reload`, `Load`, `CreateCube`, `CreateModel`, `SavePrefab`, `InstantiatePrefab`, `ObjectEnabled`, `SetObjectEnabled`, `RenameObject`, `AvailableTypes`, `AddComponent`, `ScriptClasses`, `RemoveComponent`, `sky_editor_log_count`, `ClearLogs`, `ReadLogs`, `ReadPackages`, `SetPackageActive`, `RefreshPackages`, `InstallPackage`, `NewScene`, `SaveScene`, `OpenScene`, `CommitEdit`, `sky_editor_can_undo`, `sky_editor_can_redo`, `Undo`, `Redo`, `Duplicate`, `Delete`, `SetPosition`, `SetLocalEuler`, `SetScale`, `ReadComponents`, `SetComponentField`, `SetScriptField`, `ReadMaterials`, `SetMaterialField`, `GenerateTerrain`, `AvailableMeshes`, `MeshDisplayName`, `ListProject`, `Dispose`, `picker`

**Создай файл** `editor/avalonia/Engine/EngineInterop.cs`
В файле должны быть: `EngineInterop`, `Resolve`, `Candidates`, `sky_editor_create`, `sky_editor_destroy`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`, `sky_editor_child_at`, `sky_editor_object_name`, `sky_editor_object_exists`, `sky_editor_get_transform`, `sky_editor_set_position`, `sky_editor_set_scale`, `sky_editor_component_count`, `sky_editor_component_type`, `sky_editor_component_display_name`, `sky_editor_component_field_count`, `sky_editor_component_field_name`, `sky_editor_component_field_type`, `sky_editor_component_field_value`, `sky_editor_set_component_field`, `sky_editor_terrain_generate`, `sky_editor_material_count`, `sky_editor_material_name`, `sky_editor_material_field_count`, `sky_editor_material_field_name`, `sky_editor_material_field_value`, `sky_editor_set_material_field`, `sky_editor_vfs_count`, `sky_editor_vfs_entry`, `sky_editor_create_primitive`, `sky_editor_create_mesh_object`, `sky_editor_log_count`, `sky_editor_log_level`, `sky_editor_log_text`, `sky_editor_log_clear`, `sky_editor_package_count`, `sky_editor_package_info`, `sky_editor_package_active`, `sky_editor_package_set_active`, `sky_editor_package_refresh`, `sky_editor_package_install`, `sky_editor_set_object_enabled`, `sky_editor_object_enabled`, `sky_editor_rename_object`, `sky_editor_available_type_count`, `sky_editor_available_type_id`, `sky_editor_available_type_name`, `sky_editor_available_type_category`, `sky_editor_add_component`, `sky_editor_remove_component`, `sky_editor_new_scene`, `sky_editor_save_scene`, `sky_editor_open_scene`, `sky_editor_commit_edit`, `sky_editor_undo`, `sky_editor_redo`, `sky_editor_can_undo`, `sky_editor_can_redo`, `sky_editor_undo_label`, `sky_editor_redo_label`, `sky_editor_duplicate`, `sky_editor_delete`, `sky_editor_attach_viewport`, `sky_editor_render_viewport`, `sky_editor_detach_viewport`, `sky_editor_render_offscreen`, `sky_editor_render_game_offscreen`, `sky_editor_viewport_orbit`, `sky_editor_viewport_pan`, `sky_editor_viewport_zoom`, `sky_editor_pick`, `sky_editor_frame_object`, `sky_editor_project`, `sky_editor_world_position`, `sky_editor_camera_position`, `sky_editor_set_view_2d`, `sky_editor_view_2d`, `sky_editor_look_along_axis`, `sky_editor_camera_basis`, `sky_editor_get_world_transform`, `sky_editor_set_world_position`, `sky_editor_translate_self`, `sky_editor_set_local_euler`, `sky_editor_rotate_world_axis`, `sky_editor_play`, `sky_editor_pause`, `sky_editor_stop`, `sky_editor_play_state`, `sky_editor_tick_play`, `sky_editor_set_key_state`, `sky_editor_save_prefab`, `sky_editor_instantiate_prefab`, `sky_editor_script_class_count`, `sky_editor_script_class_name`, `sky_editor_script_field_count`, `sky_editor_script_field_name`, `sky_editor_script_field_type`, `sky_editor_script_field_value`, `sky_editor_set_script_field`, `ReadString`

**Создай файл** `editor/avalonia/MainViewModel.cs`
В файле должны быть: `MainViewModel`, `EditorSession`, `RefreshMaterials`, `GenerateTerrain`, `RefreshConsole`, `ClearConsole`, `RefreshPackages`, `InstallPackage`, `TogglePackage`, `RediscoverPackages`, `IsNullOrEmpty`, `NewScene`, `SaveScene`, `OpenScene`, `LoadSelection`, `SelectById`, `ReloadTransform`, `CreateCube`, `DuplicateSelected`, `DeleteSelected`, `AvailableTypes`, `AddComponent`, `RemoveComponent`, `RefreshComponents`, `SetPos`, `SetRot`, `SetScale`, `CommitEdit`, `Undo`, `Redo`, `ReselectAfterHistory`, `sky_editor_play_state`, `Play`, `Pause`, `Stop`, `SetView2D`, `CurrentVfsDir`, `RefreshProject`, `ProjectEntry`, `IsModelAsset`, `IsPrefabAsset`, `AssetRefFor`, `CreateModelFromAsset`, `InstantiatePrefabFromAsset`, `SaveSelectedAsPrefab`, `OpenProjectEntry`, `Find`, `FirstWithComponents`, `Fmt`, `OnPropertyChanged`, `GizmoTool`

**Создай файл** `editor/avalonia/MainWindow.axaml.cs`
В файле должны быть: `MainWindow`, `DockFactory`, `UpdateTransport`, `SetClass`, `ResetLayout`, `OnResetLayout`, `OnNewScene`, `OnOpenScene`, `OnSaveScene`, `OnSaveSceneAs`, `SaveAs`, `OnQuit`, `OnUndo`, `OnRedo`, `OnCreateCube`, `OnDuplicate`, `OnDelete`, `OnSaveAsPrefab`, `OnPlay`, `OnPause`, `OnStop`, `OnToolHand`, `OnToolMove`, `OnToolRotate`, `OnToolScale`, `SelectTool`, `OnKeyDown`

**Создай файл** `editor/avalonia/Program.cs`
В файле должны быть: `BuildAvaloniaApp`, `Main`, `Screenshot`, `MainWindow`, `FindByName`, `Program`, `picker`

**Создай файл** `editor/avalonia/Views/ConsoleView.axaml.cs`
В файле должны быть: `ConsoleView`, `Poll`, `OnClear`

**Создай файл** `editor/avalonia/Views/GameView.axaml.cs`
В файле должны быть: `GameView`, `OnKeyDown`, `OnKeyUp`, `OnLostFocus`, `SendKey`, `MapKey`

**Создай файл** `editor/avalonia/Views/HierarchyView.axaml.cs`
В файле должны быть: `HierarchyView`, `OnCreate`, `OnDragOver`, `OnDrop`

**Создай файл** `editor/avalonia/Views/InspectorView.axaml.cs`
В файле должны быть: `InspectorView`, `OnRemoveComponent`, `OnAddComponentSelected`

**Создай файл** `editor/avalonia/Views/MaterialsView.axaml.cs`
В файле должны быть: `MaterialsView`

**Создай файл** `editor/avalonia/Views/PackagesView.axaml.cs`
В файле должны быть: `PackagesView`, `OnRefresh`, `OnToggle`, `OnInstall`

**Создай файл** `editor/avalonia/Views/PlaceholderView.axaml.cs`
В файле должны быть: `PlaceholderView`

**Создай файл** `editor/avalonia/Views/ProjectView.axaml.cs`
В файле должны быть: `ProjectView`, `OnOpen`, `OnPointerPressed`, `OnPointerMoved`, `DataObject`

**Создай файл** `editor/avalonia/Views/SceneView.axaml.cs`
В файле должны быть: `SceneView`, `UpdatePlayBadge`, `Bind`, `OnSpaceToggle`, `OnView3D`, `OnView2D`, `SetView2D`, `SyncViewToggles`

**Создай файл** `editor/avalonia/Views/TerrainView.axaml.cs`
В файле должны быть: `TerrainView`, `OnGenerate`


## editor/native_bridge

**Создай файл** `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`
В файле должны быть: `sky_editor_create`, `sky_editor_destroy`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`, `sky_editor_component_display_name`, `sky_editor_component_field_count`, `sky_editor_component_field_name`, `sky_editor_component_field_type`, `sky_editor_component_field_value`, `sky_editor_set_component_field`, `sky_editor_terrain_generate`, `sky_editor_material_count`, `sky_editor_material_name`, `sky_editor_material_field_count`, `sky_editor_material_field_name`, `sky_editor_material_field_value`, `sky_editor_set_material_field`, `sky_editor_vfs_count`, `sky_editor_vfs_entry`, `sky_editor_available_type_count`, `sky_editor_reload_scripts`, `sky_editor_script_class_count`, `sky_editor_log_count`, `sky_editor_log_level`, `sky_editor_log_clear`, `sky_editor_package_count`, `sky_editor_package_active`, `sky_editor_package_refresh`, `sky_editor_new_scene`, `sky_editor_save_scene`, `sky_editor_open_scene`, `sky_editor_commit_edit`, `sky_editor_undo`, `sky_editor_redo`, `sky_editor_can_undo`, `sky_editor_can_redo`, `sky_editor_delete`, `sky_editor_detach_viewport`, `scene`, `sky_editor_viewport_zoom`, `sky_editor_set_view_2d`, `sky_editor_view_2d`, `sky_editor_look_along_axis`, `sky_editor_play`, `sky_editor_pause`, `sky_editor_stop`, `sky_editor_play_state`, `sky_editor_tick_play`

**Создай файл** `editor/native_bridge/src/editor_bridge.cpp`
В файле должны быть: `BridgeSession`, `XCloseDisplay`, `self`, `scriptClasses`, `scriptClassNames`, `commitTransform`, `exists`, `localTransform`, `beginTransformEdit`, `handle`, `componentsOf`, `size_t`, `fieldValueString`, `float`, `worldTransform`, `childrenOf`, `test`, `copyString`, `sky_editor_create`, `logMsg`, `sky_editor_root_count`, `sky_editor_root_at`, `ec`, `sky_editor_child_count`, `nameOf`, `sky_editor_object_enabled`, `makeRenameCommand`, `sky_editor_available_type_count`, `availableTypes`, `sky_editor_reload_scripts`, `sky_editor_script_class_count`, `componentAt`, `field`, `scriptFields`, `int64_t`, `scriptFieldsAt`, `parseScriptField`, `setField`, `attach`, `makeAddComponentCommand`, `descriptorOf`, `makeRemoveComponentCommand`, `detach`, `push`, `sky_editor_object_exists`, `setLocalTransform`, `sky_editor_component_count`, `int32_t`, `makeFieldCommand`, `materialFieldValue`, `materialFieldSet`, `sky_editor_terrain_generate`, `sky_editor_material_count`, `allMaterials`, `findMaterial`, `updateMaterial`, `sky_editor_vfs_count`, `list`, `makeCreateSnapshotCommand`, `sky_editor_new_scene`, `sky_editor_save_scene`, `sky_editor_open_scene`, `sky_editor_instantiate_prefab`, `sky_editor_duplicate`, `makeDuplicateCommand`, `sky_editor_delete`, `makeDeleteCommand`, `sky_editor_undo`, `undo`, `sky_editor_redo`, `redo`, `sky_editor_can_undo`, `canUndo`, `sky_editor_can_redo`, `canRedo`, `sky_editor_undo_label`, `undoLabel`, `sky_editor_redo_label`, `redoLabel`, `sky_editor_log_count`, `sky_editor_log_level`, `sky_editor_package_count`, `discoveredPackages`, `sky_editor_package_active`, `sky_editor_package_refresh`, `discoverPackages`, `package`, `sky_editor_package_install`, `XOpenDisplay`, `createVulkanRendererForWindow`, `submit`, `build`, `renderFrame`, `createVulkanRenderer`, `state`, `tickFrame`, `readbackFrame`, `setCamera`, `sky_editor_viewport_zoom`, `sky_editor_frame_object`, `sky_editor_camera_position`, `sky_editor_set_view_2d`, `sky_editor_view_2d`, `sky_editor_look_along_axis`, `fill`, `rotate`, `setWorldTransform`, `sqrt`, `axisAngle`, `sky_editor_play`, `setScene`, `play`, `sky_editor_stop`, `stop`, `sky_editor_play_state`, `sky_editor_tick_play`, `sky_editor_set_key_state`, `sky_editor_detach_viewport`


## editor/shell

**Создай файл** `editor/shell/include/sky/editor/shell/editor_shell.hpp`
В файле должны быть: `IEditorShell`, `openProject`, `closeProject`, `applyLayout`, `currentLayout`, `IEditorSession`, `current`, `hasOpenProject`

**Создай файл** `editor/shell/src/editor_camera.hpp`
В файле должны быть: `rotation`, `orthoHalfHeight`, `orthoHeight`, `lookAlong`, `pose`, `rotate`, `orbit`, `clamp`, `zoom`, `pan`

**Создай файл** `editor/shell/src/editor_commands.cpp`
В файле должны быть: `push`, `notify`, `undo`, `redo`, `apply`, `exists`, `setLocalTransform`, `RenameCommand`, `renameObject`, `DuplicateCommand`, `setField`, `attach`, `updateMaterial`, `MaterialCreateCommand`, `findMaterial`, `material`, `removeMaterial`, `createMaterial`, `parentOf`, `fields`, `makeMaterialCreateCommand`

**Создай файл** `editor/shell/src/editor_commands.hpp`
В файле должны быть: `IEditorCommand`, `label`, `undo`, `redo`, `UndoStack`, `push`, `notify`, `undoLabel`, `canUndo`, `redoLabel`, `canRedo`, `setOnChanged`, `onChanged_`, `makeMaterialCreateCommand`

**Создай файл** `editor/shell/src/editor_context.cpp`
В файле должны быть: `scriptSetLocalPosition`, `localTransform`, `setLocalTransform`, `scriptSetLocalEuler`, `axisAngle`, `scriptSetLocalScale`, `scriptLogMessage`, `scriptLog`, `scriptGetLocalPosition`, `scriptIsKeyDown`, `keyDown`, `scriptGetWorldPosition`, `worldTransform`, `scriptInstantiate`, `spawnPrefabAt`, `scriptDestroyObject`, `exists`, `destroyObject`, `scriptSetVelocity`, `scriptGetVelocity`, `objectVelocity`, `path`, `populateDemoAssets`, `ofstream`, `touch`, `remove`, `EditorContext`, `createStdFileSystem`, `createVirtualFileSystem`, `createInMemoryConfigService`, `set`, `createRendererRegistry`, `registerOpenGlBackend`, `createFileSerializationBackend`, `createObjectWorld`, `createComponentWorld`, `createEcsWorld`, `createEcsObjectSync`, `createPhysicsWorld`, `createObjectPhysicsSync`, `createSchemaMigrationService`, `createSceneWorld`, `createPlayModeController`, `current_path`, `mount`, `createDirectoryMount`, `createPackageWorld`, `savePackageManifest`, `discoverPackages`, `applyPackageLock`, `createAssetDatabase`, `createObjImporter`, `createFbxImporter`, `createGltfImporter`, `createPngImporter`, `registerImporter`, `createMaterialLibrary`, `createMaterial`, `writeAll`, `encodePngRgba`, `importAsset`, `initScripting`, `buildDemoScene`, `createEmpty`, `createObject`, `addRootObject`, `createPrimitive`, `attach`, `setField`, `attachCrateBody`, `bind`, `unbind`, `destroyBody`, `detachAllFrom`, `beginPlay`, `stack`, `childrenOf`, `reloadUserScripts`, `startPlayScripts`, `endPlay`, `stopPlayScripts`, `setBodyTransform`, `setBodyVelocity`, `scriptSourceDirs`, `manifest`, `unloadUserAssembly`, `csproj`, `failed`, `loadUserAssembly`, `newestUserScriptStamp`, `createDotNetScriptHost`, `start`, `installEngineApi`, `startScriptsFor`, `componentsOf`, `descriptorOf`, `field`, `createInstance`, `setInstanceObjectId`, `fields`, `invokeLifecycle`, `tickScripts`, `beginFrame`, `destroyInstance`, `duplicateObject`, `invalid`, `parentOf`, `cloneSubtree`, `nameOf`, `setParent`, `reparent`, `rotate`, `applyTerrainBrush`, `applyEdit`, `terrainHeightAt`, `generateTerrain`, `generate`, `writeSnapshot`, `readSnapshot`, `snapshotObject`, `resolveAssetPath`, `createDirectories`, `instantiatePrefab`, `readAll`, `reader`, `restoreObject`, `state`, `installPackage`, `installer`, `writePackageLock`, `setPackageActive`, `resolve`, `registerPackage`, `activate`, `deactivate`, `discoveredPackages`, `bodyVelocity`, `hasPhysicsBody`, `initTerrain`, `createTerrainWorld`, `createGenerationPipeline`, `createTerrain`, `makeTerrainCollider`, `dataset`, `onTerrainChanged`, `detachCollider`, `resetScene`, `unloadScene`, `reattachPhysics`, `newScene`, `createScene`, `saveScene`, `saveSceneAs`, `openScene`, `loadScene`, `rootObjectsOf`, `createCrate`, `objBytes`

**Создай файл** `editor/shell/src/editor_context.hpp`
В файле должны быть: `EditorContext`, `beginPlay`, `endPlay`, `newScene`, `saveScene`, `openScene`, `tickScripts`, `reloadUserScripts`, `setKeyDown`, `applyTerrainBrush`, `terrainHeightAt`, `generateTerrain`, `createEmpty`, `createCrate`, `destroyObject`, `duplicateObject`, `reparent`, `savePrefab`, `instantiatePrefab`, `spawnPrefabAt`, `setObjectVelocity`, `objectVelocity`, `installPackage`, `setPackageActive`, `packageActive`, `snapshotObject`, `hasPhysicsBody`, `setObjectEnabled`, `objectEnabled`, `rootObjects`, `buildDemoScene`, `initTerrain`, `resetScene`, `reattachPhysics`, `initScripting`, `scriptSourceDirs`, `applyPackageLock`, `writePackageLock`, `startPlayScripts`, `stopPlayScripts`, `startScriptsFor`, `attachCrateBody`

**Создай файл** `editor/shell/src/frame_builder.hpp`
В файле должны быть: `FrameBuilder`, `setCamera`, `cameraPose`, `forEachObject`, `componentOfType`, `worldTransform`, `dataset`, `applyMaterial`, `uploadedMesh`, `findByName`, `exists`, `childrenOf`, `walk`, `componentsOf`, `descriptorOf`, `invalid`, `field`, `findMaterial`, `material`, `uploadedTexture`, `resolve`, `resolveMeshRef`, `strip`, `buildPrimitive`, `path`, `loadFbxMesh`, `loadGltfMesh`, `loadObjMesh`, `push`, `vert`, `loadPngImage`

**Создай файл** `editor/shell/src/icons.cpp`
В файле должны быть: `strokeArrowHead`, `hypot`, `drawGlyph`, `pen`, `QPointF`, `box`, `renderGlyph`, `pixmap`, `painter`, `accentFor`, `QColor`, `toolbarIcon`

**Создай файл** `editor/shell/src/icons.hpp`
В файле должны быть: `toolbarIcon`

**Создай файл** `editor/shell/src/main.cpp`
В файле должны быть: `main`, `qstrcmp`, `fromLocal8Bit`, `qputenv`, `setDefaultFormat`, `app`, `applyDarkTheme`, `set`, `window`, `singleShot`, `processEvents`, `exit`, `exec`

**Создай файл** `editor/shell/src/main_window.cpp`
В файле должны быть: `MainWindow`, `context_`, `undoStack_`, `commandBus_`, `setWindowTitle`, `setDockOptions`, `SceneView3D`, `ViewportWidget`, `QStackedWidget`, `addWidget`, `QWidget`, `setObjectName`, `QHBoxLayout`, `setContentsMargins`, `setSpacing`, `QButtonGroup`, `setExclusive`, `QToolButton`, `setProperty`, `setText`, `setCheckable`, `setChecked`, `addButton`, `addModeButton`, `addStretch`, `setCurrentIndex`, `QVBoxLayout`, `QTabWidget`, `addTab`, `tr`, `setCentralWidget`, `buildDocks`, `buildMenus`, `buildToolbar`, `statusBar`, `showMessage`, `connect`, `onSelection`, `selectObject`, `refreshTransform`, `update`, `setScene`, `onStateChanged`, `syncPlayButtons`, `QTimer`, `setInterval`, `start`, `setUndoDelegate`, `performUndo`, `setRedoDelegate`, `performRedo`, `registerHandler`, `refresh`, `QString`, `logger`, `deleteObject`, `stoull`, `duplicateObject`, `fromStdString`, `availableBackends`, `selectObjectByName`, `findByName`, `playFrames`, `play`, `tickFrame`, `menuBar`, `addMenu`, `addAction`, `addSeparator`, `updateUndoActions`, `execute`, `toggleViewAction`, `addToolBar`, `setMovable`, `setIconSize`, `setIcon`, `setToolTip`, `tool`, `setTool`, `setSizePolicy`, `makeButton`, `state`, `pause`, `stop`, `setToolButtonStyle`, `QDockWidget`, `setWidget`, `addDockWidget`, `InspectorPanel`, `setMinimumWidth`, `TerrainPanel`, `tabifyDockWidget`, `MaterialPanel`, `raise`, `generated`, `ProjectPanel`, `ConsolePanel`, `PackagePanel`, `setObject`, `parentOf`, `localTransform`, `makeReparentCommand`, `refreshAfterHistory`, `selectedObject`, `exists`, `setSelected`, `invalid`, `setEnabled`, `makeDeleteCommand`, `onFrameTick`

**Создай файл** `editor/shell/src/main_window.hpp`
В файле должны быть: `MainWindow`, `selectObjectByName`, `playFrames`, `buildMenus`, `buildToolbar`, `buildDocks`, `onFrameTick`, `onSelection`, `syncPlayButtons`, `duplicateObject`, `deleteObject`, `performUndo`, `performRedo`, `refreshAfterHistory`, `updateUndoActions`

**Создай файл** `editor/shell/src/scene_view_3d.cpp`
В файле должны быть: `quatFromYawPitch`, `objectColor`, `componentsOf`, `descriptorOf`, `invalid`, `field`, `SceneView3D`, `QOpenGLWidget`, `context_`, `useSceneCamera_`, `setFocusPolicy`, `setMinimumSize`, `initializeGL`, `getString`, `getProcAddress`, `create`, `fromStdString`, `backendName`, `QStringLiteral`, `backendInitialized`, `cameraPose`, `findByName`, `worldTransform`, `rotate`, `refreshTerrainMesh`, `destroy`, `buildTerrainMesh`, `dataset`, `createMeshFromData`, `buildCommands`, `devicePixelRatioF`, `exists`, `componentOfType`, `childrenOf`, `emitLight`, `findMaterial`, `material`, `nameOf`, `resolveTex`, `path`, `loadFbxMesh`, `loadGltfMesh`, `loadObjMesh`, `emitObject`, `paintGL`, `submit`, `renderFrame`, `rayDirectionThrough`, `width`, `height`, `terrainHit`, `pickObject`, `test`, `frameSelected`, `update`, `mousePressEvent`, `setFocus`, `pos`, `modifiers`, `setCursor`, `button`, `setSelected`, `objectPicked`, `mouseMoveEvent`, `mouseReleaseEvent`, `wheelEvent`, `angleDelta`, `clamp`, `keyPressEvent`, `key`

**Создай файл** `editor/shell/src/scene_view_3d.hpp`
В файле должны быть: `setSelected`, `update`, `frameSelected`, `objectPicked`, `backendInitialized`, `initializeGL`, `paintGL`, `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, `wheelEvent`, `keyPressEvent`, `cameraPose`, `pickObject`, `buildCommands`, `refreshTerrainMesh`, `rayDirectionThrough`, `terrainHit`

**Создай файл** `editor/shell/src/theme.cpp`
В файле должны быть: `applyDarkTheme`, `create`, `window`, `panel`, `input`, `text`, `textMuted`, `accent`, `QColor`

**Создай файл** `editor/shell/src/theme.hpp`
В файле должны быть: `applyDarkTheme`

**Создай файл** `editor/shell/src/viewport_widget.cpp`
В файле должны быть: `kAxisX`, `kAxisY`, `kAccent`, `kSelection`, `rotationAroundZ`, `snapValue`, `round`, `distanceToSegment`, `dotProduct`, `ViewportWidget`, `QWidget`, `context_`, `setMinimumSize`, `setObjectName`, `setMouseTracking`, `setFocusPolicy`, `setSelected`, `update`, `setTool`, `toolChanged`, `frameSelected`, `exists`, `worldTransform`, `worldToScreen`, `height`, `screenToWorld`, `objectRect`, `handleAt`, `QPointF`, `pickObject`, `childrenOf`, `hitTest`, `mousePressEvent`, `setFocus`, `button`, `pos`, `setCursor`, `localTransform`, `objectPicked`, `mouseMoveEvent`, `applyDrag`, `modifiers`, `invalid`, `updateCursor`, `mouseReleaseEvent`, `transformCommitted`, `wheelEvent`, `position`, `angleDelta`, `clamp`, `keyPressEvent`, `key`, `deleteRequested`, `duplicateRequested`, `redoRequested`, `undoRequested`, `parentOf`, `rotate`, `setLocalTransform`, `transformEdited`, `paintEvent`, `painter`, `sky`, `QColor`, `fmod`, `width`, `fromStdString`, `nameOf`, `fill`, `drawObject`, `drawGizmo`, `state`, `tr`

**Создай файл** `editor/shell/src/viewport_widget.hpp`
В файле должны быть: `ViewportWidget`, `setSelected`, `setTool`, `frameSelected`, `objectPicked`, `toolChanged`, `transformEdited`, `deleteRequested`, `duplicateRequested`, `undoRequested`, `redoRequested`, `paintEvent`, `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, `wheelEvent`, `keyPressEvent`, `worldToScreen`, `screenToWorld`, `objectRect`, `handleAt`, `pickObject`, `applyDrag`, `drawGizmo`, `updateCursor`


## editor/tools

**Создай файл** `editor/tools/include/sky/editor/tools/console_panel.hpp`
В файле должны быть: `ConsolePanel`

**Создай файл** `editor/tools/include/sky/editor/tools/editor_tools.hpp`
В файле должны быть: `IToolCommandBus`, `execute`, `undo`, `redo`, `ISelectionService`, `select`, `current`, `onSelectionChanged`, `IGizmoTool`, `toolId`, `activate`, `deactivate`

**Создай файл** `editor/tools/include/sky/editor/tools/hierarchy_panel.hpp`
В файле должны быть: `refresh`, `selectedObject`, `selectObject`, `objectSelected`, `createEmptyRequested`, `createCrateRequested`, `deleteRequested`, `duplicateRequested`, `reparentRequested`, `objectRenamed`, `dropEvent`, `keyPressEvent`, `addObjectItem`, `showContextMenu`

**Создай файл** `editor/tools/include/sky/editor/tools/inspector_panel.hpp`
В файле должны быть: `setObject`, `refreshTransform`, `objectEdited`, `applyTransformFromUi`, `rebuildComponentList`, `showAddComponentMenu`

**Создай файл** `editor/tools/include/sky/editor/tools/material_panel.hpp`
В файле должны быть: `refresh`, `materialsChanged`, `materialCreated`, `showSelected`, `applyEdits`, `pickColor`, `commitBaseline`, `createMaterial`, `selectedName`

**Создай файл** `editor/tools/include/sky/editor/tools/package_panel.hpp`
В файле должны быть: `refresh`, `packageActivated`, `packageDeactivated`, `activateSelected`, `deactivateSelected`, `selectedPackageId`

**Создай файл** `editor/tools/include/sky/editor/tools/project_panel.hpp`
В файле должны быть: `ProjectPanel`, `navigateTo`, `refresh`, `onItemActivated`

**Создай файл** `editor/tools/include/sky/editor/tools/terrain_panel.hpp`
В файле должны быть: `TerrainPanel`, `brushChanged`, `generateRequested`, `emitBrush`

**Создай файл** `editor/tools/src/console_panel.cpp`
В файле должны быть: `ConsolePanel`, `QWidget`, `logger_`, `QVBoxLayout`, `setContentsMargins`, `setSpacing`, `setObjectName`, `QHBoxLayout`, `QPushButton`, `addWidget`, `addStretch`, `QPlainTextEdit`, `setReadOnly`, `setMaximumBlockCount`, `connect`, `appendHtml`

**Создай файл** `editor/tools/src/hierarchy_panel.cpp`
В файле должны быть: `QTreeWidget`, `objects_`, `rootsProvider_`, `setHeaderHidden`, `setObjectName`, `setContextMenuPolicy`, `setExpandsOnDoubleClick`, `setDragDropMode`, `setDefaultDropAction`, `text`, `fromStdString`, `objectRenamed`, `refresh`, `selectedObject`, `addObjectItem`, `expandAll`, `selectObject`, `currentItem`, `invalid`, `it`, `setCurrentItem`, `dropEvent`, `setDropAction`, `accept`, `itemAt`, `pos`, `dropIndicatorPosition`, `parent`, `reparentRequested`, `keyPressEvent`, `key`, `deleteRequested`, `modifiers`, `duplicateRequested`, `QTreeWidgetItem`, `setText`, `setData`, `showContextMenu`, `menu`, `createEmptyRequested`, `createCrateRequested`, `editItem`, `mapToGlobal`

**Создай файл** `editor/tools/src/inspector_panel.cpp`
В файле должны быть: `fieldToString`, `number`, `QStringLiteral`, `fromStdString`, `QString`, `vectorRow`, `QLabel`, `setObjectName`, `addWidget`, `QWidget`, `objects_`, `components_`, `QVBoxLayout`, `setContentsMargins`, `addStretch`, `setSpacing`, `QLineEdit`, `QGroupBox`, `QFormLayout`, `ref`, `QDoubleSpinBox`, `setRange`, `setDecimals`, `setSingleStep`, `setButtonSymbols`, `applyTransformFromUi`, `connect`, `transformCommitted`, `addRow`, `QPushButton`, `setObject`, `invalid`, `setVisible`, `setText`, `refreshTransform`, `rebuildComponentList`, `setValue`, `value`, `objectEdited`, `takeAt`, `widget`, `QHBoxLayout`, `setFixedSize`, `fieldFromString`, `text`, `showAddComponentMenu`, `menu`, `QPoint`, `height`

**Создай файл** `editor/tools/src/material_panel.cpp`
В файле должны быть: `toQColor`, `clamp`, `toVec3`, `paintSwatch`, `setStyleSheet`, `MaterialPanel`, `QWidget`, `materials_`, `QHBoxLayout`, `setContentsMargins`, `QListWidget`, `setMaximumWidth`, `addWidget`, `QPushButton`, `addLayout`, `QLineEdit`, `setFixedSize`, `QSlider`, `setRange`, `setPlaceholderText`, `addRow`, `showSelected`, `connect`, `applyEdits`, `commitBaseline`, `pickColor`, `refresh`, `selectedName`, `addItem`, `fromStdString`, `sortItems`, `findItems`, `setCurrentItem`, `item`, `currentItem`, `text`, `setEnabled`, `setText`, `setValue`, `value`, `materialsChanged`, `materialCommitted`, `getColor`, `createMaterial`, `QString`, `materialCreated`

**Создай файл** `editor/tools/src/package_panel.cpp`
В файле должны быть: `QWidget`, `packages_`, `packagesRoot_`, `QVBoxLayout`, `setContentsMargins`, `setSpacing`, `QTreeWidget`, `tr`, `setRootIsDecorated`, `addWidget`, `setObjectName`, `QHBoxLayout`, `QPushButton`, `addStretch`, `connect`, `refresh`, `any_of`, `QTreeWidgetItem`, `setText`, `fromStdString`, `isActive`, `setData`, `resizeColumnToContents`, `selectedPackageId`, `currentItem`, `activateSelected`, `packageActivated`, `deactivateSelected`, `packageDeactivated`

**Создай файл** `editor/tools/src/project_panel.cpp`
В файле должны быть: `ProjectPanel`, `QWidget`, `vfs_`, `QVBoxLayout`, `setContentsMargins`, `setSpacing`, `QLabel`, `setObjectName`, `addWidget`, `QListWidget`, `setViewMode`, `setIconSize`, `setGridSize`, `setResizeMode`, `setMovement`, `setWordWrap`, `connect`, `navigateTo`, `refresh`, `setText`, `parse`, `QListWidgetItem`, `fromUtf8`, `setData`, `QStringLiteral`, `fromStdString`, `setTextAlignment`, `onItemActivated`, `toString`

**Создай файл** `editor/tools/src/terrain_panel.cpp`
В файле должны быть: `TerrainPanel`, `QWidget`, `QVBoxLayout`, `setContentsMargins`, `setSpacing`, `QGroupBox`, `QButtonGroup`, `setExclusive`, `QPushButton`, `setCheckable`, `setProperty`, `addButton`, `addWidget`, `connect`, `buttons`, `setChecked`, `emitBrush`, `addLayout`, `QSlider`, `setRange`, `setValue`, `addRow`, `QFormLayout`, `QSpinBox`, `generateRequested`, `value`, `addStretch`, `isChecked`, `property`


## editor/viewport_bridge

**Создай файл** `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`
В файле должны быть: `PlayModeController`, `setScene`, `tickFrame`

**Создай файл** `editor/viewport_bridge/include/sky/editor/viewport/tool_command_bus.hpp`
В файле должны быть: `ToolCommandBus`, `registerHandler`, `registeredCommands`, `setUndoDelegate`, `setRedoDelegate`, `createToolCommandBus`

**Создай файл** `editor/viewport_bridge/include/sky/editor/viewport/viewport_bridge.hpp`
В файле должны быть: `IPlayModeController`, `play`, `pause`, `stop`, `state`, `onStateChanged`, `IRuntimePreviewHost`, `attachSurface`, `detachSurface`, `context`

**Создай файл** `editor/viewport_bridge/src/play_mode_controller.cpp`
В файле должны быть: `PlayModeControllerImpl`, `play`, `transition`, `pause`, `stop`, `onStateChanged`, `setScene`, `tickFrame`, `callback`

**Создай файл** `editor/viewport_bridge/src/tool_command_bus.cpp`
В файле должны быть: `execute`, `second`, `registerHandler`, `registeredCommands`, `createToolCommandBus`


## engine/asset

**Создай файл** `engine/asset/include/sky/asset/asset_database.hpp`
В файле должны быть: `AssetDatabase`, `createAssetDatabase`, `assetIdFromPath`

**Создай файл** `engine/asset/include/sky/asset/asset_system.hpp`
В файле должны быть: `IAssetResolver`, `resolve`, `IAssetRegistry`, `registerAsset`, `unregisterAsset`, `dependentsOf`, `allAssets`, `IAssetImporter`, `supports`, `import`, `IImportPipeline`, `registerImporter`, `importAsset`, `reimport`

**Создай файл** `engine/asset/include/sky/asset/fbx_importer.hpp`
В файле должны быть: `createFbxImporter`

**Создай файл** `engine/asset/include/sky/asset/gltf_importer.hpp`
В файле должны быть: `createGltfImporter`

**Создай файл** `engine/asset/include/sky/asset/obj_importer.hpp`
В файле должны быть: `createObjImporter`

**Создай файл** `engine/asset/include/sky/asset/png_decoder.hpp`
В файле должны быть: `decodePng`, `encodePngRgba`, `createPngImporter`

**Создай файл** `engine/asset/src/asset_database.cpp`
В файле должны быть: `resolve`, `assetIdFromPath`, `optional`, `registerAsset`, `dependentsOf`, `allAssets`, `registerImporter`, `importAsset`, `supports`, `import`, `reimport`, `createAssetDatabase`

**Создай файл** `engine/asset/src/fbx_importer.cpp`
В файле должны быть: `transformPoint`, `transformNormal`, `sqrt`, `getMeshCount`, `getMesh`, `getGeometryData`, `getGlobalTransform`, `triangulate`, `destroy`, `supports`, `tolower`, `createFbxImporter`

**Создай файл** `engine/asset/src/gltf_importer.cpp`
В файле должны быть: `transformPoint`, `transformNormal`, `sqrt`, `fromTrs`, `int`, `decodeBase64`, `decode`, `byte`, `readU32At`, `parseJson`, `string_view`, `numberOr`, `size_t`, `floatAt`, `indexAt`, `flatNormal`, `open`, `uint32_t`, `openDocument`, `supports`, `createGltfImporter`

**Создай файл** `engine/asset/src/mini_json.hpp`
В файле должны быть: `numberOr`, `parse`, `parseValue`, `skipWhitespace`, `isspace`, `consume`, `parseObject`, `parseArray`, `parseString`, `parseBool`, `parseNull`, `parseNumber`, `parseJson`, `JsonParser`

**Создай файл** `engine/asset/src/obj_importer.cpp`
В файле должны быть: `parseFaceVertex`, `stoi`, `resolveIndex`, `flatNormal`, `sqrt`, `text`, `stream`, `getline`, `record`, `supports`, `loadObjMesh`, `createObjImporter`

**Создай файл** `engine/asset/src/png_decoder.cpp`
В файле должны быть: `readU32`, `uint32_t`, `channelsFor`, `paeth`, `int`, `decodePng`, `memcmp`, `uint8_t`, `size_t`, `raw`, `libdeflate_alloc_decompressor`, `libdeflate_free_decompressor`, `previous`, `crc32Of`, `appendU32`, `byte`, `crcInput`, `encodePngRgba`, `ihdr`, `putU32`, `appendChunk`, `supports`, `createPngImporter`

**Создай файл** `engine/asset/third_party/libdeflate/common_defs.h`
В файле должны быть: `__has_attribute`, `__has_builtin`, `likely`, `__builtin_expect`, `unlikely`, `prefetchr`, `__builtin_prefetch`, `_mm_prefetch`, `__prefetch2`, `__prefetch`, `prefetchw`, `_m_prefetchw`, `__prefetchw`, `_aligned_attribute`, `_target_attribute`, `bswap16`, `__builtin_bswap16`, `_byteswap_ushort`, `bswap32`, `__builtin_bswap32`, `_byteswap_ulong`, `bswap64`, `__builtin_bswap64`, `_byteswap_uint64`, `le16_bswap`, `le32_bswap`, `le64_bswap`, `be16_bswap`, `be32_bswap`, `be64_bswap`, `get_unaligned_le16`, `get_unaligned_be16`, `get_unaligned_le32`, `get_unaligned_be32`, `get_unaligned_le64`, `get_unaligned_leword`, `put_unaligned_le16`, `store_u16_unaligned`, `put_unaligned_be16`, `put_unaligned_le32`, `store_u32_unaligned`, `put_unaligned_be32`, `put_unaligned_le64`, `store_u64_unaligned`, `put_unaligned_leword`, `bsr32`, `__builtin_clz`, `_BitScanReverse`, `bsr64`, `__builtin_clzll`, `_BitScanReverse64`, `bsrw`, `bsf32`, `__builtin_ctz`, `_BitScanForward`, `bsf64`, `__builtin_ctzll`, `_BitScanForward64`, `bsfw`, `rbit32`, `__asm__`

**Создай файл** `engine/asset/third_party/libdeflate/lib/arm/adler32_impl.h`
В файле должны быть: `_target_attribute`, `adler32_arm_neon`, `_aligned_attribute`, `vld1q_u16`, `vdupq_n_u32`, `vdupq_n_u16`, `vld1q_u8`, `vaddq_u32`, `vpaddlq_u8`, `vget_low_u8`, `vget_high_u8`, `vpadalq_u8`, `vpadalq_u16`, `umlal2`, `vmlal_u16`, `vget_high_u16`, `vqshlq_n_u32`, `vget_low_u16`, `vgetq_lane_u32`, `vaddvq_u32`, `adler32_arm_neon_dotprod`, `vdupq_n_u8`, `vdotq_u32`, `arch_select_adler32_func`, `get_arm_cpu_features`

**Создай файл** `engine/asset/third_party/libdeflate/lib/arm/cpu_features.h`
В файле должны быть: `libdeflate_init_arm_cpu_features`, `get_arm_cpu_features`, `compat_vmull_p64`, `vmull_p64`, `vcreate_p64`

**Создай файл** `engine/asset/third_party/libdeflate/lib/arm/crc32_impl.h`
В файле должны быть: `crc32_arm_crc`, `instructions`, `_target_attribute`, `combine_crcs_slow`, `__crc32d`, `__crc32b`, `__crc32h`, `le16_bswap`, `__crc32w`, `le32_bswap`, `prefetchr`, `le64_bswap`, `get_unaligned_le64`, `get_unaligned_le32`, `get_unaligned_le16`, `clmul_u32`, `compat_vmull_p64`, `vgetq_lane_u64`, `combine_crcs_fast`, `crc32_arm_crc_pmullcombine`, `crc32_arm_pmullx4`, `_aligned_attribute`, `load_multipliers`, `crc32_slice1`, `veorq_u8`, `u32_to_bytevec`, `fold_vec`, `vld1q_u8`, `fold_partial_vec`, `vextq_u8`, `vdupq_n_u8`, `clmul_low`, `vgetq_lane_u32`, `crc32_arm_pmullx12_crc_eor3`, `arch_select_crc32_func`, `get_arm_cpu_features`

**Создай файл** `engine/asset/third_party/libdeflate/lib/arm/crc32_pmull_helpers.h`
В файле должны быть: `vreinterpretq_u8_u32`, `vdupq_n_u32`, `vreinterpretq_p64_u64`, `vgetq_lane_p64`, `__asm__`, `vreinterpretq_u8_p128`, `veor3q_u8`, `veorq_u8`, `clmul_low`, `clmul_high`, `eor3`, `vld1q_u8`, `vqtbl1q_u8`, `vshrq_n_s8`, `fold_vec`

**Создай файл** `engine/asset/third_party/libdeflate/lib/arm/crc32_pmull_wide.h`
В файле должны быть: `_aligned_attribute`, `load_multipliers`, `veorq_u8`, `u32_to_bytevec`, `vld1q_u8`, `fold_vec`, `__crc32b`, `__crc32h`, `le16_bswap`, `__crc32w`, `le32_bswap`, `__crc32d`, `le64_bswap`, `vgetq_lane_u64`, `get_unaligned_le64`, `get_unaligned_le32`, `get_unaligned_le16`

**Создай файл** `engine/asset/third_party/libdeflate/lib/arm/matchfinder_impl.h`
В файле должны быть: `matchfinder_init_neon`, `vdupq_n_s16`, `matchfinder_rebase_neon`, `vqaddq_s16`

**Создай файл** `engine/asset/third_party/libdeflate/lib/bt_matchfinder.h`
В файле должны быть: `bt_matchfinder_init`, `matchfinder_init`, `bt_matchfinder_slide_window`, `matchfinder_rebase`, `bt_left_child`, `bt_right_child`, `bt_matchfinder_get_matches`, `get_unaligned_le32`, `lz_hash`, `prefetchw`, `load_u24_unaligned`, `lz_extend`, `and`

**Создай файл** `engine/asset/third_party/libdeflate/lib/cpu_features_common.h`
В файле должны быть: `strdup`, `abort`, `strtok_r`, `free`

**Создай файл** `engine/asset/third_party/libdeflate/lib/crc32_multipliers.h`
(объявлений функций нет — данные/разметка)

**Создай файл** `engine/asset/third_party/libdeflate/lib/crc32_tables.h`
(объявлений функций нет — данные/разметка)

**Создай файл** `engine/asset/third_party/libdeflate/lib/decompress_template.h`
В файле должны быть: `get_unaligned_le16`, `unlikely`, `store_word_unaligned`

**Создай файл** `engine/asset/third_party/libdeflate/lib/deflate_compress.h`
В файле должны быть: `libdeflate_get_compression_level`

**Создай файл** `engine/asset/third_party/libdeflate/lib/deflate_constants.h`
(объявлений функций нет — данные/разметка)

**Создай файл** `engine/asset/third_party/libdeflate/lib/gzip_constants.h`
(объявлений функций нет — данные/разметка)

**Создай файл** `engine/asset/third_party/libdeflate/lib/hc_matchfinder.h`
В файле должны быть: `hc_matchfinder_init`, `matchfinder_init`, `hc_matchfinder_slide_window`, `matchfinder_rebase`, `get_unaligned_le32`, `lz_hash`, `prefetchw`, `load_u32_unaligned`, `loaded_u32_to_u24`, `lz_extend`

**Создай файл** `engine/asset/third_party/libdeflate/lib/ht_matchfinder.h`
В файле должны быть: `ht_matchfinder_init`, `matchfinder_init`, `ht_matchfinder_slide_window`, `matchfinder_rebase`, `load_u32_unaligned`, `prefetchw`, `lz_extend`

**Создай файл** `engine/asset/third_party/libdeflate/lib/lib_common.h`
В файле должны быть: `compilers`, `languages`, `void`, `libdeflate_aligned_free`, `memset`, `__builtin_memset`, `__builtin_memcpy`, `memmove`, `__builtin_memmove`, `memcmp`, `__builtin_memcmp`, `libdeflate_assertion_failed`

**Создай файл** `engine/asset/third_party/libdeflate/lib/matchfinder_common.h`
В файле должны быть: `loaded_u32_to_u24`, `load_u24_unaligned`, `matchfinder_init`, `_aligned_attribute`, `matchfinder_rebase`, `lz_hash`, `load_word_unaligned`, `bsrw`

**Создай файл** `engine/asset/third_party/libdeflate/lib/riscv/matchfinder_impl.h`
В файле должны быть: `riscv_matchfinder_vl`, `__riscv_vsetvlmax_e16m8`, `matchfinder_init_rvv`, `__riscv_vmv_v_x_i16m8`, `__riscv_vse16_v_i16m8`, `matchfinder_rebase_rvv`, `__riscv_vle16_v_i16m8`

**Создай файл** `engine/asset/third_party/libdeflate/lib/x86/adler32_impl.h`
В файле должны быть: `_target_attribute`, `arch_select_adler32_func`, `get_x86_cpu_features`

**Создай файл** `engine/asset/third_party/libdeflate/lib/x86/adler32_template.h`
В файле должны быть: `_mm_add_epi8`, `_mm_add_epi16`, `_mm_add_epi32`, `_mm_dpbusd_epi32`, `_mm_dpbusd_avx_epi32`, `_mm_load_si128`, `_mm_loadu_si128`, `_mm_madd_epi16`, `_mm_maskz_loadu_epi8`, `_mm_mullo_epi32`, `_mm_sad_epu8`, `_mm_set1_epi8`, `_mm_set1_epi32`, `_mm_setzero_si128`, `_mm_slli_epi32`, `_mm_unpacklo_epi8`, `_mm_unpackhi_epi8`, `_mm256_add_epi8`, `_mm256_add_epi16`, `_mm256_add_epi32`, `_mm256_dpbusd_epi32`, `_mm256_dpbusd_avx_epi32`, `_mm256_load_si256`, `_mm256_loadu_si256`, `_mm256_madd_epi16`, `_mm256_maskz_loadu_epi8`, `_mm256_mullo_epi32`, `_mm256_sad_epu8`, `_mm256_set1_epi8`, `_mm256_set1_epi32`, `_mm256_setzero_si256`, `_mm256_slli_epi32`, `_mm256_unpacklo_epi8`, `_mm256_unpackhi_epi8`, `_mm512_add_epi8`, `_mm512_add_epi16`, `_mm512_add_epi32`, `_mm512_dpbusd_epi32`, `_mm512_load_si512`, `_mm512_loadu_si512`, `_mm512_madd_epi16`, `_mm512_maskz_loadu_epi8`, `_mm512_mullo_epi32`, `_mm512_sad_epu8`, `_mm512_set1_epi8`, `_mm512_set1_epi32`, `_mm512_setzero_si512`, `_mm512_slli_epi32`, `_mm512_unpacklo_epi8`, `_mm512_unpackhi_epi8`, `_mm512_extracti64x4_epi64`, `_mm256_extracti128_si256`, `_mm_shuffle_epi32`, `_aligned_attribute`, `reduce_to_32bits`

**Создай файл** `engine/asset/third_party/libdeflate/lib/x86/cpu_features.h`
В файле должны быть: `libdeflate_init_x86_cpu_features`, `get_x86_cpu_features`, `__has_include`

**Создай файл** `engine/asset/third_party/libdeflate/lib/x86/crc32_impl.h`
В файле должны быть: `_target_attribute`, `arch_select_crc32_func`, `get_x86_cpu_features`

**Создай файл** `engine/asset/third_party/libdeflate/lib/x86/crc32_pclmul_template.h`
В файле должны быть: `_mm_loadu_si128`, `_mm_xor_si128`, `_mm_set_epi64x`, `_mm256_loadu_si256`, `_mm256_xor_si256`, `_mm256_zextsi128_si256`, `_mm256_set_epi64x`, `_mm512_loadu_si512`, `_mm512_xor_si512`, `_mm512_zextsi128_si512`, `_mm512_set_epi64`, `_mm_clmulepi64_si128`, `_mm256_clmulepi64_epi128`, `_mm_shuffle_epi8`, `fold_vec128`, `_mm_cvtsi32_si128`, `crc32_slice1`, `_mm_maskz_loadu_epi8`, `_mm256_inserti128_si256`, `_mm512_inserti32x4`, `_mm512_inserti64x4`, `fold_vec`, `_mm256_extracti128_si256`, `fold_lessthan16bytes`, `_mm_bsrli_si128`, `_mm_extract_epi32`

**Создай файл** `engine/asset/third_party/libdeflate/lib/x86/decompress_impl.h`
В файле должны быть: `_target_attribute`, `_bzhi_u64`, `_bzhi_u32`, `arch_select_decompress_func`

**Создай файл** `engine/asset/third_party/libdeflate/lib/x86/matchfinder_impl.h`
В файле должны быть: `matchfinder_init_avx2`, `_mm256_set1_epi16`, `matchfinder_rebase_avx2`, `_mm256_adds_epi16`, `matchfinder_init_sse2`, `_mm_set1_epi16`, `matchfinder_rebase_sse2`, `_mm_adds_epi16`

**Создай файл** `engine/asset/third_party/libdeflate/lib/zlib_constants.h`
(объявлений функций нет — данные/разметка)

**Создай файл** `engine/asset/third_party/libdeflate/libdeflate.h`
В файле должны быть: `libdeflate_alloc_compressor`, `libdeflate_free_compressor`, `libdeflate_alloc_decompressor`, `libdeflate_alloc_decompressor_ex`, `libdeflate_free_decompressor`, `libdeflate_adler32`, `libdeflate_crc32`, `void`, `libdeflate_alloc_compressor_ex`, `memset`

**Создай файл** `engine/asset/third_party/openfbx/ofbx.cpp`
В файле должны быть: `PlacementNewHelper`, `free`, `grow`, `static_assert`, `Value`, `Hasher`, `Key`, `alignas`, `StringView`, `memcmp`, `read_value`, `decodeIndex`, `codeIndex`, `Allocator`, `allocate`, `assert`, `OptionalError`, `is_error`, `value`, `getValue`, `isError`, `pack`, `setTranslation`, `makeIdentity`, `rotationX`, `rotationY`, `rotationZ`, `getRotationMatrix`, `fbxTimeToSeconds`, `double`, `secondsToFbxTime`, `i64`, `copyString`, `toU64`, `strtoull`, `toI64`, `atoll`, `toInt`, `toU32`, `toBool`, `toDouble`, `toFloat`, `parseMemory`, `parseVecData`, `parseVertexData`, `parseDouble`, `bool`, `pushJob`, `getCount`, `getProperty`, `getNext`, `findChild`, `resolveProperty`, `resolveEnumProperty`, `resolveVec3Property`, `isString`, `getType`, `isLong`, `decompress`, `libdeflate_alloc_decompressor`, `libdeflate_deflate_decompress`, `libdeflate_free_decompressor`, `read`, `Error`, `readShortString`, `readLongString`, `readProperty`, `strlen`, `readElementOffset`, `readElement`, `isEndLine`, `skipInsignificantWhitespaces`, `isspace`, `skipLine`, `skipWhitespaces`, `isTextTokenChar`, `isalnum`, `readTextToken`, `readTextProperty`, `isdigit`, `readTextElement`, `tokenizeText`, `tokenize`, `Vec2Attributes`, `int`, `Vec3Attributes`, `Vec4Attributes`, `patchAttributes`, `getPartition`, `postprocess`, `Mesh`, `Object`, `GeometryImpl`, `Geometry`, `MeshImpl`, `getGeometricMatrix`, `Material`, `MaterialImpl`, `getReflectionColor`, `getAmbientColor`, `getEmissiveColor`, `getDiffuseFactor`, `getSpecularFactor`, `getReflectionFactor`, `getShininess`, `getShininessExponent`, `getAmbientFactor`, `getBumpFactor`, `getEmissiveFactor`, `getOpacity`, `LimbNodeImpl`, `NullImpl`, `NodeAttribute`, `NodeAttributeImpl`, `Shape`, `ShapeImpl`, `Cluster`, `ClusterImpl`, `AnimationStack`, `AnimationLayer`, `AnimationCurve`, `AnimationCurveNode`, `AnimationStackImpl`, `getLayer`, `AnimationCurveImpl`, `Skin`, `SkinImpl`, `BlendShapeChannel`, `BlendShapeChannelImpl`, `resolveObjectLinkReverse`, `BlendShape`, `BlendShapeImpl`, `Texture`, `Pose`, `PoseImpl`, `TextureImpl`, `getEmbeddedData`, `LightImpl`, `Light`, `doesDrawVolumetricLight`, `doesDrawGroundProjection`, `doesDrawFrontFacingVolumetricLight`, `CameraImpl`, `Camera`, `CalculateFOV`, `atan`, `Root`, `getEmbeddedDataCount`, `isEmbeddedBase64`, `getEmbeddedBase64Data`, `getEmbeddedFilename`, `getAnimationStack`, `getMesh`, `getGeometry`, `getTakeInfo`, `getCamera`, `getCameraCount`, `getLight`, `getLightCount`, `Scene`, `finalize`, `scene`, `element`, `is_node`, `node_attribute`, `AnimationCurveNodeImpl`, `getBone`, `getCurve`, `getNodeLocalTransform`, `getKeyTime`, `getKeyValue`, `getKeyCount`, `float`, `getCoord`, `AnimationLayerImpl`, `getCurveNode`, `parseVideo`, `parseGeometryMaterials`, `parseGeometryUVs`, `parseGeometryTangents`, `parseGeometryColors`, `parseGeometryNormals`, `parseMesh`, `parseTexture`, `parseLight`, `parseCamera`, `toObjectID`, `tmp`, `parsePose`, `getValues`, `parseCluster`, `parseNodeAttribute`, `parseMaterial`, `fromString`, `parseMemoryText`, `parseMemoryLinked`, `typeMatch`, `parseTextArray`, `parseBinaryArrayLinked`, `parseArray`, `parseAnimationCurve`, `parseGeometry`, `parseConnections`, `parseTakes`, `getFramerateFromTimeMode`, `parseGlobalSettings`, `get_property_raw`, `get_property`, `get_time_property`, `toCoordinateAxis`, `parseHeaders`, `get_string_property`, `sync_job_processor`, `fn`, `parseObjects`, `f`, `isNode`, `getRotationOrder`, `getRotationOffset`, `getRotationPivot`, `getPostRotation`, `getScalingOffset`, `getScalingPivot`, `evalLocal`, `getLocalScaling`, `getLocalTranslation`, `getPreRotation`, `getLocalRotation`, `getGlobalTransform`, `getParent`, `getLocalTransform`, `getScene`, `resolveObjectLink`, `load`, `release`, `strncmp`, `root`, `getError`, `isConvex`, `triangulate`

**Создай файл** `engine/asset/third_party/openfbx/ofbx.h`
В файле должны быть: `static_assert`, `void`, `toU64`, `toI64`, `toInt`, `toU32`, `toBool`, `toDouble`, `toFloat`, `toString`, `getType`, `getNext`, `getValue`, `getCount`, `getValues`, `IElement`, `getFirstChild`, `getSibling`, `getID`, `getFirstProperty`, `Object`, `getScene`, `resolveObjectLink`, `resolveObjectLinkReverse`, `getRotationOrder`, `getRotationOffset`, `getRotationPivot`, `getPostRotation`, `getScalingOffset`, `getScalingPivot`, `getPreRotation`, `getLocalTranslation`, `getLocalRotation`, `getLocalScaling`, `getGlobalTransform`, `getLocalTransform`, `evalLocal`, `Pose`, `getMatrix`, `getNode`, `Texture`, `getFileName`, `getRelativeFileName`, `getEmbeddedData`, `Light`, `getLightType`, `doesCastLight`, `doesDrawVolumetricLight`, `doesDrawGroundProjection`, `doesDrawFrontFacingVolumetricLight`, `getColor`, `getIntensity`, `getInnerAngle`, `getOuterAngle`, `getFog`, `getDecayType`, `getDecayStart`, `doesEnableNearAttenuation`, `getNearAttenuationStart`, `getNearAttenuationEnd`, `doesEnableFarAttenuation`, `getFarAttenuationStart`, `getFarAttenuationEnd`, `getShadowTexture`, `doesCastShadows`, `getShadowColor`, `Camera`, `getProjectionType`, `getApertureMode`, `getFilmHeight`, `getFilmWidth`, `getAspectHeight`, `getAspectWidth`, `getNearPlane`, `getFarPlane`, `getOrthoZoom`, `doesAutoComputeClipPanes`, `getGateFit`, `getFilmAspectRatio`, `getFocalLength`, `getFocusDistance`, `getBackgroundColor`, `getInterestPosition`, `Material`, `getDiffuseColor`, `getSpecularColor`, `getReflectionColor`, `getAmbientColor`, `getEmissiveColor`, `getDiffuseFactor`, `getSpecularFactor`, `getReflectionFactor`, `getShininess`, `getShininessExponent`, `getAmbientFactor`, `getBumpFactor`, `getEmissiveFactor`, `getOpacity`, `getShadingModel`, `getTexture`, `Cluster`, `getIndices`, `getIndicesCount`, `getWeights`, `getWeightsCount`, `getTransformMatrix`, `getTransformLinkMatrix`, `getLink`, `Skin`, `getClusterCount`, `getCluster`, `BlendShapeChannel`, `getDeformPercent`, `getShapeCount`, `getShape`, `BlendShape`, `getBlendShapeChannelCount`, `getBlendShapeChannel`, `NodeAttribute`, `getAttributeType`, `getPositions`, `getNormals`, `getUVs`, `getColors`, `getTangents`, `getPartitionCount`, `getPartition`, `getMaterialMapSize`, `getMaterialMap`, `hasVertices`, `Geometry`, `getGeometryData`, `getSkin`, `getBlendShape`, `Shape`, `getVertices`, `getVertexCount`, `getIndexCount`, `Mesh`, `getPose`, `getGeometry`, `getGeometricMatrix`, `getMaterial`, `getMaterialCount`, `AnimationStack`, `getLayer`, `AnimationLayer`, `getCurveNode`, `AnimationCurve`, `getKeyCount`, `getKeyTime`, `getKeyValue`, `AnimationCurveNode`, `getBoneLinkProperty`, `getCurve`, `getNodeLocalTransform`, `getBone`, `destroy`, `getRootElement`, `getRoot`, `getMeshCount`, `getMesh`, `getGeometryCount`, `getAnimationStackCount`, `getAnimationStack`, `getCameraCount`, `getCamera`, `getLightCount`, `getLight`, `getAllObjects`, `getAllObjectCount`, `getEmbeddedDataCount`, `getEmbeddedFilename`, `isEmbeddedBase64`, `getEmbeddedBase64Data`, `getTakeInfo`, `getSceneFrameRate`, `getGlobalSettings`, `getHeaders`, `load`, `getError`, `fbxTimeToSeconds`, `secondsToFbxTime`, `triangulate`, `default_delete`


## engine/component

**Создай файл** `engine/component/include/sky/component/component_model.hpp`
В файле должны быть: `IComponentRegistry`, `registerComponentType`, `availableTypes`, `IComponentAttachmentService`, `attach`, `detach`, `IComponentQueryService`, `ownerOf`

**Создай файл** `engine/component/include/sky/component/component_world.hpp`
В файле должны быть: `ComponentWorld`, `detachAllFrom`, `createComponentWorld`

**Создай файл** `engine/component/src/component_world.cpp`
В файле должны быть: `registerComponentType`, `availableTypes`, `attach`, `invalid`, `detach`, `detachAllFrom`, `fields`, `componentsOf`, `descriptorOf`, `ownerOf`, `createComponentWorld`


## engine/core

**Создай файл** `engine/core/include/sky/core/config_service.hpp`
В файле должны быть: `IConfigService`, `getString`, `getInt`, `getBool`, `set`

**Создай файл** `engine/core/include/sky/core/diagnostics.hpp`
В файле должны быть: `IDiagnosticsSink`, `counter`, `timingMicros`

**Создай файл** `engine/core/include/sky/core/event_bus.hpp`
В файле должны быть: `IEventBus`, `subscribe`, `unsubscribe`, `publish`

**Создай файл** `engine/core/include/sky/core/handle.hpp`
(объявлений функций нет — данные/разметка)

**Создай файл** `engine/core/include/sky/core/job_scheduler.hpp`
В файле должны быть: `IJobScheduler`, `schedule`, `scheduleAfter`, `wait`

**Создай файл** `engine/core/include/sky/core/logger.hpp`
В файле должны быть: `ILogger`, `log`, `info`, `warning`, `error`

**Создай файл** `engine/core/include/sky/core/math.hpp`
В файле должны быть: `rotate`, `compose`, `divide`, `invCompose`, `conjugate`

**Создай файл** `engine/core/include/sky/core/runtime_services.hpp`
В файле должны быть: `createConsoleLogger`, `createInMemoryConfigService`, `createEventBus`, `createThreadPoolScheduler`, `counterValue`, `lastTimingMicros`, `createInMemoryDiagnostics`

**Создай файл** `engine/core/src/console_logger.cpp`
В файле должны быть: `levelName`, `log`, `createConsoleLogger`

**Создай файл** `engine/core/src/diagnostics.cpp`
В файле должны быть: `counter`, `timingMicros`, `counterValue`, `lastTimingMicros`, `createInMemoryDiagnostics`

**Создай файл** `engine/core/src/event_bus.cpp`
В файле должны быть: `subscribe`, `unsubscribe`, `publish`, `handler`, `createEventBus`

**Создай файл** `engine/core/src/job_scheduler.cpp`
В файле должны быть: `ThreadPoolScheduler`, `workerLoop`, `schedule`, `enqueue`, `scheduleAfter`, `wait`, `findRunnable`, `job`, `createThreadPoolScheduler`

**Создай файл** `engine/core/src/memory_config_service.cpp`
В файле должны быть: `getString`, `getInt`, `getBool`, `set`, `createInMemoryConfigService`


## engine/ecs

**Создай файл** `engine/ecs/include/sky/ecs/ecs.hpp`
В файле должны быть: `IEcsComponentStore`, `componentType`, `has`, `remove`, `IEcsSystem`, `name`, `update`, `IEcsWorld`, `createEntity`, `destroyEntity`, `isAlive`, `store`, `IEcsSystemScheduler`, `registerSystem`, `unregisterSystem`, `tick`, `IEcsQueryService`

**Создай файл** `engine/ecs/include/sky/ecs/ecs_world.hpp`
В файле должны быть: `set`, `get`, `EcsWorld`, `storeFor`, `stores`, `type_index`, `createEcsWorld`

**Создай файл** `engine/ecs/include/sky/ecs/object_sync.hpp`
В файле должны быть: `IEcsObjectSync`, `bind`, `unbind`, `entityOf`, `objectOf`, `pushAuthoringState`, `pullEcsResults`

**Создай файл** `engine/ecs/src/ecs_world.cpp`
В файле должны быть: `createEntity`, `destroyEntity`, `remove`, `store`, `out_of_range`, `tick`, `update`, `all_of`, `has`, `createEcsWorld`

**Создай файл** `engine/ecs/src/object_sync.cpp`
В файле должны быть: `EcsObjectSyncImpl`, `bind`, `unbind`, `entityOf`, `objectOf`, `invalid`, `pushAuthoringState`, `pullEcsResults`


## engine/mapgen

**Создай файл** `engine/mapgen/include/sky/mapgen/generation_pipeline.hpp`
В файле должны быть: `createGenerationPipeline`

**Создай файл** `engine/mapgen/include/sky/mapgen/map_generation.hpp`
В файле должны быть: `IGenerationResult`, `succeeded`, `terrainOutput`, `placements`, `IGenerationPipeline`, `availableStages`, `generate`

**Создай файл** `engine/mapgen/include/sky/mapgen/materialize.hpp`
(объявлений функций нет — данные/разметка)

**Создай файл** `engine/mapgen/src/generation_pipeline.cpp`
В файле должны быть: `splitMix64`, `hashToUnitFloat`, `valueNoise`, `placements`, `availableStages`, `generate`, `fillHeightfield`, `scatterObjects`, `assetIdFromPath`, `createGenerationPipeline`

**Создай файл** `engine/mapgen/src/materialize.cpp`
(объявлений функций нет — данные/разметка)


## engine/object

**Создай файл** `engine/object/include/sky/object/object_model.hpp`
В файле должны быть: `IObjectFactory`, `createObject`, `destroyObject`, `IObjectHierarchyAccess`, `setParent`, `parentOf`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`, `IObjectQueryService`, `exists`, `nameOf`, `findByName`, `invCompose`

**Создай файл** `engine/object/include/sky/object/object_world.hpp`
В файле должны быть: `ObjectWorld`, `renameObject`, `createObjectWorld`

**Создай файл** `engine/object/src/object_world.cpp`
В файле должны быть: `createObject`, `renameObject`, `destroyObject`, `detachFromParent`, `setParent`, `wouldCreateCycle`, `parentOf`, `invalid`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`, `compose`, `exists`, `nameOf`, `findByName`, `createObjectWorld`


## engine/package

**Создай файл** `engine/package/include/sky/package/package_installer.hpp`
В файле должны быть: `cacheRoot`

**Создай файл** `engine/package/include/sky/package/package_lock.hpp`
В файле должны быть: `manifestChecksum`

**Создай файл** `engine/package/include/sky/package/package_system.hpp`
В файле должны быть: `IPackageResolver`, `IPackageRegistry`, `registerPackage`, `manifest`, `activePackages`, `IExtensionRegistry`, `registerExtension`, `IPackageActivationService`, `activate`, `deactivate`

**Создай файл** `engine/package/include/sky/package/package_world.hpp`
В файле должны быть: `PackageWorld`, `discoverPackages`, `discoveredPackages`

**Создай файл** `engine/package/include/sky/package/semver.hpp`
В файле должны быть: `str`, `parseVersion`, `optional`, `satisfies`

**Создай файл** `engine/package/src/package_installer.cpp`
В файле должны быть: `runCommand`, `shellQuote`, `copyTree`, `exists`, `isTarball`, `tolower`, `findPackageDir`, `cachePackageDir`, `loadPackageManifest`

**Создай файл** `engine/package/src/package_world.cpp`
В файле должны быть: `loadPackageManifest`, `reader`, `categoryOf`, `discoverPackages`, `readManifest`, `parseVersion`, `discoveredPackages`, `satisfies`, `registerPackage`, `manifest`, `activePackages`, `registerExtension`, `activate`, `deactivate`, `manifestChecksum`, `readManifestImpl`


## engine/physics

**Создай файл** `engine/physics/include/sky/physics/physics.hpp`
В файле должны быть: `IPhysicsWorld`, `createBody`, `destroyBody`, `attachCollider`, `detachCollider`, `step`, `drainCollisionEvents`, `IPhysicsQueryService`, `bodyTransform`, `IPhysicsSyncContract`, `pushKinematicState`, `pullSimulationResults`

**Создай файл** `engine/physics/include/sky/physics/physics_world.hpp`
В файле должны быть: `PhysicsWorld`, `setGravity`, `setBodyVelocity`, `bodyVelocity`, `setBodyTransform`, `createPhysicsWorld`, `ObjectPhysicsSync`, `bind`, `unbind`

**Создай файл** `engine/physics/src/physics_world.cpp`
В файле должны быть: `overlaps`, `createBody`, `destroyBody`, `attachCollider`, `invalid`, `detachCollider`, `step`, `resolveHeightfields`, `detectAndResolve`, `drainCollisionEvents`, `exchange`, `worldAabb`, `bodyTransform`, `setBodyVelocity`, `bodyVelocity`, `setBodyTransform`, `sampleHeightfield`, `sample`, `heightAt`, `sqrt`, `resolve`, `centerOf`, `ObjectPhysicsSyncImpl`, `bind`, `pushKinematicState`, `pullSimulationResults`, `createPhysicsWorld`


## engine/platform

**Создай файл** `engine/platform/include/sky/platform/cocoa_window_system.hpp`
В файле должны быть: `CocoaWindowSystem`, `connected`, `metalLayer`, `createCocoaWindowSystem`

**Создай файл** `engine/platform/include/sky/platform/file_system.hpp`
В файле должны быть: `IFileSystem`, `exists`, `isDirectory`, `readAll`, `createDirectories`, `remove`, `list`

**Создай файл** `engine/platform/include/sky/platform/input_source.hpp`
В файле должны быть: `IInputSource`, `setEventCallback`, `poll`

**Создай файл** `engine/platform/include/sky/platform/platform_services.hpp`
В файле должны быть: `createStdFileSystem`, `createChronoTimerService`, `createStdThreading`, `createHeadlessWindowSystem`

**Создай файл** `engine/platform/include/sky/platform/threading.hpp`
В файле должны быть: `IThreadingPrimitives`, `launchThread`, `hardwareConcurrency`, `currentThreadId`

**Создай файл** `engine/platform/include/sky/platform/timer_service.hpp`
В файле должны быть: `ITimerService`, `monotonicNow`, `frameTimestamp`

**Создай файл** `engine/platform/include/sky/platform/virtual_file_system.hpp`
В файле должны быть: `parse`, `IVfsMount`, `readOnly`, `exists`, `read`, `write`, `remove`, `IVirtualFileSystem`, `unmountAll`, `aliases`, `readAll`, `list`, `createVirtualFileSystem`

**Создай файл** `engine/platform/include/sky/platform/window_system.hpp`
В файле должны быть: `IWindowSystem`, `createWindow`, `destroyWindow`, `setTitle`, `pumpEvents`

**Создай файл** `engine/platform/include/sky/platform/x11_window_system.hpp`
В файле должны быть: `X11WindowSystem`, `connected`, `createX11WindowSystem`

**Создай файл** `engine/platform/src/chrono_timer_service.cpp`
В файле должны быть: `monotonicNow`, `now`, `frameTimestamp`, `createChronoTimerService`

**Создай файл** `engine/platform/src/cocoa_window_system.mm`
В файле должны быть: `CocoaWindowSystemImpl`, `createWindow`, `NSMakeRect`, `MTLCreateSystemDefaultDevice`, `CGSizeMake`, `destroyWindow`, `setTitle`, `NSMakeSize`, `pumpEvents`, `translate`, `setEventCallback`, `metalLayer`, `buttonOf`, `callback_`, `createCocoaWindowSystem`, `connected`

**Создай файл** `engine/platform/src/headless_window_system.cpp`
В файле должны быть: `createWindow`, `setTitle`, `createHeadlessWindowSystem`

**Создай файл** `engine/platform/src/pak_archive.cpp`
В файле должны быть: `appendRaw`, `appendValue`, `readValue`, `PakMount`, `exists`, `read`, `list`, `collectFiles`, `valid`

**Создай файл** `engine/platform/src/std_file_system.cpp`
В файле должны быть: `exists`, `isDirectory`, `readAll`, `stream`, `writeAll`, `createDirectories`, `remove`, `list`, `createStdFileSystem`

**Создай файл** `engine/platform/src/std_threading.cpp`
В файле должны быть: `launchThread`, `thread`, `hardwareConcurrency`, `currentThreadId`, `get_id`, `createStdThreading`

**Создай файл** `engine/platform/src/virtual_file_system.cpp`
В файле должны быть: `parse`, `replace`, `DirectoryMount`, `exists`, `read`, `remove`, `list`, `aliases`, `resolveStack`, `any_of`, `readAll`, `readOnly`, `write`, `createVirtualFileSystem`

**Создай файл** `engine/platform/src/x11_window_system.cpp`
В файле должны быть: `X11WindowSystemImpl`, `XOpenDisplay`, `XInternAtom`, `XDestroyWindow`, `XCloseDisplay`, `createWindow`, `DefaultScreen`, `BlackPixel`, `XSetWMProtocols`, `XStoreName`, `XMapWindow`, `XFlush`, `destroyWindow`, `setTitle`, `XResizeWindow`, `pumpEvents`, `XNextEvent`, `translate`, `setEventCallback`, `handleOf`, `emit`, `callback_`, `XLookupKeysym`, `createX11WindowSystem`, `connected`


## engine/project

**Создай файл** `engine/project/include/sky/project/project_model.hpp`
В файле должны быть: `IProjectRepository`, `createProject`, `openProject`, `saveProject`, `closeProject`, `IProjectQueryService`, `descriptor`, `sceneList`

**Создай файл** `engine/project/include/sky/project/project_repository.hpp`
В файле должны быть: `ProjectRepository`

**Создай файл** `engine/project/src/project_repository.cpp`
В файле должны быть: `ProjectRepositoryImpl`, `createProject`, `invalid`, `openProject`, `reader`, `saveProject`, `persist`, `descriptor`, `sceneList`, `readStringList`, `consumer`


## engine/rendering

**Создай файл** `engine/rendering/include/sky/rendering/material.hpp`
В файле должны быть: `IMaterialLibrary`, `createMaterial`, `updateMaterial`, `removeMaterial`, `material`, `allMaterials`, `createMaterialLibrary`

**Создай файл** `engine/rendering/include/sky/rendering/null_renderer.hpp`
В файле должны быть: `NullRenderer`, `frameCount`, `commandsInLastFrame`, `liveResourceCount`, `createNullRenderer`

**Создай файл** `engine/rendering/include/sky/rendering/renderer_registry.hpp`
В файле должны быть: `IRendererRegistry`, `registerBackend`, `availableBackends`, `hasBackend`, `createRendererRegistry`

**Создай файл** `engine/rendering/include/sky/rendering/rendering.hpp`
В файле должны быть: `IRenderSurface`, `width`, `height`, `present`, `IRenderResourceFactory`, `destroy`, `IRenderer`, `backendName`, `attachSurface`, `submit`, `renderFrame`

**Создай файл** `engine/rendering/src/material_library.cpp`
В файле должны быть: `createMaterial`, `invalid`, `updateMaterial`, `removeMaterial`, `findMaterial`, `material`, `allMaterials`, `createMaterialLibrary`

**Создай файл** `engine/rendering/src/null_renderer.cpp`
В файле должны быть: `OffscreenSurface`, `submit`, `renderFrame`, `present`, `invalid`, `size_t`, `destroy`, `createNullRenderer`

**Создай файл** `engine/rendering/src/renderer_registry.cpp`
В файле должны быть: `RendererRegistryImpl`, `createNullRenderer`, `registerBackend`, `availableBackends`, `hasBackend`, `second`, `createRendererRegistry`


## engine/rendering_opengl

**Создай файл** `engine/rendering_opengl/include/sky/rendering_opengl/opengl_backend.hpp`
В файле должны быть: `OpenGlRenderer`, `ready`, `createOpenGlRenderer`, `registerOpenGlBackend`

**Создай файл** `engine/rendering_opengl/src/opengl_renderer.cpp`
В файле должны быть: `void`, `GLuint`, `GLint`, `GLenum`, `load`, `loader`, `resolve`, `identity`, `fromTransform`, `perspective`, `orthographic`, `lookAt`, `sqrt`, `normalize`, `cross`, `viewFromCameraPose`, `layout`, `main`, `vec4`, `mat3`, `distributionGGX`, `geometrySchlick`, `fresnelSchlick`, `clamp`, `mix`, `dFdx`, `dFdy`, `inversesqrt`, `dot`, `texture`, `vec2`, `vec3`, `length`, `OpenGlRendererImpl`, `buildProgram`, `makeSolidTexture`, `setupVertexAttributes`, `attachSurface`, `submit`, `renderFrame`, `rotate`, `flushLights`, `bindMap`, `present`, `invalid`, `size_t`, `destroy`, `compile`, `createOpenGlRenderer`, `registerOpenGlBackend`


## engine/rendering_vulkan

**Создай файл** `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
В файле должны быть: `VulkanRenderer`, `ready`, `readbackFrame`, `frameWidth`, `frameHeight`, `presentedFrames`

**Создай файл** `engine/rendering_vulkan/src/vulkan_renderer.cpp`
В файле должны быть: `identity`, `fromTransform`, `perspective`, `orthographic`, `viewFromCameraPose`, `lookAlong`, `sqrt`, `norm`, `cross`, `width_`, `height_`, `initFrameResources`, `initShadowResources`, `VulkanRendererImpl`, `vkDeviceWaitIdle`, `destroyBuffer`, `destroyTexture`, `vkDestroySampler`, `vkDestroyDescriptorSetLayout`, `vkDestroyDescriptorPool`, `vkDestroyPipeline`, `vkDestroyPipelineLayout`, `vkDestroyFramebuffer`, `vkDestroyRenderPass`, `vkDestroyImageView`, `vkDestroyImage`, `vkFreeMemory`, `vkDestroySemaphore`, `vkDestroySwapchainKHR`, `vkDestroyCommandPool`, `vkDestroyDevice`, `vkDestroySurfaceKHR`, `vkDestroyInstance`, `attachSurface`, `submit`, `renderFrame`, `vkResetCommandBuffer`, `vkBeginCommandBuffer`, `vkCmdSetViewport`, `vkCmdSetScissor`, `vkCmdDraw`, `vkCmdEndRenderPass`, `vkCmdBeginRenderPass`, `vkCmdBindPipeline`, `bindMaterial`, `drawBuffer`, `vkEndCommandBuffer`, `vkQueueSubmit`, `vkQueueWaitIdle`, `present`, `invalid`, `size_t`, `uploadTexture`, `destroy`, `readbackFrame`, `pixels`, `initInstanceAndDevice`, `vkEnumeratePhysicalDevices`, `devices`, `vkGetPhysicalDeviceQueueFamilyProperties`, `families`, `vkGetDeviceQueue`, `vkGetPhysicalDeviceMemoryProperties`, `vkGetImageMemoryRequirements`, `vkBindImageMemory`, `vkCreateImageView`, `initSwapchainTarget`, `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`, `formats`, `vkGetSwapchainImagesKHR`, `initTarget`, `createShader`, `vkCreateShaderModule`, `initPipeline`, `vkDestroyShaderModule`, `vkGetBufferMemoryRequirements`, `vkBindBufferMemory`, `vkMapMemory`, `createVertexBuffer`, `vkUnmapMemory`, `vkUpdateDescriptorSets`, `VkDeviceSize`, `vkCmdBindVertexBuffers`, `vkDestroyBuffer`, `ready`, `createVulkanRenderer`


## engine/scene

**Создай файл** `engine/scene/include/sky/scene/scene_authoring.hpp`
В файле должны быть: `primitiveMeshName`

**Создай файл** `engine/scene/include/sky/scene/scene_system.hpp`
В файле должны быть: `ISceneRepository`, `createScene`, `loadScene`, `saveScene`, `unloadScene`, `ISceneRuntime`, `activate`, `deactivate`, `activeContext`, `tick`, `ISceneQueryService`, `loadedScenes`, `descriptor`, `sceneOf`

**Создай файл** `engine/scene/include/sky/scene/scene_world.hpp`
В файле должны быть: `SceneWorld`, `addRootObject`, `createSceneWorld`

**Создай файл** `engine/scene/src/scene_authoring.cpp`
В файле должны быть: `cloneRecursive`, `primitiveMeshName`, `invalid`

**Создай файл** `engine/scene/src/scene_world.cpp`
В файле должны быть: `reader`, `writeTransform`, `readTransform`, `SceneWorldImpl`, `deps_`, `createScene`, `loadScene`, `invalid`, `migrate`, `unloadScene`, `addRootObject`, `readField`, `saveScene`, `writeScene`, `rootObjectsOf`, `deactivate`, `activate`, `tick`, `pushKinematicState`, `step`, `pullSimulationResults`, `pushAuthoringState`, `pullEcsResults`, `loadedScenes`, `descriptor`, `sceneOf`, `flatten`, `fields`, `writeField`, `createSceneWorld`


## engine/scripting

**Создай файл** `engine/scripting/include/sky/scripting/dotnet_host.hpp`
В файле должны быть: `DotNetScriptHost`, `probeValue`, `installEngineApi`, `beginFrame`, `scriptClassNames`, `loadUserAssembly`, `unloadUserAssembly`, `createDotNetScriptHost`

**Создай файл** `engine/scripting/include/sky/scripting/script_host.hpp`
В файле должны быть: `IScriptHost`, `start`, `shutdown`, `loadAssembly`, `loadedAssemblies`, `createInstance`, `destroyInstance`, `IDomainReloadPolicy`, `policy`, `canReloadNow`, `requestReload`

**Создай файл** `engine/scripting/include/sky/scripting/script_runtime.hpp`
В файле должны быть: `ScriptRuntime`, `createScriptRuntime`

**Создай файл** `engine/scripting/include/sky/scripting/scripting_boundary.hpp`
В файле должны быть: `IScriptBindingService`, `registerBinding`, `unbindInstance`, `IScriptLifecycleBridge`, `dispatchAll`, `INativeHandleRegistry`, `allocate`, `release`, `resolve`

**Создай файл** `engine/scripting/src/dotnet_host.cpp`
В файле должны быть: `int32_t`, `uint64_t`, `void`, `int64_t`, `splitLines`, `discoverHostfxr`, `DotNetScriptHostImpl`, `config_`, `start`, `dlopen`, `dlsym`, `initialize`, `resolve`, `shutdown`, `close_`, `loadAssembly`, `managedLoadAssembly_`, `createInstance`, `managedCreateInstance_`, `destroyInstance`, `managedDestroyInstance_`, `probeValue`, `managedGetProbe_`, `installEngineApi`, `managedInitialize_`, `managedSetObjectId_`, `beginFrame`, `managedTickFrame_`, `scriptClassNames`, `buffer`, `scriptFields`, `loadUserAssembly`, `managedLoadUserAssembly_`, `unloadUserAssembly`, `managedUnloadUserAssembly_`, `createDotNetScriptHost`, `available`

**Создай файл** `engine/scripting/src/script_runtime.cpp`
В файле должны быть: `registerBinding`, `bindingFor`, `invalid`, `allocate`, `unbindInstance`, `release`, `dispatchAll`, `resolve`, `createScriptRuntime`


## engine/serialization

**Создай файл** `engine/serialization/include/sky/serialization/backends.hpp`
В файле должны быть: `SchemaMigrationService`, `createSchemaMigrationService`

**Создай файл** `engine/serialization/include/sky/serialization/byte_stream.hpp`
В файле должны быть: `writeString`, `writeU32`, `writeRaw`, `writeBytes`, `writeU64`, `readString`, `readU32`, `remaining`, `value`, `readBytes`, `readU64`, `readRaw`

**Создай файл** `engine/serialization/include/sky/serialization/serialization.hpp`
В файле должны быть: `ISerializationBackend`, `write`, `read`, `ISchemaMigrationService`

**Создай файл** `engine/serialization/src/file_serialization_backend.cpp`
В файле должны быть: `FileSerializationBackend`, `write`, `read`, `reader`

**Создай файл** `engine/serialization/src/schema_migration.cpp`
В файле должны быть: `createSchemaMigrationService`


## engine/terrain

**Создай файл** `engine/terrain/include/sky/terrain/terrain.hpp`
В файле должны быть: `ITerrainService`, `createTerrain`, `destroyTerrain`, `applyEdit`, `replaceDataset`, `ITerrainQueryService`, `dataset`, `heightAt`, `ITerrainPersistenceContract`, `save`, `load`

**Создай файл** `engine/terrain/include/sky/terrain/terrain_integration.hpp`
В файле должны быть: `makeTerrainCollider`, `buildTerrainMesh`

**Создай файл** `engine/terrain/include/sky/terrain/terrain_world.hpp`
В файле должны быть: `TerrainWorld`, `onTerrainChanged`

**Создай файл** `engine/terrain/src/terrain_integration.cpp`
В файле должны быть: `makeTerrainCollider`, `vertexAt`, `normalAt`, `sqrt`, `buildTerrainMesh`, `appendVertex`

**Создай файл** `engine/terrain/src/terrain_world.cpp`
В файле должны быть: `TerrainWorldImpl`, `createTerrain`, `invalid`, `notify`, `applyEdit`, `sqrt`, `blendWithNeighbours`, `replaceDataset`, `dataset`, `heightAt`, `sample`, `save`, `load`, `reader`, `setStoragePath`, `hook`


## managed/SkyEngine.Managed

**Создай файл** `managed/SkyEngine.Managed/Bootstrap.cs`
В файле должны быть: `LoadAssembly`, `LoadUserAssembly`, `AssemblyLoadContext`, `MemoryStream`, `UnloadUserAssembly`, `Initialize`, `TickFrame`, `SetObjectId`, `NativeHandle`, `DestroyInstance`, `InvokeLifecycle`, `GetScriptClasses`, `GetScriptFields`, `SetScriptField`, `FieldKind`, `GetProbe`, `ResolveType`, `IProbe`, `Bootstrap`, `picker`, `shadows`

**Создай файл** `managed/SkyEngine.Managed/Debug.cs`
В файле должны быть: `Log`, `LogWarning`, `LogError`, `Write`, `Debug`

**Создай файл** `managed/SkyEngine.Managed/Engine.cs`
В файле должны быть: `SetVec3Fn`, `GetVec3Fn`, `LogFn`, `IsKeyDownFn`, `InstantiateFn`, `DestroyFn`, `RaycastFn`, `Install`, `Engine`, `Api`

**Создай файл** `managed/SkyEngine.Managed/Input.cs`
В файле должны быть: `GetKey`, `KeyCode`, `Input`

**Создай файл** `managed/SkyEngine.Managed/NativeHandle.cs`
В файле должны быть: `NativeHandle`, `struct`

**Создай файл** `managed/SkyEngine.Managed/Physics.cs`
В файле должны быть: `Raycast`, `RaycastHit`, `Physics`

**Создай файл** `managed/SkyEngine.Managed/ScriptComponent.cs`
В файле должны быть: `SetLocalPosition`, `SetLocalEuler`, `SetLocalScale`, `SetVelocity`, `id`, `Instantiate`, `Destroy`, `OnCreate`, `OnStart`, `OnUpdate`, `OnFixedUpdate`, `OnDestroy`, `for`, `ScriptComponent`

**Создай файл** `managed/SkyEngine.Managed/Time.cs`
В файле должны быть: `Time`


## managed/SkyEngine.TestScripts

**Создай файл** `managed/SkyEngine.TestScripts/CrateRain.cs`
В файле должны быть: `OnStart`, `OnUpdate`, `CrateRain`

**Создай файл** `managed/SkyEngine.TestScripts/GroundProbe.cs`
В файле должны быть: `OnStart`, `GroundProbe`

**Создай файл** `managed/SkyEngine.TestScripts/Rotator.cs`
В файле должны быть: `OnStart`, `OnUpdate`, `Rotator`

**Создай файл** `managed/SkyEngine.TestScripts/SelfDestruct.cs`
В файле должны быть: `OnUpdate`, `SelfDestruct`

**Создай файл** `managed/SkyEngine.TestScripts/Spawner.cs`
В файле должны быть: `OnUpdate`, `Spawner`

**Создай файл** `managed/SkyEngine.TestScripts/Spinner.cs`
В файле должны быть: `OnCreate`, `OnStart`, `OnUpdate`, `Spinner`, `Faulty`

**Создай файл** `managed/SkyEngine.TestScripts/WasdMover.cs`
В файле должны быть: `OnUpdate`, `WasdMover`


## player

**Создай файл** `player/src/main.cpp`
В файле должны быть: `mapPlatformKey`, `runHeadless`, `createVulkanRenderer`, `builder`, `setScene`, `play`, `tickFrame`, `submit`, `renderFrame`, `readbackFrame`, `frameWidth`, `frameHeight`, `encodePngRgba`, `writeAll`, `runWindowed`, `createCocoaWindowSystem`, `createX11WindowSystem`, `createWindow`, `metalLayer`, `nativeHandles`, `setEventCallback`, `now`, `pumpEvents`, `uint64_t`, `presentedFrames`, `main`


## tests

**Создай файл** `tests/asset_project_tests.cpp`
В файле должны быть: `supports`, `path`, `testAssetDatabase`, `createAssetDatabase`, `registerImporter`, `importAsset`, `assetIdFromPath`, `findBySourcePath`, `resolve`, `dependentsOf`, `reimport`, `unregisterAsset`, `testProjectRoundTrip`, `createStdFileSystem`, `createFileSerializationBackend`, `createProjectRepository`, `createProject`, `exists`, `openProject`, `descriptor`, `sceneList`, `closeProject`, `main`, `summary`

**Создай файл** `tests/core_tests.cpp`
В файле должны быть: `testConfigService`, `createInMemoryConfigService`, `set`, `getInt`, `getBool`, `getString`, `testEventBus`, `createEventBus`, `publish`, `unsubscribe`, `testJobScheduler`, `createThreadPoolScheduler`, `schedule`, `scheduleAfter`, `wait`, `yield`, `testDiagnostics`, `createInMemoryDiagnostics`, `counter`, `timingMicros`, `counterValue`, `lastTimingMicros`, `testMath`, `sqrt`, `rotate`, `compose`, `main`, `summary`

**Создай файл** `tests/dotnet_host_tests.cpp`
В файле должны быть: `startHost`, `createDotNetScriptHost`, `start`, `testLifecycleThroughRealDotNet`, `createInstance`, `loadedAssemblies`, `probeValue`, `destroyInstance`, `testSceneTickDrivesCSharp`, `createScriptRuntime`, `createObjectWorld`, `createComponentWorld`, `createStdFileSystem`, `createFileSerializationBackend`, `createSceneWorld`, `createScene`, `createObject`, `addRootObject`, `bindInstance`, `resolve`, `dispatch`, `activate`, `tick`, `unbindInstance`, `main`, `summary`

**Создай файл** `tests/editor_bridge_tests.cpp`
В файле должны быть: `nameOf`, `sky_editor_object_name`, `sky_editor_component_count`, `sky_editor_component_type`, `testBridgeLifecycleAndHierarchy`, `sky_editor_create`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`, `sky_editor_destroy`, `testBridgeAuthoring`, `sky_editor_create_primitive`, `sky_editor_set_position`, `sky_editor_get_transform`, `sky_editor_duplicate`, `sky_editor_delete`, `strlen`, `testBridgePicking`, `sky_editor_frame_object`, `sky_editor_viewport_zoom`, `sky_editor_pick`, `sky_editor_viewport_orbit`, `sky_editor_world_position`, `testBridgeComponentFields`, `sky_editor_component_field_name`, `sky_editor_set_component_field`, `sky_editor_component_field_value`, `testBridgeTransformSpaces`, `sky_editor_set_world_position`, `sky_editor_set_local_euler`, `sky_editor_translate_self`, `sky_editor_get_world_transform`, `testBridgeViewport`, `createX11WindowSystem`, `puts`, `createWindow`, `pumpEvents`, `nativeHandles`, `sky_editor_attach_viewport`, `sky_editor_render_viewport`, `XSync`, `int`, `XGetPixel`, `XDestroyImage`, `sky_editor_detach_viewport`, `destroyWindow`, `testBridgeScriptLog`, `sky_editor_log_count`, `sky_editor_log_text`, `sky_editor_stop`, `testBridgeScriptInput`, `sky_editor_set_key_state`, `sky_editor_tick_play`, `testBridgeScriptClasses`, `sky_editor_script_class_count`, `sky_editor_script_class_name`, `testBridgeScriptFields`, `sky_editor_script_field_count`, `sky_editor_script_field_name`, `sky_editor_script_field_type`, `sky_editor_script_field_value`, `sky_editor_set_script_field`, `testBridgeUserScripts`, `sky_editor_assets_root`, `path`, `source`, `sky_editor_add_component`, `testBridgePrefabs`, `sky_editor_instantiate_prefab`, `testBridgeScriptEngineApi`, `attachScript`, `scriptFieldIndex`, `testBridgePackagePersistence`, `remove`, `sky_editor_package_count`, `sky_editor_package_info`, `findPackages`, `sky_editor_package_set_active`, `testBridgePackageInstall`, `createStdFileSystem`, `createFileSerializationBackend`, `savePackageManifest`, `testBridgePackageCode`, `setActive`, `testBridgeCrateRain`, `main`, `summary`

**Создай файл** `tests/header_check.cpp`
В файле должны быть: `main`, `invalid`, `puts`

**Создай файл** `tests/integrity_tests.cpp`
В файле должны быть: `testRoot`, `update`, `testEcsObjectSync`, `createObjectWorld`, `createEcsWorld`, `createEcsObjectSync`, `createObject`, `setLocalTransform`, `bind`, `entityOf`, `objectOf`, `pushAuthoringState`, `registerSystem`, `tick`, `pullEcsResults`, `localTransform`, `unregisterSystem`, `unbind`, `isAlive`, `createComponentWorld`, `createStdFileSystem`, `createFileSerializationBackend`, `createSchemaMigrationService`, `SceneFixture`, `createSceneWorld`, `testComponentFieldRoundTrip`, `createScene`, `addRootObject`, `attach`, `setField`, `fields`, `saveScene`, `loadScene`, `findByName`, `componentsOf`, `field`, `testLegacySceneMigration`, `write`, `canMigrate`, `hillDataset`, `testTerrainPhysicsIntegration`, `createPhysicsWorld`, `createBody`, `attachCollider`, `makeTerrainCollider`, `step`, `drainCollisionEvents`, `bodyTransform`, `testTerrainRenderIntegration`, `buildTerrainMesh`, `sqrt`, `createNullRenderer`, `createMeshFromData`, `testGenerationMaterialization`, `createTerrainWorld`, `createGenerationPipeline`, `createTerrain`, `generate`, `succeeded`, `dataset`, `terrainOutput`, `placements`, `activate`, `activeContext`, `sceneOf`, `main`, `summary`

**Создай файл** `tests/package_tests.cpp`
В файле должны быть: `packagesRoot`, `savePackageManifest`, `testDiscoveryAndResolution`, `createStdFileSystem`, `createFileSerializationBackend`, `writePackage`, `createPackageWorld`, `discoverPackages`, `resolve`, `registerPackage`, `activate`, `activePackages`, `extensionsByCategory`, `deactivate`, `testCycleDetection`, `testSemver`, `testMinimalVersionSelection`, `writePackageVersion`, `testLockRoundTrip`, `manifestChecksum`, `savePackageLock`, `loadPackageLock`, `testInstaller`, `puts`, `payload`, `installer`, `exists`, `main`, `summary`

**Создай файл** `tests/physics_tests.cpp`
В файле должны быть: `testGravityIntegration`, `createPhysicsWorld`, `step`, `bodyVelocity`, `bodyTransform`, `testCollisionAndResolution`, `drainCollisionEvents`, `testRaycast`, `raycast`, `testObjectSync`, `createObjectWorld`, `createObjectPhysicsSync`, `createObject`, `setLocalTransform`, `createBody`, `bind`, `pushKinematicState`, `pullSimulationResults`, `localTransform`, `unbind`, `main`, `summary`

**Создай файл** `tests/platform_serialization_tests.cpp`
В файле должны быть: `tempDir`, `testFileSystem`, `createStdFileSystem`, `writeAll`, `exists`, `isDirectory`, `readAll`, `list`, `remove`, `testByteStream`, `reader`, `testSerializationRoundTrip`, `createFileSerializationBackend`, `write`, `read`, `testTimerAndThreading`, `createChronoTimerService`, `frameTimestamp`, `createStdThreading`, `hardwareConcurrency`, `currentThreadId`, `testHeadlessWindowSystem`, `createHeadlessWindowSystem`, `createWindow`, `setTitle`, `pumpEvents`, `destroyWindow`, `main`, `summary`

**Создай файл** `tests/runtime_tests.cpp`
В файле должны быть: `update`, `testPlayModeDrivesRuntime`, `createObjectWorld`, `createComponentWorld`, `createStdFileSystem`, `createFileSerializationBackend`, `createEcsWorld`, `createPhysicsWorld`, `createObjectPhysicsSync`, `createScene`, `createObject`, `setLocalTransform`, `addRootObject`, `createBody`, `bind`, `createEntity`, `spinSystem`, `registerSystem`, `createPlayModeController`, `play`, `setScene`, `onStateChanged`, `tickFrame`, `localTransform`, `activeContext`, `pause`, `stop`, `main`, `summary`

**Создай файл** `tests/scene_tests.cpp`
В файле должны быть: `createObjectWorld`, `createComponentWorld`, `createStdFileSystem`, `createFileSerializationBackend`, `createSceneWorld`, `testSceneRoundTrip`, `registerComponentType`, `createScene`, `createObject`, `setParent`, `attach`, `addRootObject`, `saveScene`, `sceneOf`, `loadScene`, `descriptor`, `findByName`, `childrenOf`, `nameOf`, `localTransform`, `componentsOf`, `activate`, `activeContext`, `deactivate`, `unloadScene`, `exists`, `loadedScenes`, `testLoadRejectsCorruptScene`, `writeAll`, `testSceneAuthoring`, `createPrimitive`, `field`, `setField`, `setLocalTransform`, `duplicateObject`, `main`, `summary`

**Создай файл** `tests/scripting_rendering_tests.cpp`
В файле должны быть: `loadAssembly`, `loadedAssemblies`, `createInstance`, `destroyInstance`, `testScriptingBoundary`, `createScriptRuntime`, `registerBinding`, `bindingFor`, `bindInstance`, `resolve`, `dispatch`, `unbindInstance`, `testNullRenderer`, `createNullRenderer`, `backendName`, `createOffscreenSurface`, `width`, `attachSurface`, `createFromAsset`, `liveResourceCount`, `submit`, `renderFrame`, `frameCount`, `commandsInLastFrame`, `destroy`, `testRendererRegistry`, `createRendererRegistry`, `hasBackend`, `create`, `main`, `summary`

**Создай файл** `tests/sky_test.hpp`
В файле должны быть: `summary`

**Создай файл** `tests/terrain_mapgen_tests.cpp`
В файле должны быть: `flatDataset`, `testTerrainEditingAndHooks`, `createStdFileSystem`, `createFileSerializationBackend`, `createTerrainWorld`, `onTerrainChanged`, `createTerrain`, `applyEdit`, `heightAt`, `testTerrainPersistence`, `setStoragePath`, `save`, `load`, `dataset`, `testGenerationDeterminism`, `createGenerationPipeline`, `availableStages`, `generate`, `succeeded`, `terrainOutput`, `placements`, `replaceDataset`, `main`, `summary`

**Создай файл** `tests/undo_tests.cpp`
В файле должны быть: `testTransformUndoRedo`, `stack`, `findByName`, `localTransform`, `setLocalTransform`, `makeTransformCommand`, `testDeleteRestoresSubtree`, `makeDeleteCommand`, `exists`, `componentsOf`, `childrenOf`, `testCreateRenameReparent`, `makeCreateCommand`, `renameObject`, `makeRenameCommand`, `nameOf`, `invalid`, `parentOf`, `testHistoryDiscipline`, `makeDuplicateCommand`, `testToolCommandBus`, `createToolCommandBus`, `execute`, `undo`, `redo`, `registerHandler`, `registeredCommands`, `setUndoDelegate`, `setRedoDelegate`, `main`, `summary`

**Создай файл** `tests/vfs_tests.cpp`
В файле должны быть: `testRoot`, `bytes`, `result`, `testPathParsing`, `parse`, `testDirectoryMountAndRouting`, `createStdFileSystem`, `createVirtualFileSystem`, `createDirectories`, `createDirectoryMount`, `aliases`, `writeAll`, `remove`, `testOverlayPriorities`, `exists`, `unmountAll`, `testPakArchive`, `buildPakArchive`, `createPakMount`, `mount`, `main`, `summary`

**Создай файл** `tests/visual_pipeline_tests.cpp`
В файле должны быть: `testRoot`, `bytes`, `testObjParsing`, `createStdFileSystem`, `loadObjMesh`, `sqrt`, `writeText`, `createAssetDatabase`, `createObjImporter`, `registerImporter`, `importAsset`, `resolve`, `createNullRenderer`, `createMeshFromData`, `remove`, `testUvParsing`, `testFbxImport`, `loadFbxMesh`, `empty`, `createFbxImporter`, `base64Encode`, `triangleGltfJson`, `triangleGlb`, `bin`, `byte`, `push32`, `uint32_t`, `testGltfImport`, `loadGltfMesh`, `writeAll`, `createGltfImporter`, `testPngRoundTrip`, `encodePngRgba`, `decodePng`, `createPngImporter`, `createTextureFromData`, `testMaterialLibrary`, `createMaterialLibrary`, `createMaterial`, `findMaterial`, `material`, `updateMaterial`, `allMaterials`, `main`, `summary`

**Создай файл** `tests/vulkan_tests.cpp`
В файле должны быть: `size_t`, `testVulkanFrame`, `createVulkanRenderer`, `backendName`, `frameWidth`, `submit`, `renderFrame`, `readbackFrame`, `pixelAt`, `testVulkanTexturing`, `createTextureFromData`, `commands`, `testVulkanPbrMaps`, `renderCube`, `testVulkanResourcesAndRegistry`, `triangle`, `createMeshFromData`, `destroy`, `createRendererRegistry`, `registerVulkanBackend`, `hasBackend`, `create`, `testSwapchainPresentation`, `createX11WindowSystem`, `puts`, `createWindow`, `pumpEvents`, `nativeHandles`, `presentedFrames`, `XSync`, `XGetPixel`, `XDestroyImage`, `destroyWindow`, `main`, `summary`

**Создай файл** `tests/world_tests.cpp`
В файле должны быть: `testWorldTransformWriteback`, `createObjectWorld`, `createObject`, `setParent`, `setWorldTransform`, `worldTransform`, `testObjectHierarchy`, `parentOf`, `childrenOf`, `findByName`, `setLocalTransform`, `destroyObject`, `exists`, `testComponentWorld`, `createComponentWorld`, `registerComponentType`, `availableTypes`, `attach`, `componentsOf`, `ownerOf`, `descriptorOf`, `detach`, `detachAllFrom`, `update`, `typeid`, `testEcsWorld`, `createEcsWorld`, `createEntity`, `isAlive`, `entitiesWith`, `registerSystem`, `tick`, `destroyEntity`, `unregisterSystem`, `main`, `summary`

**Создай файл** `tests/x11_window_tests.cpp`
В файле должны быть: `sendKeyEvent`, `DefaultRootWindow`, `XFlush`, `sendClose`, `XInternAtom`, `XSendEvent`, `firstNativeWindow`, `XQueryTree`, `XFree`, `testWindowLifecycleAndInput`, `createX11WindowSystem`, `puts`, `createWindow`, `windowSize`, `setTitle`, `pumpEvents`, `setEventCallback`, `XOpenDisplay`, `XSync`, `XCloseDisplay`, `destroyWindow`, `main`, `summary`
