# modules

## Название проекта
Sky Engine

## Основание
- Project brief: `../brief/project-brief.md`
- Project concept: `../concept/project-concept.md`
- System architecture: `../architecture/system-architecture.md`

## Список модулей

### Platform Layer
#### Назначение
Платформенный слой скрывает различия между `Windows`, `macOS` и `Linux` и предоставляет единый набор базовых системных возможностей.
#### Архитектурная роль
Infrastructure.
#### Стек реализации
- Основной язык: `C++20`
- Платформенные `API`: `Win32`, `POSIX`, системные вызовы `macOS`
- Сопутствующий стек: стандартная библиотека `C++`
#### Подробное описание простыми словами
Это фундамент, который позволяет движку вообще разговаривать с операционной системой. Если упростить, именно этот модуль отвечает за то, чтобы программа могла открыть окно, прочитать файл, получить нажатие клавиши, измерить время и запустить поток. Для человека, далекого от разработки, это можно сравнить с инженерными коммуникациями здания: не видно в готовом результате, но без них ничего не работает.
#### Ключевые абстракции
- `IWindowSystem`
- `IInputSource`
- `IFileSystem`
- `ITimerService`
- `IThreadingPrimitives`
#### Ответственность
Предоставляет платформенные сервисы всем `native`-модулям.
#### Входы
Запросы от `Editor Shell`, `OpenGL Backend`, `Vulkan Backend`, `Core Foundation`.
#### Выходы
`window handle`, события ввода, файловые операции, таймеры и примитивы многопоточности.
#### Зависимости
Не зависит от верхнеуровневых модулей.
#### Владелец данных / зона данных
`OS handle`, процессное платформенное состояние.
#### Участие в data flow
Обслуживает все файловые, оконные и input-операции.
#### Участие в user flow
Косвенно участвует во всех сценариях `editor` и `runtime`.
#### Публичные контракты
- `IWindowSystem`
- `IInputSource`
- `IFileSystem`
- `ITimerService`
#### Ограничения
Не знает о сценах, ассетах, scripting и `ECS`.
#### DoD модуля
- [ ] окно создается на всех целевых desktop-platform
- [ ] input-события унифицированы
- [ ] контракт файловой системы стабилен

### Core Foundation
#### Назначение
Базовый слой общих движковых сервисов, на который опираются все остальные `native`-модули.
#### Архитектурная роль
Infrastructure / shared base.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: стандартная библиотека `C++`, собственные `utility`-типы, собственная `jobs`-подсистема
#### Подробное описание простыми словами
Если `Platform Layer` отвечает за связь с операционной системой, то `Core Foundation` отвечает за внутреннюю служебную жизнь самого движка. Здесь находятся журналирование, конфигурация, обработка событий, планирование фоновых задач и базовые вспомогательные механизмы. Для человека вне разработки это похоже на административный штаб и внутреннюю логистику: пользователю они почти не видны, но без них система быстро становится неуправляемой.
#### Ключевые абстракции
- `ILogger`
- `IConfigService`
- `IEventBus`
- `IJobScheduler`
- `IDiagnosticsSink`
#### Ответственность
Логирование, конфигурация, диагностика, `job scheduling`, базовые события и утилиты.
#### Входы
Запросы от всех `native`-модулей.
#### Выходы
Логи, события, сервисы конфигурации и диагностики.
#### Зависимости
`Platform Layer`
#### Владелец данных / зона данных
Runtime-конфигурация, состояние планировщика задач, данные диагностики.
#### Участие в data flow
Сопровождает выполнение всех потоков данных и обеспечивает служебную инфраструктуру.
#### Участие в user flow
Косвенно участвует во всех пользовательских сценариях.
#### Публичные контракты
- `ILogger`
- `IEventBus`
- `IJobScheduler`
#### Ограничения
Не хранит `gameplay`- и `domain`-состояние.
#### DoD модуля
- [ ] логирование доступно всем модулям
- [ ] `jobs` доступны всем модулям
- [ ] события не привязаны к конкретной `domain`-логике

### Project Model
#### Назначение
Модуль описывает локальный проект как единицу конфигурации, хранения и запуска.
#### Архитектурная роль
Domain / orchestration.
#### Стек реализации
- Основной язык: `C++20`
- Форматы данных: текстовые конфигурации проекта, собственные проектные дескрипторы
#### Подробное описание простыми словами
Этот модуль отвечает на вопрос: что такое конкретный проект Sky Engine и из чего он состоит. Именно он знает, где лежат сцены, какие пакеты подключены, какие настройки у проекта и какие рабочие каталоги использовать. Для человека вне разработки это аналог папки проекта с правилами: как бухгалтерская карточка, план помещений и реестр документов в одном месте.
#### Ключевые абстракции
- `ProjectDescriptor`
- `ProjectHandle`
- `ProjectSettings`
- `IProjectRepository`
#### Ответственность
Конфигурация проекта, список сцен, ссылки на пакеты, структура каталогов проекта.
#### Входы
Команды `open`, `create`, `save`.
#### Выходы
Разрешенный `project context`.
#### Зависимости
`Serialization and Persistence`, `Package System`
#### Владелец данных / зона данных
Метаданные проекта и структура проекта на диске.
#### Участие в data flow
Является входной точкой для всего `project-level` состояния.
#### Участие в user flow
Создание, открытие и сохранение проекта.
#### Публичные контракты
- `IProjectRepository`
- `IProjectQueryService`
#### Ограничения
Не импортирует ассеты и не выполняет `rendering`.
#### DoD модуля
- [ ] проект открывается локально
- [ ] конфигурация проекта воспроизводима

### Package System
#### Назначение
Модуль локальной пакетной модели и механизма расширения движка.
#### Архитектурная роль
Domain / extension infrastructure.
#### Стек реализации
- Основной язык: `C++20`
- Форматы данных: собственные `package manifest`, локальные метаданные зависимостей
#### Подробное описание простыми словами
Этот модуль нужен для того, чтобы Sky Engine можно было расширять без хаотического копирования файлов в проект. Он знает, какие пакеты подключены, как они зависят друг от друга и какие новые возможности они добавляют движку. Для неразработчика это можно сравнить с системой модулей в сложной технике: базовая машина одна, но дополнительные блоки расширяют ее функции по понятным правилам.
#### Ключевые абстракции
- `PackageManifest`
- `PackageRegistry`
- `ExtensionPoint`
- `IPackageResolver`
- `IPackageActivationService`
#### Ответственность
Обнаружение пакетов, разрешение зависимостей, активация пакетов, регистрация `extension point`.
#### Входы
Локальные `manifest`, ссылки на пакеты из `Project Model`.
#### Выходы
Граф активированных пакетов и набор зарегистрированных расширений.
#### Зависимости
`Project Model`, `Serialization and Persistence`, `Asset System`
#### Владелец данных / зона данных
`package registry`, `manifest`, `extension binding`.
#### Участие в data flow
Разрешает `package graph` и определяет доступные расширения.
#### Участие в user flow
Добавление, удаление и активация локального пакета.
#### Публичные контракты
- `IPackageResolver`
- `IPackageRegistry`
- `IExtensionRegistry`
#### Ограничения
Не использует внешний `registry` и не зависит от `SaaS`.
#### DoD модуля
- [ ] локальные пакеты обнаруживаются
- [ ] граф зависимостей строится
- [ ] `extension point` регистрируются

### Asset System
#### Назначение
Единый модуль идентичности, импорта, метаданных и зависимостей ассетов.
#### Архитектурная роль
Domain / infrastructure.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: собственные `import pipeline`, собственные дескрипторы ресурсов, обработка файлов
#### Подробное описание простыми словами
Ассетами в движке являются модели, текстуры, звуки, материалы и другие ресурсы. Этот модуль следит за тем, чтобы каждый ресурс имел понятный идентификатор, мог быть найден, правильно импортирован и связан с другими ресурсами. Для человека вне разработки это похоже на библиотечный каталог или складскую систему: не просто хранить вещи, а точно знать, что это за вещь, где она лежит и с чем связана.
#### Ключевые абстракции
- `AssetId`
- `AssetDescriptor`
- `AssetRegistry`
- `IAssetResolver`
- `IAssetImporter`
#### Ответственность
Идентичность ассета, `import orchestration`, метаданные, граф зависимостей, кеширование.
#### Входы
Исходные файлы, команды импорта, запросы на разрешение ассетов.
#### Выходы
Разрешенные ассеты и их метаданные.
#### Зависимости
`Serialization and Persistence`, `Core Foundation`
#### Владелец данных / зона данных
`asset registry`, `metadata`, `dependency graph`.
#### Участие в data flow
Импортирует ассеты, выдает им идентичность и отслеживает связи между ними.
#### Участие в user flow
Импорт ассетов, просмотр `Asset Browser`, загрузка ресурсов в сцену.
#### Публичные контракты
- `IAssetResolver`
- `IAssetRegistry`
- `IImportPipeline`
#### Ограничения
Не владеет сценой и не управляет `runtime loop`.
#### DoD модуля
- [ ] `AssetId` стабилен
- [ ] импорт воспроизводим
- [ ] граф зависимостей строится корректно

### Serialization and Persistence
#### Назначение
Модуль сохранения и восстановления постоянного состояния проекта, сцены, ассетов и terrain-данных.
#### Архитектурная роль
Infrastructure.
#### Стек реализации
- Основной язык: `C++20`
- Форматы данных: собственные форматы `project`, `scene`, `terrain`, `asset metadata`
#### Подробное описание простыми словами
Этот модуль отвечает за память на диске. Он сохраняет проект, сцену, настройки, terrain и другую важную информацию так, чтобы ее можно было потом точно восстановить. Для человека вне разработки это похоже на архив и систему делопроизводства одновременно: не только хранить документы, но и гарантировать, что завтра они откроются в понятном и правильном виде.
#### Ключевые абстракции
- `ISerializationBackend`
- `SchemaVersion`
- `ProjectSerializer`
- `SceneSerializer`
- `TerrainSerializer`
#### Ответственность
Контракты сохранения, версии схем, миграции форматов.
#### Входы
Запросы `save` и `load` от `domain`-модулей.
#### Выходы
Файлы на диске и восстановленное состояние.
#### Зависимости
`Platform Layer`, `Core Foundation`
#### Владелец данных / зона данных
Форматы данных, версии схем, правила миграции.
#### Участие в data flow
Обеспечивает `round-trip` постоянного состояния проекта и рабочих данных.
#### Участие в user flow
Открытие и сохранение проекта, сцены, terrain и метаданных.
#### Публичные контракты
- `ISerializationBackend`
- `ISchemaMigrationService`
#### Ограничения
Не определяет `domain ownership`.
#### DoD модуля
- [ ] `project` и `scene` проходят `round-trip`
- [ ] схема версионирования предусмотрена

### Scene System
#### Назначение
Модуль жизненного цикла сцены и владения контекстом активной сцены.
#### Архитектурная роль
Domain / orchestration.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: собственные структуры scene-графа и runtime-контекста
#### Подробное описание простыми словами
Сцена — это конкретный рабочий или игровой мир, который пользователь открывает и редактирует. Этот модуль знает, какая сцена сейчас активна, какие объекты в нее входят, когда сцена загружается, когда выгружается и как переводится в режим исполнения. Для далекого от разработки человека это можно сравнить с управлением театральной сценой: есть декорации, состав, активная постановка и режим репетиции или показа.
#### Ключевые абстракции
- `SceneHandle`
- `SceneDescriptor`
- `SceneRuntimeContext`
- `ISceneRepository`
- `ISceneRuntime`
#### Ответственность
Загрузка, выгрузка, активация сцены, принадлежность объектов сцене, активный контекст.
#### Входы
Команды открытия, сохранения и активации сцены.
#### Выходы
`active scene context`.
#### Зависимости
`Serialization and Persistence`, `Asset System`, `Object Model`, `Component Model`, `ECS Layer`
#### Владелец данных / зона данных
Загруженные сцены, ссылки сцены, состояние активной сцены.
#### Участие в data flow
Собирает сцену из `serialization`, иерархии объектов, компонентов и `ECS`-проекции.
#### Участие в user flow
Создание сцены, открытие сцены, активация сцены в `play mode`.
#### Публичные контракты
- `ISceneRepository`
- `ISceneRuntime`
- `ISceneQueryService`
#### Ограничения
Не владеет внутренним хранилищем `Object Model` и `Component Model`.
#### DoD модуля
- [ ] жизненный цикл сцены определен явно
- [ ] активный контекст сцены воспроизводим

### Object Model
#### Назначение
Модель объектов, ориентированная на удобную структуру сцены в `editor` и `runtime`.
#### Архитектурная роль
Domain.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: собственные `handle`, `transform`-деревья, object-иерархия
#### Подробное описание простыми словами
Этот модуль отвечает за привычную для человека картину сцены: родительские и дочерние объекты, их положение, вращение и масштаб. Именно он делает сцену понятной для чтения и редактирования. Для недевелопера это можно сравнить с планом помещения или деревом организационной структуры, где видно, что входит во что и как части связаны друг с другом.
#### Ключевые абстракции
- `ObjectHandle`
- `TransformNode`
- `IObjectFactory`
- `IObjectHierarchyAccess`
#### Ответственность
Иерархия объектов, `transform`, идентичность объекта.
#### Входы
Команды создания, перемещения и удаления объектов.
#### Выходы
`ObjectHandle`, обновленная иерархия.
#### Зависимости
`Scene System`, `Component Model`
#### Владелец данных / зона данных
Иерархия объектов, `transform`, метаданные объекта.
#### Участие в data flow
Материализует и изменяет объектную структуру сцены.
#### Участие в user flow
Редактирование `hierarchy` и `transform`.
#### Публичные контракты
- `IObjectFactory`
- `IObjectHierarchyAccess`
- `IObjectQueryService`
#### Ограничения
Не владеет принадлежностью объекта сцене без участия `Scene System`.
#### DoD модуля
- [ ] иерархия объектов стабильна
- [ ] у `transform` один источник владения

### Component Model
#### Назначение
Модель присоединяемых `native`- и `script`-компонентов.
#### Архитектурная роль
Domain.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: собственные реестры компонентов, метаданные `Inspector`, интеграция с `C#` через `Scripting Boundary`
#### Подробное описание простыми словами
Компоненты — это способ добавлять объекту свойства и поведение, не превращая его в жестко зашитую сущность. Один объект может получить компонент физики, компонент визуализации, компонент логики и так далее. Для человека вне разработки это похоже на набор модулей на машине: сама машина одна, но ее функции определяются тем, какие блоки к ней подключены.
#### Ключевые абстракции
- `ComponentHandle`
- `ComponentDescriptor`
- `IComponentRegistry`
- `IComponentAttachmentService`
#### Ответственность
Экземпляры компонентов, их дескрипторы и метаданные для `Inspector`.
#### Входы
Команды добавления, удаления и обновления компонента.
#### Выходы
Экземпляры компонентов и события их жизненного цикла.
#### Зависимости
`Object Model`, `Scripting Boundary`, `Serialization and Persistence`
#### Владелец данных / зона данных
Хранилище компонентов и их метаданные.
#### Участие в data flow
Заполняет объекты функциональным содержимым и связывает `native`-компоненты со `script`-логикой.
#### Участие в user flow
Добавление, удаление и редактирование компонентов в `Inspector`.
#### Публичные контракты
- `IComponentRegistry`
- `IComponentAttachmentService`
- `IComponentQueryService`
#### Ограничения
Не владеет `script runtime` и `physics world`.
#### DoD модуля
- [ ] `native`- и `script`-компоненты регистрируются
- [ ] инспектируемые поля формализованы

### ECS Layer
#### Назначение
`Data-oriented` слой для высокочастотного выполнения систем и массовой обработки сущностей.
#### Архитектурная роль
Domain / runtime execution.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: собственные `entity`-идентификаторы, `component storage`, `system scheduler`
#### Подробное описание простыми словами
Этот модуль нужен для тех участков движка, где объектов очень много и работать с ними поштучно уже слишком дорого. Он превращает мир в более удобную для вычислений форму, чтобы симуляции и массовые обновления шли быстрее. Для неразработчика это можно сравнить с переходом от ручной работы с отдельными карточками к промышленной обработке данных партиями.
#### Ключевые абстракции
- `EntityId`
- `IEcsWorld`
- `IEcsSystem`
- `IEcsComponentStore`
#### Ответственность
Хранение сущностей, хранилища `ECS`-компонентов, планирование `system`.
#### Входы
Команды создания и обновления `entity`, `frame tick`.
#### Выходы
Обновленное `ECS`-состояние и результаты работы систем.
#### Зависимости
`Core Foundation`, `Physics Engine`, `Rendering Abstraction`
#### Владелец данных / зона данных
`entities`, `ECS component storage`, состояние выполнения систем.
#### Участие в data flow
Выполняет `data-oriented` обновление `runtime`-состояния.
#### Участие в user flow
Косвенно участвует в `play mode`.
#### Публичные контракты
- `IEcsWorld`
- `IEcsSystemScheduler`
- `IEcsQueryService`
#### Ограничения
Не должен неявно дублировать владение данными из `Object Model`.
#### DoD модуля
- [ ] `ECS world` изолирован
- [ ] `sync contract` с `object world` определен явно

### Physics Engine
#### Назначение
Собственный модуль физической симуляции.
#### Архитектурная роль
Domain / runtime execution.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: собственные математические структуры, собственный `physics world`, собственные алгоритмы столкновений и симуляции
#### Подробное описание простыми словами
Этот модуль отвечает за поведение объектов в физическом смысле: столкновения, движение, реакции на силу и ограничения. Если в сцене куб падает на пол, именно здесь решается, как и почему это происходит. Для далекого от разработки человека это похоже на цифровой эквивалент физических законов внутри виртуального мира.
#### Ключевые абстракции
- `PhysicsWorld`
- `RigidBodyHandle`
- `ColliderHandle`
- `IPhysicsWorld`
- `IPhysicsQueryService`
#### Ответственность
Коллизии, `rigid body`, симуляция, запросы к физическому миру.
#### Входы
Обновления тел и коллайдеров, `frame tick`, физические запросы.
#### Выходы
Результаты симуляции и события коллизий.
#### Зависимости
`Scene System`, `Object Model`, `ECS Layer`
#### Владелец данных / зона данных
`physics world`, коллайдеры, тела, состояние решателя.
#### Участие в data flow
Поддерживает физическое состояние и возвращает результаты симуляции в другие модули.
#### Участие в user flow
`play mode`, обновление коллизий terrain.
#### Публичные контракты
- `IPhysicsWorld`
- `IPhysicsQueryService`
- `IPhysicsSyncContract`
#### Ограничения
Physics backend должен быть заменяем через контракт.
#### DoD модуля
- [ ] физический мир работает независимо
- [ ] `sync` со сценой, объектами и `ECS` описан явно

### Rendering Abstraction
#### Назначение
Backend-независимая граница рендера между `engine core` и графическими backend.
#### Архитектурная роль
Domain / adapter boundary.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: собственные `render command`, `render resource`, `material` и `pipeline`-контракты
#### Подробное описание простыми словами
Этот модуль нужен для того, чтобы остальной движок не зависел напрямую от `OpenGL` или `Vulkan`. Он принимает данные мира и переводит их в универсальный язык рендеринга, который затем уже исполняет конкретный backend. Для человека вне разработки это можно сравнить с переводчиком между архитектурой сцены и конкретной графической машиной.
#### Ключевые абстракции
- `RenderDevice`
- `RenderResourceHandle`
- `RenderCommand`
- `IRenderer`
- `IRenderSurface`
#### Ответственность
Контракты рендера, ресурсы, командная модель, вывод в viewport.
#### Входы
Данные сцены для рендера и запрос на построение кадра.
#### Выходы
Поток render-команд и итоговое изображение кадра.
#### Зависимости
`Asset System`, `Platform Layer`
#### Владелец данных / зона данных
Backend-независимое `render state` и контракты ресурсов.
#### Участие в data flow
Преобразует состояние мира в backend-независимое представление для графического backend.
#### Участие в user flow
Preview в viewport и рендеринг `runtime`.
#### Публичные контракты
- `IRenderer`
- `IRenderSurface`
- `IRenderResourceFactory`
#### Ограничения
`Gameplay`- и `editor`-логика не должны знать backend-специфичные детали.
#### DoD модуля
- [ ] переключение backend не ломает контракты
- [ ] вывод в viewport стабилен

### OpenGL Backend
#### Назначение
Реализация `Rendering Abstraction` через `OpenGL`.
#### Архитектурная роль
Adapter.
#### Стек реализации
- Основной язык: `C++20`
- Графический стек: `OpenGL`
#### Подробное описание простыми словами
Это одна из конкретных графических реализаций движка. Она получает универсальные команды рендера и превращает их в вызовы `OpenGL`, которые уже понимает драйвер видеокарты. Для неразработчика это похоже на один из возможных двигателей машины: общая конструкция автомобиля одна, но конкретный мотор внутри может быть разным.
#### Ключевые абстракции
- `OpenGlRenderDevice`
- `OpenGlShaderResource`
#### Ответственность
Создание и управление GPU-объектами `OpenGL`, исполнение render-команд.
#### Входы
Render-команды и графические ресурсы.
#### Выходы
Готовый кадр.
#### Зависимости
`Rendering Abstraction`, `Platform Layer`
#### Владелец данных / зона данных
`OpenGL`-специфичные GPU-объекты.
#### Участие в data flow
Исполняет backend-поток графических команд.
#### Участие в user flow
Рендеринг viewport и `runtime`.
#### Публичные контракты
Экспортирует реализацию `IRenderer`.
#### Ограничения
Не должен протекать за границу `Rendering Abstraction`.
#### DoD модуля
- [ ] контракт `IRenderer` реализован

### Vulkan Backend
#### Назначение
Реализация `Rendering Abstraction` через `Vulkan`.
#### Архитектурная роль
Adapter.
#### Стек реализации
- Основной язык: `C++20`
- Графический стек: `Vulkan`
#### Подробное описание простыми словами
Это вторая конкретная графическая реализация движка. Она делает ту же работу, что и `OpenGL Backend`, но через `Vulkan`, который дает другой уровень управления и производительности. Для недевелопера это можно представить как альтернативную силовую установку с тем же назначением, но с другим внутренним устройством и другими эксплуатационными особенностями.
#### Ключевые абстракции
- `VulkanRenderDevice`
- `VulkanPipelineState`
#### Ответственность
Создание и управление GPU-объектами `Vulkan`, исполнение render-команд.
#### Входы
Render-команды и графические ресурсы.
#### Выходы
Готовый кадр.
#### Зависимости
`Rendering Abstraction`, `Platform Layer`
#### Владелец данных / зона данных
`Vulkan`-специфичные GPU-объекты.
#### Участие в data flow
Исполняет backend-поток графических команд.
#### Участие в user flow
Рендеринг viewport и `runtime`.
#### Публичные контракты
Экспортирует реализацию `IRenderer`.
#### Ограничения
Не должен протекать за границу `Rendering Abstraction`.
#### DoD модуля
- [ ] контракт `IRenderer` реализован

### Scripting Boundary
#### Назначение
Жесткая граница взаимодействия между `native`-частью движка и managed-слоем `C#`.
#### Архитектурная роль
Integration / bridge.
#### Стек реализации
- Основной язык: `C++20`
- Интеграционный стек: `C#/.NET interop`, собственные `handle`, `binding registry`, marshaling
#### Подробное описание простыми словами
Этот модуль — официальный и строго контролируемый переводчик между ядром движка и пользовательской логикой на `C#`. Он следит за тем, чтобы скрипты могли обращаться к возможностям движка, но при этом не ломали правила владения объектами и жизненным циклом. Для далекого от разработки человека это можно сравнить с пропускным пунктом и переводческим центром в одном лице: через него можно пройти и поговорить, но только по установленным правилам.
#### Ключевые абстракции
- `NativeHandle`
- `ManagedTypeBinding`
- `BindingRegistry`
- `IScriptBindingService`
- `IScriptLifecycleBridge`
#### Ответственность
Реестр типов, `handle`, `binding`, marshaling, `lifecycle bridge`, изоляция ошибок managed-кода.
#### Входы
Жизненный цикл native-объектов и компонентов, запросы на связывание `script`.
#### Выходы
Создание managed-представлений, вызовы callbacks, события жизненного цикла `script`.
#### Зависимости
`Scene System`, `Object Model`, `Component Model`, `Managed Runtime Host`
#### Владелец данных / зона данных
Таблицы `handle`, метаданные `binding`, отображение `native <-> managed`.
#### Участие в data flow
Связывает native-состояние и managed-поведение без передачи владения состоянием.
#### Участие в user flow
Привязка `script`, выполнение `script` в `play mode`, `reload`.
#### Публичные контракты
- `IScriptBindingService`
- `IScriptLifecycleBridge`
- `INativeHandleRegistry`
#### Ограничения
Managed-сторона не получает владение native-состоянием.
#### DoD модуля
- [ ] модель `handle` формализована
- [ ] жизненный цикл callbacks контролируется явно

### Managed Runtime Host
#### Назначение
Изолированный host для выполнения `C#`-кода внутри движка.
#### Архитектурная роль
Integration / hosted runtime.
#### Стек реализации
- Основной язык: `C#`
- Среда исполнения: `.NET`
- Вспомогательная интеграция: вызовы из `C++20` через `Scripting Boundary`
#### Подробное описание простыми словами
Этот модуль поднимает и обслуживает управляемую среду, в которой запускаются скрипты движка. Он загружает сборки, следит за их жизненным циклом и обеспечивает исполнение логики, написанной на `C#`. Для неразработчика это похоже на отдельный рабочий отсек внутри машины, где выполняются сценарии поведения, но который не получает право переписать фундаментальные правила работы самого двигателя.
#### Ключевые абстракции
- `ManagedDomain`
- `AssemblyCatalog`
- `IScriptHost`
- `IDomainReloadPolicy`
#### Ответственность
Загрузка `assembly`, жизненный цикл managed-домена, оркестрация `reload`.
#### Входы
`assembly`, события `reload` и `compile`, запросы на вызов managed-кода.
#### Выходы
Окружение исполнения managed-кода.
#### Зависимости
`Scripting Boundary`
#### Владелец данных / зона данных
`assemblies`, managed-экземпляры, состояние managed-домена.
#### Участие в data flow
Исполняет managed-поведение по запросу `Scripting Boundary`.
#### Участие в user flow
Авторинг `script` и выполнение в `play mode`.
#### Публичные контракты
- `IScriptHost`
- `IDomainReloadPolicy`
#### Ограничения
Не владеет состоянием движка.
#### DoD модуля
- [ ] `runtime host` изолирован
- [ ] политика `reload` определена

### Editor Shell
#### Назначение
Главная оболочка `editor`-приложения.
#### Архитектурная роль
Tooling / orchestration.
#### Стек реализации
- Основной язык: `C++20`
- UI-стек: `Qt5`
#### Подробное описание простыми словами
Это внешняя оболочка программы, которую пользователь видит при запуске редактора. Именно она отвечает за главное окно, панели, меню, вкладки и общее пространство работы. Для далекого от разработки человека это аналог приборной панели, кабинета управления или интерфейса оператора.
#### Ключевые абстракции
- `EditorSession`
- `DockLayout`
- `IEditorShell`
#### Ответственность
Главное окно, панели, docking, жизненный цикл `editor session`.
#### Входы
Команды запуска, открытия проекта, перехода в `play mode`.
#### Выходы
Рабочее пространство `editor`.
#### Зависимости
`Platform Layer`, `Project Model`, `Editor Tools`, `Viewport Runtime Bridge`
#### Владелец данных / зона данных
Состояние оболочки `editor`, состояние UI-сессии.
#### Участие в data flow
Оркестрирует вход пользователя в сценарии работы с проектом.
#### Участие в user flow
Все пользовательские сценарии `editor`.
#### Публичные контракты
- `IEditorShell`
- `IEditorSession`
#### Ограничения
Не владеет сценой и `runtime state`.
#### DoD модуля
- [ ] жизненный цикл `editor workspace` контролируем

### Editor Tools
#### Назначение
Набор инструментов авторинга для работы со сценой, объектами, ассетами, пакетами и terrain.
#### Архитектурная роль
Tooling.
#### Стек реализации
- Основной язык: `C++20`
- UI-стек: `Qt5`
- Дополнительный стек: вызовы в `engine core` через внутренние контракты
#### Подробное описание простыми словами
Если `Editor Shell` — это оболочка редактора, то `Editor Tools` — это сами рабочие инструменты внутри нее. Именно здесь находятся список объектов, инспектор свойств, браузер ассетов, инструменты terrain и другие элементы, которыми реально пользуется создатель игры. Для недевелопера это аналог набора станков и панелей управления внутри мастерской.
#### Ключевые абстракции
- `SelectionContext`
- `InspectorContext`
- `IToolCommandBus`
- `IGizmoTool`
#### Ответственность
`Hierarchy`, `Inspector`, `Asset Browser`, `Package UI`, terrain-инструменты, gizmo.
#### Входы
Команды пользователя.
#### Выходы
Authoring-команды в `Project Model`, `Scene System`, `Object Model`, `Component Model`, `Asset System`, `Package System`, `Terrain and Landscape`.
#### Зависимости
`Scene System`, `Object Model`, `Component Model`, `Asset System`, `Package System`, `Terrain and Landscape`
#### Владелец данных / зона данных
Состояние выбора, состояние инструментов, контекст `Inspector`.
#### Участие в data flow
Преобразует действия пользователя в авторские изменения данных проекта и сцены.
#### Участие в user flow
Редактирование сцены, объектов, ассетов, пакетов и terrain.
#### Публичные контракты
- `IToolCommandBus`
- `ISelectionService`
#### Ограничения
Не должен напрямую изменять скрытые внутренние структуры других модулей.
#### DoD модуля
- [ ] все действия проходят через контракты
- [ ] `selection` и `inspector` остаются согласованными

### Viewport Runtime Bridge
#### Назначение
Явная граница между `editor` и `runtime` внутри `embedded viewport`.
#### Архитектурная роль
Integration / orchestration.
#### Стек реализации
- Основной язык: `C++20`
- UI-стек для размещения viewport: `Qt5`
- Интеграция: собственные контракты синхронизации `editor/runtime`
#### Подробное описание простыми словами
Этот модуль отвечает за то, чтобы пользователь мог нажать `Play` внутри редактора и увидеть работающий мир, не выходя из редактора. Он связывает творческий режим редактирования и режим выполнения, не позволяя им смешаться хаотично. Для человека вне разработки это похоже на стекло между диспетчерской и испытательным стендом: через него видно результат, но он же отделяет рабочую подготовку от реального запуска.
#### Ключевые абстракции
- `PlayModeState`
- `ViewportContext`
- `IPlayModeController`
- `IRuntimePreviewHost`
#### Ответственность
`play`, `stop`, `pause`, размещение viewport, синхронизация `editor/runtime state`.
#### Входы
Команды `play mode`, запросы на `render surface`.
#### Выходы
Preview `runtime` и события перехода состояний.
#### Зависимости
`Editor Shell`, `Scene System`, `Rendering Abstraction`, `Physics Engine`, `Scripting Boundary`
#### Владелец данных / зона данных
Состояние `play mode`, состояние viewport, состояние синхронизации preview.
#### Участие в data flow
Оркестрирует переход от `authoring state` к `runtime execution state`.
#### Участие в user flow
`play`, `pause`, `stop`, встроенный preview.
#### Публичные контракты
- `IPlayModeController`
- `IRuntimePreviewHost`
#### Ограничения
Не должен скрывать переходы между `editor state` и `runtime state`.
#### DoD модуля
- [ ] переходы `play/stop` определены явно
- [ ] состояние viewport воспроизводимо

### Terrain and Landscape
#### Назначение
Модуль terrain-данных и логики их редактирования.
#### Архитектурная роль
Domain.
#### Стек реализации
- Основной язык: `C++20`
- Дополнительный UI-слой для инструментов: `Qt5` через `Editor Tools`
- Сопутствующий стек: собственные форматы terrain-данных, интеграция с `Rendering Abstraction` и `Physics Engine`
#### Подробное описание простыми словами
Этот модуль отвечает за рельеф, ландшафт и большие поверхности игрового мира. Он хранит форму terrain, позволяет ее редактировать и обеспечивает, чтобы terrain одинаково понимали и графика, и физика, и система сохранения. Для неразработчика это похоже на работу с цифровым ландшафтным макетом местности.
#### Ключевые абстракции
- `TerrainHandle`
- `TerrainDataset`
- `ITerrainService`
- `ITerrainPersistenceContract`
#### Ответственность
Terrain-данные, модель редактирования, сохранение, `render`- и `physics`-hooks.
#### Входы
Команды редактирования terrain и результаты генерации.
#### Выходы
Обновленное terrain-состояние.
#### Зависимости
`Asset System`, `Serialization and Persistence`, `Rendering Abstraction`, `Physics Engine`
#### Владелец данных / зона данных
Terrain-данные и terrain-метаданные.
#### Участие в data flow
Изменяет постоянное и runtime-представление terrain.
#### Участие в user flow
Создание terrain, редактирование terrain, сохранение terrain.
#### Публичные контракты
- `ITerrainService`
- `ITerrainQueryService`
#### Ограничения
Не владеет оркестрацией `Map Generation`.
#### DoD модуля
- [ ] terrain-состояние сериализуемо
- [ ] интеграция с `rendering` и `physics` определена

### Map Generation
#### Назначение
Модуль процедурной генерации карты и заполнения terrain и сцены.
#### Архитектурная роль
Domain / orchestration.
#### Стек реализации
- Основной язык: `C++20`
- Сопутствующий стек: собственные алгоритмы генерации, интеграция с `Terrain and Landscape`, `Scene System`, `Asset System`
#### Подробное описание простыми словами
Этот модуль автоматически строит части мира по заданным правилам вместо ручного размещения каждого элемента. Он может менять terrain, создавать структуру местности и размещать объекты в сцене. Для человека вне разработки это похоже на генератор чернового плана территории, который быстро создает основу мира, а затем художник и дизайнер ее дорабатывают.
#### Ключевые абстракции
- `GenerationProfile`
- `GenerationRequest`
- `IGenerationPipeline`
- `IGenerationResult`
#### Ответственность
Оркестрация генерации, построение выходных данных, расстановка объектов и модификация terrain.
#### Входы
Команды генерации и профили генерации.
#### Выходы
Результаты генерации для terrain и сцены.
#### Зависимости
`Terrain and Landscape`, `Scene System`, `Asset System`
#### Владелец данных / зона данных
Настройки генерации, промежуточное состояние генерации, итоговые данные генерации.
#### Участие в data flow
Производит terrain- и object-выходы для последующей интеграции в мир сцены.
#### Участие в user flow
Запуск генерации и применение ее результатов.
#### Публичные контракты
- `IGenerationPipeline`
- `IGenerationResult`
#### Ограничения
Не владеет постоянным хранением сцены и terrain.
#### DoD модуля
- [ ] генерация воспроизводима
- [ ] интеграция со сценой и terrain определена

## Межмодульные связи

```text
Project Model -> Package System -> Editor Tools
Project Model -> Asset System -> Serialization and Persistence

Scene System -> Object Model -> Component Model
Scene System -> ECS Layer
Scene System -> Physics Engine
Scene System -> Rendering Abstraction через render-данные

Component Model -> Scripting Boundary -> Managed Runtime Host
Rendering Abstraction -> OpenGL Backend | Vulkan Backend

Editor Shell -> Editor Tools -> Scene/Object/Component/Asset/Package/Terrain
Editor Shell -> Viewport Runtime Bridge -> runtime stack
Map Generation -> Terrain and Landscape -> Scene System
```

### Подробная схема межмодульного взаимодействия

```text
Editor Shell
  -> Project Model
  -> Editor Tools
  -> Viewport Runtime Bridge

Project Model
  -> Package System
  -> Asset System
  -> Serialization and Persistence

Package System
  -> Editor Tools
  -> Asset System
  -> Scripting Boundary

Asset System
  -> Serialization and Persistence
  -> Scene System
  -> Rendering Abstraction
  -> Terrain and Landscape

Scene System
  -> Object Model
  -> Component Model
  -> ECS Layer
  -> Physics Engine
  -> Rendering Abstraction

Object Model
  -> Component Model
  -> Scripting Boundary

Component Model
  -> Scripting Boundary
  -> Serialization and Persistence

ECS Layer
  -> Physics Engine
  -> Rendering Abstraction

Physics Engine
  -> Rendering Abstraction

Scripting Boundary
  -> Managed Runtime Host

Terrain and Landscape
  -> Physics Engine
  -> Rendering Abstraction
  -> Serialization and Persistence

Map Generation
  -> Terrain and Landscape
  -> Scene System
  -> Asset System
```

### Схема контрактных границ

```text
Project Model
  exports -> IProjectRepository, IProjectQueryService
  imports -> ISerializationBackend, IPackageResolver

Package System
  exports -> IPackageRegistry, IExtensionRegistry
  imports -> IProjectRepository, IAssetRegistry

Asset System
  exports -> IAssetResolver, IAssetRegistry, IImportPipeline
  imports -> ISerializationBackend

Scene System
  exports -> ISceneRepository, ISceneRuntime, ISceneQueryService
  imports -> IAssetResolver, IObjectFactory, IComponentAttachmentService

Object Model
  exports -> IObjectFactory, IObjectHierarchyAccess
  imports -> ISceneRuntime

Component Model
  exports -> IComponentRegistry, IComponentAttachmentService
  imports -> IObjectHierarchyAccess, IScriptBindingService

ECS Layer
  exports -> IEcsWorld, IEcsSystemScheduler
  imports -> IPhysicsSyncContract, IRenderer

Physics Engine
  exports -> IPhysicsWorld, IPhysicsQueryService, IPhysicsSyncContract
  imports -> ISceneQueryService, IEcsWorld

Rendering Abstraction
  exports -> IRenderer, IRenderSurface, IRenderResourceFactory
  imports -> IAssetResolver, IWindowSystem

Scripting Boundary
  exports -> IScriptBindingService, IScriptLifecycleBridge, INativeHandleRegistry
  imports -> IScriptHost, ISceneQueryService, IComponentQueryService

Managed Runtime Host
  exports -> IScriptHost, IDomainReloadPolicy
  imports -> IScriptLifecycleBridge
```

## Матрица взаимодействий модулей

| Источник | Получатель | Тип взаимодействия | Данные / команда | Синхронно / асинхронно |
| --- | --- | --- | --- | --- |
| `Project Model` | `Package System` | query/command | `package refs`, активация | sync |
| `Package System` | `Editor Tools` | event/query | доступные расширения | sync |
| `Asset System` | `Serialization and Persistence` | command | запись `metadata` | async |
| `Scene System` | `Object Model` | command | создать или присоединить объект к сцене | sync |
| `Object Model` | `Component Model` | command | добавить или удалить компонент | sync |
| `Component Model` | `Scripting Boundary` | command | привязка `script component` | sync |
| `Scripting Boundary` | `Managed Runtime Host` | invocation | создать или вызвать managed peer | sync |
| `Scene System` | `ECS Layer` | sync-contract | проекция сцены в `entity world` | sync |
| `ECS Layer` | `Physics Engine` | command/query | состояние тел, входы симуляции | sync |
| `Scene System` | `Physics Engine` | command | пересборка `physics state` | sync |
| `Scene System` | `Rendering Abstraction` | data feed | видимые render-объекты | sync |
| `Rendering Abstraction` | `OpenGL Backend` | command stream | render-команды и ресурсы | sync |
| `Rendering Abstraction` | `Vulkan Backend` | command stream | render-команды и ресурсы | sync |
| `Editor Shell` | `Viewport Runtime Bridge` | command | `play`, `stop`, `pause` | sync |
| `Viewport Runtime Bridge` | `Scene System` | command | активация `runtime scene` | sync |
| `Viewport Runtime Bridge` | `Rendering Abstraction` | command | привязка `render surface` | sync |
| `Terrain and Landscape` | `Physics Engine` | command | пересборка terrain-collision | async |
| `Map Generation` | `Terrain and Landscape` | command | применить результат генерации terrain | async |
| `Map Generation` | `Scene System` | command | расставить сгенерированные объекты | async |

## Data flow по модулям
- `Project open`: `Editor Shell -> Project Model -> Package System -> Asset System -> Scene System`
- `Scene load`: `Scene System -> Serialization and Persistence -> Object Model -> Component Model -> ECS Layer / Physics Engine -> Rendering Abstraction`
- `Play mode`: `Editor Shell -> Viewport Runtime Bridge -> Scene System -> Scripting Boundary -> Managed Runtime Host -> Physics Engine / ECS Layer -> Rendering Abstraction`
- `Asset import`: `Asset System -> Serialization and Persistence -> обновление Editor Tools`
- `Terrain generation`: `Map Generation -> Terrain and Landscape -> Scene System / Physics Engine / Rendering Abstraction -> Serialization and Persistence`

### Подробная схема data flow: Scene / Object / Component / ECS

```text
Scene System
  владеет принадлежностью объектов сцене и active scene context
  -> поручает Object Model материализовать hierarchy

Object Model
  владеет object hierarchy и transforms
  -> поручает Component Model присоединить компоненты к объектам

Component Model
  владеет экземплярами компонентов
  -> поручает Scripting Boundary связать script-компоненты
  -> проецирует часть данных в ECS Layer, если нужен data-oriented execution

ECS Layer
  владеет entities и ECS component data
  -> передает simulation-oriented state в Physics Engine и Rendering Abstraction
```

### Подробная схема data flow: один кадр в play mode

```text
Viewport Runtime Bridge
 -> Scene System
 -> Object Model / Component Model
 -> ECS Layer
 -> Physics Engine
 -> Scripting Boundary
 -> Managed Runtime Host
 -> Rendering Abstraction
 -> OpenGL Backend / Vulkan Backend
 -> Viewport Runtime Bridge показывает кадр
```

### Подробная схема data flow: привязка script-компонента

```text
Component Model
 -> IScriptBindingService
 -> Scripting Boundary определяет ManagedTypeBinding
 -> Managed Runtime Host создает managed instance
 -> INativeHandleRegistry связывает native object/component с managed peer
 -> IScriptLifecycleBridge вызывает lifecycle callbacks
```

### Подробная схема data flow: постоянное хранение

```text
Project Model ------------------------------+
Scene System -------------------------------|
Component Model ----------------------------|-> Serialization and Persistence
Terrain and Landscape ----------------------|
Asset System -------------------------------+
                                             -> файлы на диске
                                             -> восстановленное состояние обратно в модули
```

## User flow по модулям
- создание и открытие проекта: `Editor Shell`, `Project Model`, `Package System`
- работа со сценой: `Editor Tools`, `Scene System`, `Object Model`, `Component Model`
- привязка `script`: `Component Model`, `Scripting Boundary`, `Managed Runtime Host`
- `play mode`: `Viewport Runtime Bridge`, `Scene System`, `Physics Engine`, `ECS Layer`, `Rendering Abstraction`
- работа с terrain: `Terrain and Landscape`, `Map Generation`, `Editor Tools`
- сохранение: `Serialization and Persistence`, `Asset System`, `Project Model`

### Схема user flow по модулям

```text
User action: Open Project
 -> Editor Shell
 -> Project Model
 -> Package System
 -> Asset System

User action: Open Scene
 -> Editor Tools
 -> Scene System
 -> Object Model
 -> Component Model

User action: Attach Script
 -> Inspector / Editor Tools
 -> Component Model
 -> Scripting Boundary
 -> Managed Runtime Host

User action: Play
 -> Viewport Runtime Bridge
 -> Scene System
 -> ECS Layer
 -> Physics Engine
 -> Scripting Boundary
 -> Rendering Abstraction

User action: Generate Terrain
 -> Editor Tools
 -> Map Generation
 -> Terrain and Landscape
 -> Physics Engine
 -> Rendering Abstraction

User action: Save
 -> Project Model / Scene System / Asset System / Terrain and Landscape
 -> Serialization and Persistence
```

## Риски декомпозиции
- `Object Model` и `ECS Layer` могут пересекаться по владению состоянием
- `Scripting Boundary` может стать слишком тяжелым интеграционным слоем
- `Managed Runtime Host` может усилить зависимость от `.NET`
- собственный `Physics Engine` может разрастись раньше базового `runtime`
- `Package System` может потребовать слишком раннего усложнения
- `Qt` должен быть строго удержан внутри `editor-layer`

## Открытые вопросы
- какой именно `C# runtime host` будет выбран
- какая стратегия форматов данных станет базовой для `project`, `scene` и `assets`
- как будет устроен `sync model` между `object world` и `ECS world`
- как будет устроен `reload policy` и сохранение managed-state

## Решение о переходе к backlog
- [x] Project brief согласован.
- [x] Project concept согласован.
- [x] System architecture согласована.
- [x] Полный список модулей описан.
- [x] Межмодульная схема взаимодействия описана.
- [x] Data flow по модулям описан.
- [x] User flow по модулям описан.
- [x] Modules согласованы.
- [x] Можно формировать эпики, спринты и фичи.
