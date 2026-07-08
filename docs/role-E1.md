# Техническое задание · Контур E1 «Ядро и данные»

**Область ответственности.** Математический фундамент, объектная и компонентная модели, сцена, сериализация, отмена операций, сборочная точка редактора.

Все пути и имена методов взяты из фактического репозитория и совпадают с проектом 1:1. «Сделай файл» — файл создаётся на этом этапе; «Дополни файл» — в существующий файл добавляются перечисленные методы.


---

## Этап 1 (Неделя 1). Математика, объектная и компонентная модели

**Общее описание задач контура.**

Реализовать математику, службы ядра, объектную модель (иерархия и трансформы) и компонентную модель с полями-данными.

- **Сделай файл** `engine/core/include/sky/core/math.hpp`
  В файле должны быть: `rotate`, `compose`, `divide`, `invCompose`, `conjugate`
- **Сделай файл** `engine/core/include/sky/core/handle.hpp`
  (данные/разметка — функций нет)
- **Сделай файл** `engine/core/include/sky/core/logger.hpp`
  В файле должны быть: `ILogger`, `log`, `info`, `warning`, `error`
- **Сделай файл** `engine/core/include/sky/core/config_service.hpp`
  В файле должны быть: `IConfigService`, `getString`, `getInt`, `getBool`, `set`
- **Сделай файл** `engine/core/include/sky/core/runtime_services.hpp`
  В файле должны быть: `createConsoleLogger`, `createInMemoryConfigService`, `createEventBus`, `createThreadPoolScheduler`, `counterValue`, `lastTimingMicros`, `createInMemoryDiagnostics`
- **Сделай файл** `engine/core/src/console_logger.cpp`
  В файле должны быть: `levelName`, `log`, `createConsoleLogger`
- **Сделай файл** `engine/core/src/memory_config_service.cpp`
  В файле должны быть: `getString`, `getInt`, `getBool`, `set`, `createInMemoryConfigService`
- **Сделай файл** `engine/object/include/sky/object/object_model.hpp`
  В файле должны быть: `IObjectFactory`, `createObject`, `destroyObject`, `IObjectHierarchyAccess`, `setParent`, `parentOf`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`, `IObjectQueryService`, `exists`, `nameOf`, `findByName`, `invCompose`
- **Сделай файл** `engine/object/include/sky/object/object_world.hpp`
  В файле должны быть: `ObjectWorld`, `renameObject`, `createObjectWorld`
- **Сделай файл** `engine/object/src/object_world.cpp`
  В файле должны быть: `createObject`, `renameObject`, `destroyObject`, `detachFromParent`, `setParent`, `wouldCreateCycle`, `parentOf`, `invalid`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`, `compose`, `exists`, `nameOf`, `findByName`, `createObjectWorld`
- **Сделай файл** `engine/component/include/sky/component/component_model.hpp`
  В файле должны быть: `IComponentRegistry`, `registerComponentType`, `availableTypes`, `IComponentAttachmentService`, `attach`, `detach`, `IComponentQueryService`, `ownerOf`
- **Сделай файл** `engine/component/include/sky/component/component_world.hpp`
  В файле должны быть: `ComponentWorld`, `detachAllFrom`, `createComponentWorld`
- **Сделай файл** `engine/component/src/component_world.cpp`
  В файле должны быть: `registerComponentType`, `availableTypes`, `attach`, `invalid`, `detach`, `detachAllFrom`, `fields`, `componentsOf`, `descriptorOf`, `ownerOf`, `createComponentWorld`
- **Сделай файл** `tests/core_tests.cpp`
  В файле должны быть: `testConfigService`, `createInMemoryConfigService`, `set`, `getInt`, `getBool`, `getString`, `testEventBus`, `createEventBus`, `publish`, `unsubscribe`, `testJobScheduler`, `createThreadPoolScheduler`, `schedule`, `scheduleAfter`, `wait`, `testDiagnostics`, `createInMemoryDiagnostics`, `counter`, `timingMicros`, `counterValue`, `lastTimingMicros`, `testMath`, `rotate`, `compose`, `main`, `summary`
- **Сделай файл** `tests/world_tests.cpp`
  В файле должны быть: `testWorldTransformWriteback`, `createObjectWorld`, `createObject`, `setParent`, `setWorldTransform`, `worldTransform`, `testObjectHierarchy`, `parentOf`, `childrenOf`, `findByName`, `setLocalTransform`, `destroyObject`, `exists`, `testComponentWorld`, `createComponentWorld`, `registerComponentType`, `availableTypes`, `attach`, `componentsOf`, `ownerOf`, `descriptorOf`, `detach`, `detachAllFrom`, `update`, `testEcsWorld`, `createEcsWorld`, `createEntity`, `isAlive`, `entitiesWith`, `registerSystem`, `tick`, `destroyEntity`, `unregisterSystem`, `main`, `summary`

**На выходе должно получиться (список артефактов):**
- Библиотеки `sky_core`, `sky_object`, `sky_component` собраны.
- Тесты `core_tests`, `world_tests` зелёные.

**Критерий правильности:** `ctest -R core_tests|world_tests` проходит; поворот (0,0,1) на 90° вокруг Y = (1,0,0)±1e-5; ребёнок под повёрнутым родителем даёт верный мировой трансформ; поле каждого из 5 типов round-trip'ится.


---

## Этап 2 (Неделя 2). Сцена и сборочная точка

**Общее описание задач контура.**

Реализовать модель сцены и собрать движок в единый EditorContext с демо-сценой.

- **Сделай файл** `engine/scene/include/sky/scene/scene_system.hpp`
  В файле должны быть: `ISceneRepository`, `createScene`, `loadScene`, `saveScene`, `unloadScene`, `ISceneRuntime`, `activate`, `deactivate`, `activeContext`, `tick`, `ISceneQueryService`, `loadedScenes`, `descriptor`, `sceneOf`
- **Сделай файл** `engine/scene/include/sky/scene/scene_authoring.hpp`
  В файле должны быть: `primitiveMeshName`
- **Сделай файл** `engine/scene/include/sky/scene/scene_world.hpp`
  В файле должны быть: `SceneWorld`, `addRootObject`, `createSceneWorld`
- **Сделай файл** `engine/scene/src/scene_authoring.cpp`
  В файле должны быть: `cloneRecursive`, `primitiveMeshName`, `invalid`
- **Сделай файл** `engine/scene/src/scene_world.cpp`
  В файле должны быть: `writeTransform`, `readTransform`, `SceneWorldImpl`, `createScene`, `loadScene`, `invalid`, `migrate`, `unloadScene`, `addRootObject`, `readField`, `saveScene`, `writeScene`, `rootObjectsOf`, `deactivate`, `activate`, `tick`, `pushKinematicState`, `step`, `pullSimulationResults`, `pushAuthoringState`, `pullEcsResults`, `loadedScenes`, `descriptor`, `sceneOf`, `fields`, `writeField`, `createSceneWorld`
- **Сделай файл** `editor/shell/src/editor_context.hpp`
  В файле должны быть: `EditorContext`, `beginPlay`, `endPlay`, `newScene`, `saveScene`, `openScene`, `tickScripts`, `reloadUserScripts`, `setKeyDown`, `applyTerrainBrush`, `terrainHeightAt`, `generateTerrain`, `createEmpty`, `createCrate`, `destroyObject`, `duplicateObject`, `reparent`, `savePrefab`, `instantiatePrefab`, `spawnPrefabAt`, `setObjectVelocity`, `objectVelocity`, `installPackage`, `setPackageActive`, `packageActive`, `snapshotObject`, `hasPhysicsBody`, `setObjectEnabled`, `objectEnabled`, `rootObjects`, `buildDemoScene`, `initTerrain`, `resetScene`, `reattachPhysics`, `initScripting`, `scriptSourceDirs`, `applyPackageLock`, `writePackageLock`, `startPlayScripts`, `stopPlayScripts`, `startScriptsFor`, `attachCrateBody`
- **Сделай файл** `editor/shell/src/editor_context.cpp`
  В файле должны быть: `scriptSetLocalPosition`, `localTransform`, `setLocalTransform`, `scriptSetLocalEuler`, `axisAngle`, `scriptSetLocalScale`, `scriptLogMessage`, `scriptLog`, `scriptGetLocalPosition`, `scriptIsKeyDown`, `keyDown`, `scriptGetWorldPosition`, `worldTransform`, `scriptInstantiate`, `spawnPrefabAt`, `scriptDestroyObject`, `exists`, `destroyObject`, `scriptSetVelocity`, `scriptGetVelocity`, `objectVelocity`, `path`, `populateDemoAssets`, `ofstream`, `touch`, `remove`, `EditorContext`, `createStdFileSystem`, `createVirtualFileSystem`, `createInMemoryConfigService`, `set`, `createRendererRegistry`, `registerOpenGlBackend`, `createFileSerializationBackend`, `createObjectWorld`, `createComponentWorld`, `createEcsWorld`, `createEcsObjectSync`, `createPhysicsWorld`, `createObjectPhysicsSync`, `createSchemaMigrationService`, `createSceneWorld`, `createPlayModeController`, `current_path`, `mount`, `createDirectoryMount`, `createPackageWorld`, `savePackageManifest`, `discoverPackages`, `applyPackageLock`, `createAssetDatabase`, `createObjImporter`, `createFbxImporter`, `createGltfImporter`, `createPngImporter`, `registerImporter`, `createMaterialLibrary`, `createMaterial`, `writeAll`, `encodePngRgba`, `importAsset`, `initScripting`, `buildDemoScene`, `createEmpty`, `createObject`, `addRootObject`, `createPrimitive`, `attach`, `setField`, `attachCrateBody`, `bind`, `unbind`, `destroyBody`, `detachAllFrom`, `beginPlay`, `stack`, `childrenOf`, `reloadUserScripts`, `startPlayScripts`, `endPlay`, `stopPlayScripts`, `setBodyTransform`, `setBodyVelocity`, `scriptSourceDirs`, `manifest`, `unloadUserAssembly`, `csproj`, `failed`, `loadUserAssembly`, `newestUserScriptStamp`, `createDotNetScriptHost`, `start`, `installEngineApi`, `startScriptsFor`, `componentsOf`, `descriptorOf`, `field`, `createInstance`, `setInstanceObjectId`, `fields`, `invokeLifecycle`, `tickScripts`, `beginFrame`, `destroyInstance`, `duplicateObject`, `invalid`, `parentOf`, `cloneSubtree`, `nameOf`, `setParent`, `reparent`, `rotate`, `applyTerrainBrush`, `applyEdit`, `terrainHeightAt`, `generateTerrain`, `generate`, `writeSnapshot`, `readSnapshot`, `snapshotObject`, `resolveAssetPath`, `createDirectories`, `instantiatePrefab`, `readAll`, `restoreObject`, `state`, `installPackage`, `installer`, `writePackageLock`, `setPackageActive`, `resolve`, `registerPackage`, `activate`, `deactivate`, `discoveredPackages`, `bodyVelocity`, `hasPhysicsBody`, `initTerrain`, `createTerrainWorld`, `createGenerationPipeline`, `createTerrain`, `makeTerrainCollider`, `dataset`, `onTerrainChanged`, `detachCollider`, `resetScene`, `unloadScene`, `reattachPhysics`, `newScene`, `createScene`, `saveScene`, `saveSceneAs`, `openScene`, `loadScene`, `rootObjectsOf`, `createCrate`, `objBytes`

**На выходе должно получиться (список артефактов):**
- Библиотека `sky_scene` собрана; `EditorContext` строит демо-сцену.

**Критерий правильности:** Метод перечисления корневых объектов возвращает именованные объекты демо-сцены; сцена пригодна для обхода рендером.


---

## Этап 3 (Недели 3–4). Отмена операций, формат сцены SKYB

**Общее описание задач контура.**

Реализовать стек команд отмены и бинарный формат сцены SKYB.

- **Сделай файл** `editor/shell/src/editor_commands.hpp`
  В файле должны быть: `IEditorCommand`, `label`, `undo`, `redo`, `UndoStack`, `push`, `notify`, `undoLabel`, `canUndo`, `redoLabel`, `canRedo`, `setOnChanged`, `onChanged_`, `makeMaterialCreateCommand`
- **Сделай файл** `editor/shell/src/editor_commands.cpp`
  В файле должны быть: `push`, `notify`, `undo`, `redo`, `apply`, `exists`, `setLocalTransform`, `RenameCommand`, `renameObject`, `DuplicateCommand`, `setField`, `attach`, `updateMaterial`, `MaterialCreateCommand`, `findMaterial`, `material`, `removeMaterial`, `createMaterial`, `parentOf`, `fields`, `makeMaterialCreateCommand`
- **Дополни файл** `editor/shell/src/editor_context.cpp`
  В файле должны быть: `snapshotObject`, `restoreObject`, `newScene`, `saveScene`, `openScene`, `duplicateObject`, `reparent`, `destroyObject`
- **Дополни файл** `engine/scene/src/scene_world.cpp`
  В файле должны быть: `saveSceneAs`, `rootObjectsOf`
- **Сделай файл** `tests/undo_tests.cpp`
  В файле должны быть: `testTransformUndoRedo`, `stack`, `findByName`, `localTransform`, `setLocalTransform`, `makeTransformCommand`, `testDeleteRestoresSubtree`, `makeDeleteCommand`, `exists`, `componentsOf`, `childrenOf`, `testCreateRenameReparent`, `makeCreateCommand`, `renameObject`, `makeRenameCommand`, `nameOf`, `invalid`, `parentOf`, `testHistoryDiscipline`, `makeDuplicateCommand`, `testToolCommandBus`, `createToolCommandBus`, `execute`, `undo`, `redo`, `registerHandler`, `registeredCommands`, `setUndoDelegate`, `setRedoDelegate`, `main`, `summary`
- **Сделай файл** `tests/scene_tests.cpp`
  В файле должны быть: `createObjectWorld`, `createComponentWorld`, `createStdFileSystem`, `createFileSerializationBackend`, `createSceneWorld`, `testSceneRoundTrip`, `registerComponentType`, `createScene`, `createObject`, `setParent`, `attach`, `addRootObject`, `saveScene`, `sceneOf`, `loadScene`, `descriptor`, `findByName`, `childrenOf`, `nameOf`, `localTransform`, `componentsOf`, `activate`, `activeContext`, `deactivate`, `unloadScene`, `exists`, `loadedScenes`, `testLoadRejectsCorruptScene`, `writeAll`, `testSceneAuthoring`, `createPrimitive`, `field`, `setField`, `setLocalTransform`, `duplicateObject`, `main`, `summary`

**На выходе должно получиться (список артефактов):**
- Отмена/повтор всех операций; сцена сохраняется в SKYB и открывается идентично.
- Тесты `undo_tests`, `scene_tests` зелёные.

**Критерий правильности:** save→open восстанавливает граф объектов, трансформы и поля компонентов; undo/redo работают для transform/field/create/delete/duplicate/reparent/rename.


---

## Этап 4 (Недели 5–6). Восстановление физики из сцены

**Общее описание задач контура.**

Реализовать воссоздание физических тел из компонентов при открытии сцены.

- **Дополни файл** `editor/shell/src/editor_context.cpp`
  В файле должны быть: `reattachPhysics`

**На выходе должно получиться (список артефактов):**
- При `openScene` физические тела воссоздаются из rigidbody/collider-компонентов.

**Критерий правильности:** Сохранённая сцена с физикой после открытия падает так же, как до сохранения.


---

## Этап 5 (Недели 7–8). Префабы

**Общее описание задач контура.**

Реализовать префабы SKYP поверх снимка объекта.

- **Дополни файл** `editor/shell/src/editor_context.cpp`
  В файле должны быть: `savePrefab`, `instantiatePrefab`, `spawnPrefabAt`

**На выходе должно получиться (список артефактов):**
- Префаб сохраняется в `.skyprefab` и инстанцируется идентично.

**Критерий правильности:** savePrefab→instantiatePrefab даёт объект с теми же компонентами, полями, физикой и детьми.
