# День 2

## feature/core-services
Цель фичи: единый журнал и служба конфигурации — используются всеми подсистемами (контур E1).
Описание фичи (для чего): логирование и конфиг нужны всем модулям; выносятся за интерфейсы с фабриками.
Пошаговое описание действий:
Сделай файл engine/core/include/sky/core/logger.hpp
В файле engine/core/include/sky/core/logger.hpp должны быть enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}, virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0 и сокращения void info/warning/error(std::string_view category, std::string_view message)
В методах должна быть реализована логика: log записывает строку журнала (level — важность, category — подсистема-источник, message — текст); info/warning/error — сокращения для частых уровней (вызывают log).
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
КРИТЕРИЙ ПРАВИЛЬНОСТИ: поворот (0,0,1) на 90° вокруг Y = (1,0,0)±1e-5; ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире = (0,0,-1); поле каждого из 5 типов записывается и читается без потерь.

## feature/test-harness
Цель фичи: лёгкий тест-фреймворк и CI на GitHub Actions (контур E5).
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
КРИТЕРИЙ ПРАВИЛЬНОСТИ: красный CI блокирует слияние; сломанный тест краснеет; редактор E3 вызывает sky_editor_create.

## feature/docking-layout
Цель фичи: компоновка перетаскиваемых панелей-заглушек редактора (контур E3).
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
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build = 0 ошибок; сессия движка создаётся из .NET (не-null); есть сброс компоновки.
