# Спринт 2. День 3

## feature/editor-context

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/object-model`, `feature/component-model` (Спринт 1), `feature/scene-world`, `feature/scene-authoring`

**Цель фичи:** сборочная точка `EditorContext` — собирает движок в один объект и строит демо-сцену.

**Описание фичи:** `EditorContext` создаёт подсистемы, строит демо-сцену из примитивов, света и камеры и отдаёт корневые объекты (нужно E3 для дерева, E2 для обхода). Третья фича Этапа 2 контура E1, закрывает веху M1.

**Общий порядок реализации фичи:**
1. Объявить `class EditorContext` в `editor_context.hpp`.
2. Реализовать конструктор (создание подсистем + `buildDemoScene()`) в `editor_context.cpp`.
3. Реализовать `rootObjects`, `createEmpty`, `createPrimitive` и приватный `buildDemoScene`.

**Файлы фичи:**
1. `editor/shell/src/editor_context.hpp`
2. `editor/shell/src/editor_context.cpp`

### Файл: `editor/shell/src/editor_context.hpp`

**Назначение файла:** объявление сборочной точки редактора.

**Пошаговое описание действий:**
1. Объявить `class EditorContext`.
2. Объявить конструктор, `rootObjects`, `createEmpty`, `createPrimitive`.
3. Объявить приватный `buildDemoScene()`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class EditorContext` — собирает движок в один объект и строит демо-сцену.

*Функции / методы:*
- `EditorContext()`
- `std::vector<object::ObjectHandle> rootObjects() const`
- `object::ObjectHandle createEmpty(const std::string& name)`
- `object::ObjectHandle createPrimitive(scene::PrimitiveKind kind, const std::string& name)`
- приватный `void buildDemoScene()`

*Логика функций / методов:*
- `EditorContext()` — конструктор: создаёт подсистемы, вызывает `buildDemoScene()`.
- `rootObjects() const` — Возвращает: корневые объекты (нужно E3 для дерева, E2 для обхода).
- `createEmpty(name)` — Возвращает: хэндл пустого объекта.
- `createPrimitive(kind, name)` — Возвращает: хэндл примитива.
- `buildDemoScene()` — наполняет сцену примитивами, светом, камерой.

**Результат по файлу:** контракт сборочной точки зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется; методы используют `object::ObjectHandle`/`scene::PrimitiveKind`.

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** реализация сборочной точки и демо-сцены.

**Пошаговое описание действий:**
1. Реализовать конструктор: создать подсистемы, вызвать `buildDemoScene()`.
2. Реализовать `rootObjects`, `createEmpty`, `createPrimitive`.
3. Реализовать `buildDemoScene()` (примитивы, свет, камера).

**Что должно быть в файле:**

*Структуры / классы / enum:*
- реализация `EditorContext` (владеет подсистемами движка).

*Функции / методы:*
- `EditorContext`, `rootObjects`, `createEmpty`, `createPrimitive`, `buildDemoScene`.

*Логика функций / методов:*
- `EditorContext()` — создаёт подсистемы (миры объектов/компонентов, сцену и т.д.) и вызывает `buildDemoScene()`.
- `rootObjects()` — возвращает корневые объекты демо-сцены.
- `createEmpty(name)` — создаёт пустой объект, возвращает его хэндл.
- `createPrimitive(kind, name)` — создаёт примитив (через авторинг сцены), возвращает хэндл.
- `buildDemoScene()` — наполняет сцену примитивами, светом и камерой.

**Результат по файлу:** `EditorContext` строит демо-сцену.

**Критерий правильности по файлу:**
1. `rootObjects()` возвращает именованные объекты демо-сцены.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/shell/src/editor_context.hpp`
2. `editor/shell/src/editor_context.cpp`
3. библиотека `sky_scene` собрана; `EditorContext` строит демо-сцену.

**Общий критерий правильности:**
1. `rootObjects()` возвращает именованные объекты демо-сцены.
2. Сцена пригодна для обхода рендером контура E2.
