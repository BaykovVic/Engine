# Спринт 1. День 2

## Контур E1 (Ядро и данные)

feature/core-services

Цель фичи: единый журнал и служба конфигурации — используются всеми подсистемами.
Описание фичи (для чего): логирование и конфиг нужны всем модулям; выносятся за интерфейсы с фабриками.
Пошаговое описание действий:

Сделай файл engine/core/include/sky/core/logger.hpp
enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical} объявляется здесь. В файле должны быть функции/методы:
- `virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0`
  Что делает: записывает строку журнала. Параметры: level — важность, category — подсистема-источник, message — текст. Возвращает: ничего.
- `void info/warning/error(std::string_view category, std::string_view message)`
  Что делает: сокращения для частых уровней (вызывают log). Возвращает: ничего.

Сделай файл engine/core/include/sky/core/config_service.hpp
В файле должны быть функции/методы:
- `virtual std::optional<std::string> getString(std::string_view key) const = 0`
  Что делает: читает строковое значение. Параметры: key — имя настройки. Возвращает: значение или nullopt.
- `virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0` — то же для целого. Возвращает: число или nullopt.
- `virtual std::optional<bool> getBool(std::string_view key) const = 0` — то же для логического. Возвращает: булево или nullopt.
- `virtual void set(std::string_view key, std::string value) = 0`
  Что делает: устанавливает значение. Параметры: key, value. Возвращает: ничего.

Сделай файл engine/core/src/console_logger.cpp
В файле должны быть функции/методы:
- `std::unique_ptr<ILogger> createConsoleLogger()` — журнал в stdout/stderr.
В методах должна быть реализована логика: реализация ILogger, пишущая в stdout/stderr.

Сделай файл engine/core/src/memory_config_service.cpp
В файле должны быть функции/методы:
- `std::unique_ptr<IConfigService> createInMemoryConfigService()` — конфиг на map ключ→значение.
В методах должна быть реализована логика: реализация IConfigService поверх map.

На выходе должно получиться:
- engine/core/include/sky/core/logger.hpp
- engine/core/include/sky/core/config_service.hpp
- engine/core/src/console_logger.cpp
- engine/core/src/memory_config_service.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: записанные конфиг-значения читаются обратно нужного типа (tests/core_tests.cpp).

## Контур E2 (Рендеринг)

feature/vulkan-offscreen

Цель фичи: инициализация Vulkan, закадровая цель, конвейер, отрисовка треугольника, чтение кадра.
Описание фичи (для чего): первый реальный кадр и readbackFrame() — основа верификации всего проекта.
Пошаговое описание действий:

Сделай файл engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp
class VulkanRenderer : public rendering::IRenderer — добавляет функции/методы:
- `virtual bool ready() const = 0` — Возвращает: инициализировался ли рендерер.
- `virtual std::vector<std::uint8_t> readbackFrame() = 0` — Возвращает: пиксели последнего кадра (RGBA).
- `virtual std::uint32_t frameWidth() const = 0` / `frameHeight() const = 0` — Возвращает: размеры кадра.
- `std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width, std::uint32_t height)`
  Что делает: создаёт закадровый рендерер. Параметры: width, height. Возвращает: рендерер или nullptr, если Vulkan недоступен.

Сделай файл engine/rendering_vulkan/src/vulkan_renderer.cpp
В файле должны быть приватные методы initInstanceAndDevice(), initOffscreenTarget(), initPipeline(), renderFrame(), readbackFrame() и вспомогательные findMemoryType, createImage, createBuffer, createShader. В методах должна быть реализована логика:
- initInstanceAndDevice() — создаёт VkInstance, выбирает устройство и графическую очередь, создаёт VkDevice.
- initOffscreenTarget() — создаёт изображения цвета (RGBA) и глубины (D32), проход рендеринга и кадровый буфер.
- initPipeline() — создаёт графический конвейер; формат вершины — позиция+нормаль+uv.
- renderFrame() — записывает команды, очищает, рисует треугольник (vkCmdDraw).
- readbackFrame() — копирует изображение цвета в буфер, доступный CPU, возвращает пиксели.

Сделай файл engine/rendering_vulkan/shaders/mesh.vert
В файле должен быть минимальный вершинный шейдер (позже дорастёт до PBR); компилируется glslangValidator -V в mesh.vert.spv.h.

Сделай файл engine/rendering_vulkan/shaders/mesh.frag
В файле должен быть минимальный фрагментный шейдер (позже дорастёт до PBR); компилируется glslangValidator -V в mesh.frag.spv.h.

На выходе должно получиться:
- engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp
- engine/rendering_vulkan/src/vulkan_renderer.cpp
- engine/rendering_vulkan/shaders/mesh.vert, mesh.frag (и встроенные mesh.vert.spv.h, mesh.frag.spv.h)
КРИТЕРИЙ ПРАВИЛЬНОСТИ: на triangle.png центральный пиксель отличается от углового; readbackFrame() возвращает непустой массив.

## Контур E3 (Редактор .NET)

feature/docking-layout

Цель фичи: компоновка перетаскиваемых панелей-заглушек редактора.
Описание фичи (для чего): система докинга задаёт структуру рабочего пространства; на место заглушек позже встанут живые панели.
Пошаговое описание действий:

Сделай файл editor/avalonia/Docking/DockFactory.cs
В файле должны быть функции/методы:
- `class DockFactory` — `CreateLayout()` собирает раскладку из панелей (Hierarchy/Scene/Inspector/…).

Сделай файл editor/avalonia/Docking/Tools.cs
В файле должны быть функции/методы:
- классы-инструменты (по одному на панель): `HierarchyTool`, `InspectorTool`, `SceneDocument`, …

Сделай файл editor/avalonia/Views/PlaceholderView.axaml.cs
В файле должны быть функции/методы:
- `PlaceholderView` — пустая панель-заглушка.

На выходе должно получиться:
- editor/avalonia/Docking/DockFactory.cs
- editor/avalonia/Docking/Tools.cs
- editor/avalonia/Views/PlaceholderView.axaml.cs
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build — 0 ошибок; окно открывается; панели перетаскиваются; есть сброс компоновки.

## Контур E4 (Рантайм и физика)

feature/physics-object-sync

Цель фичи: явная синхронизация физического мира с объектным до и после шага.
Описание фичи (для чего): переносит трансформы объектов в тела перед шагом и результаты обратно после — без неявного двойного владения.
Пошаговое описание действий:

Сделай файл engine/physics/include/sky/physics/physics_world.hpp (дополнение)
class ObjectPhysicsSync : IPhysicsSyncContract — добавляет функции/методы:
- `virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0` — связывает тело с объектом.
- `virtual void unbind(RigidBodyHandle body) = 0` — разрывает связь.
- `virtual void pushKinematicState() = 0` — до шага переносит трансформы объектов в тела.
- `virtual void pullSimulationResults() = 0` — после шага переносит результат обратно.
- `std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)` — фабрика.
В методах должна быть реализована логика:
- bind/unbind — вести карту связей тело↔объект.
- pushKinematicState() — до шага для каждой связи записать мировой трансформ объекта в тело.
- pullSimulationResults() — после шага записать трансформ тела обратно в локальный трансформ объекта.

Сделай файл tests/physics_tests.cpp
В файле должны быть проверки: падение ≈4.9 м/с, куб на полу, тело на heightfield, синхронизация объекта.

На выходе должно получиться:
- дополненный engine/physics/include/sky/physics/physics_world.hpp с ObjectPhysicsSync и его реализация
- tests/physics_tests.cpp
- библиотека sky_physics собрана; тест physics_tests зелёный
КРИТЕРИЙ ПРАВИЛЬНОСТИ: тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности; привязанный объект синхронно опускается в объектном мире.

## Контур E5 (Пайплайн и QA)

feature/test-harness

Цель фичи: лёгкий тест-фреймворк и CI на GitHub Actions.
Описание фичи (для чего): макросы для юнит-тестов без падения процесса; CI ловит поломки сборки и нестабильные тесты, красный статус блокирует слияние.
Пошаговое описание действий:

Сделай файл tests/sky_test.hpp
В файле должны быть функции/методы:
- макрос `CHECK(condition)` — фиксирует провал с файлом и строкой, не роняя процесс.
- `inline int summary(const char* suite)` — печатает «N проверок, M провалов». Возвращает: код выхода (0 = успех).

Сделай файл .github/workflows/ci.yml
В файле должна быть реализована логика: CI на Ubuntu — установка зависимостей (ninja, lavapipe, xvfb), cmake --build, xvfb-run ctest; красный статус блокирует слияние.

На выходе должно получиться:
- tests/sky_test.hpp
- .github/workflows/ci.yml
КРИТЕРИЙ ПРАВИЛЬНОСТИ: красный CI блокирует слияние; сломанный тест краснеет.

## Контур E6 (Data-oriented / ECS)

feature/ecs-tests

Цель фичи: покрыть ECS-ядро тестами и подтвердить сборку sky_ecs.
Описание фичи (для чего): зафиксировать, что система обновляет только подходящие сущности, а результат читается через хранилище.
Пошаговое описание действий:

Сделай файл tests/runtime_tests.cpp (ECS-часть)
В файле должна быть система, обновляющая только сущности с нужным компонентом; чтение результата через storeFor<T>().get(...).

Сделай файл tests/world_tests.cpp (ECS-часть)
В файле должны быть проверки создания/уничтожения сущности и запроса entitiesWith по типам.

На выходе должно получиться:
- ECS-часть tests/runtime_tests.cpp и tests/world_tests.cpp
- библиотека sky_ecs собрана; ECS-часть тестов зелёная
КРИТЕРИЙ ПРАВИЛЬНОСТИ: создание/уничтожение сущности; система обновляет только сущности с нужным компонентом; запрос по типам корректен.
