# Sky Engine · Недели 3–4 (M2 «Авторинг») — пофайловая детализация

Продолжение [week2-files.md](week2-files.md). Легенда: `[H]` заголовок,
`[S]` исходник, `[T]` тест, `[B]` сборка. Цель: сцену можно собрать руками,
сохранить, открыть, сыграть, выйти — и она вернулась в исходное состояние.

---

## E1 · Undo/Redo, SKYB, операции с объектами

#### `[H] editor/shell/src/editor_commands.hpp` `[S] editor_commands.cpp`
- `class IEditorCommand { virtual void redo() = 0; virtual void undo() = 0; virtual std::string label() const = 0; }`
- `class UndoStack { explicit UndoStack(EditorContext&); void push(unique_ptr<IEditorCommand>); bool undo(); bool redo(); bool canUndo/Redo(); }`
- фабрики команд (по одной на операцию):
  `makeTransformCommand`, `makeFieldCommand`, `makeCreateCommand`,
  `makeCreateSnapshotCommand`, `makeDeleteCommand`, `makeDuplicateCommand`,
  `makeReparentCommand`, `makeRenameCommand`, `makeAddComponentCommand`,
  `makeRemoveComponentCommand`, `makeMaterialCreateCommand`, `makeMaterialEditCommand`.

#### `[H] editor/shell/src/editor_context.hpp` `[S] editor_context.cpp` (дополнить)
- `struct ComponentSnapshot { typeId; map<string,FieldValue> fields; }`
- `struct ObjectSnapshot { name; Transform local; bool hasPhysicsBody; vector<ComponentSnapshot>; vector<ObjectSnapshot> children; }`
- `ObjectSnapshot snapshotObject(ObjectHandle) const;` — lossless-снимок поддерева.
- `ObjectHandle restoreObject(const ObjectSnapshot&, parent);` — переиспользуется undo/префабами/сценами.
- `void newScene(); bool saveScene(path); bool openScene(path);`
- `duplicateObject`, `reparent`, `destroyObject`, `createEmpty/createPrimitive/createCrate`.

#### `[S] engine/scene/src/scene_world.cpp` (дополнить)
- `saveSceneAs` — реальный SKYB: magic "SKYB", schema `sky.scene`, обход рутов → имя, трансформ, компоненты с полями (`ByteWriter`); загрузка обратно с миграцией.

## E2 · Пикинг, проекция, Game-вью, скайбокс

#### `[H] editor/shell/src/editor_camera.hpp`
- `class EditorCamera { orbit(dx,dy); pan(...); zoom(...); Transform pose(); float orthoHeight(); }`
- `ObjectHandle pick(screenX,screenY,w,h)` — луч в сцену (в мосте), `project(world,w,h)→screen`.

#### `[S] engine/rendering_vulkan/src/vulkan_renderer.cpp` (дополнить)
- обработка `RenderCommandType::SetSky` — купол-скайбокс с `skyMode` в push-константах.

## E3 · Гизмо, Inspector, панели

#### `[S] editor/avalonia/Controls/VulkanViewport.cs` (дорастить)
- рисование гизмо move/rotate/scale поверх кадра; drag-состояния (`_dragAxis`, `_rotateAxis`, `_scaleUniform`); scene-гизмо в углу.
- свойства `SelectedId`, `LocalSpace`, `Tool`.

#### `[S] editor/avalonia/MainViewModel.cs`
- `enum GizmoTool { Hand, Move, Rotate, Scale }`; свойство `Tool` (хоткеи Q/W/E/R);
- `PositionX/Y/Z`, `RotationX/Y/Z`, `ScaleX/Y/Z` (двусторонние); `SelectedComponents`; `CreateCube/Duplicate/Delete/Undo/Redo`.

#### `[S] editor/avalonia/Engine/EditorSession.cs` (дополнить)
- `class ComponentView { int Index; string TypeId, DisplayName; List<ComponentField> Fields; }`
- `class ComponentField : INotifyPropertyChanged` — `IsScalar/IsBool/IsVec3/IsMeshRef`, `Value` (пишет через ABI), X/Y/Z, `BoolValue`.
- `ReadComponents(id)`, `SetComponentField(...)`, `AvailableTypes()`, `AddComponent/RemoveComponent`.

#### `[S] editor/avalonia/Views/{InspectorView,ConsoleView,MaterialsView,PackagesView}.axaml{,.cs}`
- Inspector: `ItemsControl` по компонентам, шаблоны полей (TextBox/CheckBox/Vec3/ComboBox), кнопка Add Component (Flyout).
- Console: список `LogLine` с цветом по уровню; Materials/Packages — на живых данных через ABI.

## E4 · Play mode и ввод

#### `[H] editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`
- `enum class PlayModeState { Editing, Playing, Paused };`
- `class PlayModeController { setScene(scene); bool play(); void pause(); void stop(); tickFrame(dt); PlayModeState state(); }`

#### `[S] editor/shell/src/editor_context.cpp` (дополнить)
- `beginPlay()` — снапшот локальных трансформов всех объектов;
- `endPlay()` — восстановление + re-seat физических тел с нулевой скоростью;
- `setKeyDown(int key, bool)`, `keyDown(int)` — состояние клавиш; портируемые кей-коды (общий enum с C#).

## E5 · Импортеры, VFS, bridge-тесты

#### `[H] engine/asset/include/sky/asset/obj_importer.hpp` `[S] obj_importer.cpp`
- `class ObjImporter : IAssetImporter { ImportResult import(bytes); }` — парсинг v/vn/vt/f.

#### `[H] engine/asset/include/sky/asset/png_decoder.hpp` `[S] png_decoder.cpp`
- `struct ImageData { width,height; vector<uint8_t> pixels; }`
- `optional<ImageData> decodePng(bytes)`, `vector<byte> encodePngRgba(ImageData)`.

#### `[H] engine/asset/include/sky/asset/asset_database.hpp` `[S] asset_database.cpp`
- `class AssetDatabase { AssetId importAsset(path); optional<AssetRecord> resolve(id); ... }`

#### `[H] engine/platform/include/sky/platform/virtual_file_system.hpp` `[S] virtual_file_system.cpp`
- `class IVirtualFileSystem { mount(scheme, mount, priority); resolve("assets://..."); list(dir); }`

#### `[T] tests/editor_bridge_tests.cpp`
- по группе ABI на тест: жизненный цикл/иерархия, авторинг, пикинг, трансформы в 3 пространствах, поля компонентов, вьюпорт.

## E6 · Прикладные системы поверх ECS

#### `[H+S] engine/ecs/include/sky/ecs/systems/*` (+ `src`)
- система частиц (компонент `Particle`, обновление позиций/жизни);
- пакетная обработка трансформов (одна система над всеми `EcsTransform`).

#### `[S] engine/ecs` ← `engine/mapgen/materialize.hpp`
- объекты `materializeGenerationResult(...)` привязываются к сущностям и участвуют в ECS-обработке.

**На выходе:** частицы видны в кадре; пакетная система применяется ко всем сущностям с компонентом за такт.

**Ворота M2:** собрать сцену руками → save → open → play → stop → сцена в
исходном состоянии; всё покрыто bridge-тестами.
