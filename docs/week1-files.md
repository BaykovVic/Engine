# Sky Engine · Неделя 1 — пофайловая детализация

Что именно создать, файл за файлом, с методами. Пути, имена и сигнатуры
совпадают с реальной структурой репозитория — собирая с нуля по этому
списку, команда воспроизводит фактическую раскладку движка.

Соглашения проекта:
- модуль = `engine/<mod>/{include/sky/<mod>,src}`, регистрируется через
  `sky_add_module(sky_<mod> <mod> <src...>)`;
- заголовок объявляет **интерфейс** (чистые виртуальные классы) + свободную
  фабрику `create...()`; `.cpp` прячет реализацию в анонимном namespace;
- каждый модуль сдаётся с тестом в `tests/<mod>_tests.cpp`.

Легенда: `[H]` — заголовок, `[S]` — исходник, `[T]` — тест, `[B]` — сборка.

---

## E1 · Ядро и данные

### E1-1 Математика и хэндлы

#### `[H] engine/core/include/sky/core/math.hpp`
Структуры и свободные функции (всё `constexpr`, `operator<=>` по умолчанию):
- `struct Vec2 { float x, y; }`
- `struct Vec3 { float x, y, z; }`
- `struct Quat { float x, y, z, w; }`
- `struct Transform { Vec3 position; Quat rotation; Vec3 scale{1,1,1}; }`
- `Vec3 operator+(Vec3,Vec3)`, `operator-`, `operator*(Vec3,float)`, `operator*(Vec3,Vec3)` (покомпонентно)
- `Quat operator*(Quat,Quat)` (композиция поворотов)
- `Vec3 rotate(Quat q, Vec3 v)` — поворот вектора кватернионом
- `Transform compose(Transform parent, Transform child)` — трансформ ребёнка в мире родителя
- `Quat conjugate(Quat)`, `Vec3 divide(Vec3,Vec3)`
- `Transform invCompose(Transform parent, Transform world)` — мир → локаль (нужен для reparent)

#### `[H] engine/core/include/sky/core/handle.hpp`
- `template <typename Tag> struct Handle { uint64_t value; ... }` — типобезопасный id: `isValid()`, `invalid()`, `operator==`. Разные `Tag` — несовместимые типы.

#### `[T] tests/core_tests.cpp`
- проверки арифметики `Vec3`;
- `rotate(quatY90, {0,0,1})` ≈ `{1,0,0}` с точностью `1e-5`;
- `invCompose(compose(p,c)) == c` (round-trip).

### E1-2 Логирование и конфиг

#### `[H] engine/core/include/sky/core/logger.hpp`
- `enum class LogLevel : uint8_t { Trace, Debug, Info, Warning, Error, Critical }`
- `class ILogger { virtual void log(LogLevel, std::string_view category, std::string_view message) = 0; }`

#### `[S] engine/core/src/console_logger.cpp`
- `createConsoleLogger()` → `std::unique_ptr<ILogger>`; печать с уровнем и категорией.

#### `[H] engine/core/include/sky/core/config_service.hpp`
- `class IConfigService { virtual void setString/​Int/​Bool(key,val); getString/Int/Bool(key) const; }`

#### `[S] engine/core/src/memory_config_service.cpp`
- `createMemoryConfigService()` → in-memory реализация (map ключ→значение).
- Тест round-trip добавить в `core_tests.cpp`.

### E1-3 Мир объектов

#### `[H] engine/object/include/sky/object/object_model.hpp`
Три контракта (чистые интерфейсы) + типы:
- `using ObjectHandle = core::Handle<ObjectTag>;`
- `struct TransformNode { core::Transform local; ObjectHandle parent; std::vector<ObjectHandle> children; }`
- `class IObjectFactory { ObjectHandle createObject(name); void destroyObject(ObjectHandle); }`
- `class IObjectHierarchyAccess { setParent; parentOf; childrenOf; setLocalTransform; localTransform; worldTransform; }`
- `class IObjectQueryService { exists; nameOf; findByName; }`
- свободная `inline void setWorldTransform(IObjectHierarchyAccess&, ObjectHandle, world)` через `invCompose`.

#### `[H] engine/object/include/sky/object/object_world.hpp`
- `class ObjectWorld : public IObjectFactory, IObjectHierarchyAccess, IObjectQueryService { virtual void renameObject(ObjectHandle, name) = 0; }`
- `std::unique_ptr<ObjectWorld> createObjectWorld();`

#### `[S] engine/object/src/object_world.cpp`
- реализация в анонимном namespace: карта `id → TransformNode + name`;
- `worldTransform` = `compose` локальных вверх по родителям;
- `destroyObject` рекурсивно сносит поддерево.

#### `[T] tests/world_tests.cpp`
- ребёнок `{1,0,0}` под родителем, повёрнутым на 90° вокруг Y → в мире `{0,0,-1}`;
- `findByName`/`nameOf`/`renameObject`; удаление поддерева.

### E1-4 Мир компонентов

#### `[H] engine/component/include/sky/component/component_model.hpp`
- `using ComponentHandle = core::Handle<ComponentTag>;`
- `using FieldValue = std::variant<float, std::int64_t, bool, std::string, core::Vec3>;`
- `struct InspectableField { std::string name; std::string typeName; }`
- `struct ComponentDescriptor { std::string typeId, displayName; bool ...; std::vector<InspectableField> fields; std::string category; }`
- `class IComponentRegistry { registerComponentType; availableTypes; }`
- `class IComponentAttachmentService { attach(ObjectHandle,typeId); detach(ComponentHandle); }`
- `class IComponentQueryService { componentsOf; descriptorOf; ownerOf; }`
- `class IComponentDataAccess { setField; field; fields; }` (setField/field по имени, fields → карта)

#### `[H] engine/component/include/sky/component/component_world.hpp`
- `class ComponentWorld : public всех интерфейсов выше {}`
- `std::unique_ptr<ComponentWorld> createComponentWorld(object::IObjectQueryService&);`

#### `[S] engine/component/src/component_world.cpp`
- хранилище `component → {typeId, fields}`; `descriptorOf` по реестру типов.

#### `[T] tests/world_tests.cpp` (дополнить)
- зарегистрировать тип с полями всех 5 типов, attach, `setField`/`field` round-trip каждого; `fields()` возвращает всю карту; `Vec3` round-trip покомпонентно.

---

## E2 · Рендеринг

### E2-1…E2-4 Vulkan-бэкенд (один модуль, растёт по тикетам)

#### `[H] engine/rendering/include/sky/rendering/rendering.hpp`
Контракт рендера (общий, не только Vulkan):
- `enum class RenderCommandType { SetViewport, SetCamera, AddLight, SetSky, DrawMesh }`
- `struct RenderCommand { ... type, transform, color, ... }`
- `class IRenderer { backendName; submit(span<RenderCommand>); renderFrame; frameWidth/Height; }`

#### `[H] engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
- `class VulkanRenderer : public rendering::IRenderer { virtual bool ready() const; virtual std::vector<uint8_t> readbackFrame() = 0; ... }`
- `std::unique_ptr<VulkanRenderer> createVulkanRenderer(uint32_t w, uint32_t h);` (offscreen)
- `std::unique_ptr<VulkanRenderer> createVulkanRendererForWindow(target, w, h);` (позже, неделя 2)

#### `[S] engine/rendering_vulkan/src/vulkan_renderer.cpp`
Приватные методы реализации (по тикетам E2-1→E2-4):
- `initInstanceAndDevice()` — VkInstance, выбор VkPhysicalDevice + графической очереди, VkDevice *(E2-1)*
- `initTarget()` → `initOffscreenTarget()` — color(RGBA)+depth(D32) образы, render pass, framebuffer *(E2-2)*
- `initPipeline()` — layout, `createShader()`, графический пайплайн; вершинный формат position+normal+uv *(E2-3)*
- `renderFrame()` — command buffer, clear, `vkCmdDraw` треугольника *(E2-3)*
- `readbackFrame()` — copy image→host buffer, вернуть пиксели *(E2-4)*
- хелперы: `findMemoryType`, `createImage`, `createBuffer`.

#### `[H+B] engine/rendering_vulkan/shaders/{mesh.vert,mesh.frag}` + `*.spv.h`
- минимальный вершинный/фрагментный шейдер (позже дорастёт до PBR);
- компиляция `glslangValidator -V` → встраивание массива `uint32_t` в `.spv.h`.

#### `[T] tests/vulkan_tests.cpp`
- на lavapipe: рендерер `ready()`, кадр рендерится, `readbackFrame()` непустой; центральный пиксель ≠ угловой (треугольник виден).

---

## E3 · Редактор (.NET)

### E3-1 Каркас Avalonia

#### `[B] editor/avalonia/SkyEditor.csproj`
- `net8.0`, PackageReference: Avalonia, Avalonia.Desktop, Avalonia.Themes.Fluent, Avalonia.Headless.

#### `[S] editor/avalonia/Program.cs`
- `static AppBuilder BuildAvaloniaApp()`
- `static int Main(string[] args)` — обычный запуск + ветка `--screenshot` (headless, задел для E5-3).

#### `[S] editor/avalonia/App.axaml{,.cs}`
- `Application` с FluentTheme; `OnFrameworkInitializationCompleted` открывает `MainWindow`.

#### `[S] editor/avalonia/MainWindow.axaml{,.cs}`
- окно с заголовком «Sky Engine»; меню-заглушка (File/Edit/...).

### E3-2 Докинг

#### `[S] editor/avalonia/Docking/DockFactory.cs`
- `class DockFactory : Factory` — `CreateLayout()` собирает раскладку из панелей.

#### `[S] editor/avalonia/Docking/Tools.cs`
- классы-инструменты (по одному на панель): `EditorTool : Tool` c заголовком.

#### `[S] editor/avalonia/Views/*.axaml{,.cs}`
- заглушки: `HierarchyView`, `SceneView`, `InspectorView`, `ConsoleView`, `ProjectView` — пустые `UserControl` с заголовком.

### E3-3 hello-bridge

#### `[S] editor/avalonia/Engine/EngineInterop.cs`
- `static class EngineInterop` со статическим ctor: `NativeLibrary.SetDllImportResolver(...)`.
- `Resolve(name, assembly, path)` + `Candidates()` — ищет `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в build-дереве.
- `[DllImport(Lib)] static extern IntPtr sky_editor_create();`
- `[DllImport(Lib)] static extern void sky_editor_destroy(IntPtr ctx);`

#### `[S] editor/avalonia/Engine/EditorSession.cs`
- `class EditorSession : IDisposable` — в ctor `sky_editor_create()`, проверка не-null; `Dispose` → `destroy`; свойство `IntPtr Native`.
- В `MainWindow`: создать сессию, в статус-баре показать «bridge OK».

---

## E4 · Рантайм и физика

### E4-1…E4-4 Модуль физики

#### `[H] engine/physics/include/sky/physics/physics.hpp`
Типы и контракты:
- `using RigidBodyHandle = core::Handle<RigidBodyTag>; using ColliderHandle = ...;`
- `enum class BodyType { Static, Dynamic }`
- `enum class ColliderShape { Box, Sphere, Capsule, TerrainHeightfield }`
- `struct RigidBodyDesc { BodyType type; float mass; core::Transform transform; }`
- `struct HeightfieldDesc { uint32_t resolution; core::Vec3 scale; std::vector<float> heights; }`
- `struct ColliderDesc { ColliderShape shape; core::Vec3 halfExtents; float radius; HeightfieldDesc heightfield; }`
- `struct RaycastHit { ColliderHandle collider; core::Vec3 point, normal; float distance; }`
- `struct CollisionEvent { ColliderHandle first, second; }`
- `class IPhysicsWorld { createBody; destroyBody; attachCollider; detachCollider; step(dt); drainCollisionEvents; }`
- `class IPhysicsQueryService { raycast(origin,dir,maxDist); bodyTransform(body); }`
- `class IPhysicsSyncContract { pushKinematicState(); pullSimulationResults(); }`

#### `[H] engine/physics/include/sky/physics/physics_world.hpp`
- `class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService { setGravity; setBodyVelocity; bodyVelocity; setBodyTransform; }`
- `std::unique_ptr<PhysicsWorld> createPhysicsWorld();`
- `class ObjectPhysicsSync : IPhysicsSyncContract { bind(body,object); unbind(body); }`
- `std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&);`

#### `[S] engine/physics/src/physics_world.cpp`
Реализация по тикетам:
- интегрирование гравитации в `step` *(E4-1)*
- `worldAabb`, `rayVsAabb`, `detectAndResolve()` — расталкивание боксов, сбор `CollisionEvent` *(E4-2)*
- `sampleHeightfield` (билинейно), `resolveHeightfields()` *(E4-3)*
- `ObjectPhysicsSync`: `pushKinematicState`/`pullSimulationResults` синхронизируют пары до/после шага *(E4-4)*

#### `[T] tests/physics_tests.cpp`
- падение за 1 с ≈ 4.9 м; куб замирает на статическом полу; тело на heightfield встаёт на поверхность; привязанный объект опускается в объектном мире.

---

## E5 · Пайплайн и QA

### E5-1 Каркас сборки

#### `[B] CMakeLists.txt` (корень)
- `project(SkyEngine ... LANGUAGES C CXX)`, C++20, `CMAKE_POSITION_INDEPENDENT_CODE ON`;
- опции `SKY_BUILD_OPENGL_BACKEND/VULKAN_BACKEND/TESTS`;
- `add_subdirectory(engine)`, `add_subdirectory(editor)`, `add_subdirectory(player)`, `add_subdirectory(tests)`.

#### `[B] engine/CMakeLists.txt`
- `function(sky_add_module NAME DIR)` — STATIC при наличии исходников, INTERFACE иначе; public include `<DIR>/include`; `cxx_std_20`; алиас `sky::<name>`.
- регистрация модулей недели: `sky_add_module(sky_core core ...)`, `sky_object`, `sky_component`, `sky_physics`, `sky_serialization`, `sky_platform`, `sky_rendering`, `sky_rendering_vulkan`.
- `target_link_libraries` по графу зависимостей (см. `docs/skeleton.html`).

### E5-2/E5-3 CI

#### `[B] .github/workflows/ci.yml`
- job `linux` на ubuntu: установка `ninja libx11-dev libvulkan-dev mesa-vulkan-drivers vulkan-tools xvfb`;
- `setup-dotnet@v4` (8.0.x);
- шаги: `cmake -B build -G Ninja` → `cmake --build build` → **`dotnet build editor/avalonia`** (C#-ошибка валит джобу) → `xvfb-run ctest --test-dir build` → headless-смоук с сохранением PNG → `upload-artifact` картинки.

### E5-4 Тест-харнесс

#### `[H] tests/sky_test.hpp`
- `namespace sky::test { inline int& checks(); inline int& failures(); inline int summary(const char* suite); }`
- `#define CHECK(cond)` — инкремент счётчика, при провале печать `файл:строка`, без завершения процесса.

#### `[B] tests/CMakeLists.txt`
- `function(sky_add_test NAME)` — `add_executable`, линк `sky::engine`, `add_test`;
- регистрация `core_tests`, `world_tests`, `physics_tests`, `vulkan_tests` (под `SKY_BUILD_VULKAN_BACKEND`+lavapipe).

### E5-доп · Затравка C ABI (для E3-3)

#### `[H] editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`
- `extern "C"`, `SKY_BRIDGE_API`, `typedef struct SkyEditorContext SkyEditorContext;`, `typedef uint64_t SkyObjectId;`
- неделя 1 — минимум: `sky_editor_create(void)`, `sky_editor_destroy(ctx)`, `sky_editor_root_count(ctx)`, `sky_editor_root_at(ctx,i)`, `sky_editor_object_name(ctx,id,buf,cap)`.

#### `[S] editor/native_bridge/src/editor_bridge.cpp`
- `struct BridgeSession { /* неделя 1: пусто или заглушка сцены */ };`
- реализация `create`/`destroy` + enumeration поверх `object::ObjectWorld` (когда E1-3 готов; до того — возвращать 0 объектов).

#### `[B] editor/native_bridge/CMakeLists.txt`
- `add_library(sky_editor_bridge SHARED ...)`, visibility hidden, алиас `sky::editor_bridge`.

## E6 · Data-oriented системы (ECS) — **обязательно к 17 июля**

### ECS-мир и планировщик систем

#### `[H] engine/ecs/include/sky/ecs/ecs.hpp`
- `struct EntityId { uint32_t index; uint32_t generation; }` — сущность с поколением.
- `class IEcsComponentStore { componentType(); has(EntityId); remove(EntityId); count(); }`
- `template<T> class TypedComponentStore : IEcsComponentStore { set(EntityId,T); get(EntityId)→T*; }`
- `struct EcsTransform { core::Transform value; }` — базовый компонент.
- `class IEcsSystem { name(); update(double dt); }`
- `class IEcsWorld { createEntity(); destroyEntity(id); isAlive(id); store(type_index); }`
- `class IEcsSystemScheduler { registerSystem; unregisterSystem; tick(dt); }`
- `class IEcsQueryService { entitiesWith(set<type_index>); }`

#### `[H] engine/ecs/include/sky/ecs/ecs_world.hpp`
- `class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService { template<T> TypedComponentStore<T>& storeFor(); StoreMap& stores(); }`
- `std::unique_ptr<EcsWorld> createEcsWorld();`

#### `[S] engine/ecs/src/ecs_world.cpp`
- пул сущностей с поколениями; типизированные хранилища; планировщик (однопоточный tick); запрос по набору типов.

#### `[T] tests/runtime_tests.cpp`, `integrity_tests.cpp`
- система, удваивающая координату, меняет только сущности с нужным компонентом; `storeFor<EcsTransform>().get(...)` возвращает обновлённое значение.

---

## Порядок создания файлов (кратчайший путь к зелёному CI)

```
День 1 (параллельно, разными людьми):
  E5:  CMakeLists.txt · engine/CMakeLists.txt (sky_add_module) · tests/sky_test.hpp
  E1:  math.hpp · handle.hpp  → core_tests.cpp
  E2:  vulkan_backend.hpp + vulkan_renderer.cpp::initInstanceAndDevice
  E3:  SkyEditor.csproj · Program.cs · App/MainWindow
  E4:  physics.hpp (типы+контракты)

День 2–3:
  E1:  object_model.hpp · object_world.{hpp,cpp} → world_tests.cpp
  E2:  offscreen-таргет + пайплайн + треугольник
  E3:  DockFactory + панели-заглушки
  E4:  physics_world.{hpp,cpp}::step (гравитация) → physics_tests.cpp
  E5:  .github/workflows/ci.yml (C++ build + ctest)

День 4–5:
  E1:  component_model.hpp · component_world.{hpp,cpp} → доп. world_tests
  E2:  readbackFrame → tests/vulkan_tests.cpp (triangle.png)
  E3:  EngineInterop.cs + editor_bridge.h/.cpp (create/destroy) → «bridge OK»
  E4:  AABB-резолв · heightfield · ObjectPhysicsSync
  E5:  CI: dotnet build редактора + скриншот-артефакт
```

Каждый файл — с тестом на его суставе; ворота недели считаются взятыми,
когда весь `ctest` зелёный в CI и `triangle.png` лежит артефактом.

---

Продолжение: [week2-files.md](week2-files.md) · [weeks3-4-files.md](weeks3-4-files.md) · [weeks5-6-files.md](weeks5-6-files.md) · [weeks7-8-files.md](weeks7-8-files.md)
