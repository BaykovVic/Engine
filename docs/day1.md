# День 1 — старт: сборка, математика, каркас редактора

Фичи дня (в порядке реализации). Одна фича = ветка `feature/<название>` = один PR в `develop`.

---

## feature/build-system (E5)
**Цель фичи:** система сборки CMake для движка, редактора, плеера и тестов.
**Описание фичи (для чего):** единый механизм сборки модулей на C++20; регистрация модулей и связей одной функцией. Нужна первой — без неё ничего не собирается.
**Пошаговое описание действий:**
- Сделай файл `engine/CMakeLists.txt` — функция `sky_add_module(NAME DIR sources…)`.
- В функции должна быть реализована логика: статическая библиотека с public-include-путями и стандартом C++20; регистрация модулей и связей.
- Сделай файл `CMakeLists.txt` (корневой) — объявление проекта, опции, `add_subdirectory` для движка/редактора/плеера/тестов.
- Сделай файл `tests/CMakeLists.txt`.
**На выходе должно получиться:** `CMakeLists.txt`, `engine/CMakeLists.txt`, `tests/CMakeLists.txt`; проект собирается через CMake.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `cmake -S . -B build && cmake --build build` проходит хотя бы с одним модулем; следующий модуль подключается одной строкой `sky_add_module`.

## feature/math-and-handles (E1)
**Цель фичи:** математические типы и типобезопасные идентификаторы — фундамент для всех.
**Описание фичи (для чего):** `Vec3/Quat/Transform` и `Handle` используют все модули; заголовок `math.hpp` отдаётся первым коммитом — по нему стартуют E2 и E4.
**Пошаговое описание действий:**
- Сделай файл `engine/core/include/sky/core/math.hpp` (`namespace sky::core`).
- В файле должны быть структуры `Vec3`, `Quat`, `Transform` и функции `operator+`, `operator*`(вектор·число), `operator*`(кватернион), `rotate`, `compose`, `conjugate`, `invCompose` (все `constexpr`).
- В функциях должна быть реализована логика: сложение/масштаб векторов; композиция поворотов; `rotate` — поворот вектора кватернионом; `compose` — локальный трансформ ребёнка в систему родителя; `conjugate` — сопряжённый кватернион; `invCompose` — обратная к `compose`.
- Сделай файл `engine/core/include/sky/core/handle.hpp` (`namespace sky::core`).
- В файле должен быть шаблон `Handle<Tag>` с `isValid()`, `invalid()`, `operator==`.
- В методах должна быть реализована логика: хранит `value`; разные теги (`ObjectTag`, `ComponentTag`) дают несовместимые типы.
**На выходе должно получиться:** `math.hpp`, `handle.hpp` (header-only, `sky::core`).
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `rotate(90° вокруг Y, {0,0,1}) ≈ {1,0,0}` (±1e-5); `invCompose(parent, compose(parent,child)) == child`.

## feature/editor-shell (E3)
**Цель фичи:** каркас приложения редактора на Avalonia (.NET 8) с окном «Sky Engine».
**Описание фичи (для чего):** стартовый скелет, без которого нет панелей и вызовов движка; независим от нативного кода, стартует параллельно.
**Пошаговое описание действий:**
- Сделай файл `editor/avalonia/SkyEditor.csproj` — проект .NET 8 с пакетами Avalonia.
- Сделай файл `editor/avalonia/Program.cs` — метод `static int Main(string[] args)` с веткой `--screenshot` (headless), возвращает код выхода.
- Сделай файл `editor/avalonia/App.axaml.cs` — класс `App` с `OnFrameworkInitializationCompleted()` (открывает `MainWindow`).
- Сделай файл `editor/avalonia/MainWindow.axaml.cs` — окно «Sky Engine», меню File/Edit/GameObject и обработчики.
**На выходе должно получиться:** `SkyEditor.csproj`, `Program.cs`, `App.axaml.cs`, `MainWindow.axaml.cs`; окно с меню.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `dotnet build` = 0 ошибок; окно «Sky Engine» открывается с меню. *(C-интерфейс — позже, `feature/engine-bridge`.)*
