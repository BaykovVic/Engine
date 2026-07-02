# Backlog редактора

Живой список того, что ещё нужно сделать в редакторе (Avalonia + нативный
мост). Обновляется по мере работы. Для каждого пункта помечено, где правка:
**[движок]** — C++/ABI, **[UI]** — Avalonia/C#, **[оба]** — и там, и там.

## Системы координат и гизмо ← приоритет

1. ✅ **СДЕЛАНО — собственная (локальная) СК + гизмо в локальной СК.**
   Тумблер Global/Local в шапке сцены; в Local оси гизмо строятся из мировой
   ориентации объекта (`get_world_transform`), драг идёт вдоль повёрнутых осей.
   Запись через `set_world_position` корректна и для вложенных объектов
   (мир→локаль через `invCompose`/`setWorldTransform` в ядре). Проверено
   юнит-тестами и живым скриншотом (наклонённые оси на повёрнутом объекте).
2. ✅ **СДЕЛАНО — трансформы через API в трёх пространствах.**
   `get_world_transform`/`set_world_position` (мир), `translate_self`
   (относительно себя, вдоль своих осей), `get_transform`/`set_position`
   (относительно родителя), `set_local_euler` (поворот относительно родителя).
   Покрыто `editor_bridge_tests` и `world_tests`.
3. ✅ **СДЕЛАНО — Rotate-гизмо.** Три спроецированных кольца вокруг X/Y/Z;
   драг по кольцу → поворот вокруг мировой оси через
   `sky_editor_rotate_world_axis` (инкрементально, корректно для вложенных
   объектов через `setWorldTransform`). Знак поворота — по положению камеры
   (`sky_editor_camera_position`).
4. ✅ **СДЕЛАНО — Scale-гизмо.** Оси с квадратными ручками (масштаб по
   локальной оси) + центральный хэндл равномерного масштаба, через
   `sky_editor_set_scale`.
5. ✅ **СДЕЛАНО — Синхронизация активного инструмента.** Тулбар
   hand/move/rotate/scale переключает тип гизмо (взаимоисключающие тумблеры
   через общий `MainViewModel.Tool`); хоткеи Q/W/E/R.
6. **Привязка (snapping)** **[UI]** — шаг сетки для move, угол для rotate,
   шаг для scale (с зажатым Ctrl, как в текущем 2D-вьюпорте Qt).
7. **Пивот: center vs pivot** **[оба]** — гизмо в центре bbox или в origin
   объекта.

## Inspector

- **Редактируемые rotation/scale** **[оба]** — сейчас редактируется только
  position. Нужны `set_rotation`/`set_scale` в ABI и поля в инспекторе
  (rotation как Эйлеровы углы).
- **Редактирование полей компонентов** **[оба]** — все типы (`float`/`int`/
  `bool`/`string`/`Vec3`) через ABI (`get_field`/`set_field`), а не только
  трансформ.
- **Add/Remove Component** **[оба]** — ✅ кнопка Add Component открывает флаут
  со списком зарегистрированных типов (`availableTypes`, с категориями); ✕ на
  карточке detach'ит. ABI `available_type_*`/`add_component`/`remove_component`,
  обе операции undo'абельны (attach/remove-команды с восстановлением полей).
  Осталось: поиск в списке + иконки категорий.
- **Rename объекта** **[оба]** — ✅ поле имени в инспекторе редактируемо,
  `sky_editor_rename_object` (undo через RenameCommand), иерархия обновляется.

## Панели на живые данные

- **Project** **[оба]** — ✅ обзор VFS (сетка файлов, крошки, иконки).
- **Console** **[оба]** — ✅ живой лог: бридж пишет события редактора
  (create/delete/scene/play/компоненты) в буфер с уровнями (LogLevel), ABI
  `log_count/level/text/clear`, панель ConsoleView (цвет по уровню, автоскролл,
  Clear). Осталось: пробросить и логи самих движковых подсистем.
- **Materials** **[оба]** — ✅ двухпанельный редактор: список материалов со
  свотчами + поля (baseColor/roughness/metallic/emissive + PBR-слоты
  albedo/normal/roughness/metallic/occlusion/height + uvTiling + parallax),
  всё на живых данных через material-ABI.
- **Terrain** **[оба]** — ✅ Sculpt (Raise/Lower/Smooth, Radius/Strength) +
  Map Generation (Seed/Generate, перегенерирует террейн). Осталось: рисование
  кистью по террейну во вьюпорте.
- **Packages** **[оба]** — ✅ список обнаруженных пакетов (имя/версия/статус) на
  живых данных (`PackageWorld.discoveredPackages`), Activate/Deactivate через
  `IPackageActivationService`, Refresh (rediscover). ABI `package_count/info/
  active/set_active/refresh`, панель PackagesView.
  ✅ P1 менеджера пакетов: честный semver (`*`, `>=`, `^`, точные версии),
  MVS-резолв по нескольким версиям одного пакета с детектом конфликтов,
  активация тянет зависимости, состояние персистится в `Packages/sky.lock`
  (версии + чексуммы манифестов + active) и восстанавливается при старте.
  ✅ P2: установка из трёх источников — локальный каталог, tarball
  (.tar/.tar.gz/.tgz), git URL/путь (`#tag` пинит тег) — через immutable
  version-addressed кэш (`<id>/<version>`); поле Install в панели Packages;
  ABI `package_install`. Дальше (P3+): вкладка Browse со статическим
  индексом реестра, учёт чексумм-дрейфа в UI, managed-код в пакетах.

## Операции со сценой

- **Undo/Redo** **[оба]** — ✅ `UndoStack` подключён к бриджу; правки
  (трансформ/поле/create/duplicate/delete) пишутся автоматически, драг
  коалесится в одну запись через `commit_edit`. ABI `undo/redo/can_*/label`,
  Edit-меню + Ctrl+Z/Y. `ObjectSnapshot` расширен значениями полей, поэтому
  undo delete / redo create восстанавливают материалы/mesh-ref. Проверено.
- **Play/Pause/Stop** **[оба]** — `PlayModeController` существует; связать
  транспорт-кнопки тулбара через ABI (тик плеймода в кадровом цикле).
- **New/Open/Save Scene** **[оба]** — ✅ через `ISceneRepository` (SKYB,
  `saveSceneAs`/`rootObjectsOf`) + ABI (`new/save/open_scene`) + File-меню с
  диалогами Avalonia (Ctrl+N/O/S). Round-trip графа объектов (иерархия +
  трансформы + компоненты) проверен; физика восстанавливается из
  rigidbody-компонентов. Осталось: heightfield террейна и live-состояние
  физики в формате (сейчас террейн — фикстура, переинициализируется).
- **Иерархия: drag-drop reparent, rename, контекст-меню** **[оба]** —
  `reparent`/`renameObject` через ABI; меню Create/Rename/Duplicate/Delete.

## Вьюпорт

- **Game-вью** **[оба]** — ✅ рендер через Main Camera (`render_game_offscreen`).
- **Paused/Playing-бейдж** **[UI]** — ✅ по состоянию плеймода.
- **Scene-гизмо** (угол сцены) **[оба]** — ✅ Unity-style виджет: оси X/Y/Z
  ориентируются по камере (`camera_basis`), клик по конусу снапит вид
  (`look_along_axis`), метка Persp/Iso переключает проекцию. Осталось: сетка
  пола.
- **Framing (F)** — навести камеру на выбранный (`frame_object` уже есть).
- **2D-режим** — ✅ ортографический вид на плоскость YZ (взгляд вдоль +X, Z
  вправо/Y вверх). Тумблер 3D/2D в шапке сцены; камера блокирует orbit,
  pan/zoom работают; `project`/`rayThrough`/`rayOrigin` согласованы с орто,
  так что гизмо и пик совпадают. ABI `sky_editor_set_view_2d`.
- **Оверлей статистики** — tri-count свести из рендер-команд (fps есть).

## Раскладка и тема

- **Полный порт `SkyStyles.axaml`** **[UI]** — ✅ тема доведена до дизайна
  (карточки, табы, шапки, чипы, бейджи, статус-бар, dock-chrome) + polish-пасс
  по контролам из `SkyStyles.axaml.reference`: ComboBox, CheckBox (акцент),
  Slider, тонкий ScrollBar, ContextMenu/MenuFlyout/FlyoutPresenter, Separator,
  ToolTip, ListBoxItem. Полный 1:1 (переименование классов вьюх под
  kebab-конвенцию эталона) не делался — расхождение только в именах.
- **Докинг** **[UI]** — ✅ сделано на Dock.Avalonia: панели Hierarchy/Scene/
  Game/Inspector/Terrain/Materials/Project/Console/Packages — перетаскиваемые,
  отрываемые, группируемые вкладки; Layout → Reset Layout. Осталось:
  **персист раскладки на диск** (Dock.Serializer пока несовместимой версии).

## Платформа

- **Валидация macOS-слоя** на реальном маке (Cocoa + Metal-surface,
  см. `docs/platform-macos.md`).
- **CI** (GitHub Actions) — Linux обязательно, macOS-раннер для проверки
  Apple-путей.
