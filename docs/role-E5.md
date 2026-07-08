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
- корневой `CMakeLists.txt` — проект, опции, `add_subdirectory` для движка/редактора/плеера/тестов.

### feature/test-harness

#### Файл `tests/sky_test.hpp`
- макрос `CHECK(condition)` — фиксирует провал с файлом и строкой, не роняя процесс.
- `inline int summary(const char* suite)` — печатает «N проверок, M провалов». Возвращает: код выхода (0 = успех).

#### Файл `.github/workflows/ci.yml`
CI на Ubuntu: установка зависимостей (ninja, lavapipe, xvfb), `cmake --build`,
`xvfb-run ctest`. Красный статус блокирует слияние.

### feature/c-abi-seed

#### Файлы `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`, `editor/native_bridge/src/editor_bridge.cpp`
Плоский C-интерфейс (совместно с E1). На этом этапе — минимум:
- `SkyEditorContext* sky_editor_create(void)` — Возвращает: указатель на сессию (собирает движок и демо-сцену).
- `void sky_editor_destroy(SkyEditorContext* ctx)` — уничтожает сессию.
- `int32_t sky_editor_root_count(SkyEditorContext* ctx)` — Возвращает: число корневых объектов.
- `SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index)` — Возвращает: id корневого объекта.
- `int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)` — пишет имя в буфер. Возвращает: длину.

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

#### Дополнение файла `.github/workflows/ci.yml`
- шаг `dotnet build editor/avalonia` (ошибка C# валит задачу) + публикация PNG-артефакта.

#### Файл `tests/runtime_tests.cpp`
Проверяет, что play-режим действительно продвигает рантайм (ECS-система крутит объект).

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
- `virtual ImportResult import(...) = 0` — Возвращает: результат импорта.
`class AssetDatabase`:
- `AssetId importAsset(const std::filesystem::path& path)` — импортирует ассет. Возвращает: id.
- `std::optional<AssetDescriptor> resolve(AssetId id)` — Возвращает: описание ассета или `nullopt`.
- `std::unique_ptr<AssetDatabase> createAssetDatabase(...)` — фабрика.

### feature/importers-obj-png

#### Файлы `engine/asset/include/sky/asset/obj_importer.hpp`, `engine/asset/src/obj_importer.cpp`
- `std::unique_ptr<IAssetImporter> createObjImporter(...)` — импортёр OBJ (парсинг v/vn/vt/f).

#### Файлы `engine/asset/include/sky/asset/png_decoder.hpp`, `engine/asset/src/png_decoder.cpp`
`struct ImageData { std::uint32_t width, height; std::vector<std::uint8_t> pixels; }`.
- `std::optional<ImageData> decodePng(const std::vector<std::byte>& bytes)` — декодирует PNG. Возвращает: изображение или `nullopt`.
- `std::vector<std::byte> encodePngRgba(const ImageData& image)` — кодирует RGBA в PNG. Возвращает: байты файла.
- `std::unique_ptr<IAssetImporter> createPngImporter(platform::IFileSystem& fileSystem)` — импортёр PNG.

### feature/vfs-and-bridge-tests

#### Файлы `engine/platform/include/sky/platform/{file_system,virtual_file_system}.hpp` + `src/{std_file_system,virtual_file_system}.cpp`
`class IFileSystem` (exists/isDirectory/readAll/writeAll/list) и
`class IVirtualFileSystem`:
- `void mount(const std::string& scheme, std::unique_ptr<IVfsMount> mount, int priority)` — монтирует схему (напр. `assets`).
- `std::optional<std::vector<std::byte>> readAll(const std::string& ref)` — читает по ссылке `assets://…`. Возвращает: байты или `nullopt`.
- `std::vector<...> list(const std::string& dir)` — Возвращает: содержимое каталога.

#### Файлы `tests/{editor_bridge_tests,asset_project_tests,vfs_tests}.cpp`
Тест на каждую группу C-интерфейса и на импорт/VFS.

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

#### Дополнение `CMakeLists.txt` и `.github/workflows/ci.yml`
- цель `sky_managed` и переменная `SKY_MANAGED_DIR` (совместно с E4).
- шаг CI «player smoke»: `sky_player --headless player-smoke.png --frames 60` + артефакт.

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
- `bool satisfies(const Version& version, const std::string& requirement)` — проверяет соответствие констрейнту (`*`, `>=`, `^`, точная). Возвращает: да/нет.
- `class PackageWorld`:
  - `std::size_t discoverPackages(const std::filesystem::path& packagesRoot)` — сканирует каталог. Возвращает: число найденных пакетов.
  - `std::vector<PackageManifest> resolve(const std::vector<std::string>& refs)` — разрешает граф зависимостей (MVS). Возвращает: пакеты в порядке активации (пусто при конфликте).
  - `std::vector<PackageManifest> discoveredPackages() const` — Возвращает: все обнаруженные.

### feature/package-lock-and-install

#### Файлы `engine/package/include/sky/package/{package_lock,package_installer}.hpp`, `engine/package/src/package_installer.cpp`
- `std::uint64_t manifestChecksum(const PackageManifest& manifest)` — Возвращает: контрольную сумму манифеста.
- `bool savePackageLock(...)` / `std::optional<std::vector<LockedPackage>> loadPackageLock(...)` — запись/чтение `sky.lock` (версии + чексуммы + active).
- `class PackageInstaller`:
  - `std::optional<PackageManifest> install(const std::string& source, const std::filesystem::path& projectPackages)`
    Что делает: устанавливает пакет из источника (каталог / tarball / git с `#tag`) через immutable-кэш. Параметры: `source`, `projectPackages` — куда установить. Возвращает: манифест или `nullopt`.

### feature/package-tests

#### Файл `tests/package_tests.cpp`
semver, MVS, конфликты, lock round-trip, установка из трёх источников.

**На выходе:** пакет ставится из каталога/архива/git; версии резолвятся; `sky.lock` персистит; код пакета работает в Play.
**Критерий правильности этапа:** `package_tests` зелёный (semver, MVS, конфликты,
lock round-trip, установка).
