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
- **Add/Remove Component** **[UI]** — кнопка Add Component открывает попап со
  списком типов (из `availableTypes`) с категориями (Rendering/Physics/…,
  поле `category` уже есть) и поиском; крестик на карточке — detach.

## Панели на живые данные

- **Project** **[оба]** — ✅ обзор VFS (сетка файлов, крошки, иконки).
- **Console** **[оба]** — поток лога движка (`ILogger`) с уровнями
  info/warn/error. ABI-сток логов в managed (ещё placeholder).
- **Materials** **[оба]** — ✅ двухпанельный редактор: список материалов со
  свотчами + поля (baseColor/roughness/metallic/emissive + PBR-слоты
  albedo/normal/roughness/metallic/occlusion/height + uvTiling + parallax),
  всё на живых данных через material-ABI.
- **Terrain** **[оба]** — ✅ Sculpt (Raise/Lower/Smooth, Radius/Strength) +
  Map Generation (Seed/Generate, перегенерирует террейн). Осталось: рисование
  кистью по террейну во вьюпорте.
- **Packages** **[оба]** — таблица пакетов, Activate/Deactivate/Refresh (ещё
  placeholder).

## Операции со сценой

- **Undo/Redo** **[оба]** — командный стек уже есть в движке (`UndoStack`);
  вывести через ABI и в Edit-меню (Ctrl+Z/Ctrl+Y).
- **Play/Pause/Stop** **[оба]** — `PlayModeController` существует; связать
  транспорт-кнопки тулбара через ABI (тик плеймода в кадровом цикле).
- **New/Open/Save Scene** **[оба]** — `ISceneRepository` (SKYB) через ABI +
  File-меню; диалоги файлов Avalonia.
- **Иерархия: drag-drop reparent, rename, контекст-меню** **[оба]** —
  `reparent`/`renameObject` через ABI; меню Create/Rename/Duplicate/Delete.

## Вьюпорт

- **Game-вью** **[оба]** — ✅ рендер через Main Camera (`render_game_offscreen`).
- **Paused/Playing-бейдж** **[UI]** — ✅ по состоянию плеймода.
- **Мини-ось** (внизу слева) и сетка пола **[UI/движок]** — осталось.
- **Framing (F)** — навести камеру на выбранный (`frame_object` уже есть).
- **2D-режим** — ✅ ортографический вид на плоскость YZ (взгляд вдоль +X, Z
  вправо/Y вверх). Тумблер 3D/2D в шапке сцены; камера блокирует orbit,
  pan/zoom работают; `project`/`rayThrough`/`rayOrigin` согласованы с орто,
  так что гизмо и пик совпадают. ABI `sky_editor_set_view_2d`.
- **Оверлей статистики** — tri-count свести из рендер-команд (fps есть).

## Раскладка и тема

- **Полный порт `SkyStyles.axaml`** **[UI]** — ✅ по сути сделано: тема
  доведена до дизайна (карточки, табы, шапки, чипы, бейджи, статус-бар) в
  `SkyTheme.axaml`; dock-chrome стилизован.
- **Докинг** **[UI]** — ✅ сделано на Dock.Avalonia: панели Hierarchy/Scene/
  Game/Inspector/Terrain/Materials/Project/Console/Packages — перетаскиваемые,
  отрываемые, группируемые вкладки; Layout → Reset Layout. Осталось:
  **персист раскладки на диск** (Dock.Serializer пока несовместимой версии).

## Платформа

- **Валидация macOS-слоя** на реальном маке (Cocoa + Metal-surface,
  см. `docs/platform-macos.md`).
- **CI** (GitHub Actions) — Linux обязательно, macOS-раннер для проверки
  Apple-путей.
