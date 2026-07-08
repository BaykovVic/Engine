# Спринт 4. День 3

## feature/scripting-integration

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 1
- **Зависимости:** `feature/dotnet-host` (хост скриптов), `feature/managed-runtime` (managed-сборка), `feature/play-mode` (цикл воспроизведения)

**Цель фичи:** интеграция скриптов в цикл воспроизведения — исполнение C#-скриптов в редакторе и проигрывателе.

**Описание фичи:** третья фича Этапа 4 контура E4 — сборочная точка редактора создаёт хост, загружает сборку, ставит обратный API и прогоняет скрипты по циклу Play; закрывает Этап 4 контура E4 тестами.

**Общий порядок реализации фичи:**
1. Реализовать `initScripting`, `startPlayScripts`, `tickScripts`, `stopPlayScripts` в `editor_context.cpp`.
2. Реализовать файловые колбэки `scriptSetLocal*`, `scriptLogMessage`, `scriptIsKeyDown` и структуру `SkyScriptApi`.
3. Написать тесты `dotnet_host_tests` и `scripting_rendering_tests`.

**Файлы фичи:**
1. `editor/shell/src/editor_context.cpp`
2. `tests/dotnet_host_tests.cpp`
3. `tests/scripting_rendering_tests.cpp`

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** дополнение сборочной точки редактора — интеграция скриптинга.

**Пошаговое описание действий:**
1. Реализовать `initScripting()`.
2. Реализовать `startPlayScripts()`, `tickScripts()`, `stopPlayScripts()`.
3. Реализовать файловые колбэки и структуру `SkyScriptApi`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- структура `SkyScriptApi` (таблица нативных указателей).

*Функции / методы:*
- `void initScripting()`
- `void startPlayScripts()`
- `void tickScripts(double deltaSeconds)`
- `void stopPlayScripts()`
- файловые колбэки `scriptSetLocal*`, `scriptLogMessage`, `scriptIsKeyDown`.

*Логика функций / методов:*
- `initScripting()` — создаёт хост, загружает сборку, ставит обратный API (`installEngineApi`).
- `startPlayScripts()` — создаёт инстансы для всех `sky.script`, вызывает OnCreate/OnStart.
- `tickScripts(deltaSeconds)` — вызывает OnUpdate каждый кадр.
- `stopPlayScripts()` — OnDestroy + уничтожение инстансов.
- файловые колбэки `scriptSetLocal*`, `scriptLogMessage`, `scriptIsKeyDown` и структура `SkyScriptApi` — таблица нативных указателей для managed-стороны.

**Результат по файлу:** скрипты исполняются в цикле воспроизведения.

**Критерий правильности по файлу:**
1. Скрипт на C# исполняется в редакторе и в проигрывателе.

### Файл: `tests/dotnet_host_tests.cpp`

**Назначение файла:** тесты хоста .NET.

**Пошаговое описание действий:**
1. Проверить запуск хоста, загрузку сборки и жизненный цикл инстанса.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:*
- проверяет запуск хоста, загрузку сборки, создание инстанса и вызовы жизненного цикла.

**Результат по файлу:** зелёный тест `dotnet_host_tests`.

**Критерий правильности по файлу:**
1. Тест `dotnet_host_tests` зелёный.

### Файл: `tests/scripting_rendering_tests.cpp`

**Назначение файла:** тесты скриптинга в связке с рендером.

**Пошаговое описание действий:**
1. Проверить скрипт-вращатель в Play и его результат.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовые функции.

*Логика функций / методов:*
- проверяет, что скрипт-вращатель даёт 90°/с (за 1 с = 45°) в Play редактора и в плеере.

**Результат по файлу:** зелёный тест `scripting_rendering_tests`.

**Критерий правильности по файлу:**
1. Скрипт-вращатель даёт 90°/с (за 1 с = 45°).

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/editor_context.cpp` (методы `initScripting`, `startPlayScripts`, `tickScripts`, `stopPlayScripts`, `SkyScriptApi`)
2. `tests/dotnet_host_tests.cpp`
3. `tests/scripting_rendering_tests.cpp`

**Общий критерий правильности:**
1. Скрипт на C# исполняется в редакторе и в проигрывателе; `Debug.Log` в консоль; `Time`/`Input` доступны.
2. Скрипт-вращатель даёт 90°/с (за 1 с = 45°) в Play редактора и в плеере.
3. Тест `dotnet_host_tests` зелёный.
