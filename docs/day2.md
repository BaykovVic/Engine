# День 2 — сервисы ядра, тест-харнесс, докинг

Фичи дня (в порядке реализации). Одна фича = ветка `feature/<название>` = один PR в `develop`.

---

## feature/core-services (E1)
**Цель фичи:** единый журнал и служба конфигурации для всех подсистем.
**Описание фичи (для чего):** логирование и конфиг нужны всем модулям; выносятся за интерфейсы с фабриками.
**Пошаговое описание действий:**
- Сделай файл `engine/core/include/sky/core/logger.hpp`.
- В файле должны быть `enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}` и методы `log(level, category, message)`, сокращения `info/warning/error`.
- В методах должна быть реализована логика: `log` записывает строку журнала; сокращения вызывают `log` с нужным уровнем.
- Сделай файл `engine/core/include/sky/core/config_service.hpp`.
- В файле должны быть методы `getString`, `getInt`, `getBool`, `set`.
- В методах должна быть реализована логика: чтение значения по ключу (`nullopt`, если нет); `set` кладёт значение.
- Сделай файлы `engine/core/src/console_logger.cpp`, `engine/core/src/memory_config_service.cpp`.
- В файлах должны быть фабрики `createConsoleLogger()` (журнал в stdout/stderr) и `createInMemoryConfigService()` (конфиг на `map`).
**На выходе должно получиться:** `logger.hpp`, `config_service.hpp`, `console_logger.cpp`, `memory_config_service.cpp`.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** записанные конфиг-значения читаются обратно нужного типа (`tests/core_tests.cpp`).

## feature/test-harness (E5)
**Цель фичи:** лёгкий тест-фреймворк и CI на GitHub Actions.
**Описание фичи (для чего):** макросы для юнит-тестов без падения процесса; CI ловит поломки сборки и нестабильные тесты, красный статус блокирует слияние.
**Пошаговое описание действий:**
- Сделай файл `tests/sky_test.hpp` — макрос `CHECK(condition)` и функция `inline int summary(const char* suite)`.
- В них должна быть реализована логика: `CHECK` фиксирует провал с файлом/строкой, не роняя процесс; `summary` печатает «N проверок, M провалов» и возвращает код выхода.
- Сделай файл `.github/workflows/ci.yml` — CI на Ubuntu: установка зависимостей (ninja, lavapipe, xvfb), `cmake --build`, `xvfb-run ctest`.
**На выходе должно получиться:** `tests/sky_test.hpp`, `.github/workflows/ci.yml`; работающий CI.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** красный CI блокирует слияние; сломанный тест краснеет.

## feature/docking-layout (E3)
**Цель фичи:** компоновка перетаскиваемых панелей-заглушек редактора.
**Описание фичи (для чего):** система докинга задаёт структуру рабочего пространства (Hierarchy/Scene/Inspector…); на место заглушек позже встанут живые панели.
**Пошаговое описание действий:**
- Сделай файл `editor/avalonia/Docking/DockFactory.cs` — класс `DockFactory` с `CreateLayout()` (собирает раскладку панелей).
- Сделай файл `editor/avalonia/Docking/Tools.cs` — классы-инструменты `HierarchyTool`, `InspectorTool`, `SceneDocument`, … (привязка инструмента к панели).
- Сделай файл `editor/avalonia/Views/PlaceholderView.axaml.cs` — `PlaceholderView` — пустая панель-заглушка.
**На выходе должно получиться:** `DockFactory.cs`, `Tools.cs`, `PlaceholderView.axaml.cs`; перетаскиваемая компоновка.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `dotnet build` = 0 ошибок; есть сброс компоновки.
