# Спринт 1. День 3

## Общие требования ко всем фичам

1. **Один PR — одна фича.** Не смешивать фичи и не менять файлы чужих контуров (например, `engine/core/include/sky/core/math.hpp` принадлежит E1).
2. **Никаких артефактов сборки в git**: `.exe`, `.o`, `.obj`, `.spv`, каталоги `build/` — запрещены. Временные файлы (`tests/tmp/`) в PR не включать.
3. **Namespace модуля обязателен** (`sky::core`, `sky::rendering`, `sky::object`, `sky::physics`, `sky::ecs`, ...). Код в глобальном namespace не принимается.
4. **Интерфейсы**: секция `public:`, виртуальный деструктор `virtual ~IИмя() = default;`, чисто виртуальные методы (`= 0`).
5. **Сигнатуры из задания копируются символ в символ** — включая `const`, `[[nodiscard]]`, типы возврата и параметры по умолчанию.
6. **Include-стиль**: `#include "sky/<модуль>/<файл>.hpp"` при `-Iengine/<модуль>/include`; пути от корня репозитория запрещены. `<bits/stdc++.h>` запрещён, `#pragma once` обязателен в каждом заголовке.
7. **Хэндлы** — только `core::Handle<Tag>`; в контейнерах ключ — `handle.value`. Собственные `std::hash<Handle>` и операторы в чужие заголовки не добавлять.
8. **Критерий приёмки каждой фичи — её приёмочный тест** (указан в конце блока фичи). PR без зелёного теста не рассматривается.

## feature/object-model

- **Исполнитель:** E1 (Ядро и данные)
- **Порядок реализации:** 1
- **Зависимости:** `feature/math-and-handles` (`core::Transform`, `Handle`)

**Цель фичи:** единственный владелец иерархии сцены и трансформов.

**Описание фичи:** объекты, их дерево и трансформы; остальные модули держат только хендлы.

**Обязательные требования:**

Все объявления — в пространстве имён `sky::object`; объявления копируются символ в символ:

```cpp
// engine/object/include/sky/object/object_model.hpp
struct ObjectTag {};
using ObjectHandle = core::Handle<ObjectTag>;

class IObjectFactory {
public:
    virtual ~IObjectFactory() = default;
    virtual ObjectHandle createObject(const std::string& name) = 0;
    virtual void destroyObject(ObjectHandle object) = 0;
};

class IObjectHierarchyAccess {
public:
    virtual ~IObjectHierarchyAccess() = default;
    virtual void setParent(ObjectHandle child, ObjectHandle parent) = 0;
    [[nodiscard]] virtual ObjectHandle parentOf(ObjectHandle object) const = 0;
    [[nodiscard]] virtual std::vector<ObjectHandle> childrenOf(ObjectHandle object) const = 0;
    virtual void setLocalTransform(ObjectHandle object, const core::Transform& transform) = 0;
    [[nodiscard]] virtual core::Transform localTransform(ObjectHandle object) const = 0;
    [[nodiscard]] virtual core::Transform worldTransform(ObjectHandle object) const = 0;
};

class IObjectQueryService {
public:
    virtual ~IObjectQueryService() = default;
    [[nodiscard]] virtual bool exists(ObjectHandle object) const = 0;
    [[nodiscard]] virtual std::string nameOf(ObjectHandle object) const = 0;
    [[nodiscard]] virtual std::vector<ObjectHandle> findByName(const std::string& name) const = 0;
};

// engine/object/include/sky/object/object_world.hpp
class ObjectWorld : public IObjectFactory,
                    public IObjectHierarchyAccess,
                    public IObjectQueryService {
public:
    ~ObjectWorld() override = default;
    virtual void renameObject(ObjectHandle object, const std::string& name) = 0;
};

std::unique_ptr<ObjectWorld> createObjectWorld();
```

- **КРИТИЧНО — `findByName` возвращает ВСЕ совпадения**: несколько одноимённых объектов → все их хэндлы; ни одного совпадения → ПУСТОЙ вектор. Вариант «первый найденный» — ошибка, не проходящая приёмочный тест.
- **Порядок хэндлов в результате `findByName` не гарантирован** — приёмочный тест сверяет множества, а не последовательности.
- **Одно хранилище** `map<uint64_t, Record>` (ключ — `handle.value`, запись — {локальный трансформ, родитель, дети, имя}); отдельный индекс имён не заводить — `findByName` проходит по этому же хранилищу.
- **Защита от циклов в `setParent`**: перевешивание объекта под самого себя или под собственного потомка отклоняется (иначе `worldTransform` зацикливается по цепочке родителей).
- **`<vector>` включается явно** в `object_model.hpp` — `std::vector` присутствует в сигнатурах контрактов.

**Общий порядок реализации фичи:**
1. Объявить три контракта в `object_model.hpp`.
2. Объявить `ObjectWorld` и фабрику в `object_world.hpp`.
3. Реализовать `ObjectWorld` в `object_world.cpp`.

**Файлы фичи:**
1. `engine/object/include/sky/object/object_model.hpp`
2. `engine/object/include/sky/object/object_world.hpp`
3. `engine/object/src/object_world.cpp`

### Файл: `engine/object/include/sky/object/object_model.hpp`

**Назначение файла:** контракты объектной модели.

**Пошаговое описание действий:**
1. Объявить `ObjectHandle`.
2. Объявить `IObjectFactory`, `IObjectHierarchyAccess`, `IObjectQueryService`.
3. Объявить свободную `setWorldTransform`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `using ObjectHandle = core::Handle<ObjectTag>`
- `class IObjectFactory`
- `class IObjectHierarchyAccess`
- `class IObjectQueryService`

*Функции / методы:*
- `IObjectFactory`: `createObject(const std::string& name)`, `destroyObject(ObjectHandle)`
- `IObjectHierarchyAccess`: `setParent`, `parentOf`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`
- `IObjectQueryService`: `exists`, `nameOf`, `findByName`
- `inline void setWorldTransform(IObjectHierarchyAccess& access, ObjectHandle object, const core::Transform& world)`

*Логика функций / методов:*
- `createObject` — создаёт объект; `destroyObject` — удаляет объект и его поддерево.
- `setParent` — перевешивает `child` под `parent` (invalid = корень); `worldTransform` — композиция локальных вверх по цепочке.
- `exists`/`nameOf`/`findByName` — запросы для чтения.
- `setWorldTransform` — задаёт мировой трансформ, пересчитывая локальный через `invCompose`.

**Результат по файлу:** контракты объектной модели зафиксированы.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/object/include/sky/object/object_world.hpp`

**Назначение файла:** единый мир объектов.

**Пошаговое описание действий:**
1. Объявить `ObjectWorld`, наследующий три контракта.
2. Добавить `renameObject` и фабрику `createObjectWorld`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class ObjectWorld : IObjectFactory, IObjectHierarchyAccess, IObjectQueryService`

*Функции / методы:*
- `virtual void renameObject(ObjectHandle object, const std::string& name) = 0`
- `std::unique_ptr<ObjectWorld> createObjectWorld()`

*Логика функций / методов:*
- `renameObject` — переименовывает объект.
- `createObjectWorld` — фабрика мира объектов.

**Результат по файлу:** интерфейс мира объектов с фабрикой.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/object/src/object_world.cpp`

**Назначение файла:** реализация мира объектов.

**Пошаговое описание действий:**
1. Завести хранилище объектов.
2. Реализовать иерархию и трансформы.
3. Реализовать рекурсивное удаление.

**Что должно быть в файле:**

*Структуры / классы / enum:* скрытый класс-реализация `ObjectWorld`.

*Функции / методы:*
- `createObject`, `destroyObject`, `setParent`, `parentOf`, `childrenOf`, `setLocalTransform`, `localTransform`, `worldTransform`, `exists`, `nameOf`, `findByName`, `renameObject`, `createObjectWorld`.

*Логика функций / методов:*
- хранилище id → {локальный трансформ, родитель, дети, имя}.
- `worldTransform` = `compose` локальных вверх по цепочке родителей.
- `destroyObject` рекурсивно удаляет объект и всё поддерево.

**Результат по файлу:** рабочий мир объектов.

**Критерий правильности по файлу:**
1. `worldTransform` ребёнка корректен относительно повёрнутого родителя.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/object/include/sky/object/object_model.hpp`
2. `engine/object/include/sky/object/object_world.hpp`
3. `engine/object/src/object_world.cpp`

**Общий критерий правильности:**
1. Ребёнок (1,0,0) под родителем, повёрнутым на 90° вокруг Y, в мире = (0,0,-1).

- **Приёмочный тест:** `tests/day3/object_model_tests.cpp` — создание/рекурсивное удаление поддерева, `setParent` с защитой от циклов, `worldTransform` относительно повёрнутого родителя, `findByName` (несколько одноимённых → все хэндлы; ни одного → пустой вектор); код выхода 0.

---

## feature/vulkan-tests

- **Исполнитель:** E2 (Рендеринг)
- **Порядок реализации:** 2
- **Зависимости:** `feature/vulkan-offscreen` (день 2)

**Цель фичи:** проверить Vulkan-рендерер на программном драйвере lavapipe.

**Описание фичи:** автотест закадрового рендера без видеокарты (в CI).

**Обязательные требования:**

Это мета-фича: новых объявлений она не вводит, её обязательные требования — покрытие приёмочных проверок тестом. Тест обязан покрывать:

- **Создание рендерера**: `createVulkanRenderer(W, H)`; если Vulkan-драйвера на машине нет и фабрика вернула `nullptr` — тест завершается штатным пропуском (ранний выход с кодом 0 после `CHECK`-диагностики), а не падением.
- **Размер кадра**: `readbackFrame()` возвращает буфер RGBA8 ровно `W*H*4` байт; `frameWidth()`/`frameHeight()` равны запрошенным размерам.
- **Треугольник**: после отрисовки тестовой геометрии центральный пиксель окрашен геометрией, угловые пиксели остаются цветом фона (очистки) — центр отличается от углов.
- **Регистрация в CI**: тест регистрируется в `tests/CMakeLists.txt` через `add_test` (в этом репозитории — обёртка `sky_add_test(vulkan_tests)` с линковкой `sky::sky_rendering_vulkan`), чтобы `ctest` запускал его на lavapipe.

**Общий порядок реализации фичи:**
1. Написать тест на lavapipe.

**Файлы фичи:**
1. `tests/vulkan_tests.cpp`

### Файл: `tests/vulkan_tests.cpp`

**Назначение файла:** тест Vulkan-рендерера.

**Пошаговое описание действий:**
1. Создать рендерер, отрисовать кадр.
2. Прочитать кадр и сверить пиксели.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:* тестовая функция.

*Логика функций / методов:* `ready()` истинно, кадр рендерится, `readbackFrame()` непустой, центральный пиксель отличается от углового.

**Результат по файлу:** зелёный тест `vulkan_tests`.

**Критерий правильности по файлу:**
1. Тест проходит на lavapipe.

### На выходе должно получиться

**Список артефактов фичи:**
1. `tests/vulkan_tests.cpp`
2. библиотека `sky_rendering_vulkan` собрана; формируется `triangle.png`; тест `vulkan_tests` зелёный на lavapipe

**Общий критерий правильности:**
1. На `triangle.png` центральный пиксель отличается от углового.
2. `readbackFrame()` возвращает непустой массив.

- **Приёмочный тест:** `tests/day3/vulkan_tests_checklist.md` — покрытие `tests/vulkan_tests.cpp` сверяется по чек-листу (создание рендерера / штатный пропуск без драйвера, размер `readbackFrame` = W\*H\*4, центр окрашен / углы фон, регистрация через `add_test` в `tests/CMakeLists.txt`); приёмка — все пункты чек-листа отмечены.

---

## feature/engine-bridge

- **Исполнитель:** E3 (Редактор .NET)
- **Порядок реализации:** 3
- **Зависимости:** `feature/c-abi-seed` (`libsky_editor_bridge.so`, ниже в этом дне)

**Цель фичи:** первичная связь редактора с движком через C-интерфейс (P/Invoke).

**Описание фичи:** загрузка нативного моста и создание/уничтожение сессии движка из .NET — фундамент всех дальнейших вызовов.

**Обязательные требования:**

Точный список P/Invoke-объявлений этого дня (`DllImport`, объявления копируются символ в символ):

```csharp
// editor/avalonia/Engine/EngineInterop.cs
internal static class EngineInterop
{
    private const string Lib = "sky_editor_bridge";

    static EngineInterop()
    {
        NativeLibrary.SetDllImportResolver(typeof(EngineInterop).Assembly, Resolve);
    }

    private static IntPtr Resolve(string name, Assembly assembly, DllImportSearchPath? path);
    private static string[] Candidates();

    [DllImport(Lib)] public static extern IntPtr sky_editor_create();
    [DllImport(Lib)] public static extern void sky_editor_destroy(IntPtr ctx);
    [DllImport(Lib)] public static extern int sky_editor_root_count(IntPtr ctx);
    [DllImport(Lib)] public static extern ulong sky_editor_root_at(IntPtr ctx, int index);
    [DllImport(Lib)] public static extern int sky_editor_object_name(IntPtr ctx, ulong obj, byte[] buffer, int capacity);

    public static string ReadString(Func<byte[], int, int> call);
}
```

- **Маршаллинг**: `SkyEditorContext*` → `IntPtr`, `SkyObjectId` (uint64) → `ulong`, `int32_t` → `int`; строки читаются через каллер-владеющий буфер `byte[]` + `int capacity` (UTF-8), декодирование — хелпером `ReadString` двухфазным запросом длины.
- **Резолвер**: `NativeLibrary.SetDllImportResolver` + `Candidates()` — поиск `libsky_editor_bridge.so` (`.dylib`/`.dll` по ОС) сначала по `SKY_BRIDGE_PATH`, затем рядом со сборкой и в дереве сборки CMake (`build/editor/native_bridge`).
- **`EditorSession : IDisposable`**: конструктор вызывает `sky_editor_create()` и бросает исключение при `IntPtr.Zero`; `Dispose()` вызывает `sky_editor_destroy` ровно один раз и зануляет указатель; свойство `IntPtr Native` отдаёт нативный хэндл сессии.
- **Нативная библиотека — фича `feature/c-abi-seed`** (E5, этот же день, порядок реализации ПОЗЖЕ: 5 против 3): работать строго по контракту заголовка `editor_bridge.h`; критерий «библиотека находится и загружается» проверяется после мержа E5, до этого достаточно `dotnet build` без ошибок.

**Общий порядок реализации фичи:**
1. Объявить P/Invoke и резолвер в `EngineInterop.cs`.
2. Обернуть сессию в `EditorSession.cs`.

**Файлы фичи:**
1. `editor/avalonia/Engine/EngineInterop.cs`
2. `editor/avalonia/Engine/EditorSession.cs`

### Файл: `editor/avalonia/Engine/EngineInterop.cs`

**Назначение файла:** P/Invoke-объявления и резолвер нативной библиотеки.

**Пошаговое описание действий:**
1. Объявить `sky_editor_create`/`destroy`.
2. Реализовать резолвер библиотеки.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `static class EngineInterop`

*Функции / методы:*
- `[DllImport] static extern IntPtr sky_editor_create()`
- `[DllImport] static extern void sky_editor_destroy(IntPtr ctx)`
- `static IntPtr Resolve(...)`
- `static string[] Candidates()`

*Логика функций / методов:*
- `sky_editor_create` — возвращает указатель на сессию движка; `sky_editor_destroy` — уничтожает сессию.
- `Resolve`/`Candidates` — находят `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в дереве сборки.

**Результат по файлу:** доступ к нативному мосту из .NET.

**Критерий правильности по файлу:**
1. Библиотека моста находится и загружается.

### Файл: `editor/avalonia/Engine/EditorSession.cs`

**Назначение файла:** обёртка над сессией движка.

**Пошаговое описание действий:**
1. В конструкторе создать сессию.
2. Реализовать `Dispose`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class EditorSession : IDisposable`

*Функции / методы:*
- конструктор `EditorSession()`
- `Dispose()`
- свойство `IntPtr Native`

*Логика функций / методов:*
- конструктор вызывает `sky_editor_create` и проверяет не-null.
- `Dispose()` вызывает `sky_editor_destroy`.
- `Native` — нативный указатель сессии.

**Результат по файлу:** управляемая обёртка сессии.

**Критерий правильности по файлу:**
1. Сессия создаётся (не-null) и освобождается в `Dispose`.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/avalonia/Engine/EngineInterop.cs`
2. `editor/avalonia/Engine/EditorSession.cs`

**Общий критерий правильности:**
1. `dotnet build` — 0 ошибок.
2. Сессия движка создаётся из .NET (не-null).

- **Приёмочный тест:** `tests/day3/engine_bridge_check.py` — `python3 engine_bridge_check.py <корень_репозитория>`: сверяет P/Invoke-объявления пяти функций дня, резолвер (`SKY_BRIDGE_PATH`, кандидаты) и `Dispose`-семантику `EditorSession`; код выхода 0.

---

## feature/player-runtime

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 4
- **Зависимости:** рендерер (`feature/vulkan-offscreen`, день 2) — доступен уже сейчас. Полный плеер требует `EditorContext` (`feature/editor-context`) и построитель кадра (`feature/frame-builder`) — обе фичи появляются ПОЗЖЕ этого дня, поэтому в этот день реализуется ядро (см. обязательные требования), остальное — заготовка.

**Цель фичи:** автономный проигрыватель с собственным циклом и режимами запуска.

**Описание фичи:** проигрыватель с точкой входа, безоконным и оконным режимами, разбором аргументов и переносимым кодом клавиш. Этап 2 контура E4.

**Обязательные требования:**

Полный цикл плеера (`tickFrame → build → renderFrame`) зависит от `EditorContext` и построителя кадра, которых в этот день ещё нет. Ядро этого дня — таблица маппинга клавиш (в этом репозитории живёт в `player/src/main.cpp`); её сигнатура и таблица копируются символ в символ:

```cpp
// Platform keysym -> the engine's portable key codes (SkyEngine.KeyCode):
// ASCII uppercase for letters/digits, Space = 32, named keys from 256.
int mapPlatformKey(std::int32_t keysym) {
    if (keysym >= 'a' && keysym <= 'z') return keysym - 'a' + 'A';
    if ((keysym >= 'A' && keysym <= 'Z') || (keysym >= '0' && keysym <= '9') ||
        keysym == ' ') {
        return keysym;
    }
    switch (keysym) { // X11 keysyms (Cocoa layer reports the same values)
        case 0xff1b: return 256; // Escape
        case 0xff0d: return 257; // Enter
        case 0xff09: return 258; // Tab
        case 0xffe1: return 259; // Shift_L
        case 0xffe3: return 260; // Control_L
        case 0xffe9: return 261; // Alt_L
        case 0xff51: return 262; // Left
        case 0xff53: return 263; // Right
        case 0xff52: return 264; // Up
        case 0xff54: return 265; // Down
        default: return 0;
    }
}

int runHeadless(EditorContext& context, int frames, const char* screenshotPath);
int runWindowed(EditorContext& context, int frameLimit);
int main(int argc, char** argv);
```

- **Таблица маппинга — единственная точка перевода** платформенных keysym в переносимые коды; значения (буквы/цифры — ASCII в верхнем регистре, Space = 32, именованные с 256: Escape=256 … Down=265, неизвестный keysym → 0) согласованы с C#-стороной (`sky_editor_set_key_state`) и не меняются.
- **`main` пишется сейчас**: разбор `--frames N`, `--headless out.png`, `--scene path` и возврат кода выхода не зависят от отсутствующих фич.
- **`runHeadless`/`runWindowed` в этот день — заготовки** с точными сигнатурами; полный цикл `tickFrame → build → renderFrame`, сохранение PNG и оконный режим дописываются после мержа `feature/editor-context` и `feature/frame-builder`.
- **Собственные дубли `EditorContext`/`FrameBuilder` не заводить** — заготовки работают по будущим заголовкам, а не по локальным копиям.

**Общий порядок реализации фичи:**
1. Реализовать `main` с разбором `--frames N`, `--headless out.png`, `--scene path`.
2. Реализовать `runHeadless` (цикл `tickFrame → build → renderFrame`, сохранение PNG).
3. Реализовать `runWindowed` и `mapPlatformKey`.

**Файлы фичи:**
1. `player/src/main.cpp`

### Файл: `player/src/main.cpp`

**Назначение файла:** точка входа и цикл автономного проигрывателя.

**Пошаговое описание действий:**
1. Реализовать `int main(int argc, char** argv)` — разбор `--frames N`, `--headless out.png`, `--scene path`.
2. Реализовать `int runHeadless(EditorContext&, int frames, const char* screenshotPath)`.
3. Реализовать `int runWindowed(EditorContext&, int frameLimit)`.
4. Реализовать `int mapPlatformKey(std::int32_t keysym)`.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет.

*Функции / методы:*
- `int main(int argc, char** argv)`
- `int runHeadless(EditorContext& context, int frames, const char* screenshotPath)`
- `int runWindowed(EditorContext& context, int frameLimit)`
- `int mapPlatformKey(std::int32_t keysym)`

*Логика функций / методов:*
- `main` — точка входа; разбирает `--frames N`, `--headless out.png`, `--scene path`. Возвращает: код выхода.
- `runHeadless` — безоконный прогон — цикл `tickFrame → build → renderFrame`, сохранение PNG. Параметры: `context`, `frames` — число кадров, `screenshotPath` — файл. Возвращает: код выхода.
- `runWindowed` — оконный прогон (окно X11/Cocoa + swapchain-рендерер). Параметры: `context`, `frameLimit`. Возвращает: код выхода.
- `mapPlatformKey` — переводит платформенный код клавиши в переносимый (общий с C#). Возвращает: переносимый код или 0.

**Результат по файлу:** бинарь `sky_player`; безоконный режим пишет PNG.

**Критерий правильности по файлу:**
1. `sky_player --headless out.png` формирует изображение кадра.

### На выходе должно получиться

**Список артефактов фичи:**
1. `player/src/main.cpp`
2. бинарь `sky_player`; безоконный режим пишет PNG.

**Общий критерий правильности:**
1. `sky_player --headless out.png` формирует изображение кадра.

- **Приёмочный тест:** `tests/day3/player_runtime_tests.cpp` — проверяет ядро дня: таблицу `mapPlatformKey` (буквы/цифры → ASCII в верхнем регистре, Space = 32, именованные клавиши 256–265, неизвестный keysym → 0) и разбор аргументов `--frames`/`--headless`/`--scene`; код выхода 0.

---

## feature/c-abi-seed

- **Исполнитель:** E5 (Пайплайн и QA, совместно с E1)
- **Порядок реализации:** 5
- **Зависимости:** модуль `object` (`feature/object-model`) для перечисления корней

**Цель фичи:** первичный плоский C-интерфейс движка `sky_editor_*`.

**Описание фичи:** плоский набор C-функций, через который .NET-редактор общается с C++-движком; на этом этапе минимум — сессия и перечисление корней.

**Обязательные требования:**

Сигнатуры этого дня из `editor_bridge.h` (копируются символ в символ):

```c
/* editor/native_bridge/include/sky/editor/bridge/editor_bridge.h */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define SKY_BRIDGE_API __declspec(dllexport)
#else
#define SKY_BRIDGE_API __attribute__((visibility("default")))
#endif

/* Opaque editor session. */
typedef struct SkyEditorContext SkyEditorContext;

/* An object id; 0 is the invalid handle. */
typedef uint64_t SkyObjectId;

SKY_BRIDGE_API SkyEditorContext* sky_editor_create(void);
SKY_BRIDGE_API void sky_editor_destroy(SkyEditorContext* ctx);
SKY_BRIDGE_API int32_t sky_editor_root_count(SkyEditorContext* ctx);
SKY_BRIDGE_API SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index);
SKY_BRIDGE_API int32_t sky_editor_object_name(SkyEditorContext* ctx,
                                              SkyObjectId object, char* buffer,
                                              int32_t capacity);

#ifdef __cplusplus
}
#endif
```

- **Весь интерфейс — `extern "C"` + явный экспорт `SKY_BRIDGE_API`**: без C++ name mangling, иначе P/Invoke не найдёт символы; заголовок должен включаться и из C, и из C++.
- **`SkyObjectId` = `uint64_t`; значение 0 — невалидный хэндл**: `sky_editor_root_at` с индексом вне диапазона возвращает 0; функции с плохим `ctx`/хэндлом не падают.
- **Владение сессией**: `sky_editor_create` собирает движок с демо-сценой и передаёт владение вызывающему; освобождение — только парным `sky_editor_destroy`; повторный destroy и чужие указатели контрактом не поддерживаются.
- **Контракт буфера имени `sky_editor_object_name`**: пишет в `buffer` NUL-терминированную строку, при нехватке `capacity` — усечённую, но возвращает ПОЛНУЮ длину имени независимо от усечения (двухфазный запрос: сначала узнать длину, затем прочитать в буфер достаточного размера); null-буфер (или `capacity` 0) → ничего не писать, вернуть длину.

**Общий порядок реализации фичи:**
1. Объявить C-функции в `editor_bridge.h`.
2. Реализовать их в `editor_bridge.cpp`.

**Файлы фичи:**
1. `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`
2. `editor/native_bridge/src/editor_bridge.cpp`

### Файл: `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`

**Назначение файла:** объявления плоского C-интерфейса.

**Пошаговое описание действий:**
1. Объявить `create`/`destroy` сессии.
2. Объявить перечисление корней и чтение имени.

**Что должно быть в файле:**

*Структуры / классы / enum:* непрозрачный тип `SkyEditorContext`; тип `SkyObjectId`.

*Функции / методы:*
- `SkyEditorContext* sky_editor_create(void)`
- `void sky_editor_destroy(SkyEditorContext* ctx)`
- `int32_t sky_editor_root_count(SkyEditorContext* ctx)`
- `SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index)`
- `int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)`

*Логика функций / методов:*
- `sky_editor_create` — возвращает указатель на сессию (собирает движок и демо-сцену).
- `sky_editor_destroy` — уничтожает сессию.
- `sky_editor_root_count` — число корневых объектов.
- `sky_editor_root_at` — id корневого объекта.
- `sky_editor_object_name` — пишет имя в буфер; возвращает длину.

**Результат по файлу:** заголовок C-интерфейса.

**Критерий правильности по файлу:**
1. Заголовок пригоден для P/Invoke из .NET.

### Файл: `editor/native_bridge/src/editor_bridge.cpp`

**Назначение файла:** реализация C-интерфейса.

**Пошаговое описание действий:**
1. Реализовать сборку движка и демо-сцены в сессии.
2. Реализовать перечисление корней и чтение имени.

**Что должно быть в файле:**

*Структуры / классы / enum:* определение `SkyEditorContext` (сессия).

*Функции / методы:*
- `sky_editor_create`, `sky_editor_destroy`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_object_name`.

*Логика функций / методов:*
- `create` собирает движок и демо-сцену в сессии; `destroy` уничтожает; `root_count`/`root_at` перечисляют корни; `object_name` пишет имя в буфер и возвращает длину.

**Результат по файлу:** библиотека `libsky_editor_bridge.so`.

**Критерий правильности по файлу:**
1. `sky_editor_create` возвращает не-null сессию.

### На выходе должно получиться

**Список артефактов фичи:**
1. `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`
2. `editor/native_bridge/src/editor_bridge.cpp`
3. библиотека `libsky_editor_bridge.so`

**Общий критерий правильности:**
1. Красный CI блокирует слияние; сломанный тест краснеет.
2. Редактор E3 вызывает `sky_editor_create` и получает не-null сессию.

- **Приёмочный тест:** `tests/day3/c_abi_seed_tests.cpp` — линкуется с `libsky_editor_bridge.so`: create/destroy сессии, перечисление корней (`root_count`/`root_at`, невалидный индекс → 0), двухфазное чтение имени (полная длина при усечении, NUL-терминация, null-буфер → длина); код выхода 0.

---

## feature/ecs-object-sync

- **Исполнитель:** E6 (Data-oriented / ECS)
- **Порядок реализации:** 6
- **Зависимости:** `feature/ecs-core` (Спринт 1), `feature/object-model` (этот же день, мержится ПЕРВОЙ). Интеграция в `scene_world.cpp` — отдельный шаг ПОСЛЕ этого дня (совместно с E1) и в объём фичи не входит.

**Цель фичи:** явный контракт синхронизации ECS с объектным миром.

**Описание фичи:** `IEcsObjectSync` связывает объекты и сущности и переносит трансформ вокруг такта (push до, pull после), без неявного двойного владения. Этап 2 контура E6.

**Обязательные требования:**

Объявления из `engine/ecs/include/sky/ecs/object_sync.hpp` (копируются символ в символ):

```cpp
// engine/ecs/include/sky/ecs/object_sync.hpp
struct EcsTransform {
    core::Transform value;
};

class IEcsObjectSync {
public:
    virtual ~IEcsObjectSync() = default;

    /// Creates (or reuses) the entity bound to the object.
    virtual EntityId bind(object::ObjectHandle object) = 0;
    virtual void unbind(object::ObjectHandle object) = 0;
    [[nodiscard]] virtual EntityId entityOf(object::ObjectHandle object) const = 0;
    [[nodiscard]] virtual object::ObjectHandle objectOf(EntityId entity) const = 0;

    virtual void pushAuthoringState() = 0;
    virtual void pullEcsResults() = 0;
};

std::unique_ptr<IEcsObjectSync> createEcsObjectSync(
    EcsWorld& ecs, object::IObjectHierarchyAccess& hierarchy);
```

- **Все объявления — в `namespace sky::ecs`**; заголовок включает `sky/core/math.hpp`, `sky/ecs/ecs_world.hpp` и `sky/object/object_model.hpp`.
- **`feature/object-model` мержится в этот же день** — мерж-порядок: сначала E1 (object-model), затем эта фича; до мержа E1 работать по контракту `object_model.hpp` из блока E1 этого задания.
- **Интеграция в `scene_world.cpp` — НЕ в этот день**: точка интеграции (цикл такта `pushAuthoringState() → tick(dt) → pullEcsResults()`) фиксируется в этом контракте, а правка `scene_world.cpp` выполняется совместно с E1 отдельным шагом позже.
- **Данные пересекают границу миров только через этот контракт**: `pushAuthoringState()` до такта копирует локальный трансформ привязанных объектов в компонент `EcsTransform`; `pullEcsResults()` после такта пишет результат обратно в объектный мир; ни один мир не лезет в хранилище другого — двойного владения нет.
- **Двустороннее отображение**: ключи контейнеров — `handle.value`/`entity.value`; повторный `bind` того же объекта возвращает уже связанную сущность, `unbind` убирает обе стороны связи.

**Общий порядок реализации фичи:**
1. Объявить `EcsTransform` и `IEcsObjectSync` в `object_sync.hpp`.
2. Объявить фабрику `createEcsObjectSync`.
3. Реализовать привязку и двустороннюю синхронизацию в `object_sync.cpp`.

**Файлы фичи:**
1. `engine/ecs/include/sky/ecs/object_sync.hpp`
2. `engine/ecs/src/object_sync.cpp`

### Файл: `engine/ecs/include/sky/ecs/object_sync.hpp`

**Назначение файла:** контракт синхронизации ECS↔объектный мир.

**Пошаговое описание действий:**
1. Объявить `struct EcsTransform { core::Transform value; }`.
2. Объявить `class IEcsObjectSync` с методами связывания и синхронизации.
3. Объявить фабрику `createEcsObjectSync`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct EcsTransform { core::Transform value; }`
- `class IEcsObjectSync`

*Функции / методы:*
- `virtual EntityId bind(object::ObjectHandle object) = 0`
- `virtual void unbind(object::ObjectHandle object) = 0`
- `virtual EntityId entityOf(object::ObjectHandle object) const = 0`
- `virtual object::ObjectHandle objectOf(EntityId entity) const = 0`
- `virtual void pushAuthoringState() = 0`
- `virtual void pullEcsResults() = 0`
- `std::unique_ptr<IEcsObjectSync> createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)`

*Логика функций / методов:*
- `EcsTransform` — базовый компонент трансформа.
- `bind(object)` — связывает объект с сущностью. Возвращает: сущность.
- `unbind(object)` — разрывает связь.
- `entityOf(object)` — Возвращает: сущность по объекту.
- `objectOf(entity)` — Возвращает: объект по сущности.
- `pushAuthoringState()` — до такта переносит трансформ объекта в `EcsTransform`.
- `pullEcsResults()` — после такта переносит результат обратно в объектный мир.
- `createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)` — фабрика.

**Результат по файлу:** контракт синхронизации зафиксирован.

**Критерий правильности по файлу:**
1. Заголовок компилируется; использует `EntityId` и `object::ObjectHandle`.

### Файл: `engine/ecs/src/object_sync.cpp`

**Назначение файла:** реализация синхронизации ECS↔объектный мир.

**Пошаговое описание действий:**
1. Реализовать привязку сущность↔объект (`bind`/`unbind`/`entityOf`/`objectOf`).
2. Реализовать `pushAuthoringState` и `pullEcsResults`.
3. Дать фабрику `createEcsObjectSync`; точка интеграции — цикл такта в `scene_world.cpp` (`pushAuthoringState()` → `tick(dt)` → `pullEcsResults()`), сама правка `scene_world.cpp` — позже, вне этой фичи.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- скрытый класс-реализация `IEcsObjectSync`.

*Функции / методы:*
- `bind`, `unbind`, `entityOf`, `objectOf`, `pushAuthoringState`, `pullEcsResults`, `createEcsObjectSync`.

*Логика функций / методов:*
- `bind(object)` — создаёт/находит сущность для объекта, ведёт двустороннее отображение; возвращает сущность.
- `unbind(object)` — убирает связь объекта и сущности.
- `entityOf`/`objectOf` — читают отображение в обе стороны.
- `pushAuthoringState()` — до такта переносит трансформ объекта в `EcsTransform`.
- `pullEcsResults()` — после такта переносит результат обратно в объектный мир.
- `createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)` — создаёт реализацию; интеграция в цикл такта `scene_world.cpp` (`pushAuthoringState()` → `tick(dt)` → `pullEcsResults()`) выполняется совместно с E1 позже, вне этой фичи.

**Результат по файлу:** рабочая двусторонняя синхронизация вокруг такта.

**Критерий правильности по файлу:**
1. Объект, обработанный ECS-системой, получает изменённый трансформ в объектном мире; при паузе не меняется.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/ecs/include/sky/ecs/object_sync.hpp`
2. `engine/ecs/src/object_sync.cpp`
3. привязка сущность↔объект; двусторонняя синхронизация вокруг такта; тест в `integrity_tests`.

**Общий критерий правильности:**
1. Объект, обработанный ECS-системой, получает изменённый трансформ в объектном мире.
2. При паузе трансформ не меняется.

- **Приёмочный тест:** `tests/day3/ecs_object_sync_tests.cpp` — `bind`/`unbind`/`entityOf`/`objectOf` (двустороннее отображение, повторный `bind` → та же сущность), `pushAuthoringState`/`pullEcsResults` вокруг такта (система меняет `EcsTransform` → объект получает новый трансформ), на паузе трансформ не меняется; код выхода 0.
