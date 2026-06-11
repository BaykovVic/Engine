# system-architecture

## Название проекта
Sky Engine

## Основание
- Project brief: `../brief/project-brief.md`
- Project concept: `../concept/project-concept.md`

## Архитектурная цель
Sky Engine — коммерческий Unity-подобный движок с архитектурой `native-first`. Основой движка является `C++20 core`, а `C#/.NET` используется как изолированный managed-слой для scripting. `Editor` и `runtime` образуют единую локальную систему разработки. Графическая подсистема строится через общую абстракцию `Rendering Abstraction` с backend `OpenGL` и `Vulkan`. Подсистемы `Package System`, `Terrain and Landscape`, `Map Generation`, `Physics Engine`, `Scene System`, `Object Model`, `Component Model` и `ECS Layer` должны проектироваться как части одной согласованной архитектуры.

## Архитектурные принципы
- `C++20 core` владеет основным состоянием движка.
- `C#` не владеет сценой, объектами и `runtime loop`.
- `Scripting Boundary` является отдельным слоем `native-managed` взаимодействия с контрактами на основе `handle`.
- `Editor` и `runtime` разделены, но связаны через `Viewport Runtime Bridge`.
- `Scene System`, `Object Model`, `Component Model` и `ECS Layer` имеют явные границы владения данными.
- Графическая логика зависит от `Rendering Abstraction`, а не от конкретного backend.
- `Package System` работает локально, без внешнего `registry` и без `SaaS`.
- Архитектура обязана быть `offline/local-first`.
- Требование по `vendor lock-in`: минимизировать архитектурную привязку к одному поставщику технологии.
- Практически все ключевые подсистемы должны быть реализованы самостоятельно.
- `Qt` допускается только в `editor-layer` и не должен становиться зависимостью `engine core`.

## Лицензионные ограничения и политика зависимостей
- Допустимые `open-source` лицензии: `MIT`, `BSD`, `Apache-2.0`.
- Специальное исключение: `LGPL` допускается только для `Qt`.
- Условие для `Qt`: использование без коммерческой лицензии и с соблюдением требований `LGPL`.
- Запрещенные лицензии: `MPL`, `GPL`, `AGPL`.
- `LGPL` для любых зависимостей, кроме `Qt`, запрещена.
- `copyleft`-зависимости запрещены, кроме отдельно разрешенного `Qt`.
- Коммерческие `SDK`, `API` и `SaaS` запрещены.
- Все зависимости должны быть пригодны для коммерческого продукта.
- `Editor`, `runtime`, `packages` и `project workflow` должны работать локально.

## Технологический стек

| Зона | Технология | Роль | Лицензия / правовой статус | Причина выбора | Альтернативы |
| --- | --- | --- | --- | --- | --- |
| Engine core | C++20 | основа `native runtime`, память, системные сервисы | стандарт языка | контроль, производительность, явное владение состоянием | нет |
| Editor UI | Qt5 | `editor shell` и `tooling UI` | `LGPL`, разрешено как специальное исключение | зрелая desktop UI-платформа, docking, инструменты | собственный UI framework |
| Scripting | C# | `gameplay` и managed-слой | зависит от `runtime host` | Unity-подобная модель scripting | нет |
| Rendering backend | OpenGL | графический backend | platform API | переносимость, более простой bootstrap | нет |
| Rendering backend | Vulkan | графический backend | platform API | современная явная графическая модель | нет |
| Build system | CMake | сборка `native`-части | `BSD-3-Clause` | стандартный выбор для `C++` multi-platform | Meson |

## Сторонние технологии и зависимости

| Технология / сервис / модель | Где используется | Лицензия / условия | Риск | Решение |
| --- | --- | --- | --- | --- |
| Qt5 | `editor UI` | `LGPL` | лицензионные обязательства | ограничить использование `editor-layer` и соблюдать `LGPL` |
| .NET runtime host | `scripting host` | зависит от выбранной дистрибуции | привязка к managed-окружению | изолировать в `Managed Runtime Host` |
| OpenGL/Vulkan SDK | `rendering` | platform API | сложность поддержки двух backend | спрятать за `Rendering Abstraction` |

## Контекст системы
Sky Engine состоит из локального `editor`-контура, локального `runtime`-контура и набора `core`-подсистем: `Project Model`, `Package System`, `Asset System`, `Scene System`, `Object Model`, `Component Model`, `ECS Layer`, `Physics Engine`, `Rendering Abstraction`, `Scripting Boundary`, `Terrain and Landscape`, `Map Generation`, `Serialization and Persistence`. Пользователь работает через `editor`. `Runtime` исполняется либо в `embedded play mode`, либо как отдельный `runtime target`. Все постоянные данные хранятся локально в структуре проекта.

## Модули системы
- `Platform Layer`: окно, ввод, файловая система, потоки, таймеры, платформенные сервисы.
- `Core Foundation`: логирование, конфигурация, диагностика, `jobs`, базовые события и утилиты.
- `Project Model`: конфигурация проекта, список сцен, ссылки на пакеты, структура проекта.
- `Package System`: локальные пакеты, `manifest`, `extension point`, жизненный цикл пакетов.
- `Asset System`: реестр ассетов, импорт, метаданные, граф зависимостей, кеширование.
- `Serialization and Persistence`: сохранение и восстановление проекта, сцен, ассетов и terrain-данных.
- `Scene System`: жизненный цикл сцен, загрузка, выгрузка, активный контекст сцены.
- `Object Model`: иерархия объектов, `transform`, идентичность объектов.
- `Component Model`: `native`- и `script`-компоненты, дескрипторы, инспектируемые поля.
- `ECS Layer`: `entity`, хранилища компонентов, `system scheduling`, `data-oriented execution`.
- `Physics Engine`: коллизии, `rigid body`, симуляция, запросы, контракт заменяемости physics backend.
- `Rendering Abstraction`: контракты рендера, ресурсы, материалы, пайплайны, модель команд.
- `OpenGL Backend`: реализация контрактов рендера через `OpenGL`.
- `Vulkan Backend`: реализация контрактов рендера через `Vulkan`.
- `Scripting Boundary`: реестр типов, `binding`, `handle`, marshaling, `lifecycle bridge`.
- `Managed Runtime Host`: managed-домен, `assembly`, `script instance`, оркестрация `reload`.
- `Editor Shell`: главное окно, docking, жизненный цикл сессии, панели.
- `Editor Tools`: `hierarchy`, `inspector`, `asset browser`, `package UI`, terrain-инструменты, gizmo.
- `Viewport Runtime Bridge`: `play mode`, размещение viewport, синхронизация `editor/runtime`.
- `Terrain and Landscape`: terrain-данные, модель редактирования, сохранение terrain, `rendering hooks`.
- `Map Generation`: процедурная генерация карты, заполнение terrain, расстановка объектов сцены.

## Схема взаимодействия модулей

```text
Пользователь
 -> Editor Shell
 -> Editor Tools
 -> Viewport Runtime Bridge
 -> Engine Core

Engine Core =
  Project Model
  Package System
  Asset System
  Serialization and Persistence
  Scene System
  Object Model
  Component Model
  ECS Layer
  Physics Engine
  Rendering Abstraction
  Scripting Boundary
  Terrain and Landscape
  Map Generation

Rendering Abstraction -> OpenGL Backend | Vulkan Backend
Scripting Boundary -> Managed Runtime Host -> C# Scripts
Platform Layer -> поддерживает Editor Shell, backend и runtime
Core Foundation -> используется всеми native-модулями
```

### Слоистая схема взаимодействия

```text
+--------------------------------------------------------------------------------+
|                              Пользователь / разработчик                        |
+-----------------------------------+--------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------------+
|                                  Editor Layer                                  |
| Editor Shell | Hierarchy | Inspector | Asset Browser | Package UI | Terrain UI |
+-----------------------------------+--------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------------+
|                    Editor Integration / Viewport Runtime Bridge                |
| selection | undo/redo | play/pause/stop | sync editor/runtime | viewport host  |
+-----------------------------------+--------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------------+
|                                   Engine Core                                  |
| Project | Package | Asset | Scene | Object | Component | ECS | Physics | Time  |
| Serialization | Events | Terrain | Map Generation                              |
+-------------------------------+-------------------------------+----------------+
                                |                               |
                                v                               v
+-----------------------------------------------+   +----------------------------+
|               Scripting Boundary              |   |   Rendering Abstraction    |
| handles | bindings | marshaling | lifecycle   |   | resources | commands | API |
+-------------------------------+---------------+   +-------------+--------------+
                                |                               |
                                v                               v
+-----------------------------------------------+   +----------------------------+
|              Managed Runtime Host             |   | OpenGL Backend | Vulkan    |
| assemblies | managed domain | script peers    |   | Backend        | Backend   |
+-----------------------------------------------+   +----------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------------+
|                                Platform Layer                                  |
| windowing | input | files | threads | timers | process                         |
+--------------------------------------------------------------------------------+
```

### Схема владения данными и границ модулей

```text
Project Model
  владеет -> project settings, структура путей проекта, scene registry, package refs

Package System
  владеет -> package manifests, package registry, extension bindings

Asset System
  владеет -> asset ids, metadata, dependency graph, import state

Scene System
  владеет -> загруженные сцены, active scene context, принадлежность объектов сцене

Object Model
  владеет -> object hierarchy, transforms, object handles

Component Model
  владеет -> component instances, descriptors, inspectable metadata

ECS Layer
  владеет -> entities, ECS component storage, состояние выполнения systems

Physics Engine
  владеет -> physics world, bodies, colliders, solver state

Rendering Abstraction
  владеет -> renderer-facing resources и command contracts

OpenGL Backend / Vulkan Backend
  владеют -> backend-specific GPU objects

Scripting Boundary
  владеет -> native/managed handle map, binding metadata, lifecycle bridge tables

Managed Runtime Host
  владеет -> managed domain, assemblies, managed object instances

Terrain and Landscape
  владеет -> terrain datasets, terrain chunks, terrain metadata

Map Generation
  владеет -> generation profiles, intermediate state, generation outputs
```

## Внешние системы и зависимости
- `OS platform API`
- графические драйверы для `OpenGL` и `Vulkan`
- managed `runtime host` для `C#`
- локальная файловая система

## Data flow

### Входные данные
- конфигурация проекта
- `package manifest`
- исходные файлы ассетов
- файлы сцен
- исходники `script` или `assemblies`
- команды пользователя в `editor`
- события ввода в `runtime`

### Обработка данных по модулям
- `Project Model` открывает структуру проекта.
- `Package System` обнаруживает и разрешает локальные пакеты.
- `Asset System` импортирует ресурсы и регистрирует их в реестре.
- `Serialization and Persistence` восстанавливает сцены, terrain и метаданные.
- `Scene System` создает и активирует контекст сцены.
- `Object Model` и `Component Model` восстанавливают объекты и компоненты.
- `ECS Layer` поднимает `data-oriented` состояние.
- `Physics Engine` строит и обновляет состояние физического мира.
- `Scripting Boundary` связывает native-объекты и managed-представления.
- `Rendering Abstraction` формирует render-команды.
- активный backend выводит кадр в viewport или окно `runtime`.

### Хранилища и состояние
- метаданные проекта: `Project Model`
- `package manifest` и `registry`: `Package System`
- идентичность ассетов и граф зависимостей: `Asset System`
- постоянное состояние сцены: `Serialization and Persistence` и `Scene System`
- иерархия и `transform`: `Object Model`
- экземпляры компонентов: `Component Model`
- данные `ECS`: `ECS Layer`
- физический мир: `Physics Engine`
- GPU-ресурсы и backend-объекты: `Rendering Abstraction` и backend
- native/managed `binding`: `Scripting Boundary`
- `assemblies` и состояние managed-домена: `Managed Runtime Host`
- terrain-данные: `Terrain and Landscape`

### Выходные данные
- изображение viewport или кадр `runtime`
- обновленные файлы проекта, сцен и ассетов
- сгенерированные terrain- и map-данные
- изменения состояния, вызванные `script`
- результат активации пакетов

### Схема data flow: открытие проекта

```text
Пользователь
 -> Editor Shell
 -> Project Model загружает project descriptor
 -> Package System читает package manifest и строит package graph
 -> Asset System загружает asset registry
 -> Serialization and Persistence восстанавливает editor session state
 -> Scene System открывает стартовую или последнюю сцену
 -> Object Model восстанавливает hierarchy
 -> Component Model восстанавливает компоненты
 -> Viewport Runtime Bridge подготавливает preview context
```

### Схема data flow: импорт ассета

```text
Исходный файл
 -> Asset System выбирает importer
 -> importer читает source data
 -> Asset System создает AssetId и metadata
 -> dependency graph обновляется
 -> Serialization and Persistence записывает результаты импорта
 -> Editor Tools обновляет Asset Browser
 -> Scene System и Rendering Abstraction сбрасывают зависимые кеши при необходимости
```

### Схема data flow: загрузка сцены

```text
Файл сцены
 -> Serialization and Persistence
 -> Scene System создает SceneRuntimeContext
 -> Object Model восстанавливает object hierarchy
 -> Component Model добавляет native- и script-компоненты
 -> Asset System разрешает ссылки на ресурсы
 -> ECS Layer восстанавливает ECS-представление, если оно требуется
 -> Physics Engine пересобирает physics state
 -> Rendering Abstraction пересобирает render-представление сцены
 -> Viewport Runtime Bridge обновляет preview
```

### Схема data flow: переход в play mode

```text
Команда Play
 -> Viewport Runtime Bridge
 -> Scene System активирует runtime-состояние сцены
 -> Scripting Boundary связывает script-компоненты
 -> Managed Runtime Host создает managed peers
 -> ECS Layer запускает runtime systems
 -> Physics Engine запускает simulation step
 -> Rendering Abstraction начинает формирование render commands
 -> активный backend рендерит в embedded viewport
```

### Схема data flow: один runtime frame

```text
Input
 -> Platform Layer
 -> Viewport Runtime Bridge / runtime loop
 -> Scene System распространяет frame context
 -> Object Model / Component Model применяют немедленные изменения
 -> ECS Layer обновляет entities и systems
 -> Physics Engine выполняет simulation step
 -> Scripting Boundary вызывает managed lifecycle callbacks
 -> Managed Runtime Host исполняет C#-поведение
 -> Rendering Abstraction собирает видимое состояние
 -> OpenGL Backend или Vulkan Backend отправляет GPU-команды
 -> кадр показывается в viewport или окне runtime
```

### Схема data flow: граница C++ <-> C#

```text
Создан native object или component
 -> Scene System / Component Model
 -> Scripting Boundary выделяет NativeHandle
 -> BindingRegistry определяет managed type
 -> Managed Runtime Host создает managed peer
 -> lifecycle bridge вызывает callback типа Awake/OnCreate
 -> во время runtime frame вызываются Update-подобные callbacks
 -> managed side обращается к engine API через handles
 -> authoritative state изменяется только на native side
```

### Схема data flow: terrain и generation

```text
Команда редактирования terrain или генерации карты
 -> Editor Tools
 -> Map Generation или Terrain and Landscape
 -> terrain dataset изменяется
 -> Serialization and Persistence записывает terrain state
 -> Physics Engine пересобирает terrain collision
 -> Rendering Abstraction пересобирает terrain render data
 -> Viewport Runtime Bridge обновляет viewport
```

## User flow

### Основные пользовательские сценарии
- создать проект
- открыть проект
- создать сцену
- добавить объект и компоненты
- написать и привязать `C# script`
- включить `play mode`
- редактировать terrain
- сгенерировать карту
- добавить локальный пакет

### Путь пользователя через систему

```text
Запуск Editor
 -> Open/Create Project
 -> Load Packages
 -> Load Assets
 -> Open Scene
 -> Edit Objects/Components
 -> Attach Scripts
 -> Play in Viewport
 -> Save Project/Scene
```

### Связь user flow с модулями
- запуск и открытие проекта: `Editor Shell`, `Project Model`, `Package System`
- работа со сценой: `Editor Tools`, `Scene System`, `Object Model`, `Component Model`
- scripting: `Scripting Boundary`, `Managed Runtime Host`
- `play mode`: `Viewport Runtime Bridge`, `Physics Engine`, `ECS Layer`, `Rendering Abstraction`
- terrain и generation: `Terrain and Landscape`, `Map Generation`
- сохранение: `Serialization and Persistence`, `Asset System`

### Схема user flow и переходов между модулями

```text
Create/Open Project
 -> Editor Shell
 -> Project Model
 -> Package System
 -> Asset System

Create/Open Scene
 -> Scene System
 -> Object Model
 -> Component Model

Edit Scene
 -> Editor Tools
 -> Object Model
 -> Component Model
 -> Terrain and Landscape

Attach Script
 -> Component Model
 -> Scripting Boundary
 -> Managed Runtime Host

Enter Play Mode
 -> Viewport Runtime Bridge
 -> Scene System
 -> ECS Layer
 -> Physics Engine
 -> Rendering Abstraction
 -> OpenGL Backend / Vulkan Backend

Save
 -> Serialization and Persistence
 -> Project Model / Asset System / Scene System / Terrain and Landscape
```

## Ограничения
- запрещено опираться на `cloud` или `SaaS`
- запрещено использовать платные зависимости
- `native core` не должен зависеть от `.NET` как от владельца состояния
- `Object Model` и `ECS Layer` не должны иметь неявное двойное владение данными
- backend-специфичные детали не должны протекать в `gameplay`- и `editor`-контракты
- `Package System` не должен зависеть от внешнего `registry`
- `Qt` не должен использоваться вне `editor-layer` без отдельного согласования

## Нефункциональные требования
- поддержка `Windows`, `macOS`, `Linux`
- детерминированный локальный `workflow`
- архитектурная расширяемость через `packages`
- заменяемый контракт `Physics Engine`
- заменяемая граница `Managed Runtime Host`
- согласованность `editor/runtime` в `play mode`
- воспроизводимая стратегия `serialization`

## Риски архитектуры
- слишком большой `MVP`
- сложная граница между `Scene System`, `Object Model`, `Component Model` и `ECS Layer`
- высокий риск ошибок на границе `C++ <-> C#`
- высокая стоимость поддержки двух graphics backend
- собственная физика может затормозить базовый bootstrap
- `Package System` может стать преждевременным источником сложности
- `Qt` требует дисциплины в лицензионной и архитектурной изоляции

## Открытые вопросы
- какой именно `managed runtime host` будет выбран и на каких условиях лицензирования
- какая стратегия формата данных будет базовой для `project`, `scene` и `assets`
- какой `reload policy` допустим для managed-скриптов
- как именно будет оформлен `sync contract` между `object world` и `ECS world`

## Решение о переходе к модулям
- [x] Project brief согласован.
- [x] Project concept согласован.
- [x] Лицензионные ограничения выяснены и зафиксированы.
- [x] Технологический стек выбран только в архитектуре.
- [x] Для сторонних технологий указаны лицензии или правовой статус.
- [x] Модули системы перечислены.
- [x] Схема взаимодействия модулей описана.
- [x] Data flow описан.
- [x] User flow описан.
- [x] System architecture согласована.
- [x] Можно описывать модули.
