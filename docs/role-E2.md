# Техническое задание · Контур E2 «Рендеринг»

**Область ответственности.** Контракт рендера, Vulkan-бэкенд, PBR-освещение, тени, построитель кадра.

Все пути и имена методов взяты из фактического репозитория и совпадают с проектом 1:1. «Сделай файл» — файл создаётся на этом этапе; «Дополни файл» — в существующий файл добавляются перечисленные методы.


---

## Этап 1 (Неделя 1). Vulkan от инициализации до кадра в файле

**Общее описание задач контура.**

Определить контракт рендера и реализовать Vulkan-бэкенд до отрисовки и чтения кадра.

- **Сделай файл** `engine/rendering/include/sky/rendering/rendering.hpp`
  В файле должны быть: `IRenderSurface`, `width`, `height`, `present`, `IRenderResourceFactory`, `destroy`, `IRenderer`, `backendName`, `attachSurface`, `submit`, `renderFrame`
- **Сделай файл** `engine/rendering/include/sky/rendering/renderer_registry.hpp`
  В файле должны быть: `IRendererRegistry`, `registerBackend`, `availableBackends`, `hasBackend`, `createRendererRegistry`
- **Сделай файл** `engine/rendering/include/sky/rendering/null_renderer.hpp`
  В файле должны быть: `NullRenderer`, `frameCount`, `commandsInLastFrame`, `liveResourceCount`, `createNullRenderer`
- **Сделай файл** `engine/rendering/src/null_renderer.cpp`
  В файле должны быть: `OffscreenSurface`, `submit`, `renderFrame`, `present`, `invalid`, `destroy`, `createNullRenderer`
- **Сделай файл** `engine/rendering/src/renderer_registry.cpp`
  В файле должны быть: `RendererRegistryImpl`, `createNullRenderer`, `registerBackend`, `availableBackends`, `hasBackend`, `createRendererRegistry`
- **Сделай файл** `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
  В файле должны быть: `VulkanRenderer`, `ready`, `readbackFrame`, `frameWidth`, `frameHeight`, `presentedFrames`
- **Дополни файл** `engine/rendering_vulkan/src/vulkan_renderer.cpp`
  В файле должны быть: `initInstanceAndDevice`, `createShader`, `findMemoryType`, `createImage`, `createBuffer`, `renderFrame`, `readbackFrame`, `createVulkanRenderer`
- **Сделай файл** `tests/vulkan_tests.cpp`
  В файле должны быть: `testVulkanFrame`, `createVulkanRenderer`, `backendName`, `frameWidth`, `submit`, `renderFrame`, `readbackFrame`, `pixelAt`, `testVulkanTexturing`, `createTextureFromData`, `commands`, `testVulkanPbrMaps`, `renderCube`, `testVulkanResourcesAndRegistry`, `triangle`, `createMeshFromData`, `destroy`, `createRendererRegistry`, `registerVulkanBackend`, `hasBackend`, `create`, `testSwapchainPresentation`, `createX11WindowSystem`, `createWindow`, `pumpEvents`, `nativeHandles`, `presentedFrames`, `destroyWindow`, `main`, `summary`

**На выходе должно получиться (список артефактов):**
- Библиотека `sky_rendering_vulkan` собрана; формируется `triangle.png`.
- Тест `vulkan_tests` зелёный на lavapipe.

**Критерий правильности:** На `triangle.png` центральный пиксель отличается от углового; `readbackFrame()` непустой.


---

## Этап 2 (Неделя 2). Меши, камера, освещение, построитель кадра

**Общее описание задач контура.**

Добавить отрисовку мешей, камеру, свет и построитель кадра по сцене.

- **Дополни файл** `engine/rendering_vulkan/src/vulkan_renderer.cpp`
  В файле должны быть: `createMeshFromData`, `submit`
- **Сделай файл** `editor/shell/src/frame_builder.hpp`
  В файле должны быть: `FrameBuilder`, `setCamera`, `cameraPose`, `forEachObject`, `componentOfType`, `worldTransform`, `dataset`, `applyMaterial`, `uploadedMesh`, `findByName`, `exists`, `childrenOf`, `componentsOf`, `descriptorOf`, `invalid`, `field`, `findMaterial`, `material`, `uploadedTexture`, `resolve`, `resolveMeshRef`, `buildPrimitive`, `path`, `loadFbxMesh`, `loadGltfMesh`, `loadObjMesh`, `push`, `loadPngImage`
- **Сделай файл** `editor/viewport_bridge/include/sky/editor/viewport/viewport_bridge.hpp`
  В файле должны быть: `IPlayModeController`, `play`, `pause`, `stop`, `state`, `onStateChanged`, `IRuntimePreviewHost`, `attachSurface`, `detachSurface`, `context`

**На выходе должно получиться (список артефактов):**
- Демо-сцена рендерится с освещением; `FrameBuilder` даёт непустой поток команд.

**Критерий правильности:** Кадр демо-сцены содержит освещённые меши; поток `RenderCommand` не пуст.


---

## Этап 3 (Недели 3–4). Выбор объекта, проекция, небо

**Общее описание задач контура.**

Реализовать камеру редактора, выбор объекта и небо.

- **Сделай файл** `editor/shell/src/editor_camera.hpp`
  В файле должны быть: `rotation`, `orthoHalfHeight`, `orthoHeight`, `lookAlong`, `pose`, `rotate`, `orbit`, `clamp`, `zoom`, `pan`
- **Дополни файл** `engine/rendering_vulkan/src/vulkan_renderer.cpp`
  В файле должны быть: `drawBuffer`

**На выходе должно получиться (список артефактов):**
- Клик по вьюпорту возвращает объект; мировая точка проецируется в экранную; небо отрисовано.

**Критерий правильности:** Центральный луч кадрированной камеры попадает в объект; проекция origin объекта близка к центру экрана.


---

## Этап 4 (Недели 5–6). Текстуры и слоты PBR

**Общее описание задач контура.**

Добавить загрузку текстур и привязку слотов PBR-материала.

- **Дополни файл** `engine/rendering_vulkan/src/vulkan_renderer.cpp`
  В файле должны быть: `uploadTexture`, `bindMaterial`, `createTextureFromData`
- **Дополни файл** `engine/scene/src/scene_authoring.cpp`
  В файле должны быть: `createPrimitive`

**На выходе должно получиться (список артефактов):**
- Материалы используют текстурные слоты; сфера/плоскость строятся процедурно.

**Критерий правильности:** Отсутствующие слоты заменяются нейтральными значениями; примитивы не куб-заглушки.


---

## Этап 5 (Недели 7–8). Тени

**Общее описание задач контура.**

Реализовать теневое картирование от направленного источника.

- **Сделай файл** `engine/rendering_vulkan/shaders/shadow.vert`
  В файле должны быть: `layout`, `main`, `vec4`
- **Дополни файл** `engine/rendering_vulkan/src/vulkan_renderer.cpp`
  В файле должны быть: `initShadowResources`

**На выходе должно получиться (список артефактов):**
- Объекты отбрасывают тени в редакторе и плеере (скриншот).

**Критерий правильности:** На кадре видна тень под объектом; тень движется вместе с падающим телом.
