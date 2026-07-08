# Техническое задание · Контур E4 «Рантайм и скриптинг»

**Область ответственности.** Физическая симуляция, режим воспроизведения, ввод, автономный проигрыватель и подсистема скриптинга (.NET-хостинг).

Все пути и имена методов взяты из фактического репозитория и совпадают с проектом 1:1. «Сделай файл» — файл создаётся на этом этапе; «Дополни файл» — в существующий файл добавляются перечисленные методы.


---

## Этап 1 (Неделя 1). Физическая симуляция

**Общее описание задач контура.**

Реализовать физический мир: тела, коллайдеры, гравитацию, столкновения, высотную поверхность, луч, синхронизацию.

- **Сделай файл** `engine/physics/include/sky/physics/physics.hpp`
  В файле должны быть: `IPhysicsWorld`, `createBody`, `destroyBody`, `attachCollider`, `detachCollider`, `step`, `drainCollisionEvents`, `IPhysicsQueryService`, `bodyTransform`, `IPhysicsSyncContract`, `pushKinematicState`, `pullSimulationResults`
- **Сделай файл** `engine/physics/include/sky/physics/physics_world.hpp`
  В файле должны быть: `PhysicsWorld`, `setGravity`, `setBodyVelocity`, `bodyVelocity`, `setBodyTransform`, `createPhysicsWorld`, `ObjectPhysicsSync`, `bind`, `unbind`
- **Сделай файл** `engine/physics/src/physics_world.cpp`
  В файле должны быть: `createBody`, `destroyBody`, `attachCollider`, `invalid`, `detachCollider`, `step`, `resolveHeightfields`, `detectAndResolve`, `drainCollisionEvents`, `worldAabb`, `bodyTransform`, `setBodyVelocity`, `bodyVelocity`, `setBodyTransform`, `sampleHeightfield`, `resolve`, `ObjectPhysicsSyncImpl`, `bind`, `pushKinematicState`, `pullSimulationResults`, `createPhysicsWorld`
- **Сделай файл** `tests/physics_tests.cpp`
  В файле должны быть: `testGravityIntegration`, `createPhysicsWorld`, `step`, `bodyVelocity`, `bodyTransform`, `testCollisionAndResolution`, `drainCollisionEvents`, `testRaycast`, `raycast`, `testObjectSync`, `createObjectWorld`, `createObjectPhysicsSync`, `createObject`, `setLocalTransform`, `createBody`, `bind`, `pushKinematicState`, `pullSimulationResults`, `localTransform`, `unbind`, `main`, `summary`

**На выходе должно получиться (список артефактов):**
- Библиотека `sky_physics` собрана; тест `physics_tests` зелёный.

**Критерий правильности:** Тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на heightfield; привязанный объект синхронно опускается.


---

## Этап 2 (Неделя 2). Автономный проигрыватель

**Общее описание задач контура.**

Реализовать проигрыватель с собственным циклом и режимами запуска (оконный/headless).

- **Сделай файл** `player/src/main.cpp`
  В файле должны быть: `mapPlatformKey`, `runHeadless`, `createVulkanRenderer`, `builder`, `setScene`, `play`, `tickFrame`, `submit`, `renderFrame`, `readbackFrame`, `frameWidth`, `frameHeight`, `encodePngRgba`, `writeAll`, `runWindowed`, `createCocoaWindowSystem`, `createX11WindowSystem`, `createWindow`, `metalLayer`, `nativeHandles`, `setEventCallback`, `now`, `pumpEvents`, `presentedFrames`, `main`

**На выходе должно получиться (список артефактов):**
- Бинарь `sky_player`; безоконный режим пишет PNG.

**Критерий правильности:** `sky_player --headless out.png` формирует изображение кадра.


---

## Этап 3 (Недели 3–4). Режим воспроизведения и ввод

**Общее описание задач контура.**

Реализовать управление воспроизведением со снимком/восстановлением сцены и приём ввода.

- **Сделай файл** `editor/viewport_bridge/include/sky/editor/viewport/play_mode_controller.hpp`
  В файле должны быть: `PlayModeController`, `setScene`, `tickFrame`
- **Сделай файл** `editor/viewport_bridge/src/play_mode_controller.cpp`
  В файле должны быть: `PlayModeControllerImpl`, `play`, `transition`, `pause`, `stop`, `onStateChanged`, `setScene`, `tickFrame`
- **Дополни файл** `editor/shell/src/editor_context.hpp`
  В файле должны быть: `beginPlay`, `endPlay`, `setKeyDown`, `keyDown`
- **Дополни файл** `editor/shell/src/editor_context.cpp`
  В файле должны быть: `beginPlay`, `endPlay`

**На выходе должно получиться (список артефактов):**
- Вход в play снимает состояние, выход восстанавливает; состояние клавиш доступно движку.

**Критерий правильности:** После play→stop сцена в исходном состоянии, тела без остаточной скорости.


---

## Этап 4 (Недели 5–6). Подсистема скриптинга

**Общее описание задач контура.**

Реализовать хостинг .NET, базовый класс скрипта, обратный API движка и интеграцию в play.

- **Сделай файл** `engine/scripting/include/sky/scripting/scripting_boundary.hpp`
  В файле должны быть: `IScriptBindingService`, `registerBinding`, `unbindInstance`, `IScriptLifecycleBridge`, `dispatchAll`, `INativeHandleRegistry`, `allocate`, `release`, `resolve`
- **Сделай файл** `engine/scripting/include/sky/scripting/script_host.hpp`
  В файле должны быть: `IScriptHost`, `start`, `shutdown`, `loadAssembly`, `loadedAssemblies`, `createInstance`, `destroyInstance`, `IDomainReloadPolicy`, `policy`, `canReloadNow`, `requestReload`
- **Сделай файл** `engine/scripting/include/sky/scripting/script_runtime.hpp`
  В файле должны быть: `ScriptRuntime`, `createScriptRuntime`
- **Сделай файл** `engine/scripting/include/sky/scripting/dotnet_host.hpp`
  В файле должны быть: `DotNetScriptHost`, `probeValue`, `installEngineApi`, `beginFrame`, `scriptClassNames`, `loadUserAssembly`, `unloadUserAssembly`, `createDotNetScriptHost`
- **Сделай файл** `engine/scripting/src/dotnet_host.cpp`
  В файле должны быть: `splitLines`, `discoverHostfxr`, `DotNetScriptHostImpl`, `start`, `dlopen`, `dlsym`, `initialize`, `resolve`, `shutdown`, `close_`, `loadAssembly`, `managedLoadAssembly_`, `createInstance`, `managedCreateInstance_`, `destroyInstance`, `managedDestroyInstance_`, `probeValue`, `managedGetProbe_`, `installEngineApi`, `managedInitialize_`, `managedSetObjectId_`, `beginFrame`, `managedTickFrame_`, `scriptClassNames`, `scriptFields`, `loadUserAssembly`, `managedLoadUserAssembly_`, `unloadUserAssembly`, `managedUnloadUserAssembly_`, `createDotNetScriptHost`, `available`
- **Сделай файл** `engine/scripting/src/script_runtime.cpp`
  В файле должны быть: `registerBinding`, `bindingFor`, `invalid`, `allocate`, `unbindInstance`, `release`, `dispatchAll`, `resolve`, `createScriptRuntime`
- **Сделай файл** `managed/SkyEngine.Managed/Bootstrap.cs`
  В файле должны быть: `LoadAssembly`, `LoadUserAssembly`, `AssemblyLoadContext`, `MemoryStream`, `UnloadUserAssembly`, `Initialize`, `TickFrame`, `SetObjectId`, `NativeHandle`, `DestroyInstance`, `InvokeLifecycle`, `GetScriptClasses`, `GetScriptFields`, `SetScriptField`, `FieldKind`, `GetProbe`, `ResolveType`, `IProbe`, `Bootstrap`, `picker`, `shadows`
- **Сделай файл** `managed/SkyEngine.Managed/ScriptComponent.cs`
  В файле должны быть: `SetLocalPosition`, `SetLocalEuler`, `SetLocalScale`, `SetVelocity`, `Instantiate`, `Destroy`, `OnCreate`, `OnStart`, `OnUpdate`, `OnFixedUpdate`, `OnDestroy`, `for`, `ScriptComponent`
- **Сделай файл** `managed/SkyEngine.Managed/NativeHandle.cs`
  В файле должны быть: `NativeHandle`, `struct`
- **Сделай файл** `managed/SkyEngine.Managed/Engine.cs`
  В файле должны быть: `SetVec3Fn`, `GetVec3Fn`, `LogFn`, `IsKeyDownFn`, `InstantiateFn`, `DestroyFn`, `RaycastFn`, `Install`, `Engine`, `Api`
- **Сделай файл** `managed/SkyEngine.Managed/Debug.cs`
  В файле должны быть: `Log`, `LogWarning`, `LogError`, `Write`, `Debug`
- **Сделай файл** `managed/SkyEngine.Managed/Time.cs`
  В файле должны быть: `Time`
- **Сделай файл** `managed/SkyEngine.Managed/Input.cs`
  В файле должны быть: `GetKey`, `KeyCode`, `Input`
- **Дополни файл** `editor/shell/src/editor_context.cpp`
  В файле должны быть: `initScripting`, `startPlayScripts`, `tickScripts`, `stopPlayScripts`
- **Сделай файл** `tests/dotnet_host_tests.cpp`
  В файле должны быть: `startHost`, `createDotNetScriptHost`, `start`, `testLifecycleThroughRealDotNet`, `createInstance`, `loadedAssemblies`, `probeValue`, `destroyInstance`, `testSceneTickDrivesCSharp`, `createScriptRuntime`, `createObjectWorld`, `createComponentWorld`, `createStdFileSystem`, `createFileSerializationBackend`, `createSceneWorld`, `createScene`, `createObject`, `addRootObject`, `bindInstance`, `resolve`, `dispatch`, `activate`, `tick`, `unbindInstance`, `main`, `summary`
- **Сделай файл** `tests/scripting_rendering_tests.cpp`
  В файле должны быть: `loadAssembly`, `loadedAssemblies`, `createInstance`, `destroyInstance`, `testScriptingBoundary`, `createScriptRuntime`, `registerBinding`, `bindingFor`, `bindInstance`, `resolve`, `dispatch`, `unbindInstance`, `testNullRenderer`, `createNullRenderer`, `backendName`, `createOffscreenSurface`, `width`, `attachSurface`, `createFromAsset`, `liveResourceCount`, `submit`, `renderFrame`, `frameCount`, `commandsInLastFrame`, `destroy`, `testRendererRegistry`, `createRendererRegistry`, `hasBackend`, `create`, `main`, `summary`

**На выходе должно получиться (список артефактов):**
- Скрипт на C# исполняется в редакторе и в проигрывателе; Debug.Log в консоль; Time/Input доступны.

**Критерий правильности:** Скрипт-вращатель даёт 90°/с (за 1 с = 45°) в Play редактора и в плеере; тест `dotnet_host_tests` зелёный.


---

## Этап 5 (Недели 7–8). Пользовательские сборки и игровой интерфейс

**Общее описание задач контура.**

Реализовать компиляцию пользовательских скриптов и расширить API движка функциями геймплея.

- **Дополни файл** `editor/shell/src/editor_context.cpp`
  В файле должны быть: `reloadUserScripts`, `scriptSourceDirs`
- **Дополни файл** `managed/SkyEngine.Managed/Bootstrap.cs`
  В файле должны быть: `LoadUserAssembly`, `UnloadUserAssembly`
- **Сделай файл** `managed/SkyEngine.Managed/Physics.cs`
  В файле должны быть: `Raycast`, `RaycastHit`, `Physics`
- **Дополни файл** `managed/SkyEngine.Managed/ScriptComponent.cs`
  В файле должны быть: `Instantiate`, `Destroy`, `SetVelocity`, `GetWorldPosition`

**На выходе должно получиться (список артефактов):**
- Скрипты из Assets/Scripts компилируются и работают в Play; доступны Instantiate/Destroy/velocity/raycast.

**Критерий правильности:** Правка `.cs` подхватывается при следующем Play; скрипт спавнит/уничтожает объекты и читает физический луч.
