# Sky Engine · Неделя 2 (M1 «Первый кадр») — пофайловая детализация

Продолжение [week1-files.md](week1-files.md). Легенда та же: `[H]` заголовок,
`[S]` исходник, `[T]` тест, `[B]` сборка. Цель недели — демо-сцена
рендерится в док-панели редактора, Hierarchy на живых данных.

---

## E1 · Сцена и сборочная точка

#### `[H] engine/scene/include/sky/scene/scene_world.hpp`
- `using SceneHandle = core::Handle<SceneTag>;`
- `struct SceneWorldDeps { object::IObjectFactory&; IObjectHierarchyAccess&; IObjectQueryService&; component::IComponentQueryService&; ...; serialization::ISerializationBackend*; }`
- `class SceneWorld : ISceneRepository, ISceneRuntime, ISceneQueryService { addRootObject(scene,object); saveSceneAs(scene,path,...); rootObjectsOf(scene); }`
- `std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps&);`

#### `[H] engine/scene/include/sky/scene/scene_authoring.hpp`
- `enum class PrimitiveKind { Cube, Plane, Sphere };`
- `struct AuthoringServices { object factory/hierarchy/query + component attach/query/data; }`
- `ObjectHandle createPrimitive(AuthoringServices&, PrimitiveKind, name);` — вешает `sky.mesh`.

#### `[S] engine/scene/src/scene_world.cpp`, `scene_authoring.cpp`
- реестр сцен, список рут-объектов; `saveSceneAs` (неделя 2 — заглушка, полноценный SKYB на неделе 3–4).

#### `[H] editor/shell/src/editor_context.hpp` `[S] editor_context.cpp`
Сборочная точка — собирает весь движок в один объект:
- `class EditorContext { EditorContext(); ... unique_ptr<> на каждый мир: fileSystem, vfs, renderers, objects, components, physics, scenes, ... ; SceneHandle activeScene; std::vector<ObjectHandle> rootObjects() const; }`
- приватные `buildDemoScene()`, `initTerrain()` — наполняют демо-сцену (примитивы, свет, камера).

## E2 · Меши, камера, свет, PBR v1

#### `[S] engine/rendering_vulkan/src/vulkan_renderer.cpp` (дорастить)
- `struct FrameUbo { float viewProjection[16]; cameraPos[4]; lightVec/Color/Meta[4][4]; counts[4]; }`
- `struct PushBlock { model[16]; baseColor[4]; emissive[4]; params[4]; params2[4]; }`
- матрицы: `perspective(fov,aspect,near,far)`, `orthographic(...)`, `fromTransform(Transform)`, `viewFromCameraPose(...)`
- `createMeshFromData(span<float> posNormalUv)`, `createTextureFromData(...)` (текстуры — неделя 5)
- в `renderFrame`: разбор потока `RenderCommand` (SetCamera/AddLight/DrawMesh), заполнение UBO, `drawBuffer`.

#### `[H] editor/shell/src/frame_builder.hpp`
- `class FrameBuilder { FrameBuilder(EditorContext&, IRenderer&); void setCamera(optional<Transform>, orthoHeight=0); std::vector<RenderCommand> build(width,height); }`
- приватные `forEachObject(visit)`, `applyMaterial(draw,name)` — обход сцены → команды.

#### `[H] editor/viewport_bridge/include/sky/editor/viewport/viewport_bridge.hpp`
- Qt-free фасад вьюпорта (камера-орбита, размеры), общий для редактора и плеера.

## E3 · Панель-вьюпорт и живая Hierarchy

#### `[S] editor/avalonia/Controls/VulkanViewport.cs`
- `class VulkanViewport : Control` — держит `WriteableBitmap`, таймер кадров;
- `SetContext(IntPtr)`, `RenderOnce()` — дёргает нативный offscreen-рендер и блитит пиксели в битмап; `Render(DrawingContext)`.

#### `[S] editor/avalonia/Views/SceneView.axaml{,.cs}`, `HierarchyView.axaml{,.cs}`
- SceneView хостит `VulkanViewport`; HierarchyView — `TreeView` по `Roots`.

#### `[S] editor/avalonia/Engine/EngineInterop.cs` (дополнить)
- `root_count/root_at/child_count/child_at/object_name/object_exists`, `get_transform`, `set_position`, `render_offscreen`, орбита/зум камеры.

#### `[S] editor/avalonia/Engine/EditorSession.cs`
- `class SkyObject { ulong Id; string Name; ObservableCollection<SkyObject> Children; }`
- `Reload()` — перечитывает иерархию через ABI; `Load(id)` рекурсивно; `Transform(id)`.

## E4 · Скелет плеера

#### `[S] player/src/main.cpp`
- `int main(argc,argv)` — парсинг `--frames N`, `--headless out.png`;
- `runHeadless(EditorContext&, frames, path)` и `runWindowed(EditorContext&, limit)`;
- окно через `createX11WindowSystem()`, `createVulkanRendererForWindow(target,w,h)`, цикл `tickFrame → build → renderFrame`.

## E5 · Скриншот-режим и первые тесты

#### `[S] editor/avalonia/Program.cs` (дополнить)
- ветка `--screenshot path [--demo]`: headless Avalonia (`UseHeadless`), `window.CaptureRenderedFrame().Save(path)`.

#### `[T] tests/scene_tests.cpp`, `tests/runtime_tests.cpp`
- сцена: добавить рут, перечислить, round-trip заглушки; runtime: `FrameBuilder.build` возвращает непустой поток команд на демо-сцене.

**Ворота M1:** редактор открывается, демо-сцена видна во вьюпорт-панели,
Hierarchy живая; `sky_player --headless` пишет PNG; всё зелёное в CI.
