# День 2

## E1 · `feature/core-services`

Единый журнал и служба конфигурации — используются всеми подсистемами.

### Файл `engine/core/include/sky/core/logger.hpp`
`enum class LogLevel {Trace,Debug,Info,Warning,Error,Critical}` объявляется здесь.
- `virtual void log(LogLevel level, std::string_view category, std::string_view message) = 0`
  Что делает: записывает строку журнала.
  Параметры: `level` — важность, `category` — подсистема-источник, `message` — текст. Возвращает: ничего.
- `void info/warning/error(std::string_view category, std::string_view message)`
  Что делает: сокращения для частых уровней (вызывают `log`). Возвращает: ничего.

### Файл `engine/core/include/sky/core/config_service.hpp`
- `virtual std::optional<std::string> getString(std::string_view key) const = 0`
  Что делает: читает строковое значение. Параметры: `key` — имя настройки. Возвращает: значение или `nullopt`.
- `virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0` — то же для целого. Возвращает: число или `nullopt`.
- `virtual std::optional<bool> getBool(std::string_view key) const = 0` — то же для логического. Возвращает: булево или `nullopt`.
- `virtual void set(std::string_view key, std::string value) = 0`
  Что делает: устанавливает значение. Параметры: `key`, `value`. Возвращает: ничего.

### Файлы `engine/core/src/console_logger.cpp`, `engine/core/src/memory_config_service.cpp`
Реализации интерфейсов выше плюс фабрики (объявлены в `runtime_services.hpp`):
- `std::unique_ptr<ILogger> createConsoleLogger()` — журнал в stdout/stderr.
- `std::unique_ptr<IConfigService> createInMemoryConfigService()` — конфиг на `map` ключ→значение.

**Проверка (этап E1):** записанные конфиг-значения читаются обратно нужного типа
(`tests/core_tests.cpp`).

---

## E5 · `feature/test-harness`

### Файл `tests/sky_test.hpp`
- макрос `CHECK(condition)` — фиксирует провал с файлом и строкой, не роняя процесс.
- `inline int summary(const char* suite)` — печатает «N проверок, M провалов». Возвращает: код выхода (0 = успех).

### Файл `.github/workflows/ci.yml`
CI на Ubuntu: установка зависимостей (ninja, lavapipe, xvfb), `cmake --build`,
`xvfb-run ctest`. Красный статус блокирует слияние.

**Проверка (этап E5):** красный CI блокирует слияние; сломанный тест краснеет;
редактор E3 вызывает `sky_editor_create`.

---

## E3 · `feature/docking-layout`

### Файлы `editor/avalonia/Docking/DockFactory.cs`, `Docking/Tools.cs`, `Views/PlaceholderView.axaml.cs`
- `class DockFactory` — `CreateLayout()` собирает раскладку из панелей (Hierarchy/Scene/Inspector/…).
- `Tools.cs` — классы-инструменты (по одному на панель): `HierarchyTool`, `InspectorTool`, `SceneDocument`, …
- `PlaceholderView` — пустая панель-заглушка.

**Проверка (этап E3):** `dotnet build` = 0 ошибок; окно открывается; панели
перетаскиваются; есть сброс компоновки.
