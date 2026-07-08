# Обзор по неделям (кросс-срез контуров E1–E6)

Что делает каждый контур в конкретную неделю: его этап и фичи (`feature/<название>`). Полные сигнатуры и описания методов — в ТЗ контуров ([role-E1.md](role-E1.md) … [role-E6.md](role-E6.md)).


## Неделя 1

### E1 · Ядро и данные — Математика, объектная и компонентная модели
- `feature/math-and-handles` — Математика и типобезопасные идентификаторы — фундамент, от которого зависят все.
- `feature/core-services` — Единый журнал и служба конфигурации — используются всеми подсистемами.
- `feature/object-model` — Единственный владелец иерархии сцены и трансформов.
- `feature/component-model` — Компоненты с полями-данными — одна инфраструктура для Inspector, undo, сцен и скриптов.
- `feature/core-tests`

### E2 · Рендеринг — Vulkan от инициализации до кадра в файле
- `feature/render-contract` — Общий (не только Vulkan) контракт рендера — поток команд и интерфейс рендерера.
- `feature/vulkan-offscreen` — Инициализация Vulkan, закадровая цель, конвейер, отрисовка треугольника, чтение кадра.
- `feature/vulkan-tests`

### E3 · Редактор (.NET) — Каркас, компоновка панелей, связь с движком
- `feature/editor-shell`
- `feature/docking-layout`
- `feature/engine-bridge`

### E4 · Рантайм и скриптинг — Физическая симуляция
- `feature/physics-world`
- `feature/physics-object-sync`

### E5 · Пайплайн, ассеты, пакеты — Сборка, интеграция, тесты, первичный C-интерфейс
- `feature/build-system`
- `feature/test-harness`
- `feature/c-abi-seed`

### E6 · Data-oriented (ECS) — ECS-мир и планировщик систем
- `feature/ecs-core`
- `feature/ecs-tests`


## Неделя 2

### E1 · Ядро и данные — Сцена и сборочная точка
- `feature/scene-world`
- `feature/scene-authoring`
- `feature/editor-context`

### E2 · Рендеринг — Меши, камера, освещение, построитель кадра
- `feature/vulkan-mesh-lighting`
- `feature/frame-builder`

### E3 · Редактор (.NET) — Панель вьюпорта и живое дерево объектов
- `feature/vulkan-viewport`
- `feature/hierarchy-tree`

### E4 · Рантайм и скриптинг — Автономный проигрыватель
- `feature/player-runtime`

### E5 · Пайплайн, ассеты, пакеты — Режим скриншотов и первые тесты
- `feature/ci-screenshot`

### E6 · Data-oriented (ECS) — Синхронизация с объектным миром
- `feature/ecs-object-sync`


## Недели 3–4

### E1 · Ядро и данные — Отмена операций и формат сцены SKYB
- `feature/undo-stack`
- `feature/object-snapshot`
- `feature/skyb-serialization`
- `feature/undo-scene-tests`

### E2 · Рендеринг — Выбор объекта, проекция, небо
- `feature/editor-camera`
- `feature/sky-backdrop`

### E3 · Редактор (.NET) — Гизмо, инспектор, панели данных
- `feature/gizmos`
- `feature/inspector`
- `feature/data-panels`

### E4 · Рантайм и скриптинг — Режим воспроизведения и ввод
- `feature/play-mode`
- `feature/input-state`

### E5 · Пайплайн, ассеты, пакеты — Импортёры, виртуальная ФС, тесты интерфейса
- `feature/asset-database`
- `feature/importers-obj-png`
- `feature/vfs-and-bridge-tests`

### E6 · Data-oriented (ECS) — Прикладные системы поверх ECS
- `feature/ecs-systems`


## Недели 5–6

### E1 · Ядро и данные — Восстановление физики из сцены
- `feature/physics-reattach`

### E2 · Рендеринг — Текстуры и слоты PBR
- `feature/pbr-textures`
- `feature/procedural-primitives`

### E3 · Редактор (.NET) — Выбор меша, перетаскивание, режим 2D
- `feature/mesh-picker`
- `feature/dragdrop-2d`

### E4 · Рантайм и скриптинг — Подсистема скриптинга
- `feature/dotnet-host`
- `feature/managed-runtime`
- `feature/scripting-integration`

### E5 · Пайплайн, ассеты, пакеты — Импортёры FBX/glTF, запуск сцены проигрывателем
- `feature/importers-fbx-gltf`

### E6 · Data-oriented (ECS) — Многопоточное исполнение систем
- `feature/ecs-multithreading`


## Недели 7–8

### E1 · Ядро и данные — Префабы
- `feature/prefabs`

### E2 · Рендеринг — Тени
- `feature/shadow-mapping`

### E3 · Редактор (.NET) — Выбор класса скрипта и его поля
- `feature/script-inspector`

### E4 · Рантайм и скриптинг — Пользовательские сборки и игровой интерфейс
- `feature/user-assemblies`
- `feature/gameplay-api`

### E5 · Пайплайн, ассеты, пакеты — Менеджер пакетов
- `feature/package-resolver`
- `feature/package-lock-and-install`
- `feature/package-tests`

### E6 · Data-oriented (ECS) — Профилирование систем
- `feature/ecs-profiling`
