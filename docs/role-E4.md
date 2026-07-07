# ТЗ · E4 — Рантайм и скриптинг (весь срок)

**Роль.** Владелец симуляции и «жизни»: физика, play mode, ввод,
standalone-плеер и весь скриптинг (.NET-хостинг). Твой скриптинг —
**самый длинный хвост зависимостей**, начинай его не позже недели 5.

**Стек.** C++20, .NET hosting (hostfxr), C# (managed-рантайм).
**Модули:** `physics`, `scripting`, `player`, `managed/SkyEngine.Managed`.

## Твои суставы
- **Предоставляешь:** `IPhysicsWorld/IPhysicsQueryService/IPhysicsSyncContract` (E1 sync), `PlayModeController` (E3/E5 транспорт), `IScriptHost`/`DotNetScriptHost` (E1 контекст), managed `ScriptComponent` (пользователи).
- **Потребляешь:** `object` (E1) для sync, `EditorContext` (E1) для скриптинга.

---

## Неделя 1 — физика
- `[H] physics/physics.hpp` — типы (`RigidBodyDesc`, `HeightfieldDesc`, `RaycastHit`, `CollisionEvent`) + `IPhysicsWorld/IPhysicsQueryService/IPhysicsSyncContract`.
- `[H] physics/physics_world.hpp` — `PhysicsWorld`, `ObjectPhysicsSync`, фабрики.
- `[S] physics_world.cpp` — по тикетам: гравитация в `step` (E4-1) → `rayVsAabb`/`detectAndResolve` (E4-2) → `sampleHeightfield`/`resolveHeightfields` (E4-3) → `ObjectPhysicsSync push/pull` (E4-4).
- `[T] tests/physics_tests.cpp` — падение ≈4.9 м/с; куб на полу; тело на heightfield; привязанный объект синхронизирован.
**Зависишь:** E1-3 (мир объектов для sync — до его готовности тестируй физику автономно). **Готово:** `ctest -R physics_tests`.

## Неделя 2 (M1) — скелет плеера
- `[S] player/src/main.cpp` — `--frames`, `--headless`; `runHeadless`/`runWindowed`; X11-окно + `createVulkanRendererForWindow`; цикл `tickFrame→build→renderFrame`.
**Зависишь:** E2 (swapchain-рендерер), E1 (EditorContext). **Готово:** `sky_player --headless` пишет PNG.

## Недели 3–4 (M2) — play mode и ввод
- `[H] viewport_bridge/play_mode_controller.hpp` — `PlayModeState{Editing,Playing,Paused}`, `play/pause/stop/tickFrame/state`.
- `[S] editor_context.cpp` (дополнить): `beginPlay()` (снапшот трансформов), `endPlay()` (restore + re-seat тел с нулевой скоростью), `setKeyDown/keyDown`; портируемые кей-коды (общий enum с C#).
**Разблокируешь:** E3/E5 (транспорт-кнопки), скриптинг (тик play).

## Недели 5–6 (M3) — скриптинг (критический путь)
- `[H] scripting/scripting_boundary.hpp` — `ScriptLifecycleEvent`, `AssemblyRef`.
- `[H] scripting/script_host.hpp` — `IScriptHost` (start/loadAssembly/createInstance/invokeLifecycle).
- `[H+S] scripting/dotnet_host.{hpp,cpp}` — `DotNetHostConfig`, `DotNetScriptHost` (installEngineApi/setInstanceObjectId/beginFrame/scriptClassNames); реализация через hostfxr + `UnmanagedCallersOnly`.
- `[S] managed/SkyEngine.Managed/*` — `Bootstrap.cs` (энтрипоинты), `ScriptComponent.cs` (lifecycle + SetLocal*), `NativeHandle.cs`, `Engine.cs` (reverse-API), `Debug.cs`, `Time.cs`, `Input.cs`.
- `[S] editor_context.cpp` (дополнить): `initScripting/startPlayScripts/tickScripts/stopPlayScripts`, файл-scope колбэки, `SkyScriptApi`, регистрация `sky.script`.
- `[S] player/src/main.cpp` (дополнить): `beginPlay()` + `tickScripts(dt)` в цикле; загрузка `--scene`.
- `[B] CMakeLists.txt` — `sky_managed` target, `SKY_MANAGED_DIR` в bridge/player/тесты.
**Готово (M3):** C#-скрипт крутит объект и в редакторе, и в плеере — проверка 90°/с × 1с = 45°.

## Недели 7–8 (M4) — пользовательские сборки, игровой API
- `[S] editor_context.cpp` (дополнить): `reloadUserScripts()` (генерируемый csproj над Assets/Scripts + Runtime/*.cs пакетов → dotnet build → loadUserAssembly), `scriptSourceDirs()`.
- `[S] Bootstrap.cs` (дополнить): `LoadUserAssembly/UnloadUserAssembly` — collectible `AssemblyLoadContext`.
- `[S] Engine.cs`/`ScriptComponent.cs`/`Physics.cs` (дополнить): reverse-API до 12 указателей — `GetWorldPosition/Instantiate/Destroy/SetVelocity/GetVelocity/Raycast`; хелперы по id чужого объекта; `Physics.Raycast`.
- `[S] editor_context.cpp` (дополнить): нативные колбэки + raycast по heightfield (марш+бисекция).
- `[S] managed/SkyEngine.TestScripts/CrateRain.cs` — демо-игра (dogfood API).
**Готово (M4):** пользовательский `.cs` работает в Play; мини-игра запускается в плеере.

## Твои личные ворота
- M1: плеер пишет PNG.
- M2: play/stop с восстановлением сцены.
- M3: скрипт работает в редакторе и в билде.
- M4: пользовательские скрипты + игровой API + демка.
