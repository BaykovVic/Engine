# Техническое задание · Контур E6 «Data-oriented системы (ECS)»

**Область ответственности.** ECS-мир (сущности, хранилища компонентов,
планировщик систем), синхронизация с объектным миром, прикладные системы,
многопоточное исполнение и профилирование. Контур выделен из зоны рантайма,
чтобы разгрузить контур E4. Обязательный минимум (этапы 1–2) — **к 17 июля**.

Каждый этап (неделя) разбит на **фичи** `feature/<название>`. Одна фича = одна
ветка в git. У методов указаны: **сигнатура**, **что делает**, **параметры** и
**что возвращает**.

## Обозначения

- **ECS (entity-component-system)** — данные-ориентированная модель: сущности (`EntityId`) держат компоненты в хранилищах, системы обрабатывают их пачками.
- `virtual … = 0` — чисто виртуальный метод (контракт); `template <typename T>` — обобщение по типу компонента.
- `std::type_index` — идентификатор типа во время выполнения (по нему ищется хранилище компонента).
- **push/pull-синхронизация** — перенос состояния в ECS до такта и обратно после (без неявного двойного владения).

---

## Этап 1 (Неделя 1 · к 17 июля). ECS-мир и планировщик систем

**Общее описание задач этапа.** ECS-ядро: сущности, типизированные хранилища
компонентов, планировщик систем и запросы. Две фичи.

### feature/ecs-core

#### Файл `engine/ecs/include/sky/ecs/ecs.hpp`
`struct EntityId { std::uint32_t index; std::uint32_t generation; }` — сущность
с поколением (защита от повторного использования id).

**`class IEcsComponentStore`** — хранилище одного типа компонента.
- `virtual std::type_index componentType() const = 0` — Возвращает: тип компонента.
- `virtual bool has(EntityId entity) const = 0` — Возвращает: есть ли компонент у сущности.
- `virtual void remove(EntityId entity) = 0` — удаляет компонент у сущности.
- `virtual std::size_t count() const = 0` — Возвращает: число компонентов.

**`class IEcsSystem`** — система, исполняемая каждый кадр.
- `virtual std::string name() const = 0` — Возвращает: имя системы.
- `virtual void update(double deltaSeconds) = 0` — обрабатывает подходящие сущности. Параметры: `deltaSeconds` — шаг времени.

**`class IEcsWorld`** — мир сущностей.
- `virtual EntityId createEntity() = 0` — Возвращает: новую сущность.
- `virtual void destroyEntity(EntityId entity) = 0` — уничтожает сущность.
- `virtual bool isAlive(EntityId entity) const = 0` — Возвращает: жива ли сущность.
- `virtual IEcsComponentStore& store(std::type_index componentType) = 0` — Возвращает: хранилище типа.

**`class IEcsSystemScheduler`** — планировщик.
- `virtual void registerSystem(IEcsSystem& system) = 0` — регистрирует систему.
- `virtual void unregisterSystem(IEcsSystem& system) = 0` — снимает систему.
- `virtual void tick(double deltaSeconds) = 0` — прогоняет все системы за такт. Параметры: `deltaSeconds`.

**`class IEcsQueryService`** — запросы.
- `virtual std::vector<EntityId> entitiesWith(std::set<std::type_index> types) const = 0`
  Что делает: находит сущности с заданным набором компонентов. Параметры: `types`. Возвращает: список сущностей.

#### Файлы `engine/ecs/include/sky/ecs/ecs_world.hpp`, `engine/ecs/src/ecs_world.cpp`
- `class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService` — добавляет:
  - `template <typename T> TypedComponentStore<T>& storeFor()`
    Что делает: типобезопасный доступ к хранилищу компонента T (с методами `set(entity, value)`, `get(entity)→T*`). Возвращает: хранилище T.
- `std::unique_ptr<EcsWorld> createEcsWorld()` — фабрика.

### feature/ecs-tests

#### Файлы `tests/runtime_tests.cpp`, `tests/world_tests.cpp` (ECS-часть)
Система, обновляющая только сущности с нужным компонентом; чтение результата
через `storeFor<T>().get(...)`.

**На выходе:** библиотека `sky_ecs` собрана; ECS-часть тестов зелёная.
**Критерий правильности этапа:** создание/уничтожение сущности; система
обновляет только сущности с нужным компонентом; запрос по типам корректен.

---

## Этап 2 (Неделя 2 · к 17 июля). Синхронизация с объектным миром

**Общее описание задач этапа.** Явный контракт синхронизации ECS с объектным
миром. Одна фича.

### feature/ecs-object-sync

#### Файлы `engine/ecs/include/sky/ecs/object_sync.hpp`, `engine/ecs/src/object_sync.cpp`
`struct EcsTransform { core::Transform value; }` — базовый компонент трансформа.
- `class IEcsObjectSync`:
  - `virtual EntityId bind(object::ObjectHandle object) = 0` — связывает объект с сущностью. Возвращает: сущность.
  - `virtual void unbind(object::ObjectHandle object) = 0` — разрывает связь.
  - `virtual EntityId entityOf(object::ObjectHandle object) const = 0` — Возвращает: сущность по объекту.
  - `virtual object::ObjectHandle objectOf(EntityId entity) const = 0` — Возвращает: объект по сущности.
  - `virtual void pushAuthoringState() = 0` — до такта переносит трансформ объекта в `EcsTransform`.
  - `virtual void pullEcsResults() = 0` — после такта переносит результат обратно в объектный мир.
- `std::unique_ptr<IEcsObjectSync> createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)` — фабрика.

Точка интеграции — цикл такта в `engine/scene/src/scene_world.cpp` (совместно с
контуром E1): `pushAuthoringState()` → `tick(dt)` → `pullEcsResults()`.

**На выходе:** привязка сущность↔объект; двусторонняя синхронизация вокруг такта; тест в `integrity_tests`.
**Критерий правильности этапа:** объект, обработанный ECS-системой, получает
изменённый трансформ в объектном мире; при паузе не меняется.

---

## Этап 3 (Недели 3–4 · перспектива). Прикладные системы поверх ECS

> Файлы этой фичи в текущем репозитории отсутствуют — создаются на этапе.

**Общее описание задач этапа.** Первые прикладные системы. Одна фича.

### feature/ecs-systems

#### Новые файлы `engine/ecs/src/systems/particle_system.cpp`, `.../transform_batch_system.cpp` (создаются)
- система частиц: компонент `Particle`, метод `update(double)` — продвигает
  позиции и время жизни; мёртвые частицы удаляются.
- система пакетной обработки трансформов — один `update` над всеми `EcsTransform`.
- источник сущностей — рассыпка генерации карты (`materializeGenerationResult`).

**На выходе:** хотя бы одна визуально наблюдаемая система (частицы) в кадре.
**Критерий правильности этапа:** пакетная система применяется ко всем сущностям
с компонентом за такт; частицы видны.

---

## Этап 4 (Недели 5–6 · перспектива). Многопоточное исполнение систем

**Общее описание задач этапа.** Параллельный tick систем через планировщик
задач ядра. Одна фича.

### feature/ecs-multithreading

#### Файл `engine/core/include/sky/core/job_scheduler.hpp` (использовать существующую заготовку)
- `class IJobScheduler`:
  - `virtual JobHandle schedule(Job job) = 0` — ставит задачу в очередь. Возвращает: хэндл задачи.
  - `virtual JobHandle scheduleAfter(JobHandle dependency, Job job) = 0` — задача после зависимости.
  - `virtual void wait(JobHandle job) = 0` — ждёт завершения.

#### Дополнение файла `engine/ecs/src/ecs_world.cpp`
- `tick` раскладывает независимые системы по `schedule`, зависимые — через
  `scheduleAfter`; барьер `wait` перед `pullEcsResults`.

**На выходе:** параллельный tick независимых систем через `IJobScheduler`.
**Критерий правильности этапа:** результат детерминирован и совпадает с
однопоточным (тест эквивалентности).

---

## Этап 5 (Недели 7–8 · перспектива). Профилирование систем

**Общее описание задач этапа.** Покадровые тайминги систем и вывод в редактор
(совместно с E3/E5). Одна фича.

### feature/ecs-profiling

#### Дополнение файла `engine/ecs/src/ecs_world.cpp`
- измерение длительности `update` каждой системы за такт; накопление таймингов.

#### Совместно с E3/E5
- C-интерфейс выдачи таймингов систем; панель профилировщика в редакторе.

**На выходе:** тайминги систем измеряются; панель профилировщика в редакторе.
**Критерий правильности этапа:** длительность такта каждой системы доступна
редактору и отображается.
