# Техническое задание · Контур E5 «Пайплайн, ассеты и пакеты»

**Область ответственности.** Система сборки, непрерывная интеграция (CI),
инфраструктура тестирования, импортёры ассетов, виртуальная файловая система,
C-интерфейс движка (совместно с E1) и менеджер пакетов. Контур страхует всю
команду: без CI ошибки сборки редактора и нестабильные тесты остаются
незамеченными.

Каждый этап (неделя) разбит на **фичи** `feature/<название>`. Одна фича = одна
ветка в git. У методов указаны: **сигнатура**, **что делает**, **параметры** и
**что возвращает**.

## Обозначения

- `std::optional<T>` — значение или «ничего»; `std::unique_ptr<T>` — владеющий указатель; `std::vector<T>`/`std::byte` — массив/байт.
- **CI (непрерывная интеграция)** — сервер, собирающий проект и прогоняющий тесты на каждое изменение.
- **Xvfb** — виртуальный X-сервер без экрана (для тестов в CI); **lavapipe** — программный драйвер Vulkan.
- **C-интерфейс (ABI)** — плоский набор C-функций `sky_editor_*`, через который .NET-редактор общается с C++-движком.
- **VFS** — виртуальная файловая система: ссылки вида `assets://…` разрешаются в реальные пути.
- **semver / MVS** — семантические версии (`>=1.2`, `^1`) и «minimal version selection» — выбор минимальной подходящей версии.

---

## Этап 1 (Неделя 1). Сборка, интеграция, тесты, первичный C-интерфейс

**Общее описание задач этапа.** Система сборки, тесты, CI и первичный
C-интерфейс движка (совместно с E1). Три фичи.

### feature/build-system

#### Файлы `CMakeLists.txt`, `engine/CMakeLists.txt`, `tests/CMakeLists.txt`
- `engine/CMakeLists.txt` — функция `sky_add_module(NAME DIR sources…)` создаёт
  статическую библиотеку с public-include-путями и стандартом C++20; регистрирует
  модули и связи между ними.
  Реализация: `set(SOURCES ${ARGN})`; если исходники есть — `add_library(${NAME} STATIC ${SOURCES})`, `target_include_directories(... PUBLIC .../${DIR}/include)`, `target_compile_features(... PUBLIC cxx_std_20)`; иначе `add_library(${NAME} INTERFACE)` с теми же include/features через `INTERFACE`. В конце всегда `add_library(sky::${NAME} ALIAS ${NAME})`. Ниже функции идут вызовы `sky_add_module` для всех модулей и `target_link_libraries` для рёбер зависимостей (по `docs/modules/modules.md`).
- корневой `CMakeLists.txt` — проект, опции, `add_subdirectory` для движка/редактора/плеера/тестов.
  Реализация: `cmake_minimum_required(3.20)`, `project(SkyEngine … LANGUAGES C CXX)`, задать `CMAKE_CXX_STANDARD 20`/`_REQUIRED ON`/`_EXTENSIONS OFF` и `CMAKE_POSITION_INDEPENDENT_CODE ON` (движок линкуется в .so-мост редактора); объявить `option(...)` (`SKY_BUILD_EDITOR/OPENGL/VULKAN/TESTS`); через `find_program(SKY_DOTNET dotnet)` собрать managed-цель; затем `add_subdirectory(engine|editor|player)` и, если `SKY_BUILD_TESTS`, — `enable_testing()` + `add_subdirectory(tests)`.

### feature/test-harness

#### Файл `tests/sky_test.hpp`
- макрос `CHECK(condition)` — фиксирует провал с файлом и строкой, не роняя процесс.
  Реализация: обёртка `do { … } while(false)`: инкрементирует счётчик `sky::test::checks`; если `!(condition)` — инкрементирует `sky::test::failures` и печатает `printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition)`. Счётчики — `inline int` в namespace `sky::test`.
- `inline int summary(const char* suite)` — печатает «N проверок, M провалов». Возвращает: код выхода (0 = успех).
  Реализация: `std::printf("%s: %d checks, %d failures\n", suite, checks, failures)` и `return failures == 0 ? 0 : 1;` — ненулевой код превращает провал в красный статус CTest.

#### Файл `.github/workflows/ci.yml`
CI на Ubuntu: установка зависимостей (ninja, lavapipe, xvfb), `cmake --build`,
`xvfb-run ctest`. Красный статус блокирует слияние.
Реализация: один job `linux` на `ubuntu-24.04` (`timeout-minutes: 30`), триггеры `push` (main/master), `pull_request`, `workflow_dispatch`. Шаги: `actions/checkout@v4`; `apt-get install` (`ninja-build libx11-dev libvulkan-dev mesa-vulkan-drivers vulkan-tools xvfb`); `actions/setup-dotnet@v4` (.NET 8); `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`; `cmake --build build -j"$(nproc)"`; тесты — `xvfb-run -a ctest --test-dir build --output-on-failure -j"$(nproc)"` (lavapipe даёт программный Vulkan, Xvfb — X-сервер).

### feature/c-abi-seed

#### Файлы `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`, `editor/native_bridge/src/editor_bridge.cpp`
Плоский C-интерфейс (совместно с E1). На этом этапе — минимум:
- `SkyEditorContext* sky_editor_create(void)` — Возвращает: указатель на сессию (собирает движок и демо-сцену).
  Реализация: `new BridgeSession()` (внутри — `EditorContext context`, который сам собирает движок и демо-сцену); вешает `context.scriptLog` на `logMsg(...)`, чтобы managed-`Debug.Log` шёл в панель Console; пишет строку «Sky Engine editor ready» и `reinterpret_cast<SkyEditorContext*>(session)`. Тип `SkyEditorContext` — непрозрачный, наружу отдаётся указатель.
- `void sky_editor_destroy(SkyEditorContext* ctx)` — уничтожает сессию.
  Реализация: `delete self(ctx)`, где хелпер `self()` делает `reinterpret_cast<BridgeSession*>(ctx)`; деструктор `BridgeSession` освобождает Vulkan/X11-ресурсы в правильном порядке.
- `int32_t sky_editor_root_count(SkyEditorContext* ctx)` — Возвращает: число корневых объектов.
  Реализация: `return int32_t(ec(ctx).rootObjects().size())`, где `ec()` возвращает ссылку на `EditorContext`.
- `SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index)` — Возвращает: id корневого объекта.
  Реализация: берёт `roots = ec(ctx).rootObjects()`; при `index < 0 || index >= roots.size()` возвращает `0` (невалидный id), иначе `roots[index].value` (голый `uint64`, чтобы пройти через C-ABI).
- `int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)` — пишет имя в буфер. Возвращает: длину.
  Реализация: `return copyString(ec(ctx).objects->nameOf(handle(object)), buffer, capacity)`; хелпер `copyString` копирует не более `capacity-1` байт, ставит `'\0'` и возвращает полную длину строки (идиома query-буфера: вызвать с `capacity=0`, чтобы узнать размер).

**На выходе:** проект собирается; CI прогоняет сборку и тесты; C-интерфейс create/destroy/enumerate работает.
**Критерий правильности этапа:** красный CI блокирует слияние; сломанный тест
краснеет; редактор E3 вызывает `sky_editor_create`.

---

## Этап 2 (Неделя 2, веха M1). Режим скриншотов и первые тесты

**Общее описание задач этапа.** Сборка редактора и скриншот-артефакт в CI;
первые модульные тесты. Одна фича.

### feature/ci-screenshot

#### Дополнение файла `editor/avalonia/Program.cs`
- ветка `--screenshot path` — headless Avalonia, рендер нескольких кадров,
  `window.CaptureRenderedFrame().Save(path)`.
  Реализация: `Main` ищет `--screenshot` в `args`; если найден — вызывает `Screenshot(path, demo)`. `Screenshot` конфигурирует `AppBuilder` с `UseSkia().UseHeadless(UseHeadlessDrawing=false)` и `SetupWithoutStarting()`, создаёт `MainWindow`, разбирает опции `--size WxH`/`--select`/`--demo`/`--play`, `window.Show()` + `Dispatcher.UIThread.RunJobs()`; для сведения кадра N раз зовёт `viewport.RenderOnce()` + `RunJobs()` (60 кадров при `--play`, иначе 4); затем `frame = window.CaptureRenderedFrame()`, при `null` — stderr и `return 1`, иначе `frame.Save(path)` и `return 0`.

#### Дополнение файла `.github/workflows/ci.yml`
- шаг `dotnet build editor/avalonia` (ошибка C# валит задачу) + публикация PNG-артефакта.
  Реализация: шаг `- name: Build Avalonia editor` c `run: dotnet build editor/avalonia/SkyEditor.csproj -c Release --nologo` (ненулевой код `dotnet` останавливает job); PNG плеера публикуется шагом `actions/upload-artifact@v4` с `name: player-smoke`, `path: player-smoke.png`.

#### Файл `tests/runtime_tests.cpp`
Проверяет, что play-режим действительно продвигает рантайм (ECS-система крутит объект).
Реализация: `testPlayModeDrivesRuntime()` создаёт object/component/ecs/physics/scene-миры, сцену с ящиком над статичным полом и entity с компонентом `Spin`, который крутит `SpinSystem` (`+90°/сек`). Создаёт `PlayModeController`; проверяет `CHECK(!controller->play())` без сцены, затем `setScene`; тик в режиме редактирования не двигает объект; после `play()` гоняет 120 тиков по `1.0/60.0` — ящик падает на пол (`y ≈ 1.0`), у спиннера накапливается угол (`> 170`); `pause()` замораживает мир, `stop()` возвращает `Editing`. `main()` зовёт тест и `return sky::test::summary("runtime_tests")`.

**На выходе:** CI собирает редактор; скриншот публикуется артефактом.
**Критерий правильности этапа:** артефакт-PNG доступен из прогона CI; ошибка C#
останавливает сборку.

---

## Этап 3 (Недели 3–4, веха M2). Импортёры, виртуальная ФС, тесты интерфейса

**Общее описание задач этапа.** Импортёры OBJ/PNG, база ассетов, виртуальная ФС
и тесты C-интерфейса. Три фичи.

### feature/asset-database

#### Файлы `engine/asset/include/sky/asset/{asset_system,asset_database}.hpp`, `engine/asset/src/asset_database.cpp`
`class IAssetImporter` — контракт импортёра:
- `virtual bool supports(...) const = 0` — Возвращает: поддерживает ли формат.
  Реализация: у конкретных импортёров — сравнение расширения, напр. `return sourcePath.extension() == ".obj"` (PNG — `".png"`, glTF — `".gltf"||".glb"`, FBX — расширение приводится к нижнему регистру и сравнивается с `".fbx"`).
- `virtual ImportResult import(...) = 0` — Возвращает: результат импорта.
  Реализация: у конкретных импортёров — вызвать соответствующий загрузчик (`loadObjMesh`/`loadPngImage`/`loadGltfMesh`/`loadFbxMesh`); при `nullopt` вернуть `std::nullopt`, иначе заполнить `AssetDescriptor` (только `assetType` — `"mesh"` или `"texture"`) и вернуть его. Идентичность (`id`, `sourcePath`, `contentVersion`) проставляет не импортёр, а конвейер.
`class AssetDatabase`:
- `std::optional<AssetId> importAsset(const std::filesystem::path& sourcePath)` — импортирует ассет. Возвращает: id или `nullopt`.
  Реализация: `AssetDatabaseImpl::importAsset` перебирает зарегистрированные `importers_`, берёт первый, у кого `supports(path)`; зовёт `import(path)`, при `nullopt` возвращает `std::nullopt`; иначе конвейер владеет идентичностью — `descriptor->id = assetIdFromPath(path)` (FNV-1a по нормализованному generic-пути, `0` заменяется на `1`), `descriptor->sourcePath = path`, `++contentVersion`, и `registerAsset(*descriptor)` кладёт дескриптор в `unordered_map assets_`.
- `std::optional<AssetDescriptor> resolve(AssetId id) const` — Возвращает: описание ассета или `nullopt`.
  Реализация: `assets_.find(id.value)`; если не найдено — `std::nullopt`, иначе `it->second` (копия дескриптора).
- `std::unique_ptr<AssetDatabase> createAssetDatabase()` — фабрика.
  Реализация: `return std::make_unique<AssetDatabaseImpl>()` — `Impl` в анонимном namespace реализует три контракта (`IAssetResolver`/`IAssetRegistry`/`IImportPipeline`) над `unordered_map<uint64,AssetDescriptor> assets_` и `vector<IAssetImporter*> importers_`.

### feature/importers-obj-png

#### Файлы `engine/asset/include/sky/asset/obj_importer.hpp`, `engine/asset/src/obj_importer.cpp`
- `std::unique_ptr<IAssetImporter> createObjImporter(...)` — импортёр OBJ (парсинг v/vn/vt/f).
  Реализация: `std::make_unique<ObjImporter>(fileSystem)`; `ObjImporter::supports` — расширение `.obj`, `import` зовёт свободную `loadObjMesh(fileSystem, path)`. `loadObjMesh` читает файл через `fileSystem.readAll`, построчно разбирает `istringstream`: `v`→`positions`, `vt`→`texcoords`, `vn`→`normals`, `f`→грань (хелпер `parseFaceVertex` парсит `1`,`1/2`,`1//3`,`1/2/3`; `resolveIndex` учитывает отрицательные индексы). Грани триангулируются веером `(0,i,i+1)`; при отсутствии нормали считается плоская (`flatNormal`); на выход — интерливленный `vector<float>` по 8 значений (pos3, nrm3, uv2). Любая ошибка/пустой меш → `nullopt`.

#### Файлы `engine/asset/include/sky/asset/png_decoder.hpp`, `engine/asset/src/png_decoder.cpp`
`struct ImageData { std::uint32_t width, height; std::vector<std::uint8_t> pixels; }`.
- `std::optional<ImageData> decodePng(const std::vector<std::byte>& bytes)` — декодирует PNG. Возвращает: изображение или `nullopt`.
  Реализация: проверить 8-байтовую сигнатуру; пройти поток чанков (`readU32` для длины, сравнение типа через `memcmp`): `IHDR`→ширина/высота/bitDepth/colorType/interlace, `PLTE`/`tRNS`→палитра/альфа, `IDAT`→накопить сжатые байты, `IEND`→стоп (CRC не проверяются). Поддержка только `bitDepth==8`, `interlace==0`; распаковать zlib через `libdeflate_zlib_decompress`; снять построчные фильтры (0..4, включая `paeth`) на месте; развернуть по `colorType` (grayscale/RGB/palette/gray+alpha/RGBA) в RGBA8 `ImageData`. Любое несоответствие → `nullopt`.
- `std::vector<std::byte> encodePngRgba(const ImageData& image)` — кодирует RGBA в PNG. Возвращает: байты файла.
  Реализация: пишет сигнатуру, затем чанки хелпером `appendChunk` (длина + тип+payload + CRC32 через локальную таблицу `crc32Of`): `IHDR` (width/height, bitDepth 8, colorType 6), `IDAT` и `IEND`. Сканлайны берутся с байтом фильтра 0; zlib-поток собирается из «stored» deflate-блоков (без сжатия, заголовок `0x78 0x01`) c ручным Adler-32 в хвосте.
- `std::unique_ptr<IAssetImporter> createPngImporter(platform::IFileSystem& fileSystem)` — импортёр PNG.
  Реализация: `std::make_unique<PngImporter>(fileSystem)`; `supports` — расширение `.png`; `import` зовёт `loadPngImage` (`fileSystem.readAll` + `decodePng`), при успехе возвращает `AssetDescriptor` с `assetType = "texture"`, иначе `nullopt`.

### feature/vfs-and-bridge-tests

#### Файлы `engine/platform/include/sky/platform/{file_system,virtual_file_system}.hpp` + `src/{std_file_system,virtual_file_system}.cpp`
`class IFileSystem` (exists/isDirectory/readAll/writeAll/list) и
Реализация: реальная реализация — `StdFileSystem` (`createStdFileSystem`) поверх `std::filesystem`/`fstream`: `exists`/`isDirectory` — `fs::exists`/`fs::is_directory` с `error_code`; `readAll` открывает `ifstream` в режиме `binary|ate`, читает по размеру в `vector<byte>`; `writeAll` создаёт родительские каталоги и пишет `ofstream` (`binary|trunc`); `list` собирает пути через `fs::directory_iterator`; `remove` — `fs::remove_all`.
`class IVirtualFileSystem`:
- `bool mount(const std::string& alias, std::shared_ptr<IVfsMount> mountPoint, int priority)` — монтирует схему (напр. `assets`). Возвращает: успех.
  Реализация: `VirtualFileSystem::mount(alias, mountPoint, priority)` отклоняет пустой alias/`nullptr`; добавляет `MountEntry{mount, priority}` в стек `mounts_[alias]` и `stable_sort` по убыванию приоритета (оверлей: старший приоритет перекрывает). Монтпойнты создаются `createDirectoryMount(fileSystem, root, readOnly)` — `DirectoryMount` транслирует относительные пути в `root / relative`.
- `std::optional<std::vector<std::byte>> readAll(const std::string& ref)` — читает по ссылке `assets://…`. Возвращает: байты или `nullopt`.
  Реализация: `resolveStack(ref)` разбирает ссылку `VfsPath::parse` (делит по `://`, нормализует `\`→`/`, срезает ведущие `/`, отклоняет `..`) и находит стек по alias; затем перебирает записи стека по приоритету и возвращает `entry.mount->read(...)` у первого, где `exists(relative)`; иначе `nullopt`.
- `std::vector<...> list(const std::string& dir)` — Возвращает: содержимое каталога.
  Реализация: `resolveStack(dir)`, затем сливает листинги всех монтпойнтов стека в `std::set<std::string>` (перекрытые имена схлопываются в одно), возвращает вектор из множества; пустой при неразрешимом пути.

#### Файлы `tests/{editor_bridge_tests,asset_project_tests,vfs_tests}.cpp`
Тест на каждую группу C-интерфейса и на импорт/VFS.
Реализация: `editor_bridge_tests` работает только через плоский C-ABI (без C++-типов движка): `sky_editor_create`, проверка непустых корней демо-сцены, перебор детей, имён (хелпер `nameOf` через `sky_editor_object_name`), компонентов (`hasComponent`) и т.д. `asset_project_tests` регистрирует `FakeMeshImporter` (расширение `.mesh`, объявляет зависимость на соседний `.material`), проверяет отказ неподдерживаемого источника, стабильность id (`assetIdFromPath`), `resolve`, обратный граф `dependentsOf`, `reimport` с ростом `contentVersion`. `vfs_tests` проверяет `VfsPath::parse` (alias/relative, нормализацию, отказ на escape `..`) и маршрутизацию `mount`/`exists`/`readAll`/`list` через `createDirectoryMount`. Каждый файл — свой `main()`, возвращающий `sky::test::summary(...)`.

**На выходе:** OBJ/PNG импортируются; ссылки `assets://` разрешаются; группы ABI покрыты тестами.
**Критерий правильности этапа:** `asset_project_tests`, `vfs_tests`,
`editor_bridge_tests` зелёные.

---

## Этап 4 (Недели 5–6, веха M3). Импортёры FBX/glTF, запуск сцены проигрывателем

**Общее описание задач этапа.** Импортёры FBX/glTF и запуск сохранённой сцены
проигрывателем (совместно с E4). Одна фича.

### feature/importers-fbx-gltf

#### Файлы `engine/asset/include/sky/asset/{fbx_importer,gltf_importer}.hpp` + `.cpp`, `engine/asset/src/mini_json.hpp`
- `std::unique_ptr<IAssetImporter> createFbxImporter(...)`, `createGltfImporter(...)` — импортёры FBX и glTF (для glTF используется `mini_json.hpp`).
  Реализация FBX: `make_unique<FbxImporter>`; `supports` — `.fbx` (регистронезависимо), `import` зовёт `loadFbxMesh`. `loadFbxMesh` читает байты и грузит сцену вендоренным OpenFBX (`ofbx::load`), по каждому мешу берёт `getGeometryData()`, для каждого partition/polygon вызывает `ofbx::triangulate`, трансформирует позиции/нормали глобальной матрицей меша (`transformPoint`/`transformNormal`) и пишет интерливленные `float` (pos3/nrm3/uv2); `scene->destroy()`, пустой меш → `nullopt`.
  Реализация glTF: `make_unique<GltfImporter>`; `supports` — `.gltf`/`.glb`, `import` зовёт `loadGltfMesh`. `openDocument` читает файл, для GLB разбирает контейнер (magic `glTF`, чанки JSON/BIN), для .gltf парсит текст `detail::parseJson` (парсер из `mini_json.hpp`); буферы берутся из BIN-чанка, `data:`-URI (`decodeBase64`) или соседних файлов. Затем обход графа: `emitNode` рекурсивно множит матрицы `fromTrs` (matrix или T·R·S), `emitMesh` читает примитивы (только mode 4/треугольники) через `AccessorReader` (учёт bufferView/stride/componentType), при отсутствии нормалей — `flatNormal`; результат — тот же формат `vector<float>`.

#### Дополнение `CMakeLists.txt` и `.github/workflows/ci.yml`
- цель `sky_managed` и переменная `SKY_MANAGED_DIR` (совместно с E4).
  Реализация: в корневом `CMakeLists.txt` — `find_program(SKY_DOTNET dotnet)`; если найден, `SKY_MANAGED_DIR = ${CMAKE_BINARY_DIR}/managed`, `add_custom_command` собирает `SkyEngine.Managed.dll`/`SkyEngine.TestScripts.dll` (`dotnet build … -c Release -o ${SKY_MANAGED_DIR}`) с `DEPENDS` на все `.cs`, и `add_custom_target(sky_managed ALL DEPENDS …dll)`. Нативные цели находят сборки в рантайме по `SKY_MANAGED_DIR`; без .NET скриптинг просто выключен, а зависящие от него тесты гейтятся `if(TARGET sky_managed)` в `tests/CMakeLists.txt`.
- шаг CI «player smoke»: `sky_player --headless player-smoke.png --frames 60` + артефакт.
  Реализация: шаг `- name: Player smoke (headless, scripted demo scene)` c `run: ./build/player/sky_player --headless player-smoke.png --frames 60`; следом `actions/upload-artifact@v4` (`name: player-smoke`, `path: player-smoke.png`). Ненулевой код плеера валит job.

**На выходе:** FBX и glTF импортируются; проигрыватель запускает сцену; smoke в CI.
**Критерий правильности этапа:** `sky_player --scene X.skybox` запускает сцену;
шаг smoke в CI зелёный.

---

## Этап 5 (Недели 7–8, веха M4). Менеджер пакетов

**Общее описание задач этапа.** Менеджер пакетов: манифесты, разрешение версий,
файл блокировки, установка из источников, код из пакетов. Три фичи.

### feature/package-resolver

#### Файлы `engine/package/include/sky/package/{package_system,package_world}.hpp`, `semver.hpp`, `engine/package/src/package_world.cpp`
- `struct Version {int major, minor, patch;}`; `std::optional<Version> parseVersion(const std::string& text)` — разбирает версию. Возвращает: версию или `nullopt`.
  Реализация (header-only, `semver.hpp`): `std::sscanf(text, "%d.%d.%d%n", …)` в `Version` (пропущенные поля = 0, так `"1.2"`→1.2.0); если распознано < 1 поля или любое поле < 0 — `nullopt`. Затем проверка на «мусор в хвосте»: считает префикс из цифр и точек и требует, чтобы он покрывал всю строку (`"1.2.3-beta"` → `nullopt`). `Version` сравнивается через дефолтный `operator<=>`.
- `bool satisfies(const Version& version, const std::string& requirement)` — проверяет соответствие констрейнту (`*`, `>=`, `^`, точная). Возвращает: да/нет.
  Реализация: `""`/`"*"` → `true`; префикс `">="` → `version >= parseVersion(остаток)`; префикс `"^"` → `version >= minimum && major совпадает`; иначе точная версия по «написанной точности»: число точек 2 → полное равенство, 1 → совпадение major.minor, 0 → совпадение major. Неразобранный `requirement` матчит «ничего» (fail closed).
- `class PackageWorld`:
  - `std::size_t discoverPackages(const std::filesystem::path& packagesRoot)` — сканирует каталог. Возвращает: число найденных пакетов.
    Реализация: `PackageWorldImpl` перебирает `fileSystem_.list(packagesRoot)`, для каждого подкаталога читает манифест (`loadPackageManifest` → `storage.read(dir/kPackageFileName)`, проверка schemaId/версии, чтение полей `ByteReader`); при непарсящейся версии пропускает; кладёт в двухуровневую `map<packageId, map<Version, PackageManifest>> discovered_` (несколько версий сосуществуют, `std::map` держит версии по возрастанию для MVS); возвращает счётчик добавленных.
  - `std::vector<PackageManifest> resolve(const std::vector<std::string>& refs)` — разрешает граф зависимостей (MVS). Возвращает: пакеты в порядке активации (пусто при конфликте).
    Реализация: minimal version selection. Собирает по каждой ссылке (`"id"` или `"id@requirement"`) множество констрейнтов `map<id, vector<string>>`. Итерирует до стабилизации `selected`: для каждого id берёт из `discovered_` первую (минимальную) версию, удовлетворяющую всем констрейнтам через `satisfies` — при отсутствии подходящей или неизвестном пакете возвращает `{}`; при смене выбора добавляет в требования зависимости выбранного манифеста (констрейнты только накапливаются → сходится). Затем топологическая сортировка (`visit` — DFS с `inProgress`/`done`, цикл → `{}`), возвращает манифесты в порядке активации (зависимости раньше зависимых).
  - `std::vector<PackageManifest> discoveredPackages() const` — Возвращает: все обнаруженные.
    Реализация: разворачивает `discovered_` (id→version→manifest) в плоский `vector<PackageManifest>`, обходя все версии всех пакетов.

### feature/package-lock-and-install

#### Файлы `engine/package/include/sky/package/{package_lock,package_installer}.hpp`, `engine/package/src/package_installer.cpp`
- `std::uint64_t manifestChecksum(const PackageManifest& manifest)` — Возвращает: контрольную сумму манифеста.
  Реализация: сериализует `ByteWriter`-ом поля манифеста (packageId, version, displayName, все зависимости `packageId`+`versionRequirement`, все extensionPoints) и считает FNV-1a по получившемуся буферу (`hash=1469598103934665603`, множитель `1099511628211`). Стабильна для одинакового манифеста, меняется при любой правке полей.
- `bool savePackageLock(...)` / `std::optional<std::vector<LockedPackage>> loadPackageLock(...)` — запись/чтение `sky.lock` (версии + чексуммы + active).
  Реализация: `savePackageLock` пишет `ByteWriter`-ом `U32` количество, затем на каждый `LockedPackage` — `packageId`, `version`, `manifestChecksum` (`U64`), флаг `active` (`U32`); отдаёт в `storage.write(path, {kLockSchemaId, {1,0}, buffer})`. `loadPackageLock` читает blob, проверяет schemaId/версию (иначе `nullopt`), `ByteReader`-ом восстанавливает записи; при любом сбое чтения — `nullopt` (отсутствующий файл ≠ пустой lock).
- `class PackageInstaller`:
  - `std::optional<PackageManifest> install(const std::string& source, const std::filesystem::path& projectPackages)`
    Что делает: устанавливает пакет из источника (каталог / tarball / git с `#tag`) через immutable-кэш. Параметры: `source`, `projectPackages` — куда установить. Возвращает: манифест или `nullopt`.
    Реализация: определяет тип источника — tarball (`isTarball` по расширению, распаковка `tar -xf` во `.staging`, поиск `findPackageDir`), каталог с манифестом, либо иначе git (`url#tag` → `git clone --depth 1 [--branch tag]` во `.staging`). Найденный каталог кладётся в неизменяемый кэш `cachePackageDir` (`cacheRoot/<id>/<version>`, уже закэшированная версия переиспользуется как есть; версия обязана парситься). Затем `copyTree` копирует из кэша в `projectPackages/<id>-<version>` (версионированный каталог, чтобы версии стояли рядом; `.git` удаляется), правит `rootPath` в манифесте и возвращает его; на любой ошибке — `nullopt`.

### feature/package-tests

#### Файл `tests/package_tests.cpp`
semver, MVS, конфликты, lock round-trip, установка из трёх источников.
Реализация: `main()` вызывает набор тестов. `testSemver` прогоняет таблицу `parseVersion`/`satisfies`. `testDiscoveryAndResolution` пишет пакеты (`savePackageManifest`) во временный каталог, проверяет число обнаруженных, порядок активации (зависимости раньше), отказ на неизвестную ссылку, `activate`/`deactivate` и регистрацию extension-points по категориям. `testCycleDetection` — взаимные зависимости дают пустой `resolve`. `testMinimalVersionSelection` — три версии либы, два потребителя (`>=1.2` и `^1`) → выбирается минимальная 1.5.0; проверяет прямой констрейнт `@^2`, конфликт `^1` vs `>=2` (пусто) и недостижимое `@>=3`. `testLockRoundTrip` — стабильность/изменчивость `manifestChecksum`, `savePackageLock`+`loadPackageLock`, отсутствующий файл → `nullopt`. `testInstaller` (пропускается без tar/git) ставит пакет из каталога, tarball и локального git-репозитория с тегом, проверяя кэш, версионированные каталоги проекта и отсутствие `.git`.

**На выходе:** пакет ставится из каталога/архива/git; версии резолвятся; `sky.lock` персистит; код пакета работает в Play.
**Критерий правильности этапа:** `package_tests` зелёный (semver, MVS, конфликты,
lock round-trip, установка).
