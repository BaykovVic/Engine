# День 1

## E5 · `feature/build-system`

### Файлы `CMakeLists.txt`, `engine/CMakeLists.txt`, `tests/CMakeLists.txt`
- `engine/CMakeLists.txt` — функция `sky_add_module(NAME DIR sources…)` создаёт
  статическую библиотеку с public-include-путями и стандартом C++20; регистрирует
  модули и связи между ними.
- корневой `CMakeLists.txt` — проект, опции, `add_subdirectory` для движка/редактора/плеера/тестов.

**Проверка фичи:** `cmake -S . -B build && cmake --build build` проходит с одним
модулем (напр. `sky_core`); `sky_add_module` подключает следующий модуль одной
строкой. Это скелет сборки, в который остальные контуры добавляют свои модули.

---

## E1 · `feature/math-and-handles`

Математика и типобезопасные идентификаторы — фундамент, от которого зависят все.

### Файл `engine/core/include/sky/core/math.hpp`
Свободные функции над векторами, кватернионами и трансформами (все
`constexpr`). Структуры `Vec3{x,y,z}`, `Quat{x,y,z,w}`, `Transform{position,
rotation, scale}` объявляются здесь же.
- `constexpr Vec3 operator+(const Vec3& a, const Vec3& b)`
  Что делает: покомпонентное сложение векторов.
  Параметры: `a`, `b` — слагаемые. Возвращает: сумму `{a.x+b.x, …}`.
- `constexpr Vec3 operator*(const Vec3& a, float s)`
  Что делает: масштабирование вектора числом.
  Параметры: `a` — вектор, `s` — множитель. Возвращает: `{a.x*s, …}`.
- `constexpr Quat operator*(const Quat& a, const Quat& b)`
  Что делает: композиция двух поворотов (сначала `b`, потом `a`).
  Параметры: `a`, `b` — кватернионы. Возвращает: результирующий поворот.
- `constexpr Vec3 rotate(const Quat& q, const Vec3& v)`
  Что делает: поворачивает вектор кватернионом.
  Параметры: `q` — поворот, `v` — исходный вектор. Возвращает: повёрнутый вектор.
- `constexpr Transform compose(const Transform& parent, const Transform& child)`
  Что делает: переводит локальный трансформ ребёнка в систему координат родителя (нужно для мирового трансформа).
  Параметры: `parent` — трансформ родителя, `child` — локальный трансформ ребёнка. Возвращает: трансформ ребёнка в системе родителя.
- `constexpr Quat conjugate(const Quat& q)`
  Что делает: сопряжённый кватернион (обратный поворот для единичного).
  Параметры: `q` — поворот. Возвращает: `{-q.x,-q.y,-q.z,q.w}`.
- `constexpr Transform invCompose(const Transform& parent, const Transform& world)`
  Что делает: обратная к `compose` — переводит мировой трансформ в локальный относительно родителя (нужно при смене родителя).
  Параметры: `parent` — трансформ родителя, `world` — мировой трансформ объекта. Возвращает: локальный трансформ относительно родителя.

### Файл `engine/core/include/sky/core/handle.hpp`
- `template <typename Tag> struct Handle { std::uint64_t value; … }`
  Что делает: типобезопасный идентификатор. Разные теги (`ObjectTag`,
  `ComponentTag`) дают несовместимые типы — нельзя перепутать хэндл объекта с
  хэндлом компонента. Содержит `isValid()`, статический `invalid()`, `operator==`.

**Проверка фичи:** `rotate(поворот 90° вокруг Y, {0,0,1})` ≈ `{1,0,0}` (±1e-5);
`invCompose(parent, compose(parent, child)) == child`.

---

## E3 · `feature/editor-shell`

### Файлы `editor/avalonia/SkyEditor.csproj`, `Program.cs`, `App.axaml.cs`, `MainWindow.axaml.cs`
- `csproj` — проект .NET 8 с пакетами Avalonia.
- `static int Main(string[] args)` — точка входа; ветка `--screenshot` (headless). Возвращает: код выхода.
- `App` — приложение Avalonia; `OnFrameworkInitializationCompleted()` открывает `MainWindow`.
- `MainWindow` — окно «Sky Engine», меню (File/Edit/GameObject), обработчики пунктов.

**Проверка фичи:** `dotnet build editor/avalonia` — 0 ошибок; окно «Sky Engine»
открывается с меню. C-интерфейс на этой фиче не требуется — он подключается со
второй фичи контура (`feature/engine-bridge`).
