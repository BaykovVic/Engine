# Бэклог — путь до Unity

Gap-анализ от 2026-07-09. База: 17 модулей, ~42 тыс. строк, ~100 функций C ABI,
19/19 тестов зелёные. Что уже есть — см. `docs/roles-README.md` и
`docs/plan-8-weeks-team.md`; здесь только то, чего **не хватает**, по тирам.

## Тир 1 — без этого типичную игру не сделать

### B1. Аудио
- [x] B1.1 — движковое ядро, своё (miniaudio не понадобился: WAV-декод +
  микшер ~350 строк без зависимостей):
  - Модуль `engine/audio`: `IAudioClipLibrary` (WAV PCM16/float32,
    mono/stereo, линейный ресемплинг в фиксированные 48 кГц при загрузке),
    `IAudioMixer` (голоса: volume/loop, потокобезопасный `mix()`,
    финиш не-луп голосов на последнем миксе), `IAudioOutput` — ALSA
    (`default` → `null`-синк, override через `SKY_AUDIO_DEVICE`) с
    фолбэком на null-выход с wall-clock пейсингом: семантика звука
    (голоса идут и заканчиваются) живёт и без звуковой карты.
  - Компонент `sky.audioSource` (clip/volume/loop/playOnStart; незаданные
    поля = слышно, one-shot, играть со старта) — стартует на beginPlay,
    `stopAll` на endPlay. Кэш декодированных клипов по ref.
  - Managed `Audio.Play(ref, volume, loop)` / `Audio.Stop(voice)` —
    15/16-й указатели reverse-API (layout-защита обновлена с обеих сторон).
  - Тесты: `audio_tests` (декод/микс/луп/стоп — точными числами; выходы
    дренируют микшер структурно, ALSA через null-синк),
    `testAudioSourcePlayOnStart`, бридж `testBridgeAudio`.
  - ⚠️ НЕ верифицировано и здесь не верифицируемо: звучание на слух
    (в контейнере нет звукового устройства). Слушать при первом запуске
    на реальной машине.
- [ ] B1.2: OGG (stb_vorbis, public domain), 3D-затухание от
  listener-позиции, панорама, панель Audio Source в инспекторе редактора,
  `sky.audioListener`.
- [ ] B1.3: микшерные группы/шина, громкость мастера, pitch.

### B2. Скелетная анимация
FBX/glTF импортируют только статичный меш — без скелета, скиннинга и анимаций.
- Объём: skin/joints/anim-каналы в glTF-импортёре; палитра костей в вершинном
  шейдере (UBO/SSBO); компонент `sky.animator` (клип, скорость, луп); блендинг —
  вторая очередь.
- Критерий: анимированный персонаж из glTF играет клип в Play.

### B3. Игровой UI
UI есть только у редактора; внутри игры нет Canvas/текста/кнопок — ни меню, ни HUD.
- Объём: screen-space quad'ы поверх кадра (рендер почти умеет), растровый шрифт,
  компоненты `sky.uiText`/`sky.uiImage`/`sky.uiButton`, хит-тест от Input.
- Критерий: HUD со счётом и кнопка Restart в CrateRain.

### B4. Экспорт билда (Build & Run)
Плеер запускает `.skybox` из дерева проекта, но нет упаковки в самостоятельный дистрибутив.
- Объём: pak-упаковщик ассетов (VFS уже умеет `PakMount`), копирование плеера +
  managed-рантайма, `sky_editor_export_build` + пункт меню File → Build.
- Критерий: каталог `Builds/<имя>/` запускается на чистой машине без исходников.

### B5. Взрослая физика
Сейчас только AABB без вращательной динамики.
- Объём: OBB/mesh-коллайдеры, угловая скорость и инерция, триггеры (события без
  расталкивания), joints (hinge/fixed — минимум), CCD — вторая очередь.
- Альтернатива: интеграция Jolt Physics (MIT) за контрактом `IPhysicsWorld`.

## Тир 2 — качество жизни

- **B6. Рендер** — частично сделано:
  - [x] HDR-промежуточная цель (RGBA16F) + пост-пасс с ACES-tonemap
        (`RenderCommand::exposure` у SetCamera; 0 = passthrough) — фундамент
        для bloom/AA.
  - [x] Frustum culling по bounding-сферам мешей (`culledLastFrame()`).
  - [x] Прозрачность с сортировкой back-to-front
        (`MaterialDesc::opacity`/`RenderCommand::opacity`, blend-вариант
        конвейера, прозрачные не отбрасывают тень).
  - [ ] Bloom, AA (пост-цепочка уже есть — добавляются пассами).
  - [ ] Кубмап-скайбокс и отражения.
  - [ ] Каскадные и точечные тени (сейчас одна directional shadow map).
  - [ ] LOD, инстансинг.
  - [x] Экспозиция в UI редактора — компонент `sky.environment`
        (horizon/zenith/exposure) редактируется в инспекторе.
- **B7. Частицы**: система частиц поверх ECS (см. `docs/role-E6.md`, этап 3).
- **B8. Стабильные ссылки на ассеты (GUID)** — частично сделано:
  - [x] GUID-идентичность через sidecar `<файл>.skymeta` (аналог `.meta`):
        первый импорт пишет GUID, переименование вместе с sidecar сохраняет
        `AssetId`; идентичность переживает перезапуск. Безаргументная
        `createAssetDatabase()` остаётся на path-hash (тесты/генерация).
        Тест `testSidecarGuidIdentity`.
  - [x] Ссылки в сценах по GUID: SKYB 1.2 (строковые поля несут GUID,
        миграция 1.1→1.2 цепочкой), мосты refToGuid/guidToRef в
        SceneWorldDeps, скан ассетов проекта при старте/открытии. Переименование
        источника (+sidecar) переживает save→rename→open: тесты
        `testGuidReferenceResolution`, `testLegacyV11SceneMigrates`,
        `testAssetRenameSurvivesSceneReload`.
  - [ ] Ссылки в материалах (`.skymat` придёт с B16) и префабах SKYP по GUID.
  - [ ] Импорт-кэш (Library), сжатие текстур BCn, мипмапы.
- **B9. Скриптинг**: корутины, физические колбэки (`OnCollisionEnter`), доступ к
  иерархии из C# (parent/Find/GetComponent), отладчик (attach из IDE).
- **B10. Редактор**: профилировщик (стыкуется с `feature/ecs-profiling`), окно
  анимации, настройки проекта/качества, мультивыделение.
- **B16. Data-ассеты (аналог ScriptableObject)** — ядро сделано:
  - [x] Формат `sky.data` (`DataAssetDesc`: typeId + parentGuid-наследование +
        карта FieldValue) c save/load через ISerializationBackend и
        `mergedFields` (Unigine-Properties-style overrides) — модуль component.
        Тест `testDataAsset`.
  - [x] `createDataImporter` (asset): `.skydata`→"data", `.skymat`→"material" —
        data-ассеты получают GUID-sidecar и участвуют в B8-ссылках.
  - [x] Персист материалов: правка в панели Materials пишет
        `Assets/Materials/<имя>.skymat`; при старте `.skymat` перекрывают
        встроенные дефолты. Тест `testMaterialEditsPersist` (две сессии).
  - [x] UI: кнопка «+ Data» в панели проекта (Assets/Data/DataAsset[_N]),
        двойной клик по `.skydata` открывает его в инспекторе: поля
        name/type/value, правка пишется сразу, строка добавления поля с
        выбором типа. ABI: 7 функций `sky_editor_data_*`
        (тест `testBridgeDataAssets`, включая персист между сессиями).
  - [x] Managed `DataAsset.Load("assets://Data/…")`: 13-й указатель
        reverse-API (`scriptDataAsset` отдаёт resolved-поля с наследованием,
        query-buffer с ретраем), класс `DataAsset` с типизированными
        геттерами (GetFloat/GetInt/GetBool/GetString/TryGetVec3).
        Layout-guard с обеих сторон: `static_assert(13*sizeof(void*))` в
        нативной таблице + проверка `Marshal.SizeOf<Api>` при Install —
        закрывает пункт архдолга о ручной зеркальности. E2E-тест:
        user-script читает ассет и логирует значения (testBridgeDataAssets,
        SKY_TEST_MANAGED-секция); цепочка наследования —
        `testDataAssetInheritanceChain`.
  - [x] Поле-ссылка `assetRef`: тип поля в дескрипторе компонента,
        инспектор рендерит дропдаун по Assets/Data/*.skydata («None» +
        текущее значение); компонент `sky.gameConfig` как носитель.
        Тест в testBridgeDataAssets (тип "assetRef" + запись/чтение ссылки).

  **B16 закрыт целиком** — аналог ScriptableObject: формат с наследованием,
  GUID-идентичность, редактор (создание/правка/дропдаун-ссылки), managed
  `DataAsset.Load`, персист материалов.
  - Все ингредиенты уже есть: `FieldValue`-инфраструктура (кормит инспектор,
    undo и сериализацию), `ISerializationBackend` (версии схем + миграции),
    VFS/AssetDatabase (тип ассета — строка).
  - MVP: формат `.skydata` (schemaId `sky.data`: typeId + карта FieldValue)
    через ISerializationBackend; тип ассета "data" в AssetDatabase;
    Create → Data Asset в панели проекта; инспектор редактирует поля теми же
    шаблонами, что у компонентов; поле-ссылка `assetRef` у компонентов;
    managed `DataAsset.Load("assets://Data/…")` через reverse-API.
  - Туда же: персист материалов (`.skymat` тем же механизмом).
  - Ссылки по пути ⇒ усиливает B8 (GUID): переименование data-ассета рвёт
    все ссылки — B8 лучше сделать раньше или вместе.

- **B17. Модель кадра по мотивам Unigine** — начато:
  - [x] Async-физика параллельно рендеру (B17.1):
        `SceneWorld::setPhysicsJobScheduler` + `schedulePhysics(dt)` —
        tick() приземляет результаты прошлого кадра (wait+pull), системы и
        скрипты видят их и правят объекты, затем push+планирование шагов
        этого кадра в фон; job перекрывается с рендером. Лаг 1 кадр.
        Планирование ПОСЛЕ всей скриптовой работы — иначе reverse-API
        (raycast/setVelocity) гонится с фоновым шагом. Число шагов
        решается в главном потоке ⇒ последовательность идентична
        синхронной. Drain на deactivate/unload/деструкторе/смене шедулера.
        Включено в плеере (1 worker), редактор синхронный — как у Unigine.
        Тест `testAsyncPhysicsMatchesSync`: 60 тиков sync и async дают
        бит-в-бит одинаковую позу тела.
  - [x] OnFixedUpdate в скриптах (B17.2, инкремент 1): колбэк перед КАЖДЫМ
        физическим шагом с фиксированным dt — порядок Unity (FixedUpdate →
        step → … → Update). Managed: `ScriptComponent.OnFixedUpdate`,
        `Bootstrap.InstanceHasFixedUpdate` (рефлексия с кэшем по типу);
        нативно: `IScriptHost::instanceHasFixedUpdate`,
        `SceneWorldDeps.fixedUpdate/wantsFixedUpdate`. Пока в сцене есть
        хоть один такой скрипт, степпинг прижат к sync даже с шедулером
        (авто-откат: колбэк может трогать любое состояние движка) —
        `schedulePhysics` становится no-op, бит-в-бит с sync
        (`testFixedUpdateInterleavesWithSteps`, `testBridgeFixedUpdate`).
  - [ ] FixedUpdate инкремент 2: колбэки на физическом потоке с
        ограниченным API (без Instantiate/Destroy) — вернёт async-перекрытие
        сценам с FixedUpdate-скриптами.
  - [ ] Параллельный update ECS-систем по ядрам.
  - [ ] Многопоточный рендерер.
  - Туда же (дёшево, отдельно от double precision): **camera-relative
    rendering** — CPU вычитает позицию камеры, GPU получает float-координаты
    относительно камеры; убирает джиттер вдали от начала координат без
    перехода на double.
  - Async-загрузка ассетов на job scheduler (лечит и «ретрай с диска каждый
    кадр» из перф-ревью).

- **B18. Ввод** — ядро сделано (мышь есть ⇒ B3 разблокирован):
  - [x] Краевые события `GetKeyDown`/`GetKeyUp`: защёлки pressed/released в
        EditorContext, очистка в конце `tickPlayFrame` и на старте Play
        (события edit-тайма не читаются как нажатия первого кадра);
        OS-автоповтор не пере-защёлкивает. Семантика для FixedUpdate
        зафиксирована: защёлка per render frame — все fixed-степы кадра
        видят одинаковый edge-стейт, нажатие между степами не теряется.
  - [x] Мышь до конца трубы: кнопки — те же кей-коды 323..325 (Mouse0..2,
        как в Unity), т.е. held/pressed/released — один механизм с
        клавиатурой; позиция и колесо (аккумулируется за кадр) — отдельно.
        ABI `sky_editor_set_mouse_position/_button`, `add_mouse_wheel`;
        X11-плеер пробрасывает Move/Button/Wheel (кнопки 1/3/2 → 0/1/2);
        Avalonia GameView шлёт Pointer-события в пикселях контрола.
        Reverse-API 17/18: `keyEvent(key, query)` + `mouseState(x,y,wheel)`.
        Managed: `Input.GetKeyDown/Up`, `GetMouseButton/Down/Up`,
        `MousePosition`, `MouseWheelDelta`.
  - [x] KeyCode: + Backspace/Delete/правые Shift-Ctrl-Alt/F1..F12/Mouse0..2
        (маппинг в GameView и X11-плеере).
  - [ ] Координаты мыши в редакторе — пиксели контрола Game view, НЕ
        рендер-таргета; масштабирование к рендеру сделать вместе с B3
        hit-тестом, когда появится потребитель с конкретным требованием.
  - [ ] Оси/action maps — по потребности контента.
  - Геймпад и тач: НЕ делать, пока нет способа верифицировать (в контейнере
    нет устройств; код без проверки — обуза).

- **Детерминизм-пакет** — сделан (фундамент трека симуляций):
  - [x] Порядок ECS-систем = порядок регистрации, зафиксирован тестом
        (`testEcsSystemOrderIsRegistrationOrder`).
  - [x] Сеемый ГПСЧ: `sky::core::Pcg32` (header-only) и managed
        `SkyEngine.Random` — один алгоритм бит-в-бит, общие эталонные
        векторы (seed 42 → 0xa15c02b7…) в `testPcg32` и в бридж-тесте.
        Дефолт — ФИКСИРОВАННЫЙ сид (в отличие от Unity): забыл посеять —
        всё равно воспроизводимо.
  - [x] `Time.TimeScale` (Unity-семантика: скейлит DeltaTime и аккумулятор
        fixed-степов; 0 = пауза) — 14-й указатель reverse-API (один
        `timeScale(value, apply)` на get+set), сброс в 1.0 на старте Play.
        Единая точка кадра `EditorContext::tickPlayFrame` — бридж и плеер
        больше не дублируют порядок tick/scripts/schedulePhysics.
  - [x] Тест «сценарий+сид ⇒ бит-в-бит»: `testBridgeDeterministicReplay` —
        две полные Play-сессии (сид, инжект клавиш, смена TimeScale на
        кадре 30) дают бит-идентичный трансформ, одинаковые строки логов и
        ровно 45 fixed-степов (30×scale1 + 15×scale0.5 — timeScale реально
        дошёл до аккумулятора).
  - [ ] Позже: запись/воспроизведение входного потока в файл (replay-формат),
        детерминизм при разном количестве потоков ECS (когда появится
        параллельный tick).

## Тир 3 — платформы и масштаб

- **B11. Кроссплатформенность**: окна только X11/Linux → Windows (Win32) и
  macOS (Cocoa/MoltenVK) бэкенды `IWindowSystem`; дальше — мобильные, WebGL.
- **B12. Многопоточность**: `IJobScheduler` — заготовка; параллельный tick систем
  (см. `feature/ecs-multithreading`), async-загрузка ассетов, отдельный рендер-поток.
- **B13. Сеть**: транспорт + репликация — отсутствуют полностью.
- **B14. Навигация**: NavMesh-запекание и поиск пути.
- **B15. Локализация / Addressables-аналог**.

## Архдолг (из архитектурного ревью, подтверждено по коду)

Полное ревю: 6 измерений × адверсариальная верификация. Исправлено — отмечено.

**Владение состоянием (editor/shell):**
- [x] ~~Play-снапшот покрывал только трансформы~~ — теперь полный: снапшот
  иерархии/компонентов при beginPlay; endPlay реконсилирует (выжившие — на
  месте со стабильными хэндлами, созданные в Play — удаляются, удалённые —
  воссоздаются). Тест `testPlayModeFullRestore`.
- [x] ~~Двойное владение корнями сцены~~ — `SceneWorld::setRootObjects` +
  синк из `roots_` перед сохранением. Тест `testSceneRootsSyncOnSave`.
- [x] ~~Физика: push world / pull local~~ — `pullSimulationResults` пишет
  через `object::setWorldTransform`. Тест `testParentedObjectSync`.
- [ ] Выделить `RuntimeContext` из `EditorContext` (плеер собран из сборочной
  точки редактора — инверсия слоёв; в коде помечено как кандидат).
- [ ] Два параллельных play-автомата (viewport `PlayModeController` vs
  `EditorContext.beginPlay/endPlay`) требуют ручной парности на каждом вызове.
- [ ] Undo-команды держат протухающие хэндлы после delete/undo.
- [ ] Файловые глобалы «активного» контекста не чистятся после destroy.

**Контракты:**
- [ ] Семантика `RenderCommand` определена одним бэкендом: `opacity`/`exposure`
  обещаны контрактом, OpenGL их не реализует.
- [ ] `IRendererRegistry` обходится в продакшен-путях (`BackendInit` не умеет
  передать present-target); фабрика ресурсов достаётся `dynamic_cast`-ом.
- [ ] Миграции SKYB принял только формат сцен; project/package/terrain жёстко
  падают при несовпадении версии.
- [ ] C ABI адресует компоненты позиционными индексами при существующем
  стабильном `ComponentHandle`.

**Граница native/.NET:**
- [x] ~~Layout-паритет таблицы указателей только комментарием~~ — теперь
  14 указателей под двусторонней защитой: нативный `static_assert` на
  `sizeof(SkyScriptApi)` + managed-проверка `Marshal.SizeOf<Api>` в Install.
- [ ] Нет канала ошибок у мутирующих ABI-вызовов (молчаливый no-op) и
  exception-firewall под extern "C".
- [ ] Строки: 256-байтный колпак `ReadString` без ретрая; кодировка
  недокументирована.

**Тесты:** editor UI без автотестов; 43/100 ABI-функций не покрыты; краш
кейса валит весь сьют; OpenGL-бэкенд без рантайм-тестов.

## Проверенные тупики (не повторять вслепую)

- **SSE-интринсики в `core/math.hpp` для одиночных операций** — реализовано и
  замерено (GCC 13.3, −O3, SSE2): рукописный SIMD-путь для `quat*quat` /
  `rotate` / `compose` / `invCompose` оказался **в 1.4–2 раза медленнее**
  автогенерируемого кода из скалярных `constexpr`-формул (сборка лейнов для
  12-байтного `Vec3` и выгрузка через память съедают выигрыш). Код отозван;
  замер воспроизводится `tests/math_bench.cpp` (цель `sky_math_bench`,
  только Release). SIMD в математике имеет смысл только как **батч-API по
  SoA-массивам** (например, `composeBatch` для FrameBuilder) или для Mat4
  в рендерере — не как замена одиночных операций.

## Рекомендуемый порядок (пересобран под видение: симуляции + игры)

См. `docs/vision.md` — «юзабилити Unity, инженерия Unigine».

1. **B8 GUID ассетов** — фундамент ссылок, нужен обоим трекам.
2. **B16 Data-ассеты** — конфиги симуляций и игровые данные.
3. **B17 Модель кадра** (async-физика ∥ рендеру, camera-relative,
   async-загрузка) — реализует часть B12.
4. ~~**Детерминизм-пакет**~~ — сделан: порядок систем, Pcg32/Random,
   Time.TimeScale, бит-в-бит replay-тест (см. секцию выше).
5. **B1 Аудио → B18 Ввод → B3 Игровой UI → B4 Экспорт билда** — игровой
   трек (B18 перед B3: UI-хит-тесту нужна мышь).
6. **B2 Анимация, B5 Физика** — по мере надобности контента.
7. **ECS data-oriented хранилище** — при первом сценарии на тысячи агентов.
