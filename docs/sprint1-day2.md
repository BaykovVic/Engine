# Спринт 1. День 2

## Общие требования ко всем фичам

1. **Один PR — одна фича.** Не смешивать фичи и не менять файлы чужих контуров (например, `engine/core/include/sky/core/math.hpp` принадлежит E1).
2. **Никаких артефактов сборки в git**: `.exe`, `.o`, `.obj`, `.spv`, каталоги `build/` — запрещены. Временные файлы (`tests/tmp/`) в PR не включать.
3. **Namespace модуля обязателен** (`sky::core`, `sky::rendering`, `sky::object`, `sky::physics`, `sky::ecs`, ...). Код в глобальном namespace не принимается.
4. **Интерфейсы**: секция `public:`, виртуальный деструктор `virtual ~IИмя() = default;`, чисто виртуальные методы (`= 0`).
5. **Сигнатуры из задания копируются символ в символ** — включая `const`, `[[nodiscard]]`, типы возврата и параметры по умолчанию.
6. **Include-стиль**: `#include "sky/<модуль>/<файл>.hpp"` при `-Iengine/<модуль>/include`; пути от корня репозитория запрещены. `<bits/stdc++.h>` запрещён, `#pragma once` обязателен в каждом заголовке.
7. **Хэндлы** — только `core::Handle<Tag>`; в контейнерах ключ — `handle.value`. Собственные `std::hash<Handle>` и операторы в чужие заголовки не добавлять.
8. **Критерий приёмки каждой фичи — её приёмочный тест** (указан в конце блока фичи). PR без зелёного теста не рассматривается.

## feature/core-services

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `sky_core` (сборка)

**Цель фичи:** единый журнал и служба конфигурации — используются всеми подсистемами.

**Описание фичи:** логирование и конфиг нужны всем модулям; выносятся за интерфейсы с фабриками.

**Обязательные требования:**

Все объявления — в пространстве имён `sky::core`; реализации в `.cpp` — скрытыми классами в анонимном `namespace` внутри `sky::core`.

```cpp
// engine/core/include/sky/core/logger.hpp
enum class LogLevel : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0;

    void info(std::string_view category, std::string_view message);    // вызывает log(LogLevel::Info, ...)
    void warning(std::string_view category, std::string_view message); // вызывает log(LogLevel::Warning, ...)
    void error(std::string_view category, std::string_view message);   // вызывает log(LogLevel::Error, ...)
};

// engine/core/include/sky/core/config_service.hpp
class IConfigService {
public:
    virtual ~IConfigService() = default;

    [[nodiscard]] virtual std::optional<std::string> getString(std::string_view key) const = 0;
    [[nodiscard]] virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0;
    [[nodiscard]] virtual std::optional<bool> getBool(std::string_view key) const = 0;

    virtual void set(std::string_view key, std::string value) = 0;
};

// engine/core/include/sky/core/runtime_services.hpp
std::unique_ptr<ILogger> createConsoleLogger(LogLevel minimumLevel = LogLevel::Info);
std::unique_ptr<IConfigService> createInMemoryConfigService();
```

- **Виртуальные деструкторы обязательны** у `ILogger` и `IConfigService`: оба отдаются из фабрик как `std::unique_ptr<интерфейс>`, и без `virtual ~ILogger() = default;` удаление реализации через базовый указатель — неопределённое поведение.
- **Фильтрация по `minimumLevel`**: сообщения с `level < minimumLevel` консольный логгер не выводит; поэтому порядок значений `LogLevel` значим.
- **Фабрики объявляются в `sky/core/runtime_services.hpp`**, а не в заголовках интерфейсов: интерфейсные заголовки не должны знать о реализациях.
- **Потокобезопасность консольного логгера**: вывод строки — под блокировкой (`std::scoped_lock` на `std::mutex`), чтобы строки из разных потоков не перемешивались.
- **`enum class LogLevel` копируется точно**: базовый тип `std::uint8_t`, порядок `Trace, Debug, Info, Warning, Error, Critical`.

**Общий порядок реализации фичи:**
1. Объявить `ILogger` и `LogLevel` в `logger.hpp`.
2. Объявить `IConfigService` в `config_service.hpp`.
3. Реализовать фабрики в `.cpp`.

**Файлы фичи:**
1. `engine/core/include/sky/core/logger.hpp`
2. `engine/core/include/sky/core/config_service.hpp`
3. `engine/core/include/sky/core/runtime_services.hpp` (объявления фабрик)
4. `engine/core/src/console_logger.cpp`
5. `engine/core/src/memory_config_service.cpp`

### Файл: `engine/core/include/sky/core/logger.hpp`

**Назначение файла:** контракт журнала.

**Пошаговое описание действий:**
1. Объявить `enum class LogLevel`.
2. Объявить метод `log` и сокращения `info/warning/error`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}`
- `class ILogger`

*Функции / методы:*
- `virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0`
- `void info/warning/error(std::string_view category, std::string_view message)`

*Логика функций / методов:*
- `log` — записывает строку журнала (`level` — важность, `category` — источник, `message` — текст).
- `info/warning/error` — сокращения для частых уровней (вызывают `log`).

**Результат по файлу:** контракт журнала зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/core/include/sky/core/config_service.hpp`

**Назначение файла:** контракт службы конфигурации.

**Пошаговое описание действий:**
1. Объявить методы чтения по ключу.
2. Объявить метод `set`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class IConfigService`

*Функции / методы:*
- `virtual std::optional<std::string> getString(std::string_view key) const = 0`
- `virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0`
- `virtual std::optional<bool> getBool(std::string_view key) const = 0`
- `virtual void set(std::string_view key, std::string value) = 0`

*Логика функций / методов:*
- `getString`/`getInt`/`getBool` — чтение значения по ключу (или `nullopt`).
- `set` — устанавливает значение.

**Результат по файлу:** контракт конфигурации зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/core/src/console_logger.cpp`

**Назначение файла:** реализация журнала в консоль.

**Пошаговое описание действий:**
1. Реализовать `ILogger`, пишущий в stdout/stderr.
2. Дать фабрику `std::unique_ptr<ILogger> createConsoleLogger(LogLevel minimumLevel = LogLevel::Info);` (объявляется в `sky/core/runtime_services.hpp`).

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация `ILogger`.

*Функции / методы:*
- `std::unique_ptr<ILogger> createConsoleLogger(LogLevel minimumLevel = LogLevel::Info)`

*Логика функций / методов:*
- `createConsoleLogger` — журнал, пишущий в stdout/stderr; сообщения ниже `minimumLevel` не выводятся.

**Результат по файлу:** рабочий консольный журнал.

**Критерий правильности по файлу:**
1. Записанное сообщение выводится в поток.

### Файл: `engine/core/src/memory_config_service.cpp`

**Назначение файла:** реализация конфигурации в памяти.

**Пошаговое описание действий:**
1. Реализовать `IConfigService` поверх `map`.
2. Дать фабрику `createInMemoryConfigService()`.

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация `IConfigService`.

*Функции / методы:*
- `std::unique_ptr<IConfigService> createInMemoryConfigService()`

*Логика функций / методов:*
- `createInMemoryConfigService` — конфиг на `map` ключ→значение.

**Результат по файлу:** рабочая служба конфигурации.

**Критерий правильности по файлу:**
1. Записанное значение читается обратно нужного типа.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/core/include/sky/core/logger.hpp`
2. `engine/core/include/sky/core/config_service.hpp`
3. `engine/core/include/sky/core/runtime_services.hpp`
4. `engine/core/src/console_logger.cpp`
5. `engine/core/src/memory_config_service.cpp`

**Общий критерий правильности:**
1. Записанные конфиг-значения читаются обратно нужного типа (`tests/core_tests.cpp`).

- **Приёмочный тест:** `tests/day2/core_services_tests.cpp` — компилируется против файлов фичи и проходит с кодом выхода 0.

---

## feature/vulkan-offscreen

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/render-contract` (день 1)

**Цель фичи:** инициализация Vulkan, закадровая цель, конвейер, отрисовка треугольника, чтение кадра.

**Описание фичи:** первый реальный кадр и `readbackFrame()` — основа верификации всего проекта.

**Обязательные требования:**

Все объявления — в пространстве имён `sky::rendering_vulkan`; типы контракта используются с квалификацией `rendering::` (например, `rendering::IRenderer`).

```cpp
// engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp
class VulkanRenderer : public rendering::IRenderer,
                       public rendering::IRenderResourceFactory {
public:
    ~VulkanRenderer() override = default;

    [[nodiscard]] virtual bool ready() const = 0;
    [[nodiscard]] virtual std::vector<std::uint8_t> readbackFrame() = 0;
    [[nodiscard]] virtual std::uint32_t frameWidth() const = 0;
    [[nodiscard]] virtual std::uint32_t frameHeight() const = 0;
    [[nodiscard]] virtual std::uint64_t presentedFrames() const = 0;
};

struct VulkanPresentTarget {
    void* x11Display = nullptr;
    std::uint64_t x11Window = 0;
    void* metalLayer = nullptr; // CAMetalLayer* on macOS (MoltenVK)
};

std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width,
                                                     std::uint32_t height);

std::unique_ptr<VulkanRenderer> createVulkanRendererForWindow(
    const VulkanPresentTarget& target, std::uint32_t width, std::uint32_t height);

void registerVulkanBackend(rendering::IRendererRegistry& registry,
                           std::uint32_t width = 1280, std::uint32_t height = 720);
```

- **Двойное наследование обязательно**: `VulkanRenderer` наследует и `rendering::IRenderer`, и `rendering::IRenderResourceFactory` — методы фабрики ресурсов уже в день 4 нужны на самом рендерере (в день 2 они могут возвращать `RenderResourceHandle::invalid()` как заглушки).
- **`createVulkanRenderer` возвращает `nullptr`, если рендерер не готов (`!ready()`)** — нет Vulkan-драйвера/устройства. Это штатный no-GPU путь: без исключений и крашей.
- **`createVulkanRendererForWindow` и `registerVulkanBackend`** объявляются сразу с сигнатурами выше (бэкенд регистрируется в реестре под именем `"vulkan"`); реализация оконной презентации может остаться заглушкой до фичи презентации.
- **`.spv` в git не коммитить** — бинарные шейдеры компилируются сборкой; в репозиторий входит только сгенерированный `.spv.h` (массив слов SPIR-V).
- **Не добавлять свой `math.hpp` и не менять файлы `engine/core`** — математика берётся из `sky/core/math.hpp` (контур E1) как есть.

**Общий порядок реализации фичи:**
1. Объявить `VulkanRenderer` и фабрику в `vulkan_backend.hpp`.
2. Реализовать инициализацию, конвейер, кадр и readback в `vulkan_renderer.cpp`.
3. Добавить шейдеры `mesh.vert`/`mesh.frag`.

**Файлы фичи:**
1. `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
2. `engine/rendering_vulkan/src/vulkan_renderer.cpp`
3. `engine/rendering_vulkan/shaders/mesh.vert`
4. `engine/rendering_vulkan/shaders/mesh.frag`

### Файл: `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`

**Назначение файла:** контракт Vulkan-рендерера.

**Пошаговое описание действий:**
1. Объявить `VulkanRenderer`, расширяющий `IRenderer`.
2. Объявить фабрику `createVulkanRenderer`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class VulkanRenderer : public rendering::IRenderer, public rendering::IRenderResourceFactory`

*Функции / методы:*
- `virtual bool ready() const = 0`
- `virtual std::vector<std::uint8_t> readbackFrame() = 0`
- `virtual std::uint32_t frameWidth() const = 0`
- `virtual std::uint32_t frameHeight() const = 0`
- `std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width, std::uint32_t height)`

*Логика функций / методов:*
- `ready` — инициализировался ли рендерер.
- `readbackFrame` — пиксели последнего кадра (RGBA).
- `frameWidth`/`frameHeight` — размеры кадра.
- `createVulkanRenderer` — создаёт закадровый рендерер или `nullptr`, если Vulkan недоступен.

**Результат по файлу:** контракт бэкенда с чтением кадра.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/rendering_vulkan/src/vulkan_renderer.cpp`

**Назначение файла:** реализация Vulkan-бэкенда.

**Пошаговое описание действий:**
1. Инициализировать инстанс/устройство/очередь.
2. Создать закадровую цель, проход и конвейер.
3. Нарисовать треугольник и прочитать кадр.

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация `VulkanRenderer`.

*Функции / методы:*
- `initInstanceAndDevice()`, `initOffscreenTarget()`, `initPipeline()`, `renderFrame()`, `readbackFrame()`, `findMemoryType`, `createImage`, `createBuffer`, `createShader`.

*Логика функций / методов:*
- `initInstanceAndDevice()` — создаёт `VkInstance`, выбирает устройство и графическую очередь, создаёт `VkDevice`.
- `initOffscreenTarget()` — создаёт изображения цвета (RGBA) и глубины (D32), проход рендеринга и кадровый буфер.
- `initPipeline()` — создаёт конвейер (формат вершины позиция+нормаль+uv).
- `renderFrame()` — записывает команды, очищает, рисует треугольник (`vkCmdDraw`).
- `readbackFrame()` — копирует изображение цвета в CPU-буфер, возвращает пиксели.

**Результат по файлу:** закадровый кадр, читаемый в память.

**Критерий правильности по файлу:**
1. `readbackFrame()` возвращает непустой массив.

### Файл: `engine/rendering_vulkan/shaders/mesh.vert`

**Назначение файла:** вершинный шейдер.

**Пошаговое описание действий:**
1. Написать минимальный вершинный шейдер.
2. Скомпилировать в `mesh.vert.spv.h`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (GLSL).

*Функции / методы:* `main()` шейдера.

*Логика функций / методов:* минимальный вершинный шейдер (позже дорастёт до PBR).

**Результат по файлу:** `mesh.vert.spv.h`.

**Критерий правильности по файлу:**
1. Шейдер компилируется `glslangValidator -V`.

### Файл: `engine/rendering_vulkan/shaders/mesh.frag`

**Назначение файла:** фрагментный шейдер.

**Пошаговое описание действий:**
1. Написать минимальный фрагментный шейдер.
2. Скомпилировать в `mesh.frag.spv.h`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (GLSL).

*Функции / методы:* `main()` шейдера.

*Логика функций / методов:* минимальный фрагментный шейдер (позже дорастёт до PBR).

**Результат по файлу:** `mesh.frag.spv.h`.

**Критерий правильности по файлу:**
1. Шейдер компилируется `glslangValidator -V`.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
2. `engine/rendering_vulkan/src/vulkan_renderer.cpp`
3. `engine/rendering_vulkan/shaders/mesh.vert`, `mesh.frag` (+ встроенные `.spv.h`)

**Общий критерий правильности:**
1. На `triangle.png` центральный пиксель отличается от углового.
2. `readbackFrame()` возвращает непустой массив.

- **Приёмочный тест:** `tests/day2/vulkan_offscreen_tests.cpp` — запускается при наличии Vulkan-драйвера, иначе SKIPPED/0.

---

## feature/docking-layout

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/editor-shell` (день 1)

**Цель фичи:** компоновка перетаскиваемых панелей-заглушек редактора.

**Описание фичи:** система докинга задаёт структуру рабочего пространства; на место заглушек позже встанут живые панели.

**Обязательные требования:**

Все классы — в пространстве имён `SkyEditor.Docking` (представления — `SkyEditor.Views`). В `SkyEditor.csproj` добавляются пакеты `Dock.Avalonia` **11.2.0** и `Dock.Model.Mvvm` **11.2.0** (версии Dock 11.2.x соответствуют пакетам `Avalonia.*` **11.2.1** из дня 1; версии не смешивать).

```csharp
// editor/avalonia/Docking/Tools.cs (namespace SkyEditor.Docking)
public class EditorTool : Tool          { public MainViewModel? Main { get; set; } } // база: Dock.Model.Mvvm.Controls.Tool
public class EditorDocument : Document  { public MainViewModel? Main { get; set; } } // база: Dock.Model.Mvvm.Controls.Document

public sealed class HierarchyTool : EditorTool { }
public sealed class InspectorTool : EditorTool { }
public sealed class MaterialsTool : EditorTool { }
public sealed class TerrainTool : EditorTool { }
public sealed class ProjectTool : EditorTool { }
public sealed class ConsoleTool : EditorTool { }
public sealed class PackagesTool : EditorTool { }
public sealed class SceneDocument : EditorDocument { }
public sealed class GameDocument : EditorDocument { }
public sealed class PlaceholderTool : EditorTool { }

// editor/avalonia/Docking/DockFactory.cs (namespace SkyEditor.Docking)
public sealed class DockFactory : Factory   // именно наследник Dock.Model.Mvvm.Factory
{
    public DockFactory(MainViewModel main);
    public override IRootDock CreateLayout();
    public override void InitLayout(IDockable layout); // задаёт HostWindowLocator
}
```

- **Точная структура раскладки** (`CreateLayout`): слева `ToolDock` — Hierarchy; в центре `DocumentDock` — Scene, Game; справа `ToolDock` — Inspector, Terrain, Materials; снизу `ToolDock` — Project, Console, Packages; между доками — `ProportionalDockSplitter` (панели ресайзятся).
- **`InitLayout` задаёт `HostWindowLocator`** (`new HostWindow()`), иначе вкладку нельзя вытащить в отдельное плавающее окно — критерий «панели перетаскиваются» не выполняется.
- **Хост раскладки**: в `MainWindow.axaml` — `<dock:DockControl Name="DockControl"/>`; в `App.axaml` — тема `DockFluentTheme` и `DataTemplate`-ы «инструмент → представление»; в `MainWindow.axaml.cs` — метод `ResetLayout()` (сброс компоновки).
- **`.axaml` и `.axaml.cs` идут парой**: `PlaceholderView.axaml` + `PlaceholderView.axaml.cs` (code-behind вызывает `AvaloniaXamlLoader.Load(this)`); коммитить только `.cs` без разметки — ошибка.

**Общий порядок реализации фичи:**
1. Собрать раскладку в `DockFactory`.
2. Завести классы-инструменты панелей.
3. Добавить панель-заглушку.

**Файлы фичи:**
1. `editor/avalonia/Docking/DockFactory.cs`
2. `editor/avalonia/Docking/Tools.cs`
3. `editor/avalonia/Views/PlaceholderView.axaml` и `editor/avalonia/Views/PlaceholderView.axaml.cs`
4. правки `editor/avalonia/SkyEditor.csproj` (пакеты Dock), `MainWindow.axaml`(+`.cs`), `App.axaml` — см. «Обязательные требования»

### Файл: `editor/avalonia/Docking/DockFactory.cs`

**Назначение файла:** сборка раскладки панелей.

**Пошаговое описание действий:**
1. Реализовать `CreateLayout()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class DockFactory`

*Функции / методы:*
- `CreateLayout()`

*Логика функций / методов:*
- `CreateLayout()` собирает раскладку из панелей (Hierarchy/Scene/Inspector/…).

**Результат по файлу:** перетаскиваемая раскладка панелей.

**Критерий правильности по файлу:**
1. Раскладка строится без ошибок.

### Файл: `editor/avalonia/Docking/Tools.cs`

**Назначение файла:** классы-инструменты панелей.

**Пошаговое описание действий:**
1. Завести по классу на панель.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `HierarchyTool`, `InspectorTool`, `SceneDocument`, …

*Функции / методы:* привязка инструмента к панели.

*Логика функций / методов:* каждый класс-инструмент привязан к своей панели.

**Результат по файлу:** инструменты панелей.

**Критерий правильности по файлу:**
1. Инструменты подставляются в раскладку.

### Файл: `editor/avalonia/Views/PlaceholderView.axaml.cs`

**Назначение файла:** пустая панель-заглушка.

**Пошаговое описание действий:**
1. Реализовать `PlaceholderView`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class PlaceholderView`

*Функции / методы:* нет (представление).

*Логика функций / методов:* пустая панель-заглушка на месте будущих панелей.

**Результат по файлу:** заглушка панели.

**Критерий правильности по файлу:**
1. Панель отображается пустой.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Docking/DockFactory.cs`
2. `editor/avalonia/Docking/Tools.cs`
3. `editor/avalonia/Views/PlaceholderView.axaml` и `editor/avalonia/Views/PlaceholderView.axaml.cs`

**Общий критерий правильности:**
1. `dotnet build` — 0 ошибок; панели перетаскиваются; есть сброс компоновки.

- **Приёмочный тест:** `tests/day2/docking_layout_check.py` — `python3 docking_layout_check.py <корень_репозитория>`, код выхода 0.

---

## feature/physics-object-sync

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** `feature/physics-world` (день 1); модуль `object` (`feature/object-model`, E1) для полной синхронизации

**Цель фичи:** явная синхронизация физического мира с объектным до и после шага.

**Описание фичи:** переносит трансформы объектов в тела перед шагом и результаты обратно после — без неявного двойного владения.

**Обязательные требования:**

Все объявления — в пространстве имён `sky::physics`; типы модуля объектов используются с квалификацией `object::`.

```cpp
// engine/physics/include/sky/physics/physics.hpp (контракт дня 1 — уже в репозитории)
class IPhysicsSyncContract {
public:
    virtual ~IPhysicsSyncContract() = default;

    virtual void pushKinematicState() = 0;
    virtual void pullSimulationResults() = 0;
};

// engine/physics/include/sky/physics/physics_world.hpp (дополнение этой фичи)
class ObjectPhysicsSync : public IPhysicsSyncContract {
public:
    ~ObjectPhysicsSync() override = default;

    virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0;
    virtual void unbind(RigidBodyHandle body) = 0;
};

std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(
    PhysicsWorld& physics, object::IObjectHierarchyAccess& hierarchy);
```

- **`pushKinematicState()` берёт МИРОВОЙ трансформ**: перед шагом для каждой привязанной пары читается `hierarchy.worldTransform(object)` (не локальный!) и переносится в тело через `setBodyTransform` — иначе вложенные объекты попадут в физику в координатах родителя.
- **`pullSimulationResults()` пишет через `setLocalTransform`**: после шага мировой трансформ тела пересчитывается в локальный относительно родителя объекта и записывается `hierarchy.setLocalTransform(...)` — напрямую мировой трансформ в объект не пишется.
- **`unbind` рвёт обе стороны**: после `unbind(body)` пара не участвует ни в `pushKinematicState()`, ни в `pullSimulationResults()`.
- **Фича обязана работать через абстракцию `object::IObjectHierarchyAccess`**: модуль `object` (`feature/object-model`) может быть ещё не смержен — зависеть можно только от интерфейса, в тестах допустима собственная тестовая реализация иерархии.

**Общий порядок реализации фичи:**
1. Объявить `ObjectPhysicsSync` и фабрику (дополнение `physics_world.hpp`).
2. Реализовать push/pull-синхронизацию.
3. Написать `tests/physics_tests.cpp`.

**Файлы фичи:**
1. `engine/physics/include/sky/physics/physics_world.hpp` (дополнение)
2. `tests/physics_tests.cpp`

### Файл: `engine/physics/include/sky/physics/physics_world.hpp` (дополнение) + реализация

**Назначение файла:** контракт синхронизации физики с объектным миром.

**Пошаговое описание действий:**
1. Объявить `ObjectPhysicsSync`.
2. Объявить фабрику `createObjectPhysicsSync`.
3. Реализовать `bind/unbind` и push/pull.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class ObjectPhysicsSync : IPhysicsSyncContract`

*Функции / методы:*
- `virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0`
- `virtual void unbind(RigidBodyHandle body) = 0`
- `virtual void pushKinematicState() = 0`
- `virtual void pullSimulationResults() = 0`
- `std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)`

*Логика функций / методов:*
- `bind` — связывает тело с объектом; `unbind` — разрывает связь.
- `pushKinematicState()` — до шага переносит трансформы объектов в тела.
- `pullSimulationResults()` — после шага переносит результат обратно.

**Результат по файлу:** двусторонняя синхронизация вокруг шага.

**Критерий правильности по файлу:**
1. Привязанный объект синхронно опускается в объектном мире.

### Файл: `tests/physics_tests.cpp`

**Назначение файла:** тесты физики.

**Пошаговое описание действий:**
1. Проверить падение под гравитацией.
2. Проверить куб на полу и тело на heightfield.
3. Проверить синхронизацию объекта.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:* тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на heightfield; привязанный объект синхронно опускается.

**Результат по файлу:** зелёный тест `physics_tests`.

**Критерий правильности по файлу:**
1. Все проверки проходят.

### На выходе должно получиться

**Список артефактов фичи:**
1. дополненный `engine/physics/include/sky/physics/physics_world.hpp` с `ObjectPhysicsSync` и его реализация
2. `tests/physics_tests.cpp`
3. библиотека `sky_physics` собрана; тест `physics_tests` зелёный

**Общий критерий правильности:**
1. Тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности.
2. Привязанный объект синхронно опускается в объектном мире.

- **Приёмочный тест:** `tests/day2/physics_object_sync_tests.cpp` — компилируется против файлов фичи и проходит с кодом выхода 0.

---

## feature/test-harness

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 5
- **Зависимости:** `feature/build-system` (день 1)

**Цель фичи:** лёгкий тест-фреймворк и CI на GitHub Actions.

**Описание фичи:** макросы для юнит-тестов без падения процесса; CI ловит поломки сборки и нестабильные тесты, красный статус блокирует слияние.

**Обязательные требования:**

Счётчики и `summary` — в пространстве имён `sky::test`; макрос `CHECK` — вне пространства имён (макросы не подчиняются namespace). Контракт харнесса копируется точно:

```cpp
// tests/sky_test.hpp
namespace sky::test {

inline int failures = 0;
inline int checks = 0;

inline int summary(const char* suite) {
    std::printf("%s: %d checks, %d failures\n", suite, checks, failures);
    return failures == 0 ? 0 : 1;
}

} // namespace sky::test

#define CHECK(condition)                                                      \
    do {                                                                      \
        ++sky::test::checks;                                                  \
        if (!(condition)) {                                                   \
            ++sky::test::failures;                                            \
            std::printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        }                                                                     \
    } while (false)
```

- **`CHECK` не роняет процесс**: никаких `abort()`/`exit()`/исключений — провал только увеличивает `failures` и печатает `FAILED` с `__FILE__` и `__LINE__`, остальные проверки продолжают выполняться.
- **Счётчики `checks`/`failures`** — `inline`-переменные: каждый `CHECK` инкрементирует `checks`, каждый провал — `failures`.
- **`summary` возвращает код выхода**: `0` без провалов, `1` при провалах; `main` тестового бинаря возвращает `return sky::test::summary("имя_сьюта");` — так CTest видит красный тест.
- **Регистрация тестов в `tests/CMakeLists.txt`**: каждый тестовый бинарь — `add_executable` + `add_test` (в репозитории это обёрнуто в функцию `sky_add_test(NAME)`: `add_executable(${NAME} ${NAME}.cpp)` + `add_test(NAME ${NAME} COMMAND ${NAME})`).

**Общий порядок реализации фичи:**
1. Завести `sky_test.hpp` (`CHECK`, `summary`).
2. Настроить CI в `ci.yml`.

**Файлы фичи:**
1. `tests/sky_test.hpp`
2. `.github/workflows/ci.yml`

### Файл: `tests/sky_test.hpp`

**Назначение файла:** тест-харнесс.

**Пошаговое описание действий:**
1. Реализовать макрос `CHECK`.
2. Реализовать `summary`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- макрос `CHECK(condition)`
- `inline int summary(const char* suite)`

*Логика функций / методов:*
- `CHECK` — фиксирует провал с файлом и строкой, не роняя процесс.
- `summary` — печатает «N проверок, M провалов»; возвращает код выхода (0 = успех).

**Результат по файлу:** харнесс для тестов.

**Критерий правильности по файлу:**
1. Проваленный `CHECK` отражается в `summary` ненулевым кодом.

### Файл: `.github/workflows/ci.yml`

**Назначение файла:** конвейер CI.

**Пошаговое описание действий:**
1. Установить зависимости (ninja, lavapipe, xvfb).
2. Собрать и прогнать тесты под Xvfb.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (YAML).

*Функции / методы:* нет (шаги CI).

*Логика функций / методов:* CI на Ubuntu — установка зависимостей, `cmake --build`, `xvfb-run ctest`; красный статус блокирует слияние.

**Результат по файлу:** рабочий CI.

**Критерий правильности по файлу:**
1. Красный CI блокирует слияние.

### На выходе должно получиться

**Список артефактов фичи:**
1. `tests/sky_test.hpp`
2. `.github/workflows/ci.yml`

**Общий критерий правильности:**
1. Красный CI блокирует слияние; сломанный тест краснеет.

- **Приёмочный тест:** `tests/day2/test_harness_check.py` — `python3 test_harness_check.py <корень_репозитория>`, код выхода 0.

---

## feature/ecs-tests

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** `feature/ecs-core` (день 1)

**Цель фичи:** покрыть ECS-ядро тестами и подтвердить сборку `sky_ecs`.

**Описание фичи:** зафиксировать, что система обновляет только подходящие сущности, а результат читается через хранилище.

**Обязательные требования:**

Это мета-фича: новых модулей она не добавляет, а фиксирует обязательное покрытие ECS-ядра дня 1 (`sky::ecs`). Тесты обязаны проверять следующие поведения:

- **Жизненный цикл сущности** (`IEcsWorld`): `createEntity()` возвращает живую сущность (`isAlive == true`); после `destroyEntity()` сущность мертва, а её компоненты удалены из хранилищ.
- **Типизированные хранилища** (`EcsWorld::storeFor<T>()`): записанный компонент читается обратно через `storeFor<T>().get(...)`; `has(entity)` отражает наличие компонента; `remove(entity)` исключает сущность из выборок.
- **Системы и планировщик** (`IEcsSystem`, `IEcsSystemScheduler`): зарегистрированная система получает `update(deltaSeconds)` при `tick`; система обновляет ТОЛЬКО сущности с нужным компонентом — сущности без него не затрагиваются; после `unregisterSystem` вызовы прекращаются.
- **Запросы** (`IEcsQueryService`): `entitiesWith(...)` возвращает только сущности, имеющие ВЕСЬ указанный набор компонентов; сущности с частичным набором в выборку не попадают.
- **Детерминизм**: тесты не используют `sleep`, таймеры и потоки — только детерминированные вызовы `tick`/`update` с фиксированным `deltaSeconds`; результат прогона одинаков от запуска к запуску.
- **При отсутствии харнеса E5**: если `tests/sky_test.hpp` (`feature/test-harness`) ещё не смержен — завести локальный макрос `CHECK` с теми же счётчиками прямо в тестовом файле и заменить его include-ом харнеса после слияния.

**Общий порядок реализации фичи:**
1. Написать ECS-часть `runtime_tests.cpp`.
2. Написать ECS-часть `world_tests.cpp`.

**Файлы фичи:**
1. `tests/runtime_tests.cpp` (ECS-часть)
2. `tests/world_tests.cpp` (ECS-часть)

### Файл: `tests/runtime_tests.cpp` (ECS-часть)

**Назначение файла:** тест ECS-системы.

**Пошаговое описание действий:**
1. Завести систему, обновляющую сущности с нужным компонентом.
2. Прочитать результат через `storeFor<T>().get(...)`.

**Что должно быть в файле:**

*Структуры / классы / enum:* тестовая система.

*Функции / методы:* тестовая функция.

*Логика функций / методов:* система обновляет только сущности с нужным компонентом; результат читается через `storeFor<T>().get(...)`.

**Результат по файлу:** зелёная ECS-часть `runtime_tests`.

**Критерий правильности по файлу:**
1. Обновляются только подходящие сущности.

### Файл: `tests/world_tests.cpp` (ECS-часть)

**Назначение файла:** тест мира ECS.

**Пошаговое описание действий:**
1. Проверить создание/уничтожение сущности.
2. Проверить `entitiesWith` по типам.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовая функция.

*Логика функций / методов:* `entitiesWith` возвращает только сущности с указанным набором компонентов.

**Результат по файлу:** зелёная ECS-часть `world_tests`.

**Критерий правильности по файлу:**
1. Запрос по типам корректен.

### На выходе должно получиться

**Список артефактов фичи:**
1. ECS-часть `tests/runtime_tests.cpp`
2. ECS-часть `tests/world_tests.cpp`
3. библиотека `sky_ecs` собрана; ECS-часть тестов зелёная

**Общий критерий правильности:**
1. Создание/уничтожение сущности работает.
2. Система обновляет только сущности с нужным компонентом.
3. Запрос по типам корректен.

- **Приёмочный тест:** `tests/day2/ecs_tests_checklist.md` — покрытие сверяется по чек-листу: каждый пункт «Обязательных требований» отмечен ссылкой на реализованный тест.
