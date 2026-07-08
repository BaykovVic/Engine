# Спринт 1. День 2

## Контур E1 (Ядро и данные)

feature/core-services

Цель фичи: единый журнал и служба конфигурации — используются всеми подсистемами.
Описание фичи (для чего): логирование и конфиг нужны всем модулям; выносятся за интерфейсы с фабриками.
Пошаговое описание действий:
Сделай файл engine/core/include/sky/core/logger.hpp
В файле engine/core/include/sky/core/logger.hpp должны быть enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}, virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0 и сокращения void info/warning/error(std::string_view category, std::string_view message)
В методах должна быть реализована логика: log записывает строку журнала (level — важность, category — источник, message — текст); info/warning/error — сокращения для частых уровней (вызывают log).
Сделай файл engine/core/include/sky/core/config_service.hpp
В файле engine/core/include/sky/core/config_service.hpp должны быть virtual std::optional<std::string> getString(std::string_view key) const = 0, virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0, virtual std::optional<bool> getBool(std::string_view key) const = 0, virtual void set(std::string_view key, std::string value) = 0
В методах должна быть реализована логика: чтение строкового/целого/логического значения по ключу (или nullopt); set устанавливает значение.
Сделай файл engine/core/src/console_logger.cpp
В файле engine/core/src/console_logger.cpp должна быть фабрика std::unique_ptr<ILogger> createConsoleLogger()
В функции должна быть реализована логика: журнал в stdout/stderr.
Сделай файл engine/core/src/memory_config_service.cpp
В файле engine/core/src/memory_config_service.cpp должна быть фабрика std::unique_ptr<IConfigService> createInMemoryConfigService()
В функции должна быть реализована логика: конфиг на map ключ→значение.
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
В файле engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp должны быть class VulkanRenderer : public rendering::IRenderer (virtual bool ready() const = 0, virtual std::vector<std::uint8_t> readbackFrame() = 0, virtual std::uint32_t frameWidth() const = 0, virtual std::uint32_t frameHeight() const = 0) и std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width, std::uint32_t height)
В методах должна быть реализована логика: ready — инициализировался ли рендерер; readbackFrame — пиксели последнего кадра (RGBA); frameWidth/frameHeight — размеры кадра; createVulkanRenderer создаёт закадровый рендерер или nullptr, если Vulkan недоступен.
Сделай файл engine/rendering_vulkan/src/vulkan_renderer.cpp
В файле engine/rendering_vulkan/src/vulkan_renderer.cpp должны быть приватные методы initInstanceAndDevice(), initOffscreenTarget(), initPipeline(), renderFrame(), readbackFrame() и вспомогательные findMemoryType, createImage, createBuffer, createShader
В методах должна быть реализована логика: initInstanceAndDevice создаёт VkInstance, выбирает устройство и очередь, создаёт VkDevice; initOffscreenTarget — изображения цвета (RGBA) и глубины (D32), проход и кадровый буфер; initPipeline — конвейер (вершина позиция+нормаль+uv); renderFrame записывает команды, очищает, рисует треугольник (vkCmdDraw); readbackFrame копирует цвет в CPU-буфер.
Сделай файл engine/rendering_vulkan/shaders/mesh.vert
В файле engine/rendering_vulkan/shaders/mesh.vert должен быть минимальный вершинный шейдер (компилируется glslangValidator -V в mesh.vert.spv.h)
В шейдере должна быть реализована логика: минимальный вершинный шейдер (позже дорастёт до PBR).
Сделай файл engine/rendering_vulkan/shaders/mesh.frag
В файле engine/rendering_vulkan/shaders/mesh.frag должен быть минимальный фрагментный шейдер (компилируется glslangValidator -V в mesh.frag.spv.h)
В шейдере должна быть реализована логика: минимальный фрагментный шейдер (позже дорастёт до PBR).
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
В файле editor/avalonia/Docking/DockFactory.cs должен быть class DockFactory с методом CreateLayout()
В методе должна быть реализована логика: CreateLayout() собирает раскладку из панелей (Hierarchy/Scene/Inspector/…).
Сделай файл editor/avalonia/Docking/Tools.cs
В файле editor/avalonia/Docking/Tools.cs должны быть классы-инструменты (по одному на панель): HierarchyTool, InspectorTool, SceneDocument, …
В классах должна быть реализована логика: привязка каждого инструмента к своей панели.
Сделай файл editor/avalonia/Views/PlaceholderView.axaml.cs
В файле editor/avalonia/Views/PlaceholderView.axaml.cs должен быть класс PlaceholderView
В классе должна быть реализована логика: пустая панель-заглушка.
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
Сделай файл engine/physics/include/sky/physics/physics_world.hpp (дополнение) и его реализацию
В файле engine/physics/include/sky/physics/physics_world.hpp должны быть class ObjectPhysicsSync : IPhysicsSyncContract (virtual void bind(RigidBodyHandle body, object::ObjectHandle object) = 0, virtual void unbind(RigidBodyHandle body) = 0, virtual void pushKinematicState() = 0, virtual void pullSimulationResults() = 0) и std::unique_ptr<ObjectPhysicsSync> createObjectPhysicsSync(PhysicsWorld&, object::IObjectHierarchyAccess&)
В методах должна быть реализована логика: bind связывает тело с объектом, unbind разрывает связь; pushKinematicState до шага переносит трансформы объектов в тела; pullSimulationResults после шага переносит результат обратно.
Сделай файл tests/physics_tests.cpp
В файле tests/physics_tests.cpp должны быть проверки: падение ≈4.9 м/с, куб на полу, тело на heightfield, синхронизация объекта
В тесте должна быть реализована логика: тело за 1 с падает ≈4.9 м; куб замирает на полу; тело удерживается на высотной поверхности; привязанный объект синхронно опускается.
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
В файле tests/sky_test.hpp должны быть макрос CHECK(condition) и inline int summary(const char* suite)
В методах должна быть реализована логика: CHECK фиксирует провал с файлом и строкой, не роняя процесс; summary печатает «N проверок, M провалов» и возвращает код выхода (0 = успех).
Сделай файл .github/workflows/ci.yml
В файле .github/workflows/ci.yml должен быть CI на Ubuntu
В файле должна быть реализована логика: установка зависимостей (ninja, lavapipe, xvfb), cmake --build, xvfb-run ctest; красный статус блокирует слияние.
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
В файле tests/runtime_tests.cpp должна быть система, обновляющая только сущности с нужным компонентом
В тесте должна быть реализована логика: чтение результата через storeFor<T>().get(...).
Сделай файл tests/world_tests.cpp (ECS-часть)
В файле tests/world_tests.cpp должны быть проверки создания/уничтожения сущности и запроса entitiesWith по типам
В тесте должна быть реализована логика: entitiesWith возвращает только сущности с указанным набором компонентов.
На выходе должно получиться:
- ECS-часть tests/runtime_tests.cpp и tests/world_tests.cpp
- библиотека sky_ecs собрана; ECS-часть тестов зелёная
КРИТЕРИЙ ПРАВИЛЬНОСТИ: создание/уничтожение сущности; система обновляет только сущности с нужным компонентом; запрос по типам корректен.
