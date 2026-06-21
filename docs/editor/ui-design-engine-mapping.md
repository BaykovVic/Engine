# Ревизия дизайна редактора и маппинг на движок

Документ фиксирует **весь функционал, заложенный в дизайне** (пакет Claude Design,
`Themes/SkyDark.axaml` + `Themes/SkyStyles.axaml` + скриншоты), и сопоставляет
каждую фичу с реальными возможностями движка. Это контракт, против которого
строится новый интерфейс на Avalonia: каждый UI-элемент привязывается к уже
существующему API (готовое — мапим) либо помечается как требующий доработки
движка (неготовое — реализуем до отрисовки UI).

Статусы:

- **ГОТОВО** — API движка существует, нужно только подключить к UI-хуку.
- **ЧАСТИЧНО** — есть основа, но не на уровне движка (живёт в Qt-редакторе) или
  покрывает не весь сценарий дизайна.
- **НЕТ** — на стороне движка отсутствует, требуется реализация.

Колонка «Хук» — это `Classes`/`#Name`/ресурс из дизайн-пакета, к которому
привязывается фича в новом Avalonia-UI.

---

## 1. Тулбар: инструменты трансформации и транспорт

| Фича | Хук | Движок / редактор | Статус |
|---|---|---|---|
| Hand (пан камеры) | `ToggleButton.tool` + `IconHand` | `ViewportWidget`/`SceneView3D` пан и орбита | ГОТОВО |
| Move / Rotate / Scale гизмо | `ToggleButton.tool` + `IconMove/Rotate/Scale` | `enum TransformTool`, `drawGizmo()`, `applyDrag()` (`viewport_widget.cpp`), 3D-пикинг и гизмо в `scene_view_3d.cpp` | ГОТОВО |
| Состояние инструмента (один активный) | `:checked` на `ToggleButton.tool` | `setTool()` + сигнал `toolChanged`, хоткеи Q/W/E/R | ГОТОВО |
| Play | `ToggleButton#playButton` + `IconPlay` | `PlayModeController::play()` → `ISceneRuntime::tick()` (ECS+физика+скрипты) | ГОТОВО |
| Pause | `ToggleButton#pauseButton` + `IconPause` | `PlayModeController::pause()` | ГОТОВО |
| Stop | `Button#stopButton` + `IconStop` | `PlayModeController::stop()` (ревертит в Editing) | ГОТОВО |
| Бейдж «Paused/Playing» в вьюпорте | `Border.play-badge` | `PlayModeState`, оверлей в вьюпорте | ГОТОВО |
| Layout-дропдаун | `Button#layoutButton` | контракт `IEditorShell::applyLayout()` объявлен, реализации нет | ЧАСТИЧНО |
| 2D-переключатель | `ToggleButton#toggle2D` | `ViewportWidget` (2D) ↔ `SceneView3D` (3D) уже существуют | ГОТОВО |

**Доработка:** только сохранение/восстановление и набор предустановленных
раскладок (см. §10).

---

## 2. Hierarchy (дерево сцены)

| Фича | Хук | Движок | Статус |
|---|---|---|---|
| Дерево объектов с вложенностью | `TreeView#hierarchyTree` | `TransformNode`, `ObjectHandle`, `childrenOf()`, `rootObjects` | ГОТОВО |
| Глифы типов (mesh/light/camera/empty/terrain) | `Path.icon` + `IconMesh/Light/Camera/Empty/Terrain` | тип выводится по прикреплённым компонентам; **Camera как компонент отсутствует** | ЧАСТИЧНО |
| Выделение строки | `TreeViewItem:selected` | общий selection-поток viewport→панели | ГОТОВО |
| Поиск | `TextBox` + `IconSearch` | `IObjectQueryService::findByName()` | ГОТОВО |
| Кнопка «+» (создать) | `Button.tool` + `IconAdd` | см. контекстное меню ниже | ЧАСТИЧНО |
| Контекст: Create Empty | пункт меню | `IObjectFactory::createObject()` | ГОТОВО |
| Контекст: Create Cube/примитив | пункт меню | в движке нет API примитивов; в редакторе есть `createCrate()` | ЧАСТИЧНО |
| Контекст: Rename | пункт меню | `renameObject()` | ГОТОВО |
| Контекст: Duplicate | пункт меню | клонирование живёт в редакторе (`cloneSubtree()`), не в ядре | ЧАСТИЧНО |
| Контекст: Delete | пункт меню | `destroyObject()` (рекурсивно) | ГОТОВО |

**Доработки:** (a) компонент **Camera**; (b) перенос примитивов и
duplicate/clone в API движка (сейчас только в Qt-редакторе).

---

## 3. Scene / Game вьюпорт

| Фича | Хук | Движок | Статус |
|---|---|---|---|
| Вкладки Scene / Game (без «Scene 2D») | `DocumentTabStripItem` / `TabControl.dock-tabs` | две вьюхи + 2D через `#toggle2D` | ГОТОВО |
| Встраивание рендера в область Scene | host-контрол под `NativeControlHost` | `VulkanPresentTarget{display,window}`, X11WindowSystem отдаёт нативные хэндлы | ГОТОВО (на движке); биндинг в Avalonia — новый |
| Пикинг объекта кликом | — | `pickObject()` (AABB 2D / рейкаст 3D) → `objectPicked` | ГОТОВО |
| Оси/гизмо выделения | `Path.icon.x/.y/.z` (цвета осей) | отрисовка гизмо в вьюпортах | ГОТОВО |
| Оверлей статистики (tris/fps) | `TextBlock.mono` | счётчики кадра редактора | ЧАСТИЧНО (fps есть; tris-метрику свести из рендер-команд) |

**Доработка:** связать Avalonia `NativeControlHost` с `VulkanPresentTarget`
(новый код интеграции, движок к этому готов); собрать tri-count из рендер-стата.

---

## 4. Inspector

| Фича | Хук | Движок | Статус |
|---|---|---|---|
| Пустое состояние «No object selected» | `Border.card` + `IconEmpty` | состояние UI | ГОТОВО (UI) |
| Карточки компонентов | `Border.card`, `TextBlock#componentName` | `componentsOf()`, `descriptorOf()` | ГОТОВО |
| Поля компонента (рефлексия) | поля карточки | `ComponentDescriptor.fields`, `FieldValue` (float/int/bool/string/Vec3), `field()/setField()` | ГОТОВО |
| Transform с осями X/Y/Z | `TextBlock#axisLabel.x/.y/.z`, `NumericUpDown` | `localTransform()/setLocalTransform()` (pos/rot/scale) | ГОТОВО |
| Имя типа скрипта | `TextBlock#scriptTypeName` | дескриптор `sky.script` (`managedTypeName=Game.Behaviour`) | ГОТОВО |
| Кнопка Add Component | `Button#addComponentButton` | `attach(object, typeId)` | ГОТОВО |
| Попап Add Component с поиском и категориями | `IconSearch`, секции Rendering/Physics | список через `registerComponentType` (Mesh/BoxCollider/Rigidbody/Script/Light); категорий пока нет | ЧАСТИЧНО |
| Удалить компонент | `Button#removeComponentButton` | `detach(component)` | ГОТОВО |

**Доработки:** (a) категоризация типов компонентов (поле «category» в
дескрипторе) для секций попапа; (b) типы Camera и при желании Sphere/Capsule
Collider (в физике уже есть формы `Sphere/Capsule/TerrainHeightfield`).

---

## 5. Terrain

| Фича | Хук | Движок | Статус |
|---|---|---|---|
| Кисти Raise / Lower / Smooth | `ToggleButton.sculpt` | `ITerrainService::applyEdit(TerrainEdit{operation})` | ГОТОВО |
| Radius / Strength слайдеры | `Slider` | поля `TerrainEdit.radius/strength` | ГОТОВО |
| Map Generation: Seed | `NumericUpDown` | `GenerationProfile.seed` | ГОТОВО |
| Map Generation: Generate | `Button` + `IconRefresh` | `IGenerationPipeline::generate()` (value-noise, 2 октавы) | ГОТОВО |

Полностью закрыто движком; уже работает в `editor/tools/terrain_panel.cpp`.

---

## 6. Materials (PBR-редактор)

| Фича | Хук | Движок | Статус |
|---|---|---|---|
| Список материалов | `ListBox` (Grassland/Cliff Rock/...) | `IMaterialLibrary::allMaterials()` | ГОТОВО |
| New Material | `Button.secondary` | `createMaterial()` | ГОТОВО |
| Name | `TextBox` | `MaterialDesc.name` | ГОТОВО |
| Base Color | color swatch | `MaterialDesc.baseColor` | ГОТОВО |
| Roughness / Metallic | `Slider` | `MaterialDesc.roughness/metallic` | ГОТОВО |
| Emissive | swatch | `MaterialDesc.emissive` | ГОТОВО |
| Слот Albedo | map-слот | `MaterialDesc.texturePath`, сэмплится в OpenGL/Vulkan | ГОТОВО |
| Слот Normal | map-слот | в `MaterialDesc` и шейдерах отсутствует; нет тангентов в вершине | НЕТ |
| Слот Roughness (текстура) | map-слот | отсутствует | НЕТ |
| Слот Metallic (текстура) | map-слот | отсутствует | НЕТ |
| Слот Occlusion | map-слот | отсутствует | НЕТ |
| Слот Height + Depth (parallax) | map-слот + `Slider` | отсутствует | НЕТ |
| UV Tiling | `NumericUpDown` U/V | в `MaterialDesc` нет | НЕТ |
| PNG-импорт текстур | — | `createPngImporter()` → RGBA8, `createTextureFromData()` | ГОТОВО |

**Доработка (крупнейшая):** расширить `MaterialDesc` до многослотового PBR
(normal/roughness/metallic/occlusion/height + uvTiling + parallaxDepth),
завести соответствующие сэмплеры и тангент-базис в обоих бэкендах и шейдерах.
**Решение по объёму v1 нужно с пользователем** (полный PBR vs только Albedo+Normal).

---

## 7. Project (браузер ассетов)

| Фича | Хук | Движок | Статус |
|---|---|---|---|
| Сетка папок/файлов | `ListBox#projectEntries` (WrapPanel) | `IVirtualFileSystem::list()`, навигация по папкам | ГОТОВО |
| Иконки folder/file/image | `IconFolder/File/Image` | различение dir/file по листингу | ГОТОВО |
| Метка текущего пути | `TextBlock#projectPathLabel` | путь VFS | ГОТОВО |
| Привязка файла к типу ассета | — | `IAssetResolver::findBySourcePath()`, `AssetDatabase` | ГОТОВО |

Закрыто; работает в `editor/tools/project_panel.cpp`.

---

## 8. Console

| Фича | Хук | Движок | Статус |
|---|---|---|---|
| Вывод лога (моноширинный) | `TextBox#consoleOutput` | `ILogger`, адаптер-сток в редакторе | ГОТОВО |
| Уровни info/warn/error | `TextBlock.log-info/.log-warn/.log-error` | `LogLevel` (Trace…Critical) | ГОТОВО |
| Тулбар консоли | `Border#consoleToolbar` | UI | ГОТОВО |
| Clear | `Button#consoleClearButton` + `IconClear` | очистка буфера | ГОТОВО |

Закрыто; работает в `editor/tools/console_panel.cpp` (`ConsoleLoggerAdapter`).

---

## 9. Packages

| Фича | Хук | Движок | Статус |
|---|---|---|---|
| Таблица пакетов | `ListBox.packages` + `Border.table-header` | `discoverPackages()`, `discoveredPackages()` | ГОТОВО |
| Колонки: имя/версия/статус/extensions | ячейки | `PackageManifest`, `ExtensionPoint` | ГОТОВО |
| Activate / Deactivate | `Button` | `IPackageActivationService::activate/deactivate` | ГОТОВО |
| Refresh | `Button` + `IconRefresh` | `discoverPackages()` | ГОТОВО |

Закрыто; работает в `editor/tools/package_panel.cpp`.

---

## 10. Меню, раскладки, общие сервисы

| Фича | Хук | Движок / редактор | Статус |
|---|---|---|---|
| Меню File: New/Open/Save Scene | `Menu` | `ISceneRepository::saveScene/loadScene` (SKYB, схема 1.1) | ГОТОВО |
| File: Build Settings, Revert, Quit | `Menu` | Quit/Revert — UI; Build Settings — отдельная фича, вне ядра | ЧАСТИЧНО |
| Edit: Undo/Redo | `Menu` | `UndoStack` (100), команды Transform/Rename/Create/Duplicate/Delete/Reparent/FieldEdit/Material* | ГОТОВО |
| GameObject (создание) | `Menu` | `createObject()` (+ примитивы — см. §2) | ЧАСТИЧНО |
| Window: видимость панелей | `Menu` + `IconCheck` | в Qt — авто-тогглы доков; в Avalonia делается заново | ЧАСТИЧНО |
| Window: Reset Layout / именованные раскладки | пункт меню | `DockLayout`/`IEditorShell` объявлены, не реализованы | ЧАСТИЧНО |
| Контекстные меню/попапы/тултипы | `ContextMenu`, `MenuFlyoutPresenter`, `ToolTip` | стандартные Avalonia, стили готовы | ГОТОВО (стили) |

---

## Сводка пробелов (что «неготовое» реализуем до отрисовки UI)

Отсортировано по влиянию на дизайн. Статус обновляется по мере закрытия.

1. ✅ **СДЕЛАНО — полный metal-rough PBR.** `MaterialDesc` и `DrawMesh` расширены
   слотами Normal/Roughness/Metallic/Occlusion/Height + UV Tiling + parallax Depth;
   оба бэкенда (OpenGL и Vulkan) переведены на Cook-Torrance GGX. Тангент-базис
   считается из экранных производных, поэтому вершинный формат остался
   position+normal+uv (импортёры не тронуты). Отсутствующие слоты подставляют
   нейтральные дефолты (white / flat-normal). Проверено тестом occlusion-карты в
   Vulkan; все 18 наборов зелёные.
2. ✅ **СДЕЛАНО — компонент Camera.** Зарегистрирован тип `sky.camera`
   (projection/fieldOfView/nearPlane/farPlane), прикреплён к Main Camera в демо-сцене.
3. ✅ **СДЕЛАНО — примитивы и duplicate в ядре.** Новый модуль
   `engine/scene/scene_authoring` (`createPrimitive`, `duplicateObject`), не зависит
   от UI/рендера/физики. `duplicateObject` копирует и значения полей компонентов
   (раньше Qt-клон их терял — баг исправлен и в редакторе). Покрыто тестом в
   `scene_tests`.
4. ✅ **СДЕЛАНО — категории компонентов.** Поле `category` в `ComponentDescriptor`;
   типы помечены Rendering/Physics/Scripting — основа секций попапа Add Component.
5. **Персист раскладки** — реализовать `IEditorShell::applyLayout()` +
   сохранение/восстановление состояния доков (Dock.Avalonia). Делается на этапе UI.
6. 🏗 **В РАБОТЕ — C#-editor API + Avalonia-редактор.** Рабочий вертикальный
   срез уже собран и проверен:
   - Нативный C ABI `editor/native_bridge` (`libsky_editor_bridge.so`,
     `extern "C"`, 18 функций — lifecycle, иерархия, трансформы, компоненты,
     createPrimitive/duplicate/delete, **attach/render/detach вьюпорта**).
   - Avalonia-приложение `editor/avalonia` (.NET 8): тема дизайна (токены
     `SkyDark.axaml`), раскладка панелей, Hierarchy/Inspector на живых данных
     движка через P/Invoke, и **встроенный Vulkan-вьюпорт** — свопчейн движка
     рендерит сцену в `NativeControlHost` дочерним окном (UI владеет окном,
     рендер встраивается). Общая логика «сцена→команды» вынесена в
     `frame_builder.hpp` (делят плеер и мост).
   - Проверено: `editor_bridge_tests` (C-ABI + рендер в реальное X11-окно),
     и реальный запуск под Xvfb со скриншотом (сцена видна во вьюпорте).
   Дальше: оставшиеся панели на живые данные, орбитальная камера и пикинг во
   вьюпорте, undo/redo и плеймод через ABI, полный порт дизайн-стилей
   (`SkyStyles.axaml`) под Avalonia, докинг (Dock.Avalonia) и персист раскладки.
7. **Мелочи:** tri-count в оверлей статистики; Build Settings — отдельной фичей.

## Что НЕ требует движковых правок (мапится напрямую)

Гизмо и пикинг, плеймод (play/pause/stop) и кадровый цикл, undo/redo и командная
шина, общий selection, террейн (скульпт + mapgen), CRUD материалов (без новых
слотов), проект/VFS/ассет-БД, пакеты, консоль с уровнями, save/load сцены,
встраивание Vulkan-свопчейна (движок отдаёт нативные хэндлы — нужен только
Avalonia-биндинг).

## Порядок работ

1. Закрыть пробелы 2–4 (Camera, примитивы/duplicate в ядре, категории) — мелкие,
   разблокируют Hierarchy/Inspector/GameObject-меню «как в дизайне».
2. Решить объём PBR (пробел 1) и реализовать выбранный вариант — разблокирует
   Materials-панель целиком.
3. Реализовать персист раскладки (пробел 5).
4. Спроектировать C#-editor API (пробел 6) — параллельно с UI-каркасом.
5. **Только после этого** строить весь интерфейс на Avalonia уже с финальными
   хуками из дизайн-пакета (`Sky*`-ресурсы, `#Name`/`Classes` из таблиц выше).
