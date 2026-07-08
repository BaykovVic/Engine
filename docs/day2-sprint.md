# Спринт 1 · День 2 — выдача фич (по одной на контур)

День 2: каждый контур берёт свою **2-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/core-services` | Этап 1 (Неделя 1). Математика, объектная и компонентная модели |
| **E2** Рендеринг | `feature/vulkan-offscreen` | Этап 1 (Неделя 1). Vulkan от инициализации до кадра в файле |
| **E3** Редактор(.NET) | `feature/docking-layout` | Этап 1 (Неделя 1). Каркас, компоновка панелей, связь с движком |
| **E4** Рантайм/скриптинг | `feature/physics-object-sync` | Этап 1 (Неделя 1). Физическая симуляция |
| **E5** Пайплайн/пакеты | `feature/test-harness` | Этап 1 (Неделя 1). Сборка, интеграция, тесты, первичный C-интерфейс |
| **E6** Data-oriented(ECS) | `feature/ecs-tests` | Этап 1 (Неделя 1 · к 17 июля). ECS-мир и планировщик систем |

---

## E1 · `feature/core-services`

*Этап: Этап 1 (Неделя 1). Математика, объектная и компонентная модели.*

Единый журнал и служба конфигурации — используются всеми подсистемами.

#### Файл `engine/core/include/sky/core/logger.hpp`
`enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}` объявляется здесь.
- `virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0`
  Что делает: записывает строку журнала.
  Параметры: `level` — важность, `category` — подсистема-источник, `message` — текст. Возвращает: ничего.
- `void info/warning/error(std::string_view category, std::string_view message)`
  Что делает: сокращения для частых уровней (вызывают `log`). Возвращает: ничего.

#### Файл `engine/core/include/sky/core/config_service.hpp`
- `virtual std::optional<std::string> getString(std::string_view key) const = 0`
  Что делает: читает строковое значение. Параметры: `key` — имя настройки. Возвращает: значение или `nullopt`.
- `virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0` — то же для целого. Возвращает: число или `nullopt`.
- `virtual std::optional<bool> getBool(std::string_view key) const = 0` — то же для логического. Возвращает: булево или `nullopt`.
- `virtual void set(std::string_view key, std::string value) = 0`
  Что делает: устанавливает значение. Параметры: `key`, `value`. Возвращает: ничего.

#### Файлы `engine/core/src/console_logger.cpp`, `engine/core/src/memory_config_service.cpp`
Реализации интерфейсов выше плюс фабрики (объявлены в `runtime_services.hpp`):
- `std::unique_ptr<ILogger> createConsoleLogger()` — журнал в stdout/stderr.
- `std::unique_ptr<IConfigService> createInMemoryConfigService()` — конфиг на `map` ключ→значение.

---

## E2 · `feature/vulkan-offscreen`

*Этап: Этап 1 (Неделя 1). Vulkan от инициализации до кадра в файле.*

Инициализация Vulkan, закадровая цель, конвейер, отрисовка треугольника, чтение кадра.

#### Файл `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
- `class VulkanRenderer : public rendering::IRenderer` — добавляет:
  - `virtual bool ready() const = 0` — Возвращает: инициализировался ли рендерер.
  - `virtual std::vector<std::uint8_t> readbackFrame() = 0` — Возвращает: пиксели последнего кадра (RGBA).
  - `virtual std::uint32_t frameWidth() const = 0` / `frameHeight() const = 0` — Возвращает: размеры кадра.
- `std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width, std::uint32_t height)`
  Что делает: создаёт закадровый рендерер. Параметры: `width`, `height`. Возвращает: рендерер или `nullptr`, если Vulkan недоступен.

#### Файл `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- `initInstanceAndDevice()` — создаёт `VkInstance`, выбирает устройство и графическую очередь, создаёт `VkDevice`.
- `initTarget()` — создаёт изображения цвета (RGBA) и глубины (D32), проход рендеринга и кадровый буфер.
- `initPipeline()` — создаёт графический конвейер; формат вершины — позиция+нормаль+uv.
- `renderFrame()` — записывает команды, очищает, рисует треугольник (`vkCmdDraw`).
- `readbackFrame()` — копирует изображение цвета в буфер, доступный CPU, возвращает пиксели.
- вспомогательные `findMemoryType`, `createImage`, `createBuffer`, `createShader`.

#### Файлы `engine/rendering_vulkan/shaders/mesh.vert`, `mesh.frag` (+ встроенные `.spv.h`)
Минимальные вершинный и фрагментный шейдеры (позже дорастут до PBR). Компиляция
`glslangValidator -V` → массив `std::uint32_t` встраивается в `.spv.h`.

---

## E3 · `feature/docking-layout`

*Этап: Этап 1 (Неделя 1). Каркас, компоновка панелей, связь с движком.*

#### Файлы `editor/avalonia/Docking/DockFactory.cs`, `Docking/Tools.cs`, `Views/PlaceholderView.axaml.cs`
- `class DockFactory` — `CreateLayout()` собирает раскладку из панелей (Hierarchy/Scene/Inspector/…).
- `Tools.cs` — классы-инструменты (по одному на панель): `HierarchyTool`, `InspectorTool`, `SceneDocument`, …
- `PlaceholderView` — пустая панель-заглушка.

---

## E4 · `feature/physics-object-sync`

*Этап: Этап 1 (Неделя 1). Физическая симуляция.*

#### Дополнение файла `engine/physics/include/sky/physics/physics_world.hpp` + реализация
- `class ObjectPhysicsSync : IPhysicsSyncContract`
  - `virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0` — связывает тело с объектом.
  - `virtual void unbind(RigidBodyHandle body) = 0` — разрывает связь.
  - `virtual void pushKinematicState() = 0` — до шага переносит трансформы объектов в тела.
  - `virtual void pullSimulationResults() = 0` — после шага переносит результат обратно.
- `std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)` — фабрика.

#### Файл `tests/physics_tests.cpp`
Падение ≈4.9 м/с, куб на полу, тело на heightfield, синхронизация объекта.

---

## E5 · `feature/test-harness`

*Этап: Этап 1 (Неделя 1). Сборка, интеграция, тесты, первичный C-интерфейс.*

#### Файл `tests/sky_test.hpp`
- макрос `CHECK(condition)` — фиксирует провал с файлом и строкой, не роняя процесс.
- `inline int summary(const char* suite)` — печатает «N проверок, M провалов». Возвращает: код выхода (0 = успех).

#### Файл `.github/workflows/ci.yml`
CI на Ubuntu: установка зависимостей (ninja, lavapipe, xvfb), `cmake --build`,
`xvfb-run ctest`. Красный статус блокирует слияние.

---

## E6 · `feature/ecs-tests`

*Этап: Этап 1 (Неделя 1 · к 17 июля). ECS-мир и планировщик систем.*

#### Файлы `tests/runtime_tests.cpp`, `tests/world_tests.cpp` (ECS-часть)
Система, обновляющая только сущности с нужным компонентом; чтение результата
через `storeFor<T>().get(...)`.

---
