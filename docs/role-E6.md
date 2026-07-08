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
  Реализация: в `TypedComponentStore<T>` (ecs_world.hpp) вернуть `typeid(T)`.
- `virtual bool has(EntityId entity) const = 0` — Возвращает: есть ли компонент у сущности.
  Реализация: вернуть `data_.contains(entity.value)`, где `data_` — `std::unordered_map<std::uint64_t, T>`, ключ — `EntityId::value`.
- `virtual void remove(EntityId entity) = 0` — удаляет компонент у сущности.
  Реализация: `data_.erase(entity.value)`.
- `virtual std::size_t count() const = 0` — Возвращает: число компонентов.
  Реализация: вернуть `data_.size()`.

**`class IEcsSystem`** — система, исполняемая каждый кадр.
- `virtual std::string name() const = 0` — Возвращает: имя системы.
  Реализация: конкретная система (напр. `SpinSystem` в `tests/runtime_tests.cpp`) возвращает свой строковый литерал-имя.
- `virtual void update(double deltaSeconds) = 0` — обрабатывает подходящие сущности. Параметры: `deltaSeconds` — шаг времени.
  Реализация: получить список сущностей через `world_.entitiesWith({typeid(Comp)})`, для каждой взять компонент через `world_.storeFor<Comp>().get(entity)` и изменить его поля по `deltaSeconds` (в `SpinSystem` — прибавляет `90.0f * deltaSeconds` к `angle`).

**`class IEcsWorld`** — мир сущностей.
- `virtual EntityId createEntity() = 0` — Возвращает: новую сущность.
  Реализация: в `EcsWorldImpl` (ecs_world.cpp) собрать `EntityId{nextId_++}` (счётчик стартует с 1), вставить `entity.value` в множество `alive_` (`std::unordered_set<std::uint64_t>`), вернуть сущность.
- `virtual void destroyEntity(EntityId entity) = 0` — уничтожает сущность.
  Реализация: `alive_.erase(entity.value)`; если элемент удалён (был жив), пройти по всем хранилищам `stores_` и вызвать `store->remove(entity)` для каждого.
- `virtual bool isAlive(EntityId entity) const = 0` — Возвращает: жива ли сущность.
  Реализация: вернуть `alive_.contains(entity.value)`.
- `virtual IEcsComponentStore& store(std::type_index componentType) = 0` — Возвращает: хранилище типа.
  Реализация: искать `componentType` в `stores_` (`unordered_map<std::type_index, unique_ptr<IEcsComponentStore>>`); если не найдено — бросить `std::out_of_range`, иначе разыменовать `*it->second`.

**`class IEcsSystemScheduler`** — планировщик.
- `virtual void registerSystem(IEcsSystem& system) = 0` — регистрирует систему.
  Реализация: `systems_.push_back(&system)`, где `systems_` — `std::vector<IEcsSystem*>` (хранит указатели в порядке регистрации).
- `virtual void unregisterSystem(IEcsSystem& system) = 0` — снимает систему.
  Реализация: `std::erase(systems_, &system)` — убрать указатель из вектора.
- `virtual void tick(double deltaSeconds) = 0` — прогоняет все системы за такт. Параметры: `deltaSeconds`.
  Реализация: последовательный цикл по `systems_`, для каждой вызвать `system->update(deltaSeconds)` (порядок регистрации).

**`class IEcsQueryService`** — запросы.
- `virtual std::vector<EntityId> entitiesWith(std::set<std::type_index> types) const = 0`
  Что делает: находит сущности с заданным набором компонентов. Параметры: `types`. Возвращает: список сущностей.
  Реализация: перебрать все id из `alive_`; для каждой сущности проверить `std::ranges::all_of` по `componentTypes` — что тип есть в `stores_` и `store->has(entity)`; совпавшие добавить в `result` (линейный перебор живых сущностей).

#### Файлы `engine/ecs/include/sky/ecs/ecs_world.hpp`, `engine/ecs/src/ecs_world.cpp`
- `class EcsWorld : IEcsWorld, IEcsSystemScheduler, IEcsQueryService` — добавляет:
  - `template <typename T> TypedComponentStore<T>& storeFor()`
    Что делает: типобезопасный доступ к хранилищу компонента T (с методами `set(entity, value)`, `get(entity)→T*`). Возвращает: хранилище T.
    Реализация: в `stores()` (map `type_index→unique_ptr<IEcsComponentStore>`) взять слот по ключу `std::type_index(typeid(T))`; если пуст — создать `std::make_unique<TypedComponentStore<T>>()`; вернуть `static_cast<TypedComponentStore<T>&>(*slot)`. `set` делает `data_.insert_or_assign`, `get` — `data_.find` с возвратом указателя или `nullptr`.
- `std::unique_ptr<EcsWorld> createEcsWorld()` — фабрика.
  Реализация: `return std::make_unique<EcsWorldImpl>();` (`EcsWorldImpl` — приватная реализация в анонимном namespace ecs_world.cpp).

### feature/ecs-tests

#### Файлы `tests/runtime_tests.cpp`, `tests/world_tests.cpp` (ECS-часть)
Система, обновляющая только сущности с нужным компонентом; чтение результата
через `storeFor<T>().get(...)`.
Реализация: определить тестовый компонент (напр. `struct Spin { float angle; }`) и класс-систему `SpinSystem : IEcsSystem`, хранящий ссылку на `EcsWorld&`; в `update` перебрать `world_.entitiesWith({typeid(Spin)})` и менять поле через `storeFor<Spin>().get(entity)`. В тесте создать сущности, часть без компонента, прогнать `tick`/`update`, проверить, что затронуты только сущности с компонентом.

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
    Реализация: в `EcsObjectSyncImpl` (object_sync.cpp) если `object.value` уже есть в `byObject_` — вернуть готовую сущность; иначе `ecs_.createEntity()`, записать компонент `ecs_.storeFor<EcsTransform>().set(entity, {hierarchy_.localTransform(object)})`, заполнить оба индекса `byObject_` (object→entity) и `byEntity_` (entity→object), вернуть сущность.
  - `virtual void unbind(object::ObjectHandle object) = 0` — разрывает связь.
    Реализация: найти в `byObject_`; если нет — выход; иначе `ecs_.destroyEntity(entity)`, стереть запись из `byEntity_` и `byObject_`.
  - `virtual EntityId entityOf(object::ObjectHandle object) const = 0` — Возвращает: сущность по объекту.
    Реализация: lookup в `byObject_` по `object.value`; вернуть найденную сущность или пустой `EntityId{}`.
  - `virtual object::ObjectHandle objectOf(EntityId entity) const = 0` — Возвращает: объект по сущности.
    Реализация: lookup в `byEntity_` по `entity.value`; вернуть `ObjectHandle` или `object::ObjectHandle::invalid()`.
  - `virtual void pushAuthoringState() = 0` — до такта переносит трансформ объекта в `EcsTransform`.
    Реализация: взять `ecs_.storeFor<EcsTransform>()`; пройти по всем парам `byObject_` и для каждой `set(entity, {hierarchy_.localTransform(ObjectHandle{objectId})})` — проекция авторского трансформа объекта в компонент.
  - `virtual void pullEcsResults() = 0` — после такта переносит результат обратно в объектный мир.
    Реализация: взять `ecs_.storeFor<EcsTransform>()`; для каждой пары `byObject_` прочитать `transforms.get(entity)` и, если компонент есть, записать обратно `hierarchy_.setLocalTransform(ObjectHandle{objectId}, transform->value)`.
- `std::unique_ptr<IEcsObjectSync> createEcsObjectSync(EcsWorld&, object::IObjectHierarchyAccess&)` — фабрика.
  Реализация: `return std::make_unique<EcsObjectSyncImpl>(ecs, hierarchy);` — сохраняет ссылки на `EcsWorld&` и `object::IObjectHierarchyAccess&` в реализации.

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
  Реализация: (создаётся на этапе, эталона в репозитории нет). Ориентир по существующим системам: `update` перебирает `entitiesWith({typeid(Particle)})`, через `storeFor<Particle>().get(entity)` сдвигает позицию на скорость·dt и уменьшает остаток жизни; для истёкших частиц зовёт `store(...).remove(entity)`/`destroyEntity(entity)`.
- система пакетной обработки трансформов — один `update` над всеми `EcsTransform`.
  Реализация: (создаётся на этапе, эталона в репозитории нет). Ориентир: `update` один раз проходит `entitiesWith({typeid(EcsTransform)})` и применяет пакетную операцию к каждому `storeFor<EcsTransform>().get(entity)`.
- источник сущностей — рассыпка генерации карты (`materializeGenerationResult`).
  Реализация: (создаётся на этапе, эталона в репозитории нет). Ориентир: `materializeGenerationResult` живёт в `engine/mapgen/src/materialize.cpp`; система создаёт сущности через `createEntity()` и заполняет их компоненты по данным материализации карты.

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
    Реализация: в `ThreadPoolScheduler` (engine/core/src/job_scheduler.cpp) вызвать `enqueue(job, JobHandle{})`: под `mutex_` присвоить `handle.value = nextId_++`, положить `PendingJob{job, 0}` в `pending_` и id в `queue_`, затем `wakeWorkers_.notify_one()`.
  - `virtual JobHandle scheduleAfter(JobHandle dependency, Job job) = 0` — задача после зависимости.
    Реализация: `enqueue(job, dependency)` — тот же путь, но в `PendingJob` сохраняется `dependency.value`; воркер (`findRunnable`) запустит задачу лишь когда зависимость исчезнет из `pending_` (т.е. завершится).
  - `virtual void wait(JobHandle job) = 0` — ждёт завершения.
    Реализация: под `unique_lock` ждать на условной переменной `jobDone_`, пока `pending_` содержит `job.value` (`jobDone_.wait(lock, [&]{ return !pending_.contains(job.value); })`).

#### Дополнение файла `engine/ecs/src/ecs_world.cpp`
- `tick` раскладывает независимые системы по `schedule`, зависимые — через
  `scheduleAfter`; барьер `wait` перед `pullEcsResults`.
  Реализация: (создаётся на этапе, эталона в репозитории нет — текущий `tick` последовательный). Ориентир: вместо прямого `system->update(dt)` ставить задачи `scheduler.schedule([&]{ system->update(dt); })`, зависимые системы — через `scheduleAfter(handle, ...)`, накопить хэндлы и `wait(...)` по всем как барьер перед возвратом.

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
  Реализация: (создаётся на этапе, эталона в репозитории нет). Ориентир: в цикле `tick` вокруг `system->update(dt)` замерять `std::chrono::steady_clock` до/после и накапливать длительность на систему (напр. в map по `system->name()`) для последующей выдачи.

#### Совместно с E3/E5
- C-интерфейс выдачи таймингов систем; панель профилировщика в редакторе.
  Реализация: (создаётся на этапе, эталона в репозитории нет; выполняется совместно с E3/E5).

**На выходе:** тайминги систем измеряются; панель профилировщика в редакторе.
**Критерий правильности этапа:** длительность такта каждой системы доступна
редактору и отображается.
