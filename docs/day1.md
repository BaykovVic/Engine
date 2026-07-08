# День 1

## feature/build-system
Цель фичи: система сборки — CMake-каркас движка, редактора, плеера и тестов (контур E5).
Описание фичи (для чего): без CI и сборки ошибки движка и редактора остаются незамеченными; каркас модулей нужен всем контурам.
Пошаговое описание действий:
Сделай файл engine/CMakeLists.txt
В файле engine/CMakeLists.txt должна быть функция sky_add_module(NAME DIR sources…)
В функции должна быть реализована логика: создаёт статическую библиотеку с public-include-путями и стандартом C++20; регистрирует модули и связи между ними.
Сделай файл CMakeLists.txt (корневой)
В файле CMakeLists.txt должны быть объявление проекта, опции и add_subdirectory для движка/редактора/плеера/тестов.
В файле должна быть реализована логика: конфигурация проекта и подключение подкаталогов.
Сделай файл tests/CMakeLists.txt
В файле tests/CMakeLists.txt должна быть регистрация тестовых целей.
В файле должна быть реализована логика: сборка тестов и их регистрация в ctest.
На выходе должно получиться:
- CMakeLists.txt
- engine/CMakeLists.txt
- tests/CMakeLists.txt
КРИТЕРИЙ ПРАВИЛЬНОСТИ: красный CI блокирует слияние; сломанный тест краснеет; редактор E3 вызывает sky_editor_create.

## feature/math-and-handles
Цель фичи: математика и типобезопасные идентификаторы — фундамент, от которого зависят все (контур E1).
Описание фичи (для чего): Vec3/Quat/Transform и Handle используют все модули; заголовок math.hpp отдаётся первым коммитом — по нему стартуют E2 и E4.
Пошаговое описание действий:
Сделай файл engine/core/include/sky/core/math.hpp
В файле engine/core/include/sky/core/math.hpp должны быть структуры Vec3{x,y,z}, Quat{x,y,z,w}, Transform{position,rotation,scale} и функции constexpr Vec3 operator+(const Vec3& a, const Vec3& b), constexpr Vec3 operator*(const Vec3& a, float s), constexpr Quat operator*(const Quat& a, const Quat& b), constexpr Vec3 rotate(const Quat& q, const Vec3& v), constexpr Transform compose(const Transform& parent, const Transform& child), constexpr Quat conjugate(const Quat& q), constexpr Transform invCompose(const Transform& parent, const Transform& world)
В функциях должна быть реализована логика: покомпонентное сложение векторов; масштабирование вектора числом; композиция двух поворотов (сначала b, потом a); rotate — поворачивает вектор кватернионом; compose — переводит локальный трансформ ребёнка в систему координат родителя; conjugate — сопряжённый кватернион ({-q.x,-q.y,-q.z,q.w}); invCompose — обратная к compose (мировой трансформ в локальный относительно родителя).
Сделай файл engine/core/include/sky/core/handle.hpp
В файле engine/core/include/sky/core/handle.hpp должен быть template <typename Tag> struct Handle { std::uint64_t value; … } с методами isValid(), invalid(), operator==
В методах должна быть реализована логика: типобезопасный идентификатор — разные теги (ObjectTag, ComponentTag) дают несовместимые типы; isValid(), статический invalid(), operator==.
На выходе должно получиться:
- engine/core/include/sky/core/math.hpp
- engine/core/include/sky/core/handle.hpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: rotate(поворот 90° вокруг Y, {0,0,1}) ≈ {1,0,0} (±1e-5); invCompose(parent, compose(parent, child)) == child.

## feature/editor-shell
Цель фичи: каркас приложения редактора на Avalonia (.NET 8) с окном «Sky Engine» (контур E3).
Описание фичи (для чего): проект редактора и первичное окно, без которого нет панелей и вызовов движка; связь с движком — позже.
Пошаговое описание действий:
Сделай файл editor/avalonia/SkyEditor.csproj
В файле editor/avalonia/SkyEditor.csproj должно быть описание проекта .NET 8 с пакетами Avalonia.
В файле должна быть реализована логика: конфигурация проекта .NET 8 и зависимости Avalonia.
Сделай файл editor/avalonia/Program.cs
В файле editor/avalonia/Program.cs должен быть static int Main(string[] args)
В методе должна быть реализована логика: точка входа; ветка --screenshot (headless); возвращает код выхода.
Сделай файл editor/avalonia/App.axaml.cs
В файле editor/avalonia/App.axaml.cs должен быть класс App с методом OnFrameworkInitializationCompleted()
В методе должна быть реализована логика: приложение Avalonia открывает MainWindow.
Сделай файл editor/avalonia/MainWindow.axaml.cs
В файле editor/avalonia/MainWindow.axaml.cs должен быть класс MainWindow с меню (File/Edit/GameObject) и обработчиками пунктов
В классе должна быть реализована логика: окно «Sky Engine» с меню и обработчиками пунктов меню.
На выходе должно получиться:
- editor/avalonia/SkyEditor.csproj
- editor/avalonia/Program.cs
- editor/avalonia/App.axaml.cs
- editor/avalonia/MainWindow.axaml.cs
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build = 0 ошибок; сессия движка создаётся из .NET (не-null); есть сброс компоновки.
