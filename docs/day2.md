# День 2 — сервисы ядра, тест-харнесс, докинг

## feature/core-services

Цель фичи: единый журнал и служба конфигурации для всех подсистем.
Описание фичи (для чего): логирование и конфиг нужны всем модулям; выносятся за интерфейсы с фабриками. Контур E1.
Пошаговое описание действий:
Сделай файл engine/core/include/sky/core/logger.hpp
В файле engine/core/include/sky/core/logger.hpp должны быть enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}, метод log(level, category, message) и сокращения info/warning/error.
В методах должна быть реализована логика: log записывает строку журнала; сокращения вызывают log с нужным уровнем.
Сделай файл engine/core/include/sky/core/config_service.hpp
В файле engine/core/include/sky/core/config_service.hpp должны быть методы getString, getInt, getBool, set.
В методах должна быть реализована логика: чтение значения по ключу (nullopt, если нет); set кладёт значение.
Сделай файл engine/core/src/console_logger.cpp
В файле engine/core/src/console_logger.cpp должна быть фабрика createConsoleLogger().
В функции должна быть реализована логика: журнал, пишущий в stdout/stderr.
Сделай файл engine/core/src/memory_config_service.cpp
В файле engine/core/src/memory_config_service.cpp должна быть фабрика createInMemoryConfigService().
В функции должна быть реализована логика: конфиг на map ключ→значение.
На выходе должно получиться:
- engine/core/include/sky/core/logger.hpp
- engine/core/include/sky/core/config_service.hpp
- engine/core/src/console_logger.cpp
- engine/core/src/memory_config_service.cpp
КРИТЕРИЙ ПРАВИЛЬНОСТИ: записанные конфиг-значения читаются обратно нужного типа (tests/core_tests.cpp).

## feature/test-harness

Цель фичи: лёгкий тест-фреймворк и CI на GitHub Actions.
Описание фичи (для чего): макросы для юнит-тестов без падения процесса; CI ловит поломки сборки и нестабильные тесты, красный статус блокирует слияние. Контур E5.
Пошаговое описание действий:
Сделай файл tests/sky_test.hpp
В файле tests/sky_test.hpp должны быть макрос CHECK(condition) и функция inline int summary(const char* suite).
В них должна быть реализована логика: CHECK фиксирует провал с файлом и строкой, не роняя процесс; summary печатает «N проверок, M провалов» и возвращает код выхода.
Сделай файл .github/workflows/ci.yml
В файле .github/workflows/ci.yml должен быть CI на Ubuntu.
В файле должна быть реализована логика: установка зависимостей (ninja, lavapipe, xvfb), cmake --build, xvfb-run ctest; красный статус блокирует слияние.
На выходе должно получиться:
- tests/sky_test.hpp
- .github/workflows/ci.yml
КРИТЕРИЙ ПРАВИЛЬНОСТИ: красный CI блокирует слияние; сломанный тест краснеет.

## feature/docking-layout

Цель фичи: компоновка перетаскиваемых панелей-заглушек редактора.
Описание фичи (для чего): система докинга задаёт структуру рабочего пространства (Hierarchy/Scene/Inspector…); на место заглушек позже встанут живые панели. Контур E3.
Пошаговое описание действий:
Сделай файл editor/avalonia/Docking/DockFactory.cs
В файле editor/avalonia/Docking/DockFactory.cs должен быть класс DockFactory с методом CreateLayout().
В методе должна быть реализована логика: сборка раскладки из панелей (Hierarchy/Scene/Inspector/…).
Сделай файл editor/avalonia/Docking/Tools.cs
В файле editor/avalonia/Docking/Tools.cs должны быть классы-инструменты HierarchyTool, InspectorTool, SceneDocument и др.
В классах должна быть реализована логика: привязка каждого инструмента к своей панели.
Сделай файл editor/avalonia/Views/PlaceholderView.axaml.cs
В файле editor/avalonia/Views/PlaceholderView.axaml.cs должен быть класс PlaceholderView.
В классе должна быть реализована логика: пустая панель-заглушка на месте будущих панелей.
На выходе должно получиться:
- editor/avalonia/Docking/DockFactory.cs
- editor/avalonia/Docking/Tools.cs
- editor/avalonia/Views/PlaceholderView.axaml.cs
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build = 0 ошибок; есть сброс компоновки.
