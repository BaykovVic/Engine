# ТЗ · E5 — Пайплайн, ассеты и пакеты (весь срок)

**Роль.** Владелец «инфраструктуры вокруг кода»: система сборки, CI, тест-
харнесс, ассет-импортеры, VFS, C ABI мост (совместно с E1) и — на второй
половине срока — менеджер пакетов. Ты страхуешь всю команду: без CI
C#-поломки и гонки в тестах проходят молча.

**Стек.** C++, CMake, GitHub Actions, .NET (сборка редактора в CI).
**Модули:** `asset`, `platform/vfs`, `editor/native_bridge`, `package`, CI, тесты.

## Твои суставы
- **Предоставляешь:** `sky_add_module` (сборка), `sky_test.hpp`/`CHECK` (тесты), C ABI `sky_editor_*` (совместно с E1 — дверь для E3), `IAssetImporter`/`AssetDatabase`, `IVirtualFileSystem`, `PackageWorld`.
- **Потребляешь:** все модули (линкуешь их граф), `EditorContext` (мост поверх него).

---

## Неделя 1 — сборка, CI, тесты, затравка ABI
- `[B] CMakeLists.txt` (корень) — проект, C++20, PIC, опции, `add_subdirectory`.
- `[B] engine/CMakeLists.txt` — `function(sky_add_module NAME DIR)`, регистрация модулей недели, `target_link_libraries` по графу.
- `[H] tests/sky_test.hpp` — `CHECK(cond)` (файл:строка, без падения), `summary(suite)`.
- `[B] tests/CMakeLists.txt` — `sky_add_test`, регистрация `core/world/physics/vulkan`.
- `[B] .github/workflows/ci.yml` — ubuntu, ninja+lavapipe+xvfb+dotnet; `cmake --build` → `xvfb-run ctest`.
- `[H+S] editor/native_bridge/.../editor_bridge.{h,cpp}` (совместно с E1) — затравка: `create/destroy`, `root_count/at`, `object_name`.
**Разблокируешь:** всех (сборка+тесты), E3 (заглушка ABI). **Готово:** красный CI блокирует мёрж; сломанный тест краснеет.

## Неделя 2 (M1) — скриншот-режим, первые тесты
- `[S] editor/avalonia/Program.cs` (совместно с E3) — ветка `--screenshot` (headless, `CaptureRenderedFrame().Save`).
- `[B] ci.yml` (дополнить): dotnet-сборка редактора (C#-ошибка валит джобу) + headless-скриншот-артефакт.
- `[T] tests/scene_tests.cpp`, `runtime_tests.cpp`.
**Готово:** PNG-артефакт из CI; редактор собирается в конвейере.

## Недели 3–4 (M2) — импортеры, VFS, bridge-тесты
- `[H+S] asset/obj_importer.{hpp,cpp}` — парсинг OBJ.
- `[H+S] asset/png_decoder.{hpp,cpp}` — `decodePng`/`encodePngRgba`, `ImageData`.
- `[H+S] asset/asset_database.{hpp,cpp}` — `importAsset`, `resolve`, `contentVersion`.
- `[H+S] platform/virtual_file_system.{hpp,cpp}` — `mount`, `resolve("assets://")`, `list`.
- `[H+S] editor/native_bridge/src/editor_bridge.cpp` (дополнить, с E1): группы ABI под панели E3.
- `[T] tests/editor_bridge_tests.cpp` — тест на каждую группу ABI.
**Разблокируешь:** E3 (mesh-пикер, Project), E2 (текстуры). **Готово:** OBJ/PNG импортируются; bridge-тесты зелёные.

## Недели 5–6 (M3) — FBX/glTF, запуск сцены плеером
- `[H+S] asset/fbx_importer.{hpp,cpp}`, `gltf_importer.{hpp,cpp}` (+ `mini_json.hpp`).
- `[B] CMakeLists.txt` (с E4): `sky_managed` target, прокидка `SKY_MANAGED_DIR`.
- `[S] player/src/main.cpp` (с E4): аргумент `--scene`.
- `[B] ci.yml` (дополнить): player smoke `--headless --frames 60` + артефакт.
**Готово:** плеер запускает `.skybox`; smoke в CI зелёный.

## Недели 7–8 (M4) — менеджер пакетов
- `[H+S] package/package_system.hpp` + `package_world.{hpp,cpp}` — манифесты, дискавери, активация, extension points, цикл-детект.
- `[H] package/semver.hpp` + `[H+S] package_lock.{hpp}` — `parseVersion/satisfies`, MVS-резолв, `sky.lock` (версии+чексуммы+active).
- `[H+S] package/package_installer.{hpp,cpp}` — установка dir/tarball/git через immutable-кэш.
- Managed-код в пакетах (с E4): `Runtime/*.cs` активных пакетов в пользовательскую сборку.
- `[T] tests/package_tests.cpp` — semver, MVS, конфликты, lock round-trip, installer.
- Задел: `.skypak` (cooking), demo-game wiring.
**Готово:** пакет ставится из трёх источников, версии резолвятся, код пакета работает в Play.

## Твои личные ворота
- M1: зелёный CI + скриншот-артефакт.
- M2: dotnet-сборка редактора в CI.
- M3: плеер запускает сцену; FBX/glTF импорт.
- M4: менеджер пакетов (semver+installer+код).
