# Спринт 5. День 2

## feature/gameplay-api

- **Исполнитель:** E4 (Рантайм и физика)
- **Порядок реализации:** 1
- **Зависимости:** `feature/user-assemblies` (загрузка пользовательских сборок); подсистема скриптинга и обратный API движка из Этапа 4 контура E4

**Цель фичи:** расширение API движка функциями геймплея (позиция, спавн, уничтожение, скорость, луч).

**Описание фичи:** таблица обратного API растёт до 12 указателей; скриптам становятся доступны `Instantiate`/`Destroy`/velocity/raycast, а нативная сторона реализует соответствующие колбэки и физический луч по heightfield.

**Общий порядок реализации фичи:**
1. Расширить таблицу обратного API в `Engine.cs`.
2. Добавить геймплейные методы в `ScriptComponent.cs`.
3. Добавить `Physics.Raycast` в `Physics.cs`.
4. Реализовать нативные колбэки и raycast по heightfield в `editor_context.cpp`.

**Файлы фичи:**
1. `managed/SkyEngine.Managed/Engine.cs`
2. `managed/SkyEngine.Managed/ScriptComponent.cs`
3. `managed/SkyEngine.Managed/Physics.cs`
4. `editor/shell/src/editor_context.cpp`

### Файл: `managed/SkyEngine.Managed/Engine.cs`

**Назначение файла:** дополнение таблицы делегатов обратного API геймплейными функциями.

**Пошаговое описание действий:**
1. Расширить таблицу обратного API до 12 указателей.

**Что должно быть в файле:**

*Структуры / классы / enum:* дополнение таблицы делегатов обратного API.

*Функции / методы:*
- делегаты `GetWorldPosition`, `Instantiate`, `DestroyObject`, `SetVelocity`, `GetVelocity`, `Raycast`.

*Логика функций / методов:*
- таблица обратного API растёт до 12 указателей: `GetWorldPosition`, `Instantiate`, `DestroyObject`, `SetVelocity`, `GetVelocity`, `Raycast`.

**Результат по файлу:** managed-сторона знает геймплейные нативные функции.

**Критерий правильности по файлу:**
1. Таблица обратного API содержит 12 указателей и связывается при инициализации.

### Файл: `managed/SkyEngine.Managed/ScriptComponent.cs`

**Назначение файла:** дополнение базового класса скрипта геймплейными методами.

**Пошаговое описание действий:**
1. Добавить защищённые `Instantiate`, `Destroy`, `SetVelocity`, `GetWorldPosition`.

**Что должно быть в файле:**

*Структуры / классы / enum:* дополнение `ScriptComponent`.

*Функции / методы:*
- защищённые `Instantiate(prefab, x, y, z)`, `Destroy()`, `SetVelocity(x, y, z)`, `GetWorldPosition()`.

*Логика функций / методов:*
- `ScriptComponent`: защищённые `Instantiate(prefab, x,y,z)`, `Destroy()`, `SetVelocity(x,y,z)`, `GetWorldPosition()` (в т.ч. по id чужого объекта).

**Результат по файлу:** скрипт может спавнить/уничтожать объекты и управлять скоростью.

**Критерий правильности по файлу:**
1. Скрипт спавнит/уничтожает объекты и читает мировую позицию (в т.ч. чужого объекта).

### Файл: `managed/SkyEngine.Managed/Physics.cs`

**Назначение файла:** доступ к физическому лучу из скриптов.

**Пошаговое описание действий:**
1. Добавить `Physics.Raycast`.

**Что должно быть в файле:**

*Структуры / классы / enum:* `Physics`, `RaycastHit`.

*Функции / методы:*
- `Physics.Raycast(...) → RaycastHit`

*Логика функций / методов:*
- `Physics.cs`: `Physics.Raycast(...) → RaycastHit`.

**Результат по файлу:** скрипт читает физический луч.

**Критерий правильности по файлу:**
1. `Physics.Raycast` возвращает попадание `RaycastHit`.

### Файл: `editor/shell/src/editor_context.cpp`

**Назначение файла:** дополнение сборочной точки нативными колбэками геймплейного API.

**Пошаговое описание действий:**
1. Реализовать нативные колбэки геймплея.
2. Реализовать raycast физики по heightfield.

**Что должно быть в файле:**

*Структуры / классы / enum:* нет (дополнение `EditorContext`).

*Функции / методы:*
- нативные колбэки `scriptGetWorldPosition`, `scriptInstantiate`, `scriptDestroyObject`, `scriptSetVelocity`, `scriptGetVelocity`, `scriptRaycast`.

*Логика функций / методов:*
- нативные колбэки `scriptGetWorldPosition/Instantiate/DestroyObject/SetVelocity/GetVelocity/Raycast`; raycast физики по heightfield (марш + бисекция + нормаль).

**Результат по файлу:** нативная сторона обслуживает геймплейный API скриптов.

**Критерий правильности по файлу:**
1. Скрипт спавнит/уничтожает объекты и читает физический луч.

### На выходе должно получиться

**Список артефактов фичи:**
1. `managed/SkyEngine.Managed/Engine.cs` (дополнение: таблица обратного API до 12 указателей)
2. `managed/SkyEngine.Managed/ScriptComponent.cs` (дополнение: `Instantiate`, `Destroy`, `SetVelocity`, `GetWorldPosition`)
3. `managed/SkyEngine.Managed/Physics.cs` (дополнение: `Physics.Raycast`)
4. `editor/shell/src/editor_context.cpp` (дополнение: нативные колбэки геймплея, raycast по heightfield)

**Общий критерий правильности:**
1. Доступны `Instantiate`/`Destroy`/velocity/raycast.
2. Скрипт спавнит/уничтожает объекты и читает физический луч.

---

## feature/package-lock-and-install

- **Исполнитель:** E5 (Пайплайн и QA)
- **Порядок реализации:** 2
- **Зависимости:** `feature/package-resolver` (`PackageManifest`, разрешение версий)

**Цель фичи:** файл блокировки `sky.lock` и установка пакетов из источников.

**Описание фичи:** контрольная сумма манифеста, запись/чтение `sky.lock` (версии + чексуммы + active) и установка пакета из каталога / tarball / git через immutable-кэш.

**Общий порядок реализации фичи:**
1. Объявить `manifestChecksum`, `savePackageLock`, `loadPackageLock` в `package_lock.hpp`.
2. Объявить `PackageInstaller` в `package_installer.hpp`.
3. Реализовать установку из источников в `package_installer.cpp`.

**Файлы фичи:**
1. `engine/package/include/sky/package/package_lock.hpp`
2. `engine/package/include/sky/package/package_installer.hpp`
3. `engine/package/src/package_installer.cpp`

### Файл: `engine/package/include/sky/package/package_lock.hpp`

**Назначение файла:** контрольная сумма манифеста и файл блокировки.

**Пошаговое описание действий:**
1. Объявить `manifestChecksum`.
2. Объявить `savePackageLock`/`loadPackageLock` и тип `LockedPackage`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `struct LockedPackage` (версия + чексумма + active).

*Функции / методы:*
- `std::uint64_t manifestChecksum(const PackageManifest& manifest)`
- `bool savePackageLock(...)`
- `std::optional<std::vector<LockedPackage>> loadPackageLock(...)`

*Логика функций / методов:*
- `manifestChecksum(manifest)` — Возвращает: контрольную сумму манифеста.
- `savePackageLock(...)` / `loadPackageLock(...)` — запись/чтение `sky.lock` (версии + чексуммы + active).

**Результат по файлу:** формат `sky.lock` и контрольная сумма манифеста.

**Критерий правильности по файлу:**
1. `sky.lock` записывается и читается без потерь (round-trip).

### Файл: `engine/package/include/sky/package/package_installer.hpp`

**Назначение файла:** установщик пакетов из источников.

**Пошаговое описание действий:**
1. Объявить `class PackageInstaller` с методом `install`.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- `class PackageInstaller`

*Функции / методы:*
- `std::optional<PackageManifest> install(const std::string& source, const std::filesystem::path& projectPackages)`

*Логика функций / методов:*
- `install(source, projectPackages)` — устанавливает пакет из источника (каталог / tarball / git с `#tag`) через immutable-кэш. Параметры: `source`, `projectPackages` — куда установить. Возвращает: манифест или `nullopt`.

**Результат по файлу:** интерфейс установщика пакетов.

**Критерий правильности по файлу:**
1. Заголовок компилируется.

### Файл: `engine/package/src/package_installer.cpp`

**Назначение файла:** реализация установки из источников и immutable-кэша.

**Пошаговое описание действий:**
1. Реализовать установку из каталога, tarball и git с `#tag`.
2. Провести установку через immutable-кэш.

**Что должно быть в файле:**

*Структуры / классы / enum:*
- реализация `PackageInstaller`.

*Функции / методы:*
- `install`.

*Логика функций / методов:*
- `install` — распознаёт источник (каталог / tarball / git с `#tag`), помещает пакет в immutable-кэш и устанавливает в `projectPackages`; возвращает манифест или `nullopt`.

**Результат по файлу:** рабочий установщик из трёх источников.

**Критерий правильности по файлу:**
1. Пакет ставится из каталога / архива / git.

### На выходе должно получиться

**Список артефактов фичи:**
1. `engine/package/include/sky/package/package_lock.hpp`
2. `engine/package/include/sky/package/package_installer.hpp`
3. `engine/package/src/package_installer.cpp`

**Общий критерий правильности:**
1. `sky.lock` персистит (round-trip версий, чексумм и active).
2. Пакет ставится из каталога / архива / git.
