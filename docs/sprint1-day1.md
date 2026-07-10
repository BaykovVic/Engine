# Спринт 1. День 1

## Общие требования ко всем фичам

1. **Один PR — одна фича.** Не смешивать фичи и не менять файлы чужих контуров (например, `engine/core/include/sky/core/math.hpp` принадлежит E1).
2. **Никаких артефактов сборки в git**: `.exe`, `.o`, `.obj`, `.spv`, каталоги `build/` — запрещены. Временные файлы (`tests/tmp/`) в PR не включать.
3. **Namespace модуля обязателен** (`sky::core`, `sky::rendering`, `sky::object`, `sky::physics`, `sky::ecs`, ...). Код в глобальном namespace не принимается.
4. **Интерфейсы**: секция `public:`, виртуальный деструктор `virtual ~IИмя() = default;`, чисто виртуальные методы (`= 0`).
5. **Сигнатуры из задания копируются символ в символ** — включая `const`, `[[nodiscard]]`, типы возврата и параметры по умолчанию.
6. **Include-стиль**: `#include "sky/<модуль>/<файл>.hpp"` при `-Iengine/<модуль>/include`; пути от корня репозитория запрещены. `<bits/stdc++.h>` запрещён, `#pragma once` обязателен в каждом заголовке.
7. **Хэндлы** — только `core::Handle<Tag>`; в контейнерах ключ — `handle.value`. Собственные `std::hash<Handle>` и операторы в чужие заголовки не добавлять.
8. **Критерий приёмки каждой фичи — её приёмочный тест** (указан в конце блока фичи). PR без зелёного теста не рассматривается.

## feature/math-and-handles

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** нет

**Цель фичи:** математика и типобезопасные идентификаторы — фундамент, от которого зависят все.

**Описание фичи:** `Vec3/Quat/Transform` и `Handle` используют все модули; заголовок `math.hpp` отдаётся первым коммитом — по нему стартуют E2 и E4.

**Обязательные требования:**

Все объявления фичи — в `namespace sky::core`; файлы — `engine/core/include/sky/core/math.hpp` и `engine/core/include/sky/core/handle.hpp` (включения — `#include "sky/core/math.hpp"`, см. «Общие требования»).

```cpp
// math.hpp — типы (инициализаторы по умолчанию обязательны):
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    auto operator<=>(const Vec2&) const = default;
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    auto operator<=>(const Vec3&) const = default;
};

struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    auto operator<=>(const Quat&) const = default;
};

struct Transform {
    Vec3 position{};
    Quat rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};

    auto operator<=>(const Transform&) const = default;
};

// math.hpp — свободные constexpr-функции (сигнатуры символ в символ;
// operator*(Vec3,Vec3) и divide — покомпонентные):
constexpr Vec3 operator+(const Vec3& a, const Vec3& b);
constexpr Vec3 operator-(const Vec3& a, const Vec3& b);
constexpr Vec3 operator*(const Vec3& a, float s);
constexpr Vec3 operator*(const Vec3& a, const Vec3& b);
constexpr Quat operator*(const Quat& a, const Quat& b);
constexpr Vec3 rotate(const Quat& q, const Vec3& v);
constexpr Transform compose(const Transform& parent, const Transform& child);
constexpr Quat conjugate(const Quat& q);
constexpr Vec3 divide(const Vec3& a, const Vec3& b);
constexpr Transform invCompose(const Transform& parent, const Transform& world);

// handle.hpp — целиком (нужен <cstdint>):
template <typename Tag>
struct Handle {
    std::uint64_t value = 0;

    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
    auto operator<=>(const Handle&) const = default;

    static constexpr Handle invalid() noexcept { return Handle{0}; }
};
```

- Члены инициализируются по умолчанию: `Vec3 = {0,0,0}`, `Quat = {0,0,0,1}` (единичный), `Transform.scale = {1,1,1}` — без масштаба 1 по умолчанию `compose` «молча» ломается.
- Каждому типу — `auto operator<=>(const T&) const = default;`: критерий `invCompose(parent, compose(parent, child)) == child` требует сравнения.
- `operator-(Vec3,Vec3)`, покомпонентный `operator*(Vec3,Vec3)` и `divide(Vec3,Vec3)` обязательны — без них `compose`/`invCompose` не реализуются; `Vec2` нужен потребителям (например, uv-тайлинг рендера).
- `Handle`: `isValid()` — это `value != 0`; `invalid()` — `static constexpr`, возвращает `Handle{0}`.
- Ничего сверх контракта не добавлять: ни `std::hash<Handle>` (потребители ключуют контейнеры по `handle.value`), ни `operator/` для `Vec3` (деление — только `divide()`).

**Общий порядок реализации фичи:**
1. Объявить структуры `Vec3/Quat/Transform` в `math.hpp`.
2. Реализовать `constexpr`-операторы и `rotate/compose/conjugate/invCompose`.
3. Объявить шаблон `Handle<Tag>` в `handle.hpp`.

**Файлы фичи:**
1. `engine/core/include/sky/core/math.hpp`
2. `engine/core/include/sky/core/handle.hpp`

### Файл: `engine/core/include/sky/core/math.hpp`

**Назначение файла:** базовые математические типы и свободные функции над ними.

**Пошаговое описание действий:**
1. Объявить структуры `Vec3{x,y,z}`, `Quat{x,y,z,w}`, `Transform{position,rotation,scale}`.
2. Реализовать операторы над векторами и кватернионами.
3. Реализовать `rotate`, `compose`, `conjugate`, `invCompose`.
4. Пометить всё `constexpr`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `Vec3{x,y,z}`
- `Quat{x,y,z,w}`
- `Transform{position, rotation, scale}`

*Функции / методы:*
- `constexpr Vec3 operator+(const Vec3& a, const Vec3& b)`
- `constexpr Vec3 operator*(const Vec3& a, float s)`
- `constexpr Quat operator*(const Quat& a, const Quat& b)`
- `constexpr Vec3 rotate(const Quat& q, const Vec3& v)`
- `constexpr Transform compose(const Transform& parent, const Transform& child)`
- `constexpr Quat conjugate(const Quat& q)`
- `constexpr Transform invCompose(const Transform& parent, const Transform& world)`

*Логика функций / методов:*
- `operator+(Vec3,Vec3)` — покомпонентное сложение векторов. Параметры: `a`, `b`. Возвращает: `{a.x+b.x, …}`.
- `operator*(Vec3,float)` — масштабирование вектора числом. Параметры: `a`, `s`. Возвращает: `{a.x*s, …}`.
- `operator*(Quat,Quat)` — композиция двух поворотов (сначала `b`, потом `a`). Параметры: `a`, `b`. Возвращает: результирующий поворот.
- `rotate(q,v)` — поворачивает вектор кватернионом. Параметры: `q`, `v`. Возвращает: повёрнутый вектор.
- `compose(parent,child)` — переводит локальный трансформ ребёнка в систему координат родителя. Параметры: `parent`, `child`. Возвращает: трансформ ребёнка в системе родителя.
- `conjugate(q)` — сопряжённый кватернион (обратный поворот для единичного). Параметры: `q`. Возвращает: `{-q.x,-q.y,-q.z,q.w}`.
- `invCompose(parent,world)` — обратная к `compose` (мировой трансформ в локальный относительно родителя). Параметры: `parent`, `world`. Возвращает: локальный трансформ.

**Результат по файлу:** заголовок с математикой, готовый к использованию E2 и E4.

**Критерий правильности по файлу:**
1. `rotate(поворот 90° вокруг Y, {0,0,1}) ≈ {1,0,0}` (±1e-5).
2. `invCompose(parent, compose(parent, child)) == child`.

### Файл: `engine/core/include/sky/core/handle.hpp`

**Назначение файла:** типобезопасный идентификатор для пересечения границ модулей.

**Пошаговое описание действий:**
1. Объявить шаблон `Handle<Tag>` с полем `value`.
2. Добавить `isValid()`, `invalid()`, `operator==`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `template <typename Tag> struct Handle { std::uint64_t value; … }`

*Функции / методы:*
- `bool isValid() const`
- `static Handle invalid()`
- `operator==`

*Логика функций / методов:*
- `Handle` — типобезопасный идентификатор; разные теги (`ObjectTag`, `ComponentTag`) дают несовместимые типы. `isValid()` — валиден ли; `invalid()` — недействительный хендл; `operator==` — сравнение.

**Результат по файлу:** заголовок с `Handle<Tag>`.

**Критерий правильности по файлу:**
1. `Handle<A>` и `Handle<B>` — несовместимые типы (присваивание не компилируется).

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/core/include/sky/core/math.hpp`
2. `engine/core/include/sky/core/handle.hpp`

**Общий критерий правильности:**
1. `rotate(поворот 90° вокруг Y, {0,0,1}) ≈ {1,0,0}` (±1e-5).
2. `invCompose(parent, compose(parent, child)) == child`.
- **Приёмочный тест:** `tests/day1/math_and_handles_tests.cpp` — собрать/запустить по команде из шапки; результат: `N checks, 0 failures` (для чекеров — все пункты ✓, код 0).

---

## feature/render-contract

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `core::Vec3`/`core::Transform` из `feature/math-and-handles` (поля `RenderCommand`)

**Цель фичи:** общий (не только Vulkan) контракт рендера — поток команд и интерфейс рендерера.

**Описание фичи:** единый контракт, от которого зависят все бэкенды; остальные фичи опираются на интерфейсы, а не на реализацию.

**Обязательные требования:**

Все объявления фичи — в `namespace sky::rendering`; заголовки — `engine/rendering/include/sky/rendering/rendering.hpp`, `renderer_registry.hpp`, `null_renderer.hpp` (включения — `#include "sky/rendering/rendering.hpp"`, см. «Общие требования»); реализации — `engine/rendering/src/null_renderer.cpp`, `engine/rendering/src/renderer_registry.cpp`.

```cpp
// rendering.hpp — тег и хэндл ресурса (объявить ДО RenderCommand):
struct RenderResourceTag {};
using RenderResourceHandle = core::Handle<RenderResourceTag>;

// rendering.hpp — интерфейсы (сигнатуры символ в символ):
class IRenderSurface {
public:
    virtual ~IRenderSurface() = default;

    [[nodiscard]] virtual std::uint32_t width() const = 0;
    [[nodiscard]] virtual std::uint32_t height() const = 0;
    virtual void present() = 0;
};

class IRenderResourceFactory {
public:
    virtual ~IRenderResourceFactory() = default;

    virtual RenderResourceHandle createMeshFromData(
        std::span<const float> interleavedPosNormalUv) = 0;
    virtual RenderResourceHandle createTextureFromData(
        std::uint32_t width, std::uint32_t height,
        std::span<const std::uint8_t> rgbaPixels) = 0;
    virtual void destroy(RenderResourceHandle resource) = 0;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;

    [[nodiscard]] virtual std::string backendName() const = 0;
    virtual void attachSurface(IRenderSurface& surface) = 0;
    virtual void submit(std::span<const RenderCommand> commands) = 0;
    virtual void renderFrame() = 0;
};
```

```cpp
// renderer_registry.hpp:
struct BackendInit {
    std::function<void*(const char*)> resolveGlProc;
};

using RendererFactory =
    std::function<std::unique_ptr<IRenderer>(const BackendInit& init)>;

class IRendererRegistry {
public:
    virtual ~IRendererRegistry() = default;

    virtual bool registerBackend(const std::string& name, RendererFactory factory) = 0;
    [[nodiscard]] virtual std::vector<std::string> availableBackends() const = 0;
    [[nodiscard]] virtual bool hasBackend(const std::string& name) const = 0;
    virtual std::unique_ptr<IRenderer> create(const std::string& name,
                                              const BackendInit& init) = 0;
};

std::unique_ptr<IRendererRegistry> createRendererRegistry();

// null_renderer.hpp:
class NullRenderer : public IRenderer, public IRenderResourceFactory {
public:
    ~NullRenderer() override = default;

    [[nodiscard]] virtual std::uint64_t frameCount() const = 0;
    [[nodiscard]] virtual std::size_t commandsInLastFrame() const = 0;
    [[nodiscard]] virtual std::size_t liveResourceCount() const = 0;
};

std::unique_ptr<NullRenderer> createNullRenderer();

std::unique_ptr<IRenderSurface> createOffscreenSurface(std::uint32_t width,
                                                       std::uint32_t height);
```

- `RenderResourceHandle` — это `core::Handle<RenderResourceTag>`, а НЕ `std::uint64_t` и не собственная структура; объявляется ДО `RenderCommand`, чьи поля (`resource`, `texture`) его используют.
- `IRenderSurface` обязателен: ровно `width()`, `height()`, `present()`; `renderFrame()` null-рендерера вызывает `present()` у привязанной поверхности.
- Реестр из `createRendererRegistry()` уже содержит предзарегистрированный бэкенд `"null"`; `availableBackends()` возвращает имена отсортированными по алфавиту.
- `registerBackend` возвращает `false` и ничего не меняет для пустого имени, пустой фабрики (`nullptr`) и дубликата имени.
- `create(name, init)` для неизвестного имени возвращает `nullptr`, а не бросает исключение.
- Null-рендерер проверяет вход фабрик: меш — непустой и кратный целым треугольникам (24 float), текстура — `rgbaPixels.size() == width*height*4`; при нарушении возвращается невалидный хэндл, ресурс не заводится.

**Общий порядок реализации фичи:**
1. Объявить поток команд и интерфейсы в `rendering.hpp`.
2. Объявить реестр бэкендов в `renderer_registry.hpp`.
3. Реализовать пустой рендерер и реестр (`.cpp`).

**Файлы фичи:**
1. `engine/rendering/include/sky/rendering/rendering.hpp`
2. `engine/rendering/include/sky/rendering/renderer_registry.hpp`
3. `engine/rendering/include/sky/rendering/null_renderer.hpp`
4. `engine/rendering/src/null_renderer.cpp`
5. `engine/rendering/src/renderer_registry.cpp`

### Файл: `engine/rendering/include/sky/rendering/rendering.hpp`

**Назначение файла:** поток команд отрисовки и контракт рендерера.

**Пошаговое описание действий:**
1. Объявить перечисления и структуру `RenderCommand`.
2. Объявить `IRenderer` и `IRenderResourceFactory`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `enum class RenderCommandType { BeginFrame, SetViewport, SetCamera, AddLight, SetSky, BindPipeline, DrawMesh, EndFrame }`
- `enum class LightType { Directional = 0, Point = 1 }`
- `struct RenderCommand { RenderCommandType type; core::Transform transform; core::Vec3 color; float fovDegrees; float orthoHeight; … }`
- `class IRenderer`
- `class IRenderResourceFactory`

*Функции / методы:*
- `IRenderer`: `backendName() const`, `attachSurface(IRenderSurface&)`, `submit(std::span<const RenderCommand>)`, `renderFrame()`
- `IRenderResourceFactory`: `createMeshFromData(std::span<const float>)`, `createTextureFromData(std::uint32_t w, std::uint32_t h, std::span<const std::uint8_t> rgba)`, `destroy(RenderResourceHandle)`

*Логика функций / методов:*
- `RenderCommandType` — вид команды в потоке; `RenderCommand` — одна backend-независимая команда (поля читаются по-разному в зависимости от `type`).
- `backendName()` — имя бэкенда (`"vulkan"`/`"opengl"`); `attachSurface` — привязывает поверхность вывода; `submit` — принимает поток команд кадра; `renderFrame` — рисует накопленный кадр.
- `createMeshFromData` — загружает меш (позиция+нормаль+uv), возвращает хэндл; `createTextureFromData` — загружает текстуру, возвращает хэндл; `destroy` — освобождает ресурс.

**Результат по файлу:** контракт рендера зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется; поля `RenderCommand` используют `core::Vec3`/`Transform`.

### Файл: `engine/rendering/include/sky/rendering/renderer_registry.hpp`

**Назначение файла:** реестр бэкендов рендера.

**Пошаговое описание действий:**
1. Объявить `BackendInit` и `RendererFactory`.
2. Объявить `IRendererRegistry` и `createRendererRegistry()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct BackendInit { … }`
- `class IRendererRegistry`

*Функции / методы:*
- `using RendererFactory` — функция-фабрика рендерера
- `IRendererRegistry`: `registerBackend(const std::string& name, RendererFactory)`, `create(const std::string& name, const BackendInit&)`
- `std::unique_ptr<IRendererRegistry> createRendererRegistry()`

*Логика функций / методов:*
- `registerBackend` — регистрирует фабрику бэкенда по имени; возвращает успех.
- `create` — возвращает рендерер по имени бэкенда.
- `createRendererRegistry` — фабрика реестра (с предрегистрированным `"null"`).

**Результат по файлу:** реестр, позволяющий подключать бэкенды по имени.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/rendering/include/sky/rendering/null_renderer.hpp`

**Назначение файла:** пустой рендерер для тестов контракта.

**Пошаговое описание действий:**
1. Объявить `NullRenderer` и фабрики `createNullRenderer`, `createOffscreenSurface`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class NullRenderer` (реализует `IRenderer`/`IRenderResourceFactory`)

*Функции / методы:*
- `frameCount()`, `commandsInLastFrame()`, `liveResourceCount()`
- `createNullRenderer()`, `createOffscreenSurface(w, h)`

*Логика функций / методов:*
- пустой рендерер считает кадры/команды, ничего не рисует.

**Результат по файлу:** контракт можно проверять без GPU.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/rendering/src/null_renderer.cpp`

**Назначение файла:** реализация пустого рендерера.

**Пошаговое описание действий:**
1. Реализовать скрытый класс рендерера.
2. Реализовать фабрики.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `NullRenderer`, скрытый класс поверхности.

*Функции / методы:*
- `submit`, `renderFrame`, `createMeshFromData`, `createTextureFromData`, `destroy`, `frameCount`, `commandsInLastFrame`, `liveResourceCount`, `createNullRenderer`, `createOffscreenSurface`.

*Логика функций / методов:*
- `submit(commands)` — дописать команды во внутренний буфер.
- `renderFrame()` — запомнить число команд (`commandsInLastFrame`), очистить буфер, увеличить `frameCount`, вызвать `present()` у поверхности.
- `createMeshFromData`/`createTextureFromData` — проверить вход и завести хэндл ресурса; `destroy` — убрать ресурс.
- `frameCount`/`commandsInLastFrame`/`liveResourceCount` — чтение счётчиков.

**Результат по файлу:** рабочий null-рендерер.

**Критерий правильности по файлу:**
1. `submit` + `renderFrame` увеличивают счётчики кадров/команд.

### Файл: `engine/rendering/src/renderer_registry.cpp`

**Назначение файла:** реализация реестра бэкендов.

**Пошаговое описание действий:**
1. Реализовать хранение фабрик по имени.
2. Предрегистрировать `"null"` и реализовать `create`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация реестра.

*Функции / методы:*
- `registerBackend`, `create`, `createRendererRegistry`.

*Логика функций / методов:*
- конструктор предрегистрирует бэкенд `"null"`.
- `registerBackend(name, factory)` — сохранить фабрику; отклонить пустое имя/фабрику/дубликат.
- `create(name, init)` — найти фабрику по имени и создать рендерер; для неизвестного имени вернуть `nullptr`.

**Результат по файлу:** рабочий реестр.

**Критерий правильности по файлу:**
1. null-рендерер создаётся по имени `"null"`.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/rendering/include/sky/rendering/rendering.hpp`
2. `engine/rendering/include/sky/rendering/renderer_registry.hpp`
3. `engine/rendering/include/sky/rendering/null_renderer.hpp`
4. `engine/rendering/src/null_renderer.cpp`
5. `engine/rendering/src/renderer_registry.cpp`

**Общий критерий правильности:**
1. null-рендерер регистрируется в реестре и создаётся по имени.
2. `submit` + `renderFrame` увеличивают счётчики кадров/команд.
- **Приёмочный тест:** `tests/day1/render_contract_tests.cpp` — собрать/запустить по команде из шапки; результат: `N checks, 0 failures` (для чекеров — все пункты ✓, код 0).

---

## feature/editor-shell

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** нет (C-интерфейс подключается со второй фичи контура)

**Цель фичи:** каркас приложения редактора на Avalonia (.NET 8) с окном «Sky Engine».

**Описание фичи:** стартовый скелет — точка входа, приложение Avalonia и главное окно с меню.

**Обязательные требования:**

Код — в namespace `SkyEditor`; проект — `editor/avalonia/SkyEditor.csproj` (.NET 8, `<TargetFramework>net8.0</TargetFramework>`).

Пакеты — ровно этот список и эти версии (как в `SkyEditor.csproj` этого репозитория):

```xml
    <PackageReference Include="Avalonia" Version="11.2.1" />
    <PackageReference Include="Avalonia.Desktop" Version="11.2.1" />
    <PackageReference Include="Avalonia.Themes.Fluent" Version="11.2.1" />
    <PackageReference Include="Avalonia.Fonts.Inter" Version="11.2.1" />
    <PackageReference Include="Avalonia.Headless" Version="11.2.1" />
    <PackageReference Include="Dock.Avalonia" Version="11.2.0" />
    <PackageReference Include="Dock.Model.Mvvm" Version="11.2.0" />
```

- Все пакеты `Avalonia.*` — строго одной версии (`11.2.1`); смешение версий ломает XAML-компиляцию и рантайм-биндинги.
- `<OutputType>Exe</OutputType>`, НЕ `WinExe` — сборка должна давать консольный процесс с кодом возврата на Linux/CI.
- Корень меню в `MainWindow.axaml` — элемент `<Menu>` с вложенными `<MenuItem>` (НЕ `<MenuItem>` в корне).
- `async void`-методы, не содержащие ни одного `await`, запрещены; обработчики меню без асинхронной работы объявлять обычными `void`.
- Обязательные файлы: `App.axaml` + `App.axaml.cs`, `MainWindow.axaml` + `MainWindow.axaml.cs`, `Program.cs`, `app.manifest`.

**Общий порядок реализации фичи:**
1. Завести проект .NET 8 (`csproj`) с пакетами Avalonia.
2. Реализовать точку входа и приложение.
3. Собрать главное окно с меню.

**Файлы фичи:**
1. `editor/avalonia/SkyEditor.csproj`
2. `editor/avalonia/Program.cs`
3. `editor/avalonia/App.axaml.cs`
4. `editor/avalonia/MainWindow.axaml.cs`

### Файл: `editor/avalonia/SkyEditor.csproj`

**Назначение файла:** описание проекта редактора.

**Пошаговое описание действий:**
1. Объявить проект .NET 8 (исполняемый).
2. Подключить пакеты Avalonia.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* нет (описание проекта).

*Логика функций / методов:* проект .NET 8 с пакетами Avalonia.

**Результат по файлу:** проект собирается.

**Критерий правильности по файлу:**
1. `dotnet build` проекта — 0 ошибок.

### Файл: `editor/avalonia/Program.cs`

**Назначение файла:** точка входа приложения.

**Пошаговое описание действий:**
1. Реализовать `Main`.
2. Добавить ветку `--screenshot` (headless).

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `static int Main(string[] args)`

*Логика функций / методов:*
- `Main` — точка входа; ветка `--screenshot` (headless); возвращает код выхода.

**Результат по файлу:** приложение запускается.

**Критерий правильности по файлу:**
1. Приложение стартует; ветка `--screenshot` доступна.

### Файл: `editor/avalonia/App.axaml.cs`

**Назначение файла:** приложение Avalonia.

**Пошаговое описание действий:**
1. Реализовать `OnFrameworkInitializationCompleted()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class App`

*Функции / методы:*
- `OnFrameworkInitializationCompleted()`

*Логика функций / методов:*
- `OnFrameworkInitializationCompleted()` — открывает `MainWindow`.

**Результат по файлу:** приложение открывает главное окно.

**Критерий правильности по файлу:**
1. `MainWindow` открывается при старте.

### Файл: `editor/avalonia/MainWindow.axaml.cs`

**Назначение файла:** главное окно.

**Пошаговое описание действий:**
1. Собрать окно «Sky Engine».
2. Добавить меню File/Edit/GameObject и обработчики.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class MainWindow`

*Функции / методы:*
- обработчики пунктов меню File/Edit/GameObject.

*Логика функций / методов:*
- окно «Sky Engine» с меню и обработчиками пунктов меню.

**Результат по файлу:** окно с меню.

**Критерий правильности по файлу:**
1. Окно «Sky Engine» открывается с меню File/Edit/GameObject.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/SkyEditor.csproj`
2. `editor/avalonia/Program.cs`
3. `editor/avalonia/App.axaml.cs`
4. `editor/avalonia/MainWindow.axaml.cs`

**Общий критерий правильности:**
1. `dotnet build editor/avalonia` — 0 ошибок.
2. Окно «Sky Engine» открывается с меню File/Edit/GameObject.
- **Приёмочный тест:** `tests/day1/editor_shell_check.py` — собрать/запустить по команде из шапки; результат: `N checks, 0 failures` (для чекеров — все пункты ✓, код 0).

---

## feature/physics-world

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** `core::Vec3`/`core::Transform` из `feature/math-and-handles`

**Цель фичи:** физический мир — тела, коллайдеры, гравитация, столкновения, высотная поверхность, луч.

**Описание фичи:** ядро симуляции; контракты управления и запросов плюс конкретная реализация.

**Обязательные требования:**

Все объявления фичи — в `namespace sky::physics`; заголовки — `engine/physics/include/sky/physics/physics.hpp` и `engine/physics/include/sky/physics/physics_world.hpp` (включения — `#include "sky/physics/physics.hpp"`, см. «Общие требования»); реализация — `engine/physics/src/physics_world.cpp`.

```cpp
// physics.hpp — теги, хэндлы и описания (сигнатуры символ в символ):
struct RigidBodyTag {};
struct ColliderTag {};
using RigidBodyHandle = core::Handle<RigidBodyTag>;
using ColliderHandle = core::Handle<ColliderTag>;

enum class BodyType {
    Static,
    Kinematic,
    Dynamic,
};

struct RigidBodyDesc {
    BodyType type = BodyType::Dynamic;
    float mass = 1.0f;
    core::Transform initialTransform;
};

enum class ColliderShape {
    Box,
    Sphere,
    Capsule,
    TerrainHeightfield,
};

struct HeightfieldDesc {
    std::uint32_t resolution = 0;
    core::Vec3 scale{1.0f, 1.0f, 1.0f};
    std::vector<float> heights;
};

struct ColliderDesc {
    ColliderShape shape = ColliderShape::Box;
    core::Vec3 halfExtents{0.5f, 0.5f, 0.5f};
    float radius = 0.5f;
    HeightfieldDesc heightfield;
};
```

```cpp
// physics.hpp — события, попадания и контракты:
struct RaycastHit {
    ColliderHandle collider;
    core::Vec3 point;
    core::Vec3 normal;
    float distance = 0.0f;
};

struct CollisionEvent {
    ColliderHandle first;
    ColliderHandle second;
};

class IPhysicsWorld {
public:
    virtual ~IPhysicsWorld() = default;

    virtual RigidBodyHandle createBody(const RigidBodyDesc& desc) = 0;
    virtual void destroyBody(RigidBodyHandle body) = 0;
    virtual ColliderHandle attachCollider(RigidBodyHandle body, const ColliderDesc& desc) = 0;
    virtual void detachCollider(ColliderHandle collider) = 0;

    virtual void step(double fixedDeltaSeconds) = 0;
    [[nodiscard]] virtual std::vector<CollisionEvent> drainCollisionEvents() = 0;
};

class IPhysicsQueryService {
public:
    virtual ~IPhysicsQueryService() = default;

    [[nodiscard]] virtual std::optional<RaycastHit> raycast(const core::Vec3& origin,
                                                            const core::Vec3& direction,
                                                            float maxDistance) const = 0;
    [[nodiscard]] virtual core::Transform bodyTransform(RigidBodyHandle body) const = 0;
};
```

```cpp
// physics_world.hpp — конкретный мир и фабрика:
class PhysicsWorld : public IPhysicsWorld, public IPhysicsQueryService {
public:
    ~PhysicsWorld() override = default;

    virtual void setGravity(const core::Vec3& gravity) = 0;
    virtual void setBodyVelocity(RigidBodyHandle body, const core::Vec3& velocity) = 0;
    [[nodiscard]] virtual core::Vec3 bodyVelocity(RigidBodyHandle body) const = 0;
    virtual void setBodyTransform(RigidBodyHandle body, const core::Transform& transform) = 0;
};

std::unique_ptr<PhysicsWorld> createPhysicsWorld();
```

- `CollisionEvent` хранит два `ColliderHandle` (НЕ `RigidBodyHandle` и не сырые числа) — событие описывает пару коллайдеров.
- Id тел и коллайдеров выдаёт ЕДИНЫЙ монотонный счётчик (`std::uint64_t nextId_ = 1;`): первый выданный id — 1, значение `0` зарезервировано под `Handle::invalid()`.
- Пересечение AABB — строгое (`min < other.max && max > other.min` по каждой оси): боксы, соприкасающиеся вплотную, события не дают.
- Внутренние хранилища ключуются по `handle.value` (`std::unordered_map<std::uint64_t, BodyRecord>` и т.п.), а не по самому `Handle` — `std::hash<Handle>` в контракте нет и добавлять его нельзя.
- Пары коллайдеров ОДНОГО тела события не порождают; `destroyBody` каскадно удаляет коллайдеры тела; `drainCollisionEvents()` возвращает накопленные события и очищает буфер.

**Общий порядок реализации фичи:**
1. Объявить типы и контракты в `physics.hpp`.
2. Объявить `PhysicsWorld` и фабрику в `physics_world.hpp`.
3. Реализовать солвер в `physics_world.cpp`.

**Файлы фичи:**
1. `engine/physics/include/sky/physics/physics.hpp`
2. `engine/physics/include/sky/physics/physics_world.hpp`
3. `engine/physics/src/physics_world.cpp`

### Файл: `engine/physics/include/sky/physics/physics.hpp`

**Назначение файла:** типы и контракты физики.

**Пошаговое описание действий:**
1. Объявить типы описаний и событий.
2. Объявить `IPhysicsWorld` и `IPhysicsQueryService`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `RigidBodyDesc{type,mass,transform}`
- `enum class ColliderShape{Box,Sphere,Capsule,TerrainHeightfield}`
- `ColliderDesc{shape,halfExtents,radius,heightfield}`
- `HeightfieldDesc{resolution,scale,heights}`
- `RaycastHit{collider,point,normal,distance}`
- `CollisionEvent{first,second}`
- `class IPhysicsWorld`
- `class IPhysicsQueryService`

*Функции / методы:*
- `IPhysicsWorld`: `createBody(const RigidBodyDesc&)`, `destroyBody(RigidBodyHandle)`, `attachCollider(RigidBodyHandle, const ColliderDesc&)`, `step(double fixedDeltaSeconds)`, `drainCollisionEvents()`
- `IPhysicsQueryService`: `raycast(const core::Vec3& origin, const core::Vec3& direction, float maxDistance) const`, `bodyTransform(RigidBodyHandle) const`

*Логика функций / методов:*
- `createBody` — создаёт тело (тип/масса/поза), возвращает хэндл; `destroyBody` — удаляет тело.
- `attachCollider` — навешивает коллайдер, возвращает хэндл; `step` — шаг симуляции; `drainCollisionEvents` — события столкновений за кадр.
- `raycast` — пускает луч (`origin`, `direction`, `maxDistance`), возвращает попадание или `nullopt`; `bodyTransform` — трансформ тела.

**Результат по файлу:** контракт физики зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется; типы держат `core::Vec3`/`Transform`.

### Файл: `engine/physics/include/sky/physics/physics_world.hpp`

**Назначение файла:** конкретный мир физики.

**Пошаговое описание действий:**
1. Объявить `PhysicsWorld` с расширенными методами.
2. Объявить фабрику `createPhysicsWorld()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class PhysicsWorld : IPhysicsWorld, IPhysicsQueryService`

*Функции / методы:*
- `setGravity(const core::Vec3&)`, `setBodyVelocity(RigidBodyHandle, const core::Vec3&)`, `bodyVelocity(RigidBodyHandle) const`, `setBodyTransform(RigidBodyHandle, const core::Transform&)`
- `std::unique_ptr<PhysicsWorld> createPhysicsWorld()`

*Логика функций / методов:*
- `setGravity` — задаёт гравитацию; `setBodyVelocity` — задаёт скорость; `bodyVelocity` — возвращает скорость; `setBodyTransform` — переставляет тело.
- `createPhysicsWorld` — фабрика солвера.

**Результат по файлу:** интерфейс солвера с фабрикой.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/physics/src/physics_world.cpp`

**Назначение файла:** реализация солвера.

**Пошаговое описание действий:**
1. Реализовать хранилища тел/коллайдеров.
2. Реализовать `step` (гравитация + разбор столкновений).
3. Реализовать `raycast`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `PhysicsWorld`; внутренние `BodyRecord`, `ColliderRecord`, `Aabb`.

*Функции / методы:*
- `createBody`, `destroyBody`, `attachCollider`, `detachCollider`, `step`, `drainCollisionEvents`, `raycast`, `bodyTransform`, `setGravity`, `setBodyVelocity`, `bodyVelocity`, `setBodyTransform`; приватные `detectAndResolve`, `resolveHeightfields`, `sampleHeightfield`; `createPhysicsWorld`.

*Логика функций / методов:*
- `createBody`/`destroyBody`/`attachCollider`/`detachCollider` — ведут хранилища (id с 1); `destroyBody` каскадно удаляет коллайдеры тела.
- `step(dt)` — для каждого Dynamic-тела: к скорости прибавить `gravity·dt`, затем к позиции `velocity·dt` (явный Эйлер); Static/Kinematic не двигать; затем `resolveHeightfields()` и `detectAndResolve()`.
- `detectAndResolve()` — AABB каждого коллайдера (позиция ± `halfExtents`); для пар РАЗНЫХ тел с пересечением записать `CollisionEvent` и вытолкнуть Dynamic из статического по оси наименьшего проникновения, обнулив скорость вдоль неё.
- `resolveHeightfields()` — держать Dynamic-тела над террейном: если низ тела ниже высоты heightfield — поднять и обнулить отрицательную вертикальную скорость, добавить событие.
- `sampleHeightfield(field, x, z)` — билинейная выборка высоты.
- `raycast(origin, direction, maxDistance)` — луч vs AABB (слэбы) или vs heightfield (марш+бисекция); ближайшее попадание, нормаль — грань оси входа.

**Результат по файлу:** рабочий физический солвер.

**Критерий правильности по файлу:**
1. Тело падает под гравитацией; куб замирает на полу; луч попадает в коллайдер.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/physics/include/sky/physics/physics.hpp`
2. `engine/physics/include/sky/physics/physics_world.hpp`
3. `engine/physics/src/physics_world.cpp`

**Общий критерий правильности:**
1. Тело за 1 с падает ≈4.9 м под гравитацией.
2. Куб замирает на полу (расталкивание AABB).
3. Луч попадает в коллайдер.
- **Приёмочный тест:** `tests/day1/physics_world_tests.cpp` — собрать/запустить по команде из шапки; результат: `N checks, 0 failures` (для чекеров — все пункты ✓, код 0).

---

## feature/build-system

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** нет

**Цель фичи:** скелет сборки CMake для движка, редактора, плеера и тестов.

**Описание фичи:** единый механизм сборки модулей на C++20 с одной функцией регистрации модуля; остальные контуры подключают свои модули по мере готовности.

**Обязательные требования:**

Файлы фичи — `CMakeLists.txt` (корень), `engine/CMakeLists.txt`, `tests/CMakeLists.txt`; CMake ≥ 3.20, стандарт C++20 (без расширений).

```cmake
# Корневой CMakeLists.txt — проект, стандарт и опции (умолчания именно такие):
cmake_minimum_required(VERSION 3.20)

project(SkyEngine
    VERSION 0.1.0
    DESCRIPTION "Sky Engine - native-first game engine with C++20 core"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(SKY_BUILD_EDITOR "Build the Qt5-based editor layer" OFF)
option(SKY_BUILD_OPENGL_BACKEND "Build the OpenGL rendering backend" ON)
option(SKY_BUILD_VULKAN_BACKEND "Build the Vulkan rendering backend" ON)
option(SKY_BUILD_TESTS "Build verification targets" ON)
```

```cmake
# engine/CMakeLists.txt — функция регистрации модуля (символ в символ):
function(sky_add_module NAME DIR)
    set(SOURCES ${ARGN})
    if(SOURCES)
        add_library(${NAME} STATIC ${SOURCES})
        target_include_directories(${NAME} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/include)
        target_compile_features(${NAME} PUBLIC cxx_std_20)
        set(SCOPE PUBLIC)
    else()
        add_library(${NAME} INTERFACE)
        target_include_directories(${NAME} INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/include)
        target_compile_features(${NAME} INTERFACE cxx_std_20)
    endif()
    add_library(sky::${NAME} ALIAS ${NAME})
endfunction()
```

- Опции сборки — ровно `SKY_BUILD_EDITOR` (OFF), `SKY_BUILD_OPENGL_BACKEND` (ON), `SKY_BUILD_VULKAN_BACKEND` (ON), `SKY_BUILD_TESTS` (ON); другие имена и другие умолчания не принимаются.
- Канонический список целей движка: `sky_platform`, `sky_core`, `sky_serialization`, `sky_object`, `sky_component`, `sky_ecs`, `sky_scene`, `sky_project`, `sky_asset`, `sky_physics`, `sky_package`, `sky_rendering`, `sky_scripting`, `sky_terrain`, `sky_mapgen`; за опциями — `sky_rendering_opengl`, `sky_rendering_vulkan`; зонтичная INTERFACE-цель `sky_engine` (alias `sky::engine`). Каждая цель получает alias `sky::<имя>`.
- Модуль без исходников остаётся INTERFACE-библиотекой через тот же вызов `sky_add_module(NAME DIR)` — контуры подключают реализацию по мере готовности, не меняя механизм.
- Корневой файл подключает `add_subdirectory(engine)`, `add_subdirectory(editor)`, `add_subdirectory(player)`; под `if(SKY_BUILD_TESTS)` — `enable_testing()` и `add_subdirectory(tests)`.
- Связи модулей задаются через `target_link_libraries(... PUBLIC ...)`; зависимый модуль видит заголовки зависимости по её public-include-пути.
- Пустой `CMakeLists.txt` (или файл из одних комментариев) = провал ревью: каждый из трёх файлов обязан содержать реальные цели/регистрации.

**Общий порядок реализации фичи:**
1. Завести функцию `sky_add_module` в `engine/CMakeLists.txt`.
2. Объявить корневой проект и подключить подкаталоги в `CMakeLists.txt`.
3. Завести регистрацию тестов в `tests/CMakeLists.txt`.

**Файлы фичи:**
1. `engine/CMakeLists.txt`
2. `CMakeLists.txt`
3. `tests/CMakeLists.txt`

### Файл: `engine/CMakeLists.txt`

**Назначение файла:** описывает движковые модули и связи между ними единообразно.

**Пошаговое описание действий:**
1. Объявить функцию `sky_add_module(NAME DIR sources…)`.
2. Внутри создать статическую библиотеку из `sources`.
3. Задать public-include-путь модуля и стандарт C++20.
4. Зарегистрировать модули и их зависимости через `target_link_libraries`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `sky_add_module(NAME DIR sources…)`

*Логика функций / методов:*
- `sky_add_module` — создаёт статическую библиотеку с public-include-путями и стандартом C++20; регистрирует модули и связи между ними.

**Результат по файлу:** подключение нового модуля — одна строка `sky_add_module`.

**Критерий правильности по файлу:**
1. `sky_add_module` создаёт собираемую библиотеку.
2. Модуль виден зависимым модулям по своему include-пути.

### Файл: `CMakeLists.txt`

**Назначение файла:** корневой конфиг проекта.

**Пошаговое описание действий:**
1. Объявить `project(...)` и стандарт C++20.
2. Объявить опции сборки.
3. Подключить `add_subdirectory` для движка/редактора/плеера/тестов.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* нет (декларативный CMake).

*Логика функций / методов:* проект, опции, `add_subdirectory` для движка/редактора/плеера/тестов.

**Результат по файлу:** проект конфигурируется и собирается.

**Критерий правильности по файлу:**
1. `cmake -S . -B build` проходит без ошибок конфигурации.

### Файл: `tests/CMakeLists.txt`

**Назначение файла:** регистрация тестовых целей в ctest.

**Пошаговое описание действий:**
1. Объявить сборку тестовых исполняемых файлов.
2. Слинковать их с движком.
3. Зарегистрировать через `add_test`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* нет (декларативный CMake).

*Логика функций / методов:* сборка тестов и их регистрация в ctest.

**Результат по файлу:** тесты запускаются через ctest.

**Критерий правильности по файлу:**
1. ctest видит зарегистрированные тесты.

### На выходе должно получиться

**Список артефактов фичи:**
1. `CMakeLists.txt`
2. `engine/CMakeLists.txt`
3. `tests/CMakeLists.txt`

**Общий критерий правильности:**
1. `cmake -S . -B build && cmake --build build` проходит хотя бы с одним модулем (напр. `sky_core`).
2. `sky_add_module` подключает следующий модуль одной строкой.
- **Приёмочный тест:** `tests/day1/build_system_check.py` — собрать/запустить по команде из шапки; результат: `N checks, 0 failures` (для чекеров — все пункты ✓, код 0).

---

## feature/ecs-core

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** `sky_core` (сборка модуля)

**Цель фичи:** ECS-ядро — сущности, типизированные хранилища компонентов, планировщик систем и запросы.

**Описание фичи:** сущности держат компоненты в хранилищах по типу, системы обрабатывают их пачками; на этом ядре строятся все этапы контура.

**Обязательные требования:**

Все объявления фичи — в `namespace sky::ecs`; заголовки — `engine/ecs/include/sky/ecs/ecs.hpp` и `engine/ecs/include/sky/ecs/ecs_world.hpp` (включения — `#include "sky/ecs/ecs.hpp"`, см. «Общие требования»); реализация — `engine/ecs/src/ecs_world.cpp`.

```cpp
// ecs.hpp — идентификатор сущности и контракты (сигнатуры символ в символ):
struct EntityId {
    std::uint64_t value = 0;

    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
    auto operator<=>(const EntityId&) const = default;
};

class IEcsComponentStore {
public:
    virtual ~IEcsComponentStore() = default;

    [[nodiscard]] virtual std::type_index componentType() const = 0;
    [[nodiscard]] virtual bool has(EntityId entity) const = 0;
    virtual void remove(EntityId entity) = 0;
    [[nodiscard]] virtual std::size_t count() const = 0;
};

class IEcsSystem {
public:
    virtual ~IEcsSystem() = default;

    [[nodiscard]] virtual std::string name() const = 0;
    virtual void update(double deltaSeconds) = 0;
};

class IEcsWorld {
public:
    virtual ~IEcsWorld() = default;

    virtual EntityId createEntity() = 0;
    virtual void destroyEntity(EntityId entity) = 0;
    [[nodiscard]] virtual bool isAlive(EntityId entity) const = 0;
    virtual IEcsComponentStore& store(std::type_index componentType) = 0;
};
```

```cpp
// ecs.hpp — планировщик и запросы:
class IEcsSystemScheduler {
public:
    virtual ~IEcsSystemScheduler() = default;

    virtual void registerSystem(IEcsSystem& system) = 0;
    virtual void unregisterSystem(IEcsSystem& system) = 0;
    virtual void tick(double deltaSeconds) = 0;
};

class IEcsQueryService {
public:
    virtual ~IEcsQueryService() = default;

    [[nodiscard]] virtual std::vector<EntityId> entitiesWith(
        const std::vector<std::type_index>& componentTypes) const = 0;
};

// ecs_world.hpp — мир и типобезопасный доступ к хранилищам:
class EcsWorld : public IEcsWorld, public IEcsSystemScheduler, public IEcsQueryService {
public:
    ~EcsWorld() override = default;

    template <typename T>
    TypedComponentStore<T>& storeFor() {
        auto& slot = stores()[std::type_index(typeid(T))];
        if (!slot) {
            slot = std::make_unique<TypedComponentStore<T>>();
        }
        return static_cast<TypedComponentStore<T>&>(*slot);
    }

protected:
    using StoreMap =
        std::unordered_map<std::type_index, std::unique_ptr<IEcsComponentStore>>;

    virtual StoreMap& stores() = 0;
};

std::unique_ptr<EcsWorld> createEcsWorld();
```

- `EntityId` — ровно одно поле `std::uint64_t value` (НЕ пара `index`/`generation`); `isValid()` — это `value != 0`; id выдаются с 1, `EntityId{0}` — невалидный.
- `entitiesWith` принимает `const std::vector<std::type_index>&` (НЕ `std::set`) и возвращает `std::vector<EntityId>` — только живые сущности, имеющие ВСЕ указанные компоненты.
- `TypedComponentStore<T>` (в `ecs_world.hpp`) реализует `IEcsComponentStore`; обязательные методы — `T& set(EntityId entity, T value)` и `T* get(EntityId entity)` (`nullptr`, если компонента нет); внутренний контейнер ключуется по `entity.value`.
- `destroyEntity` удаляет компоненты сущности во всех зарегистрированных хранилищах; `store(type)` для незарегистрированного типа бросает исключение.
- `tick(dt)` вызывает `update(dt)` у систем в порядке регистрации; `registerSystem`/`unregisterSystem` хранят ссылки, владение системами остаётся у вызывающего.

**Общий порядок реализации фичи:**
1. Объявить `EntityId` и контракты (store/system/world/scheduler/query) в `ecs.hpp`.
2. Объявить `EcsWorld` и `storeFor<T>()` в `ecs_world.hpp`.
3. Реализовать `EcsWorld` и фабрику в `ecs_world.cpp`.

**Файлы фичи:**
1. `engine/ecs/include/sky/ecs/ecs.hpp`
2. `engine/ecs/include/sky/ecs/ecs_world.hpp`
3. `engine/ecs/src/ecs_world.cpp`

### Файл: `engine/ecs/include/sky/ecs/ecs.hpp`

**Назначение файла:** контракты ECS.

**Пошаговое описание действий:**
1. Объявить `EntityId` с полем `value`.
2. Объявить `IEcsComponentStore`, `IEcsSystem`, `IEcsWorld`, `IEcsSystemScheduler`, `IEcsQueryService`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct EntityId { std::uint64_t value; … }` (`isValid()`, `operator<=>`)
- `class IEcsComponentStore`
- `class IEcsSystem`
- `class IEcsWorld`
- `class IEcsSystemScheduler`
- `class IEcsQueryService`

*Функции / методы:*
- `IEcsComponentStore`: `componentType()`, `has(EntityId)`, `remove(EntityId)`, `count()`
- `IEcsSystem`: `name()`, `update(double deltaSeconds)`
- `IEcsWorld`: `createEntity()`, `destroyEntity(EntityId)`, `isAlive(EntityId)`, `store(std::type_index)`
- `IEcsSystemScheduler`: `registerSystem(IEcsSystem&)`, `unregisterSystem(IEcsSystem&)`, `tick(double)`
- `IEcsQueryService`: `entitiesWith(const std::vector<std::type_index>& componentTypes)`

*Логика функций / методов:*
- `EntityId` — идентификатор сущности: поле `value` (`std::uint64_t`), id выдаются с 1, `EntityId{0}` — невалидный.
- `componentType`/`has`/`remove`/`count` — доступ к хранилищу одного типа компонента.
- `name`/`update` — система обрабатывает подходящие сущности за кадр (`deltaSeconds` — шаг времени).
- `createEntity`/`destroyEntity`/`isAlive`/`store` — мир сущностей и доступ к хранилищу типа.
- `registerSystem`/`unregisterSystem`/`tick` — регистрация систем и прогон за такт.
- `entitiesWith(types)` — находит сущности с заданным набором компонентов; возвращает список сущностей.

**Результат по файлу:** контракты ECS зафиксированы.

**Критерий правильности по файлу:**
1. Заголовок компилируется (header_check).

### Файл: `engine/ecs/include/sky/ecs/ecs_world.hpp`

**Назначение файла:** реализация-контракт мира ECS с типобезопасным хранилищем.

**Пошаговое описание действий:**
1. Объявить `EcsWorld`, наследующий три контракта.
2. Объявить шаблон `storeFor<T>()` и фабрику `createEcsWorld()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService`

*Функции / методы:*
- `template <typename T> TypedComponentStore<T>& storeFor()`
- `std::unique_ptr<EcsWorld> createEcsWorld()`

*Логика функций / методов:*
- `storeFor<T>()` — типобезопасный доступ к хранилищу компонента `T` (методы `set(entity, value)`, `get(entity)→T*`); возвращает хранилище `T`.
- `createEcsWorld()` — фабрика мира.

**Результат по файлу:** доступ к типизированным хранилищам компонентов.

**Критерий правильности по файлу:**
1. `storeFor<T>().set/get` работают для произвольного типа `T`.

### Файл: `engine/ecs/src/ecs_world.cpp`

**Назначение файла:** реализация `EcsWorld`.

**Пошаговое описание действий:**
1. Реализовать учёт живых сущностей.
2. Реализовать планировщик систем и запросы.
3. Дать фабрику `createEcsWorld()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `EcsWorld`.

*Функции / методы:*
- `createEntity`, `destroyEntity`, `isAlive`, `store`, `registerSystem`, `unregisterSystem`, `tick`, `entitiesWith`, `createEcsWorld`.

*Логика функций / методов:*
- `createEntity()` — выдать id (с 1) и пометить сущность живой.
- `destroyEntity(entity)` — убрать из живых и удалить её компоненты во всех хранилищах.
- `isAlive(entity)` — проверить, жива ли сущность.
- `store(type)` — вернуть хранилище типа (бросить, если тип не зарегистрирован).
- `registerSystem`/`unregisterSystem` — вести список систем; `tick(dt)` — вызвать `update` у всех систем по порядку.
- `entitiesWith(types)` — вернуть живые сущности, у которых есть все указанные компоненты.

**Результат по файлу:** рабочий ECS-мир.

**Критерий правильности по файлу:**
1. Система обновляет только сущности с нужным компонентом.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/ecs/include/sky/ecs/ecs.hpp`
2. `engine/ecs/include/sky/ecs/ecs_world.hpp`
3. `engine/ecs/src/ecs_world.cpp`

**Общий критерий правильности:**
1. Создание/уничтожение сущности работает.
2. `storeFor<T>().set/get` работают.
3. `entitiesWith({type})` возвращает только сущности с этим компонентом.
- **Приёмочный тест:** `tests/day1/ecs_core_tests.cpp` — собрать/запустить по команде из шапки; результат: `N checks, 0 failures` (для чекеров — все пункты ✓, код 0).
