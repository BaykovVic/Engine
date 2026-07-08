# День 1 — старт: сборка, математика, каркас редактора

## feature/build-system

Цель фичи: система сборки CMake для движка, редактора, плеера и тестов.
Описание фичи (для чего): единый механизм сборки модулей на C++20; регистрация модулей и связей одной функцией. Контур E5. Нужна первой — без неё ничего не собирается.
Пошаговое описание действий:
Сделай файл engine/CMakeLists.txt
В файле engine/CMakeLists.txt должна быть функция sky_add_module(NAME DIR sources…)
В функции должна быть реализована логика: создание статической библиотеки с public-include-путями и стандартом C++20; регистрация модулей и связей между ними.
Сделай файл CMakeLists.txt (корневой)
В файле CMakeLists.txt должны быть объявление проекта, опции сборки и вызовы add_subdirectory для движка/редактора/плеера/тестов.
В файле должна быть реализована логика: конфигурация проекта на C++20 и подключение подкаталогов.
Сделай файл tests/CMakeLists.txt
В файле tests/CMakeLists.txt должна быть функция sky_add_test(NAME) и регистрация тестов.
В функции должна быть реализована логика: сборка теста, линковка с движком, регистрация через add_test.
На выходе должно получиться:
- CMakeLists.txt
- engine/CMakeLists.txt
- tests/CMakeLists.txt
КРИТЕРИЙ ПРАВИЛЬНОСТИ: cmake -S . -B build && cmake --build build проходит хотя бы с одним модулем; следующий модуль подключается одной строкой sky_add_module.

## feature/math-and-handles

Цель фичи: математические типы и типобезопасные идентификаторы — фундамент для всех.
Описание фичи (для чего): Vec3/Quat/Transform и Handle используют все модули; math.hpp отдаётся первым коммитом — по нему стартуют E2 и E4. Контур E1.
Пошаговое описание действий:
Сделай файл engine/core/include/sky/core/math.hpp
В файле engine/core/include/sky/core/math.hpp должны быть структуры Vec3, Quat, Transform и функции operator+, operator* (вектор·число), operator* (кватернион), rotate, compose, conjugate, invCompose.
В функциях должна быть реализована логика: сложение и масштабирование векторов; композиция поворотов; rotate — поворот вектора кватернионом; compose — локальный трансформ ребёнка в систему родителя; conjugate — сопряжённый кватернион; invCompose — обратная к compose.
Сделай файл engine/core/include/sky/core/handle.hpp
В файле engine/core/include/sky/core/handle.hpp должен быть шаблон Handle<Tag> с методами isValid(), invalid(), operator==.
В методах должна быть реализована логика: хранит value; разные теги (ObjectTag, ComponentTag) дают несовместимые типы.
На выходе должно получиться:
- engine/core/include/sky/core/math.hpp
- engine/core/include/sky/core/handle.hpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: rotate(90° вокруг Y, {0,0,1}) ≈ {1,0,0} (±1e-5); invCompose(parent, compose(parent, child)) == child.

## feature/editor-shell

Цель фичи: каркас приложения редактора на Avalonia (.NET 8) с окном «Sky Engine».
Описание фичи (для чего): стартовый скелет, без которого нет панелей и вызовов движка; независим и стартует параллельно. Контур E3.
Пошаговое описание действий:
Сделай файл editor/avalonia/SkyEditor.csproj
В файле editor/avalonia/SkyEditor.csproj должно быть описание проекта .NET 8 с пакетами Avalonia.
В файле должна быть реализована логика: тип «исполняемый», net8.0, пакеты Avalonia и Dock.
Сделай файл editor/avalonia/Program.cs
В файле editor/avalonia/Program.cs должен быть метод static int Main(string[] args).
В методе должна быть реализована логика: запуск десктопного жизненного цикла, ветка --screenshot (headless), возврат кода выхода.
Сделай файл editor/avalonia/App.axaml.cs
В файле editor/avalonia/App.axaml.cs должен быть класс App с методом OnFrameworkInitializationCompleted().
В методе должна быть реализована логика: создать и показать MainWindow.
Сделай файл editor/avalonia/MainWindow.axaml.cs
В файле editor/avalonia/MainWindow.axaml.cs должен быть класс MainWindow с меню File/Edit/GameObject.
В классе должна быть реализована логика: окно «Sky Engine», пункты меню и их обработчики.
На выходе должно получиться:
- editor/avalonia/SkyEditor.csproj
- editor/avalonia/Program.cs
- editor/avalonia/App.axaml.cs
- editor/avalonia/MainWindow.axaml.cs
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build = 0 ошибок; окно «Sky Engine» открывается с меню. C-интерфейс — позже, feature/engine-bridge.
