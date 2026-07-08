# Техническое задание · Контур E2 «Рендеринг»

**Область ответственности.** Контракт рендера (поток команд отрисовки),
Vulkan-бэкенд (закадровый и оконный), PBR-освещение, тени, построитель кадра
из сцены. Механизм чтения кадра в изображение — основа верификации всего
проекта, поэтому он готов на первом этапе.

Каждый этап (неделя) разбит на **фичи** `feature/<название>` — логические
группы заданий недели. Одна фича = одна ветка в git и один запрос на слияние.
У каждого метода указаны: **сигнатура**, **что делает**, **параметры** и **что
возвращает**.

## Обозначения (C++ и Vulkan)

- `virtual … = 0` — чисто виртуальный метод (контракт, реализуется отдельным классом).
- `std::unique_ptr<T>` — владеющий указатель (сам освобождает объект).
- `std::vector<T>` — динамический массив; `std::span<const T>` — «взгляд» на непрерывный участок массива без копирования.
- `std::optional<T>` — значение или «ничего».
- **Закадровый рендер (offscreen)** — рисование не в окно, а в текстуру в памяти; из неё кадр можно прочитать в файл.
- **SPIR-V** — бинарный формат шейдеров для Vulkan; исходник `.vert`/`.frag` компилируется в `.spv` утилитой `glslangValidator`.
- **lavapipe** — программный (без видеокарты) драйвер Vulkan; используется в CI.
- **PBR** — физически корректное освещение (модель metal-rough).

---

## Этап 1 (Неделя 1). Vulkan от инициализации до кадра в файле

**Общее описание задач этапа.** Контракт рендера и Vulkan-бэкенд до отрисовки
треугольника и чтения кадра в изображение. Три фичи.

### feature/render-contract

Общий (не только Vulkan) контракт рендера — поток команд и интерфейс рендерера.

#### Файл `engine/rendering/include/sky/rendering/rendering.hpp`
Перечисления и структура команды.
- `enum class RenderCommandType { BeginFrame, SetViewport, SetCamera, AddLight, SetSky, BindPipeline, DrawMesh, EndFrame }`
  Что делает: вид команды отрисовки в потоке.
- `enum class LightType { Directional = 0, Point = 1 }` — вид источника света.
- `struct RenderCommand { RenderCommandType type; core::Transform transform; core::Vec3 color; float fovDegrees; float orthoHeight; … }`
  Что делает: одна backend-независимая команда. Поля используются по-разному в зависимости от `type` (для `SetCamera` — поза камеры и проекция, для `AddLight` — поза и цвет света, для `DrawMesh` — материал и текстуры).

**`class IRenderer`** — контракт рендерера.
- `virtual std::string backendName() const = 0` — Возвращает: имя бэкенда ("vulkan"/"opengl").
  Реализация: просто возвращает строковый литерал имени бэкенда (`return "vulkan";` в `VulkanRendererImpl`, `"null"` в `NullRendererImpl`); состояние не трогает.
- `virtual void attachSurface(IRenderSurface& surface) = 0` — привязывает поверхность вывода. Параметры: `surface`.
  Реализация: сохраняет адрес поверхности в поле-указатель (`renderSurface_ = &surface;` / `surface_ = &surface;`), чтобы в конце `renderFrame` вызвать `present()`.
- `virtual void submit(std::span<const RenderCommand> commands) = 0` — принимает поток команд кадра. Параметры: `commands`.
  Реализация: дописывает пришедшие команды в конец накопителя `pending_` — `pending_.insert(pending_.end(), commands.begin(), commands.end())`; отрисовки не выполняет.
- `virtual void renderFrame() = 0` — рисует накопленный кадр.
  Реализация (null-эталон): запоминает `commandsInLastFrame_ = pending_.size()`, очищает `pending_`, инкрементирует `frameCount_` и, если поверхность привязана, зовёт `surface_->present()`. Vulkan-версия описана ниже.

**`class IRenderResourceFactory`** — создание ресурсов GPU.
- `virtual RenderResourceHandle createMeshFromData(std::span<const float> interleavedPosNormalUv) = 0`
  Что делает: загружает меш из массива вершин (позиция+нормаль+uv). Параметры: `interleavedPosNormalUv`. Возвращает: хэндл ресурса.
  Реализация (null-эталон): проверяет, что массив не пуст и кратен 24 float (8 float на вершину × 3 вершины треугольника), иначе `RenderResourceHandle::invalid()`; иначе выдаёт свежий хэндл `nextId_++` и заносит его в набор `resources_`. Vulkan-версия описана в этапе 2.
- `virtual RenderResourceHandle createTextureFromData(std::uint32_t w, std::uint32_t h, std::span<const std::uint8_t> rgba) = 0`
  Что делает: загружает текстуру. Параметры: `w`, `h` — размеры, `rgba` — пиксели. Возвращает: хэндл.
  Реализация (null-эталон): проверяет `w,h > 0` и `rgba.size() == w*h*4`, иначе `invalid()`; иначе выдаёт хэндл `nextId_++` и кладёт в `resources_`. Vulkan-версия описана в этапе 4.
- `virtual void destroy(RenderResourceHandle resource) = 0` — освобождает ресурс.
  Реализация (null-эталон): `resources_.erase(resource.value)`. Vulkan-версия дожидается `vkDeviceWaitIdle` и разрушает буфер/текстуру.

#### Файл `engine/rendering/include/sky/rendering/renderer_registry.hpp`
`struct BackendInit { … }` — параметры инициализации бэкенда; `RendererFactory` —
тип функции-фабрики рендерера.

**`class IRendererRegistry`** — реестр бэкендов рендера.
- `virtual bool registerBackend(const std::string& name, RendererFactory factory) = 0` — регистрирует фабрику бэкенда. Возвращает: успех.
  Реализация: отвергает пустое имя или пустую `factory` (`return false`), иначе `factories_.emplace(name, std::move(factory)).second` — возвращает `false`, если имя уже занято (`std::map<std::string, RendererFactory>`).
- `virtual std::unique_ptr<IRenderer> create(const std::string& name, const BackendInit&) = 0` — Возвращает: рендерер по имени бэкенда.
  Реализация: ищет имя в `factories_`; при попадании вызывает сохранённую функцию-фабрику `it->second(init)`, иначе возвращает `nullptr`.
- `std::unique_ptr<IRendererRegistry> createRendererRegistry()` — фабрика реестра.
  Реализация: `std::make_unique<RendererRegistryImpl>()`; конструктор impl сразу регистрирует встроенный бэкенд `"null"` лямбдой, возвращающей `createNullRenderer()`.

#### Файлы `engine/rendering/include/sky/rendering/null_renderer.hpp`, `engine/rendering/src/{null_renderer,renderer_registry}.cpp`
Пустой рендерер (считает кадры/команды, ничего не рисует) — для тестов контракта, и реализация реестра.
Реализация: `NullRendererImpl` в анонимном namespace хранит `pending_`, счётчики `frameCount_`/`commandsInLastFrame_` и набор `resources_`; `createNullRenderer()` и `createOffscreenSurface()` — фабрики `make_unique`. Счётчики отдаются через `frameCount()`, `commandsInLastFrame()`, `liveResourceCount()`.

### feature/vulkan-offscreen

Инициализация Vulkan, закадровая цель, конвейер, отрисовка треугольника, чтение кадра.

#### Файл `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
- `class VulkanRenderer : public rendering::IRenderer` — добавляет:
  - `virtual bool ready() const = 0` — Возвращает: инициализировался ли рендерер.
    Реализация: возвращает поле `ready_`, которое конструктор `VulkanRendererImpl` выставляет как конъюнкцию всех шагов инициализации (`initInstanceAndDevice() && initTarget() && initPipeline() && initFrameResources() && initShadowResources()`).
  - `virtual std::vector<std::uint8_t> readbackFrame() = 0` — Возвращает: пиксели последнего кадра (RGBA).
    Реализация: см. `readbackFrame()` в `vulkan_renderer.cpp` — при `!ready_` или оконном режиме возвращает пустой вектор, иначе копирует цветное изображение в host-видимый буфер и отдаёт `width*height*4` байт.
  - `virtual std::uint32_t frameWidth() const = 0` / `frameHeight() const = 0` — Возвращает: размеры кадра.
    Реализация: возвращают поля `width_` / `height_` (в оконном режиме их подменяет `initSwapchainTarget()` на `currentExtent` поверхности).
- `std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width, std::uint32_t height)`
  Что делает: создаёт закадровый рендерер. Параметры: `width`, `height`. Возвращает: рендерер или `nullptr`, если Vulkan недоступен.
  Реализация: `std::make_unique<VulkanRendererImpl>(width, height, nullptr)` (target = nullptr → offscreen); возвращает объект, только если `renderer->ready()`, иначе `nullptr`.

#### Файл `engine/rendering_vulkan/src/vulkan_renderer.cpp`
Реализация. Приватные методы (реализуются в этой фиче):
- `initInstanceAndDevice()` — создаёт `VkInstance`, выбирает устройство и графическую очередь, создаёт `VkDevice`.
  Реализация: заполняет `VkApplicationInfo`/`VkInstanceCreateInfo` (в оконном режиме добавляет surface-расширения платформы, под MoltenVK — portability-флаги), `vkCreateInstance`; `vkEnumeratePhysicalDevices` берёт первое устройство, `vkGetPhysicalDeviceQueueFamilyProperties` находит семейство с `VK_QUEUE_GRAPHICS_BIT` (и present-поддержкой при swapchain); затем `vkCreateDevice`, `vkGetDeviceQueue`, и создаёт командный пул (`RESET_COMMAND_BUFFER_BIT`) с одним primary командным буфером.
- `initTarget()` — создаёт изображения цвета (RGBA) и глубины (D32), проход рендеринга и кадровый буфер.
  Реализация: offscreen-ветка через `createImage` создаёт цветное изображение `kColorFormat` (usage `COLOR_ATTACHMENT|TRANSFER_SRC`) и глубину `kDepthFormat` (D32), затем `VkRenderPass` из двух attachment'ов, где finalLayout цвета = `TRANSFER_SRC_OPTIMAL` (готов под readback-копию), и `VkFramebuffer` из обоих видов. В оконном режиме управление уходит в `initSwapchainTarget()`.
- `initPipeline()` — создаёт графический конвейер; формат вершины — позиция+нормаль+uv.
  Реализация: создаёт два `VkDescriptorSetLayout` (set 0 — UBO кадра для vertex+fragment; общий layout combined-image-sampler для текстур), `VkPipelineLayout` с 8 сетами (0 — UBO, 1..6 — PBR-карты, 7 — карта теней) и push-константами `PushBlock`; `createShader` из встроенных `k_mesh_vert_spv`/`k_mesh_frag_spv`; описывает binding вершины (stride 8 float) и три атрибута (`R32G32B32`, `R32G32B32`, `R32G32`), triangle-list, depth-test `LESS_OR_EQUAL`, динамические viewport/scissor; `vkCreateGraphicsPipelines` и разрушает shader-модули.
- `renderFrame()` — записывает команды, очищает, рисует треугольник (`vkCmdDraw`).
  Реализация: первым проходом по `pending_` собирает `FrameUbo` (viewProjection из `SetCamera`, до 4 источников из `AddLight`, счётчики), пишет его в `uboMapped_`; затем записывает командный буфер: depth-only проход теней, потом основной проход (clear, `vkCmdBindPipeline`, привязка set 0 и set 7, viewport/scissor, цикл по `SetSky`/`DrawMesh`); `vkQueueSubmit` + `vkQueueWaitIdle`, в оконном режиме `vkQueuePresentKHR`; в конце `pending_.clear()` и `present()` поверхности.
- `readbackFrame()` — копирует изображение цвета в буфер, доступный CPU, возвращает пиксели.
  Реализация: сбрасывает и начинает командный буфер, `vkCmdCopyImageToBuffer` из `colorImage_` (layout `TRANSFER_SRC_OPTIMAL`, оставленный render pass'ом) в host-видимый `readback_.buffer`, `vkQueueSubmit`+`vkQueueWaitIdle`, затем `std::memcpy` из отображённой памяти `readbackMapped_` в `std::vector` размером `width*height*4`.
- вспомогательные `findMemoryType`, `createImage`, `createBuffer`, `createShader`.
  Реализация: `findMemoryType` перебирает `vkGetPhysicalDeviceMemoryProperties` и возвращает индекс типа с нужными флагами (или `nullopt`); `createImage` создаёт `VkImage` (device-local), выделяет и биндит память, создаёт `VkImageView` заданного aspect; `createBuffer` создаёт `VkBuffer`, берёт host-visible|coherent память, биндит и опционально `vkMapMemory`; `createShader` — `vkCreateShaderModule` из массива `std::uint32_t` SPIR-V.

#### Файлы `engine/rendering_vulkan/shaders/mesh.vert`, `mesh.frag` (+ встроенные `.spv.h`)
Минимальные вершинный и фрагментный шейдеры (позже дорастут до PBR). Компиляция
`glslangValidator -V` → массив `std::uint32_t` встраивается в `.spv.h`.
Реализация: `mesh.vert` `main()` считает `world = pc.model * position`, отдаёт `vWorldPos`, `vNormal = mat3(model)*aNormal`, `vUv`, и `gl_Position = frame.viewProjection * world`. `mesh.frag` `main()` реализует Cook-Torrance metal-rough (описан подробно в этапе 5).

### feature/vulkan-tests

#### Файл `tests/vulkan_tests.cpp`
На lavapipe: `ready()` истинно, кадр рендерится, `readbackFrame()` непустой,
центральный пиксель отличается от углового.

**На выходе должно получиться (список артефактов):**
- Библиотека `sky_rendering_vulkan` собрана; формируется `triangle.png`.
- Тест `vulkan_tests` зелёный на lavapipe.

**Критерий правильности этапа:** на `triangle.png` центральный пиксель
отличается от углового; `readbackFrame()` возвращает непустой массив.

---

## Этап 2 (Неделя 2, веха M1). Меши, камера, освещение, построитель кадра

**Общее описание задач этапа.** Отрисовка мешей с матрицами, камера, свет и
построитель кадра, обходящий сцену. Две фичи.

### feature/vulkan-mesh-lighting

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- `createMeshFromData(std::span<const float>)` — загрузка меша (реализация уже объявленного контракта). Возвращает: хэндл.
  Реализация: при `!ready_`, пустом или не кратном 24 float массиве возвращает `invalid()`; иначе `createVertexBuffer` создаёт host-visible вершинный буфер (`vkMapMemory`+`memcpy`+`vkUnmapMemory`, `vertexCount = size/8`), новый хэндл `nextResource_++` кладётся в `meshes_`.
- `submit(std::span<const RenderCommand>)` — накопление команд кадра.
  Реализация: `pending_.insert(pending_.end(), commands.begin(), commands.end())` — команды копятся до `renderFrame`.
- в `renderFrame` — разбор потока: `SetCamera` (матрицы вида/проекции: `perspective`/`orthographic`), `AddLight` (до 4 источников в `FrameUbo`), `DrawMesh` (модельная матрица и материал в push-константах).
  Реализация: первый проход `switch` по `command.type` наполняет `FrameUbo` — `SetViewport` пересчитывает `aspect`; `SetCamera` строит `viewProjection = proj * viewFromCameraPose(...)` и `cameraPos`; `AddLight` (пока `lightCount < kMaxLights`) пишет `lightVec/lightColor/lightMeta`. Во втором проходе по буферу `DrawMesh` заполняет `PushBlock` (model, baseColor, emissive+roughness, metallic, uvTiling+parallax), зовёт `bindMaterial` и `drawBuffer` (меш из `meshes_` либо запасной `cube_`).
- матричные помощники `perspective(fov,aspect,near,far)`, `orthographic(...)`, `fromTransform(Transform)`, `viewFromCameraPose(...)`.
  Реализация: `perspective` заполняет `Mat4` для Vulkan clip space (y вниз, глубина 0..1: `m[5]=-f`); `orthographic` строит симметричную коробку по мировой высоте и aspect; `fromTransform` разворачивает кватернион+масштаб+позицию в column-major матрицу 4×4; `viewFromCameraPose` берёт сопряжённый кватернион и повёрнутую минус-позицию и прогоняет через `fromTransform` (обратная к позе матрица вида).

### feature/frame-builder

#### Файл `editor/shell/src/frame_builder.hpp`
- `FrameBuilder(EditorContext& context, rendering::IRenderResourceFactory& factory)`
  Что делает: конструктор — привязывает построитель к контексту и фабрике ресурсов. Параметры: `context`, `factory`.
  Реализация: сохраняет ссылки в поля `context_` и `factory_` (список инициализации), больше ничего не делает; кэши мешей/текстур (`meshes_`, `textures_`) пусты.
- `void setCamera(std::optional<core::Transform> pose, float orthoHeight = 0.0f)`
  Что делает: задаёт камеру кадра. Параметры: `pose` — поза камеры (nullopt = камера сцены), `orthoHeight` — высота орто-проекции (0 = перспектива).
  Реализация: запоминает `cameraOverride_ = pose` и `cameraOrthoHeight_ = orthoHeight`; в `build` при заданном override эти значения идут в команду `SetCamera`, при `nullopt` берётся `cameraPose()` из объекта "Main Camera".
- `std::vector<rendering::RenderCommand> build(std::uint32_t width, std::uint32_t height)`
  Что делает: обходит сцену и формирует поток команд отрисовки. Параметры: `width`, `height` — размер кадра. Возвращает: список команд.
  Реализация: пушит `BeginFrame`, `SetViewport(width,height)`, `SetCamera` (override либо `cameraPose()`, fov 50°); `forEachObject` рекурсивно обходит корни и для компонентов `sky.light` добавляет `AddLight` (тип/цвет/интенсивность/дальность из полей); затем `SetSky` с фиксированными горизонтом/зенитом; при валидном терреене пересобирает и рисует его меш; для каждого объекта с `sky.mesh` (кроме террейна) — `DrawMesh` с `applyMaterial(...)` и мешем из `uploadedMesh(...)`; завершает `EndFrame`.

#### Файл `editor/viewport_bridge/include/sky/editor/viewport/viewport_bridge.hpp`
Qt-free фасад вьюпорта (`IPlayModeController`, `IRuntimePreviewHost`), общий для
редактора и плеера.

**На выходе:** демо-сцена рендерится с освещением; `FrameBuilder` даёт непустой поток команд.
**Критерий правильности этапа:** кадр демо-сцены содержит освещённые меши;
поток `RenderCommand` не пуст.

---

## Этап 3 (Недели 3–4, веха M2). Выбор объекта, проекция, небо

**Общее описание задач этапа.** Камера редактора, выбор объекта лучом,
проекция мировой точки в экранную и отрисовка неба. Две фичи.

### feature/editor-camera

#### Файл `editor/shell/src/editor_camera.hpp`
`struct EditorCamera { float yawDegrees; float pitchDegrees; float distance;
core::Vec3 target; float fovDegrees; bool orthographic; … }`.
- `void orbit(float deltaYawDegrees, float deltaPitchDegrees)`
  Что делает: вращает камеру вокруг цели. Параметры: приращения углов в градусах. Возвращает: ничего.
  Реализация: `yawDegrees += deltaYawDegrees;` и `pitchDegrees = std::clamp(pitchDegrees + deltaPitchDegrees, -89.0f, 89.0f)` (тангаж зажат, чтобы не переворачивать камеру).
- `void zoom(float factor)` — приближает/отдаляет. Параметры: `factor` — коэффициент. Возвращает: ничего.
  Реализация: `distance = std::clamp(distance * factor, 1.5f, 400.0f)` — умножает дистанцию до цели на коэффициент с зажимом диапазона.
- `void pan(float deltaRight, float deltaUp)` — сдвигает цель. Параметры: смещения. Возвращает: ничего.
  Реализация: берёт из `rotation()` векторы right/up камеры, `scale = distance * 0.0015f`, и двигает `target` вдоль них: `target + right*(-deltaRight*scale) + up*(deltaUp*scale)`.
- `void lookAlong(int axis)` — снапит вид к оси. Параметры: `axis` (0=+X,1=−X,2=+Y,3=−Y,…). Возвращает: ничего.
  Реализация: `switch(axis)` выставляет фиксированные `yawDegrees`/`pitchDegrees` для шести осевых видов (+X→yaw 90; −X→yaw −90; +Y→pitch 89; −Y→pitch −89; +Z→0/0; −Z→yaw 180); `default` не меняет ничего.
- `core::Transform pose() const` — Возвращает: мировую позу камеры.
  Реализация: `p.rotation = rotation()` (из yaw/pitch); `back = rotate(p.rotation, {0,0,1})`; `p.position = target + back*distance` — камера отведена от цели вдоль локального +Z (смотрит вдоль −Z).
- `float orthoHeight() const` — Возвращает: высоту орто-проекции (0 = перспектива).
  Реализация: `orthographic ? orthoHalfHeight()*2.0f : 0.0f`, где `orthoHalfHeight() = distance * tan(fovDegrees*π/360)` — орто-высота подобрана под перспективное кадрирование на дистанции цели.

Выбор объекта лучом (`pick`) и проекция мира в экран (`project`) реализуются в
мосте контура E5 поверх этой камеры (совместная фича).
Реализация: `EditorCamera::project(world, w, h, outX, outY)` уже в `editor_camera.hpp` — переводит точку в камеро-локальное пространство сопряжённым кватернионом, отбраковывает точки за камерой (`local.z >= 0`), делит на `-z` (перспектива) либо на орто-полуразмер и мапит NDC в пиксели. Пикинг строится в E5 поверх `rayThrough(...)`/`rayOrigin(...)` (луч через пиксель) пересечением с геометрией.

### feature/sky-backdrop

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- обработка команды `RenderCommandType::SetSky` — купол-небо: горизонт (`color`)
  → зенит (`emissive`), диск солнца от направленного света. Реализуется как
  большой куб, приклеенный к камере, с флагом «небо» в push-константах.
  Реализация: в цикле draw'ов случай `SetSky` строит `Transform dome` с позицией = `cameraPos` и масштабом 300, пишет в `PushBlock` модельную матрицу, `baseColor.rgb = command.color` (горизонт) с `baseColor.w = 1.0` (флаг skyMode), `emissive.rgb = command.emissive` (зенит), зовёт `bindMaterial` (сеты должны быть привязаны) и `drawBuffer(cube_, push)`. В `mesh.frag` ветка `skyMode` смешивает горизонт↔зенит по `dir.y` и добавляет диск солнца от направленных источников через `pow(towardSun, …)`.

**На выходе:** клик по вьюпорту возвращает объект; мировая точка проецируется в экранную; небо отрисовано.
**Критерий правильности этапа:** центральный луч кадрированной камеры попадает
в объект; проекция origin объекта близка к центру экрана.

---

## Этап 4 (Недели 5–6, веха M3). Текстуры и слоты PBR

**Общее описание задач этапа.** Загрузка текстур, привязка слотов
PBR-материала и процедурные примитивы. Две фичи.

### feature/pbr-textures

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- `createTextureFromData(w, h, rgba)` — загрузка текстуры. Возвращает: хэндл.
  Реализация: при `!ready_`, нулевых размерах или `rgba.size() != w*h*4` возвращает `invalid()`; иначе `uploadTexture(w,h,data)` создаёт GPU-текстуру с дескриптором, и при валидном `texture.set` хэндл `nextResource_++` заносится в `textures_`.
- `uploadTexture(...)` — создаёт изображение GPU, вид и дескриптор.
  Реализация: заливает пиксели в staging-буфер (`TRANSFER_SRC`), создаёт device-local `VkImage` формата `kColorFormat` (`TRANSFER_DST|SAMPLED`), одноразовым командным буфером делает барьер `UNDEFINED→TRANSFER_DST`, `vkCmdCopyBufferToImage`, барьер `TRANSFER_DST→SHADER_READ_ONLY`; затем `VkImageView`, выделяет из пула один дескрипторный сет `textureSetLayout_` и `vkUpdateDescriptorSets` combined-image-sampler (`sampler_` + view). Возвращает `GpuTexture`.
- `bindMaterial(command)` — привязывает шесть дескрипторных сетов PBR
  (albedo/normal/roughness/metallic/occlusion/height); отсутствующий слот
  заменяется нейтральным значением (белый / плоская нормаль).
  Реализация: лямбда `setFor(handle, fallback)` берёт сет из `textures_` по хэндлу либо сет заглушки; собирает массив из 6 сетов (albedo/roughness/metallic/occlusion/height → `whiteTexture_`, normal → `flatNormalTexture_`) и одним `vkCmdBindDescriptorSets` привязывает их к слотам 1..6 `pipelineLayout_`.

### feature/procedural-primitives

#### Дополнение файла `engine/scene/src/scene_authoring.cpp`
- `createPrimitive(...)` для `PrimitiveKind::Sphere`/`Plane` строит вершины
  процедурно (а не куб-заглушку).
  Реализация: `createPrimitive` создаёт объект, аттачит компонент `sky.mesh`, задаёт поля `material="Default"` и `mesh=primitiveMeshName(kind)` (`"cube"/"plane"/"sphere"`); сами процедурные вершины строятся ниже по конвейеру в `FrameBuilder::buildPrimitive` (`uploadedMesh` → `createMeshFromData`): "plane" — двусторонний XZ-квад 1×1 (6+6 индексов, нормали ±Y), "sphere" — UV-сфера радиуса 0.5 из `kStacks=16 × kSlices=24` с нормалями = позиция·2 и uv по θ/φ.

**На выходе:** материалы используют текстурные слоты; сфера/плоскость строятся процедурно.
**Критерий правильности этапа:** отсутствующие текстурные слоты заменяются
нейтральными значениями; примитивы — не куб-заглушки.

---

## Этап 5 (Недели 7–8, веха M4). Тени

**Общее описание задач этапа.** Теневое картирование от направленного
источника. Одна фича.

### feature/shadow-mapping

#### Файл `engine/rendering_vulkan/shaders/shadow.vert` (+ `shadow.vert.spv.h`)
Depth-only вершинный шейдер: `lightViewProjection * model * position`; обе
матрицы в push-константах (`struct ShadowPush`).
Реализация: `main()` без выходов и без дескрипторных сетов вычисляет только `gl_Position = pc.lightViewProjection * pc.model * vec4(aPosition, 1.0)`; атрибуты нормали и uv присутствуют в формате вершины, но не используются.

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- `initShadowResources()` — создаёт сэмплируемую карту глубины 2048×2048 (D32),
  сэмплер с зажимом к границе, отдельный проход и конвейер (со slope-scaled
  bias), дескриптор set 7.
  Реализация: `createImage` строит D32-изображение `kShadowMapSize²` (`DEPTH_STENCIL_ATTACHMENT|SAMPLED`); сэмплер `NEAREST` с `CLAMP_TO_BORDER` и белым бордюром (за коробкой света = «далеко» = освещено); render pass с одним depth-attachment (finalLayout `SHADER_READ_ONLY`) и зависимостью `LATE_FRAGMENT_TESTS→FRAGMENT_SHADER`; framebuffer; layout только с push-константами `ShadowPush`; конвейер из `k_shadow_vert_spv` c включённым slope-scaled bias (`depthBiasConstantFactor=1.25`, `slopeFactor=1.75`); наконец выделяет и заполняет дескрипторный сет `shadowSet_` (combined-image-sampler карты теней), который основной проход биндит на set 7.
- в `renderFrame` — сначала depth-only проход теней по всем `DrawMesh`, затем
  основной проход с привязкой карты теней.
  Реализация: открывает `shadowPass_`/`shadowFramebuffer_` с очисткой глубины в 1.0; при наличии теневого источника биндит `shadowPipeline_`, viewport/scissor `kShadowMapSize`, и для каждого `DrawMesh` (небо тени не отбрасывает) пишет `ShadowPush{lightViewProjection, model}` push-константами и рисует меш. Проход выполняется всегда (даже без источника), чтобы карта осела в sampled-layout; в основном проходе `shadowSet_` привязан на set 7.
- `FrameUbo` растёт: `lightViewProjection[16]`, флаг теней и индекс источника.
  Реализация: в первом проходе `renderFrame` первый направленный источник становится теневым (`shadowLight`, `shadowDir`); из него строится `lightViewProjection = orthographic(90,1,1,140) * lookAlong(eye, shadowDir)` (глаз отодвинут на 60 вдоль −dir); в `FrameUbo` пишутся `counts[1]=1` (тени вкл), `counts[2]=shadowLight` и матрица `lightViewProjection`, затем `memcpy` в `uboMapped_`.
- в `mesh.frag` — функция `shadowVisibility(worldPos, ndl)`: выборка 3×3 (PCF) с
  нормаль-зависимым смещением.
  Реализация: проецирует `worldPos` через `frame.lightViewProjection`, делит на w, мапит в uv `[0,1]`; точки вне коробки/диапазона глубины возвращают 1.0 (освещено); смещение `bias = max(0.0025*(1-ndl), 0.0006)`; в цикле 3×3 по текселям сравнивает `ndc.z - bias` с выборкой карты и усредняет на 9 (доля освещённости). В основном цикле света множится на радиантность только для теневого источника (`i == counts.z`).

**На выходе:** объекты отбрасывают тени в редакторе и плеере (подтверждается скриншотом).
**Критерий правильности этапа:** на кадре видна тень под объектом; тень
движется вместе с падающим телом.
