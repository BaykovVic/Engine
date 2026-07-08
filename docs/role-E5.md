# Техническое задание · Контур E5 «Пайплайн, ассеты и пакеты»

**Область ответственности.** Система сборки, непрерывная интеграция, тесты, импортёры ассетов, виртуальная ФС, C-интерфейс движка и менеджер пакетов.

Все пути и имена методов взяты из фактического репозитория и совпадают с проектом 1:1. «Сделай файл» — файл создаётся на этом этапе; «Дополни файл» — в существующий файл добавляются перечисленные методы.


---

## Этап 1 (Неделя 1). Сборка, интеграция, тесты, первичный C-интерфейс

**Общее описание задач контура.**

Развернуть систему сборки, тесты, CI и первичный C-интерфейс движка (совместно с E1).

- **Сделай файл** `CMakeLists.txt`
  В файле должны быть: `cmake_minimum_required`, `set`, `option`, `find_program`, `endif`, `add_subdirectory`, `enable_testing`
- **Сделай файл** `engine/CMakeLists.txt`
  В файле должны быть: `function`, `set`, `add_library`, `target_compile_features`, `endif`, `endfunction`, `find_package`, `target_compile_definitions`, `target_link_libraries`, `target_sources`, `elseif`, `message`, `sky_add_module`
- **Сделай файл** `tests/CMakeLists.txt`
  В файле должны быть: `add_executable`, `target_link_libraries`, `target_compile_definitions`, `endif`, `add_test`, `function`, `target_include_directories`, `endfunction`, `sky_add_test`, `find_package`, `add_dependencies`, `set_tests_properties`
- **Сделай файл** `tests/sky_test.hpp`
  В файле должны быть: `summary`
- **Сделай файл** `.github/workflows/ci.yml`
  В файле должны быть: `Build`, `smoke`
- **Сделай файл** `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`
  В файле должны быть: `sky_editor_create`, `sky_editor_destroy`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`, `sky_editor_component_display_name`, `sky_editor_component_field_count`, `sky_editor_component_field_name`, `sky_editor_component_field_type`, `sky_editor_component_field_value`, `sky_editor_set_component_field`, `sky_editor_terrain_generate`, `sky_editor_material_count`, `sky_editor_material_name`, `sky_editor_material_field_count`, `sky_editor_material_field_name`, `sky_editor_material_field_value`, `sky_editor_set_material_field`, `sky_editor_vfs_count`, `sky_editor_vfs_entry`, `sky_editor_available_type_count`, `sky_editor_reload_scripts`, `sky_editor_script_class_count`, `sky_editor_log_count`, `sky_editor_log_level`, `sky_editor_log_clear`, `sky_editor_package_count`, `sky_editor_package_active`, `sky_editor_package_refresh`, `sky_editor_new_scene`, `sky_editor_save_scene`, `sky_editor_open_scene`, `sky_editor_commit_edit`, `sky_editor_undo`, `sky_editor_redo`, `sky_editor_can_undo`, `sky_editor_can_redo`, `sky_editor_delete`, `sky_editor_detach_viewport`, `sky_editor_viewport_zoom`, `sky_editor_set_view_2d`, `sky_editor_view_2d`, `sky_editor_look_along_axis`, `sky_editor_play`, `sky_editor_pause`, `sky_editor_stop`, `sky_editor_play_state`, `sky_editor_tick_play`
- **Дополни файл** `editor/native_bridge/src/editor_bridge.cpp`
  В файле должны быть: `sky_editor_create`, `sky_editor_destroy`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_object_name`

**На выходе должно получиться (список артефактов):**
- Проект собирается; CI прогоняет сборку и тесты; C-интерфейс `create/destroy/enumerate` работает.

**Критерий правильности:** Красный CI блокирует слияние; сломанный тест краснеет; редактор E3 вызывает `sky_editor_create`.


---

## Этап 2 (Неделя 2). Режим скриншотов и первые тесты

**Общее описание задач контура.**

Добавить сборку редактора и скриншот-артефакт в CI; первые модульные тесты.

- **Дополни файл** `editor/avalonia/Program.cs`
  В файле должны быть: `Screenshot`
- **Сделай файл** `tests/runtime_tests.cpp`
  В файле должны быть: `update`, `testPlayModeDrivesRuntime`, `createObjectWorld`, `createComponentWorld`, `createStdFileSystem`, `createFileSerializationBackend`, `createEcsWorld`, `createPhysicsWorld`, `createObjectPhysicsSync`, `createScene`, `createObject`, `setLocalTransform`, `addRootObject`, `createBody`, `bind`, `createEntity`, `spinSystem`, `registerSystem`, `createPlayModeController`, `play`, `setScene`, `onStateChanged`, `tickFrame`, `localTransform`, `activeContext`, `pause`, `stop`, `main`, `summary`

**На выходе должно получиться (список артефактов):**
- CI собирает редактор (C#-ошибка валит джобу); скриншот публикуется артефактом.

**Критерий правильности:** Артефакт-PNG доступен из прогона CI; ошибка C# останавливает сборку.


---

## Этап 3 (Недели 3–4). Импортёры, виртуальная ФС, тесты интерфейса

**Общее описание задач контура.**

Реализовать импортёры OBJ/PNG, базу ассетов, виртуальную ФС и тесты C-интерфейса.

- **Сделай файл** `engine/asset/include/sky/asset/asset_system.hpp`
  В файле должны быть: `IAssetResolver`, `resolve`, `IAssetRegistry`, `registerAsset`, `unregisterAsset`, `dependentsOf`, `allAssets`, `IAssetImporter`, `supports`, `import`, `IImportPipeline`, `registerImporter`, `importAsset`, `reimport`
- **Сделай файл** `engine/asset/include/sky/asset/asset_database.hpp`
  В файле должны быть: `AssetDatabase`, `createAssetDatabase`, `assetIdFromPath`
- **Сделай файл** `engine/asset/src/asset_database.cpp`
  В файле должны быть: `resolve`, `assetIdFromPath`, `registerAsset`, `dependentsOf`, `allAssets`, `registerImporter`, `importAsset`, `supports`, `import`, `reimport`, `createAssetDatabase`
- **Сделай файл** `engine/asset/include/sky/asset/obj_importer.hpp`
  В файле должны быть: `createObjImporter`
- **Сделай файл** `engine/asset/src/obj_importer.cpp`
  В файле должны быть: `parseFaceVertex`, `resolveIndex`, `flatNormal`, `supports`, `loadObjMesh`, `createObjImporter`
- **Сделай файл** `engine/asset/include/sky/asset/png_decoder.hpp`
  В файле должны быть: `decodePng`, `encodePngRgba`, `createPngImporter`
- **Сделай файл** `engine/asset/src/png_decoder.cpp`
  В файле должны быть: `readU32`, `channelsFor`, `paeth`, `decodePng`, `libdeflate_alloc_decompressor`, `libdeflate_free_decompressor`, `crc32Of`, `appendU32`, `encodePngRgba`, `putU32`, `appendChunk`, `supports`, `createPngImporter`
- **Сделай файл** `engine/platform/include/sky/platform/file_system.hpp`
  В файле должны быть: `IFileSystem`, `exists`, `isDirectory`, `readAll`, `createDirectories`, `remove`, `list`
- **Сделай файл** `engine/platform/src/std_file_system.cpp`
  В файле должны быть: `exists`, `isDirectory`, `readAll`, `writeAll`, `createDirectories`, `remove`, `list`, `createStdFileSystem`
- **Сделай файл** `engine/platform/include/sky/platform/virtual_file_system.hpp`
  В файле должны быть: `parse`, `IVfsMount`, `readOnly`, `exists`, `read`, `write`, `remove`, `IVirtualFileSystem`, `unmountAll`, `aliases`, `readAll`, `list`, `createVirtualFileSystem`
- **Сделай файл** `engine/platform/src/virtual_file_system.cpp`
  В файле должны быть: `parse`, `replace`, `DirectoryMount`, `exists`, `read`, `remove`, `list`, `aliases`, `resolveStack`, `readAll`, `readOnly`, `write`, `createVirtualFileSystem`
- **Сделай файл** `tests/editor_bridge_tests.cpp`
  В файле должны быть: `nameOf`, `sky_editor_object_name`, `sky_editor_component_count`, `sky_editor_component_type`, `testBridgeLifecycleAndHierarchy`, `sky_editor_create`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_child_count`, `sky_editor_destroy`, `testBridgeAuthoring`, `sky_editor_create_primitive`, `sky_editor_set_position`, `sky_editor_get_transform`, `sky_editor_duplicate`, `sky_editor_delete`, `strlen`, `testBridgePicking`, `sky_editor_frame_object`, `sky_editor_viewport_zoom`, `sky_editor_pick`, `sky_editor_viewport_orbit`, `sky_editor_world_position`, `testBridgeComponentFields`, `sky_editor_component_field_name`, `sky_editor_set_component_field`, `sky_editor_component_field_value`, `testBridgeTransformSpaces`, `sky_editor_set_world_position`, `sky_editor_set_local_euler`, `sky_editor_translate_self`, `sky_editor_get_world_transform`, `testBridgeViewport`, `createX11WindowSystem`, `createWindow`, `pumpEvents`, `nativeHandles`, `sky_editor_attach_viewport`, `sky_editor_render_viewport`, `sky_editor_detach_viewport`, `destroyWindow`, `testBridgeScriptLog`, `sky_editor_log_count`, `sky_editor_log_text`, `sky_editor_stop`, `testBridgeScriptInput`, `sky_editor_set_key_state`, `sky_editor_tick_play`, `testBridgeScriptClasses`, `sky_editor_script_class_count`, `sky_editor_script_class_name`, `testBridgeScriptFields`, `sky_editor_script_field_count`, `sky_editor_script_field_name`, `sky_editor_script_field_type`, `sky_editor_script_field_value`, `sky_editor_set_script_field`, `testBridgeUserScripts`, `sky_editor_assets_root`, `path`, `source`, `sky_editor_add_component`, `testBridgePrefabs`, `sky_editor_instantiate_prefab`, `testBridgeScriptEngineApi`, `attachScript`, `scriptFieldIndex`, `testBridgePackagePersistence`, `remove`, `sky_editor_package_count`, `sky_editor_package_info`, `findPackages`, `sky_editor_package_set_active`, `testBridgePackageInstall`, `createStdFileSystem`, `createFileSerializationBackend`, `savePackageManifest`, `testBridgePackageCode`, `setActive`, `testBridgeCrateRain`, `main`, `summary`
- **Сделай файл** `tests/asset_project_tests.cpp`
  В файле должны быть: `supports`, `path`, `testAssetDatabase`, `createAssetDatabase`, `registerImporter`, `importAsset`, `assetIdFromPath`, `findBySourcePath`, `resolve`, `dependentsOf`, `reimport`, `unregisterAsset`, `testProjectRoundTrip`, `createStdFileSystem`, `createFileSerializationBackend`, `createProjectRepository`, `createProject`, `exists`, `openProject`, `descriptor`, `sceneList`, `closeProject`, `main`, `summary`
- **Сделай файл** `tests/vfs_tests.cpp`
  В файле должны быть: `testRoot`, `bytes`, `result`, `testPathParsing`, `parse`, `testDirectoryMountAndRouting`, `createStdFileSystem`, `createVirtualFileSystem`, `createDirectories`, `createDirectoryMount`, `aliases`, `writeAll`, `remove`, `testOverlayPriorities`, `exists`, `unmountAll`, `testPakArchive`, `buildPakArchive`, `createPakMount`, `mount`, `main`, `summary`

**На выходе должно получиться (список артефактов):**
- OBJ/PNG импортируются; ссылки `assets://` разрешаются; группы ABI покрыты тестами.

**Критерий правильности:** `asset_project_tests`, `vfs_tests`, `editor_bridge_tests` зелёные.


---

## Этап 4 (Недели 5–6). Импортёры FBX/glTF, запуск сцены проигрывателем

**Общее описание задач контура.**

Реализовать импортёры FBX/glTF и запуск сохранённой сцены проигрывателем (совместно с E4).

- **Сделай файл** `engine/asset/include/sky/asset/fbx_importer.hpp`
  В файле должны быть: `createFbxImporter`
- **Сделай файл** `engine/asset/src/fbx_importer.cpp`
  В файле должны быть: `transformPoint`, `transformNormal`, `getMeshCount`, `getMesh`, `getGeometryData`, `getGlobalTransform`, `triangulate`, `destroy`, `supports`, `tolower`, `createFbxImporter`
- **Сделай файл** `engine/asset/include/sky/asset/gltf_importer.hpp`
  В файле должны быть: `createGltfImporter`
- **Сделай файл** `engine/asset/src/gltf_importer.cpp`
  В файле должны быть: `transformPoint`, `transformNormal`, `fromTrs`, `decodeBase64`, `decode`, `readU32At`, `parseJson`, `string_view`, `numberOr`, `floatAt`, `indexAt`, `flatNormal`, `open`, `openDocument`, `supports`, `createGltfImporter`
- **Сделай файл** `engine/asset/src/mini_json.hpp`
  В файле должны быть: `numberOr`, `parse`, `parseValue`, `skipWhitespace`, `isspace`, `consume`, `parseObject`, `parseArray`, `parseString`, `parseBool`, `parseNull`, `parseNumber`, `parseJson`, `JsonParser`

**На выходе должно получиться (список артефактов):**
- FBX и glTF импортируются; проигрыватель запускает сцену; player-smoke в CI.

**Критерий правильности:** `sky_player --scene X.skybox` запускает сцену; шаг smoke в CI зелёный.


---

## Этап 5 (Недели 7–8). Менеджер пакетов

**Общее описание задач контура.**

Реализовать менеджер пакетов: манифесты, разрешение версий, файл блокировки, установку, код из пакетов.

- **Сделай файл** `engine/package/include/sky/package/package_system.hpp`
  В файле должны быть: `IPackageResolver`, `IPackageRegistry`, `registerPackage`, `manifest`, `activePackages`, `IExtensionRegistry`, `registerExtension`, `IPackageActivationService`, `activate`, `deactivate`
- **Сделай файл** `engine/package/include/sky/package/package_world.hpp`
  В файле должны быть: `PackageWorld`, `discoverPackages`, `discoveredPackages`
- **Сделай файл** `engine/package/src/package_world.cpp`
  В файле должны быть: `loadPackageManifest`, `categoryOf`, `discoverPackages`, `readManifest`, `parseVersion`, `discoveredPackages`, `satisfies`, `registerPackage`, `manifest`, `activePackages`, `registerExtension`, `activate`, `deactivate`, `manifestChecksum`, `readManifestImpl`
- **Сделай файл** `engine/package/include/sky/package/semver.hpp`
  В файле должны быть: `str`, `parseVersion`, `satisfies`
- **Сделай файл** `engine/package/include/sky/package/package_lock.hpp`
  В файле должны быть: `manifestChecksum`
- **Сделай файл** `engine/package/include/sky/package/package_installer.hpp`
  В файле должны быть: `cacheRoot`
- **Сделай файл** `engine/package/src/package_installer.cpp`
  В файле должны быть: `runCommand`, `shellQuote`, `copyTree`, `exists`, `isTarball`, `tolower`, `findPackageDir`, `cachePackageDir`, `loadPackageManifest`
- **Сделай файл** `tests/package_tests.cpp`
  В файле должны быть: `packagesRoot`, `savePackageManifest`, `testDiscoveryAndResolution`, `createStdFileSystem`, `createFileSerializationBackend`, `writePackage`, `createPackageWorld`, `discoverPackages`, `resolve`, `registerPackage`, `activate`, `activePackages`, `extensionsByCategory`, `deactivate`, `testCycleDetection`, `testSemver`, `testMinimalVersionSelection`, `writePackageVersion`, `testLockRoundTrip`, `manifestChecksum`, `savePackageLock`, `loadPackageLock`, `testInstaller`, `payload`, `installer`, `exists`, `main`, `summary`

**На выходе должно получиться (список артефактов):**
- Пакет ставится из каталога/архива/git; версии резолвятся; `sky.lock` персистит; код пакета работает в Play.

**Критерий правильности:** `package_tests` зелёный: semver, MVS, конфликты, lock round-trip, установка.
