# Sky Engine

Sky Engine — коммерческий Unity-подобный игровой движок с архитектурой `native-first`:
ядро на `C++20` владеет всем состоянием движка, `C#/.NET` используется как изолированный
managed-слой для scripting. Движок работает полностью локально (`offline/local-first`).

Подробности:

- Архитектура: [`docs/architecture/system-architecture.md`](docs/architecture/system-architecture.md)
- Модули: [`docs/modules/modules.md`](docs/modules/modules.md)

## Структура репозитория

```text
engine/                 native-ядро (C++20)
  platform/             Platform Layer: окно, ввод, файлы, потоки, таймеры
  core/                 Core Foundation: логи, конфигурация, события, jobs
  project/              Project Model
  package/              Package System
  asset/                Asset System
  serialization/        Serialization and Persistence
  scene/                Scene System
  object/               Object Model
  component/            Component Model
  ecs/                  ECS Layer
  physics/              Physics Engine
  rendering/            Rendering Abstraction
  rendering_opengl/     OpenGL Backend
  rendering_vulkan/     Vulkan Backend
  scripting/            Scripting Boundary (native-сторона)
  terrain/              Terrain and Landscape
  mapgen/               Map Generation
editor/                 editor-layer (C++20 + Qt5, изолирован от engine core)
  shell/                Editor Shell
  tools/                Editor Tools
  viewport_bridge/      Viewport Runtime Bridge
managed/                Managed Runtime Host (C#/.NET)
tests/                  проверочные цели сборки
docs/                   согласованные проектные документы
```

## Сборка

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Опции:

| Опция | По умолчанию | Назначение |
| --- | --- | --- |
| `SKY_BUILD_EDITOR` | `OFF` | сборка editor-layer (требует Qt5) |
| `SKY_BUILD_OPENGL_BACKEND` | `ON` | сборка контрактного скелета OpenGL backend |
| `SKY_BUILD_VULKAN_BACKEND` | `ON` | сборка контрактного скелета Vulkan backend |

## Статус

Текущий этап — контрактный скелет: публичные интерфейсы всех модулей
зафиксированы в заголовках в соответствии с согласованными документами,
реализация наращивается поверх этих контрактов.

## Лицензионная политика зависимостей

Допустимы только `MIT`, `BSD`, `Apache-2.0`. `LGPL` разрешена исключительно для `Qt`
в editor-layer. `GPL`, `AGPL`, `MPL`, коммерческие SDK и SaaS запрещены.
