# Backlog редактора

Живой список того, что ещё нужно сделать в редакторе (Avalonia + нативный
мост). Обновляется по мере работы. Для каждого пункта помечено, где правка:
**[движок]** — C++/ABI, **[UI]** — Avalonia/C#, **[оба]** — и там, и там.

## Системы координат и гизмо ← приоритет

Сейчас move-гизмо работает только в **мировой** СК (оси X/Y/Z мира). Нужно:

1. **Собственная (локальная) СК объекта** **[оба]** — переключатель Global/Local
   (как в Unity). В Local оси гизмо повёрнуты по ориентации объекта
   (`worldTransform.rotation`), и перемещение идёт вдоль локальных осей.
   - ABI: вернуть мировую ориентацию объекта (есть `worldTransform`, нужно
     отдать кватернион) — `sky_editor_world_rotation` или расширить
     `world_position` до полного трансформа.
   - UI: тумблер «Global/Local» в тулбаре сцены; гизмо строит оси как
     `rotate(objRotation, axis)` вместо мировых; драг считает смещение вдоль
     повёрнутой оси.
2. **Гизмо в локальной СК** **[UI]** — отрисовка осей по локальному базису,
   ручки и захват вдоль них (использует п.1).
3. **Rotate-гизмо** **[оба]** — кольца вокруг X/Y/Z; драг по кольцу → поворот.
   ABI: `sky_editor_set_rotation` (сейчас есть только `set_position`).
4. **Scale-гизмо** **[оба]** — ручки масштаба по осям + равномерный центр.
   ABI: `sky_editor_set_scale`.
5. **Синхронизация активного инструмента** **[UI]** — тулбар move/rotate/scale
   (hand/move/rotate/scale) переключает тип гизмо; хоткеи Q/W/E/R.
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

- **Project** **[оба]** — обзор VFS (`list`/навигация по папкам), иконки
  folder/file/image, метка пути. ABI для листинга VFS.
- **Console** **[оба]** — поток лога движка (`ILogger`) с уровнями
  info/warn/error. ABI-сток логов в managed.
- **Materials** **[оба]** — список материалов + редактор: baseColor,
  roughness/metallic, emissive и **PBR-слоты** (Normal/Roughness/Metallic/
  Occlusion/Height + UV tiling + parallax) — движок их уже поддерживает,
  осталось вывести в UI. ABI для перечисления/правки материалов.
- **Terrain** **[оба]** — кисти Raise/Lower/Smooth, Radius/Strength, Map
  Generation (Seed/Generate). ABI поверх `ITerrainService`/mapgen.
- **Packages** **[оба]** — таблица пакетов, Activate/Deactivate/Refresh.

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

- **Сетка пола** и оси мира **[UI/движок]**.
- **Framing (F)** — навести камеру на выбранный (`frame_object` уже есть).
- **2D-режим** — переключатель 3D/2D уже в UI, привязать к ортокамере.
- **Оверлей статистики** — tris/fps (fps есть; tri-count свести из
  рендер-команд).

## Раскладка и тема

- **Полный порт `SkyStyles.axaml`** **[UI]** — разбить мульти-типовые
  селекторы дизайна под Avalonia (см. `SkyStyles.axaml.reference`).
- **Докинг + персист раскладки** **[UI]** — Dock.Avalonia, Window-меню
  (видимость панелей), Reset Layout, сохранение состояния.

## Платформа

- **Валидация macOS-слоя** на реальном маке (Cocoa + Metal-surface,
  см. `docs/platform-macos.md`).
- **CI** (GitHub Actions) — Linux обязательно, macOS-раннер для проверки
  Apple-путей.
