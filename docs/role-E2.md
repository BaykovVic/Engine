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
- `virtual void attachSurface(IRenderSurface& surface) = 0` — привязывает поверхность вывода. Параметры: `surface`.
- `virtual void submit(std::span<const RenderCommand> commands) = 0` — принимает поток команд кадра. Параметры: `commands`.
- `virtual void renderFrame() = 0` — рисует накопленный кадр.

**`class IRenderResourceFactory`** — создание ресурсов GPU.
- `virtual RenderResourceHandle createMeshFromData(std::span<const float> interleavedPosNormalUv) = 0`
  Что делает: загружает меш из массива вершин (позиция+нормаль+uv). Параметры: `interleavedPosNormalUv`. Возвращает: хэндл ресурса.
- `virtual RenderResourceHandle createTextureFromData(std::uint32_t w, std::uint32_t h, std::span<const std::uint8_t> rgba) = 0`
  Что делает: загружает текстуру. Параметры: `w`, `h` — размеры, `rgba` — пиксели. Возвращает: хэндл.
- `virtual void destroy(RenderResourceHandle resource) = 0` — освобождает ресурс.

#### Файл `engine/rendering/include/sky/rendering/renderer_registry.hpp`
`struct BackendInit { … }` — параметры инициализации бэкенда; `RendererFactory` —
тип функции-фабрики рендерера.

**`class IRendererRegistry`** — реестр бэкендов рендера.
- `virtual bool registerBackend(const std::string& name, RendererFactory factory) = 0` — регистрирует фабрику бэкенда. Возвращает: успех.
- `virtual std::unique_ptr<IRenderer> create(const std::string& name, const BackendInit&) = 0` — Возвращает: рендерер по имени бэкенда.
- `std::unique_ptr<IRendererRegistry> createRendererRegistry()` — фабрика реестра.

#### Файлы `engine/rendering/include/sky/rendering/null_renderer.hpp`, `engine/rendering/src/{null_renderer,renderer_registry}.cpp`
Пустой рендерер (считает кадры/команды, ничего не рисует) — для тестов контракта, и реализация реестра.

### feature/vulkan-offscreen

Инициализация Vulkan, закадровая цель, конвейер, отрисовка треугольника, чтение кадра.

#### Файл `engine/rendering_vulkan/include/sky/rendering_vulkan/vulkan_backend.hpp`
- `class VulkanRenderer : public rendering::IRenderer` — добавляет:
  - `virtual bool ready() const = 0` — Возвращает: инициализировался ли рендерер.
  - `virtual std::vector<std::uint8_t> readbackFrame() = 0` — Возвращает: пиксели последнего кадра (RGBA).
  - `virtual std::uint32_t frameWidth() const = 0` / `frameHeight() const = 0` — Возвращает: размеры кадра.
- `std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width, std::uint32_t height)`
  Что делает: создаёт закадровый рендерер. Параметры: `width`, `height`. Возвращает: рендерер или `nullptr`, если Vulkan недоступен.

#### Файл `engine/rendering_vulkan/src/vulkan_renderer.cpp`
Реализация. Приватные методы (реализуются в этой фиче):
- `initInstanceAndDevice()` — создаёт `VkInstance`, выбирает устройство и графическую очередь, создаёт `VkDevice`.
- `initOffscreenTarget()` — создаёт изображения цвета (RGBA) и глубины (D32), проход рендеринга и кадровый буфер.
- `initPipeline()` — создаёт графический конвейер; формат вершины — позиция+нормаль+uv.
- `renderFrame()` — записывает команды, очищает, рисует треугольник (`vkCmdDraw`).
- `readbackFrame()` — копирует изображение цвета в буфер, доступный CPU, возвращает пиксели.
- вспомогательные `findMemoryType`, `createImage`, `createBuffer`, `createShader`.

#### Файлы `engine/rendering_vulkan/shaders/mesh.vert`, `mesh.frag` (+ встроенные `.spv.h`)
Минимальные вершинный и фрагментный шейдеры (позже дорастут до PBR). Компиляция
`glslangValidator -V` → массив `std::uint32_t` встраивается в `.spv.h`.

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
- `submit(std::span<const RenderCommand>)` — накопление команд кадра.
- в `renderFrame` — разбор потока: `SetCamera` (матрицы вида/проекции: `perspective`/`orthographic`), `AddLight` (до 4 источников в `FrameUbo`), `DrawMesh` (модельная матрица и материал в push-константах).
- матричные помощники `perspective(fov,aspect,near,far)`, `orthographic(...)`, `fromTransform(Transform)`, `viewFromCameraPose(...)`.

### feature/frame-builder

#### Файл `editor/shell/src/frame_builder.hpp`
- `FrameBuilder(EditorContext& context, rendering::IRenderResourceFactory& factory)`
  Что делает: конструктор — привязывает построитель к контексту и фабрике ресурсов. Параметры: `context`, `factory`.
- `void setCamera(std::optional<core::Transform> pose, float orthoHeight = 0.0f)`
  Что делает: задаёт камеру кадра. Параметры: `pose` — поза камеры (nullopt = камера сцены), `orthoHeight` — высота орто-проекции (0 = перспектива).
- `std::vector<rendering::RenderCommand> build(std::uint32_t width, std::uint32_t height)`
  Что делает: обходит сцену и формирует поток команд отрисовки. Параметры: `width`, `height` — размер кадра. Возвращает: список команд.

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
- `void zoom(float factor)` — приближает/отдаляет. Параметры: `factor` — коэффициент. Возвращает: ничего.
- `void pan(float deltaRight, float deltaUp)` — сдвигает цель. Параметры: смещения. Возвращает: ничего.
- `void lookAlong(int axis)` — снапит вид к оси. Параметры: `axis` (0=+X,1=−X,2=+Y,3=−Y,…). Возвращает: ничего.
- `core::Transform pose() const` — Возвращает: мировую позу камеры.
- `float orthoHeight() const` — Возвращает: высоту орто-проекции (0 = перспектива).

Выбор объекта лучом (`pick`) и проекция мира в экран (`project`) реализуются в
мосте контура E5 поверх этой камеры (совместная фича).

### feature/sky-backdrop

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- обработка команды `RenderCommandType::SetSky` — купол-небо: горизонт (`color`)
  → зенит (`emissive`), диск солнца от направленного света. Реализуется как
  большой куб, приклеенный к камере, с флагом «небо» в push-константах.

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
- `uploadTexture(...)` — создаёт изображение GPU, вид и дескриптор.
- `bindMaterial(command)` — привязывает шесть дескрипторных сетов PBR
  (albedo/normal/roughness/metallic/occlusion/height); отсутствующий слот
  заменяется нейтральным значением (белый / плоская нормаль).

### feature/procedural-primitives

#### Дополнение файла `engine/scene/src/scene_authoring.cpp`
- `createPrimitive(...)` для `PrimitiveKind::Sphere`/`Plane` строит вершины
  процедурно (а не куб-заглушку).

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

#### Дополнение файла `engine/rendering_vulkan/src/vulkan_renderer.cpp`
- `initShadowResources()` — создаёт сэмплируемую карту глубины 2048×2048 (D32),
  сэмплер с зажимом к границе, отдельный проход и конвейер (со slope-scaled
  bias), дескриптор set 7.
- в `renderFrame` — сначала depth-only проход теней по всем `DrawMesh`, затем
  основной проход с привязкой карты теней.
- `FrameUbo` растёт: `lightViewProjection[16]`, флаг теней и индекс источника.
- в `mesh.frag` — функция `shadowVisibility(worldPos, ndl)`: выборка 3×3 (PCF) с
  нормаль-зависимым смещением.

**На выходе:** объекты отбрасывают тени в редакторе и плеере (подтверждается скриншотом).
**Критерий правильности этапа:** на кадре видна тень под объектом; тень
движется вместе с падающим телом.
