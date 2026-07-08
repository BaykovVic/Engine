# Спринт 1 · День 8 — выдача фич (по одной на контур)

День 8: каждый контур берёт свою **8-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/editor-context` | Этап 2 (Неделя 2, веха M1). Сцена и сборочная точка |
| **E2** Рендеринг | `feature/pbr-textures` | Этап 4 (Недели 5–6, веха M3). Текстуры и слоты PBR |
| **E3** Редактор(.NET) | `feature/data-panels` | Этап 3 (Недели 3–4, веха M2). Гизмо, инспектор, панели данных |
| **E4** Рантайм/скриптинг | `feature/scripting-integration` | Этап 4 (Недели 5–6, веха M3). Подсистема скриптинга |
| **E5** Пайплайн/пакеты | `feature/importers-fbx-gltf` | Этап 4 (Недели 5–6, веха M3). Импортёры FBX/glTF, запуск сцены проигрывателем |
| **E6** Data-oriented(ECS) | — (фичи этого контура закончились) | — |

---

## E1 · `feature/editor-context`

*Этап: Этап 2 (Неделя 2, веха M1). Сцена и сборочная точка.*

#### Файлы `editor/shell/src/editor_context.hpp`, `editor/shell/src/editor_context.cpp`
`class EditorContext` — собирает движок в один объект и строит демо-сцену.
- `EditorContext()` — конструктор: создаёт подсистемы, вызывает `buildDemoScene()`.
- `std::vector<object::ObjectHandle> rootObjects() const` — Возвращает: корневые объекты (нужно E3 для дерева, E2 для обхода).
- `object::ObjectHandle createEmpty(const std::string& name)` — Возвращает: хэндл пустого объекта.
- `object::ObjectHandle createPrimitive(scene::PrimitiveKind kind, const std::string& name)` — Возвращает: хэндл примитива.
- приватный `void buildDemoScene()` — наполняет сцену примитивами, светом, камерой.

---

## E2 · `feature/pbr-textures`

*Этап: Этап 4 (Недели 5–6, веха M3). Текстуры и слоты PBR.*

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- `createTextureFromData(w, h, rgba)` — загрузка текстуры. Возвращает: хэндл.
- `uploadTexture(...)` — создаёт изображение GPU, вид и дескриптор.
- `bindMaterial(command)` — привязывает шесть дескрипторных сетов PBR
  (albedo/normal/roughness/metallic/occlusion/height); отсутствующий слот
  заменяется нейтральным значением (белый / плоская нормаль).

---

## E3 · `feature/data-panels`

*Этап: Этап 3 (Недели 3–4, веха M2). Гизмо, инспектор, панели данных.*

#### Файлы `editor/avalonia/Views/{ConsoleView,MaterialsView,PackagesView}.axaml.cs`
Панели консоли (лог с цветом по уровню), материалов и пакетов — на живых данных
через соответствующие P/Invoke.

---

## E4 · `feature/scripting-integration`

*Этап: Этап 4 (Недели 5–6, веха M3). Подсистема скриптинга.*

#### Дополнение файла `editor/shell/src/editor_context.cpp`
- `void initScripting()` — создаёт хост, загружает сборку, ставит обратный API (`installEngineApi`).
- `void startPlayScripts()` — создаёт инстансы для всех `sky.script`, вызывает OnCreate/OnStart.
- `void tickScripts(double deltaSeconds)` — вызывает OnUpdate каждый кадр.
- `void stopPlayScripts()` — OnDestroy + уничтожение инстансов.
Плюс файловые колбэки `scriptSetLocal*`, `scriptLogMessage`, `scriptIsKeyDown` и
структура `SkyScriptApi` (таблица нативных указателей).

#### Файлы `tests/dotnet_host_tests.cpp`, `tests/scripting_rendering_tests.cpp`

---

## E5 · `feature/importers-fbx-gltf`

*Этап: Этап 4 (Недели 5–6, веха M3). Импортёры FBX/glTF, запуск сцены проигрывателем.*

#### Файлы `engine/asset/include/sky/asset/{fbx_importer,gltf_importer}.hpp` + `.cpp`, `engine/asset/src/mini_json.hpp`
- `std::unique_ptr<IAssetImporter> createFbxImporter(...)`, `createGltfImporter(...)` — импортёры FBX и glTF (для glTF используется `mini_json.hpp`).

#### Дополнение `CMakeLists.txt` и `.github/workflows/ci.yml`
- цель `sky_managed` и переменная `SKY_MANAGED_DIR` (совместно с E4).
- шаг CI «player smoke»: `sky_player --headless player-smoke.png --frames 60` + артефакт.

---
