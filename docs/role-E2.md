# ТЗ · E2 — Рендеринг (весь срок)

**Роль.** Владелец всего, что рисует: контракт рендера, Vulkan-бэкенд
(offscreen + swapchain), PBR, тени, мост вьюпорта. Твой offscreen-readback —
фундамент всей верификации проекта, поэтому он нужен рано.

**Стек.** C++20, Vulkan, GLSL. **Модули:** `rendering`, `rendering_vulkan`,
`editor/viewport_bridge`, `editor/shell/frame_builder`.

## Твои суставы

| Интерфейс | Кому |
|---|---|
| `RenderCommand`, `IRenderer` | E1 (FrameBuilder), E4 (плеер) |
| `VulkanRenderer`, `readbackFrame()` | E3 (вьюпорт-панель), E5 (скриншот-тесты) |
| `FrameBuilder` | E3, E4 (собирают кадр из сцены) |

Формат вершин фиксируешь на неделе 1 (`position+normal+uv`) — он append-only.

---

## Неделя 1 — Vulkan до треугольника в PNG
- `[H] rendering/rendering.hpp` — `RenderCommandType`, `RenderCommand`, `IRenderer`.
- `[H] rendering_vulkan/vulkan_backend.hpp` — `VulkanRenderer`, `readbackFrame()`, `createVulkanRenderer(w,h)`.
- `[S] vulkan_renderer.cpp` — по тикетам: `initInstanceAndDevice` (E2-1) → `initOffscreenTarget` (E2-2) → `initPipeline`/`renderFrame` (E2-3) → `readbackFrame` (E2-4); хелперы `findMemoryType/createImage/createBuffer`.
- `[H+B] shaders/{mesh.vert,mesh.frag}` + `.spv.h` (glslangValidator → embed).
- `[T] tests/vulkan_tests.cpp` — на lavapipe: `ready()`, кадр, непустой readback, центр ≠ угол.
**Разблокируешь:** E5 (скриншот-верификация), E3/E4 (кадр).
**Готово:** `triangle.png` из readback — треугольник виден.

## Неделя 2 (M1) — меши, камера, свет, PBR v1
- `[S] vulkan_renderer.cpp` (дополнить): `FrameUbo`, `PushBlock`, матрицы `perspective/orthographic/fromTransform/viewFromCameraPose`; `createMeshFromData`; разбор потока `SetCamera/AddLight/DrawMesh`.
- `[H] editor/shell/src/frame_builder.hpp` — `FrameBuilder(EditorContext&,IRenderer&)`, `setCamera`, `build(w,h)`, `forEachObject`, `applyMaterial`.
- `[H] viewport_bridge/viewport_bridge.hpp` — Qt-free фасад камеры/размеров.
**Зависишь:** E1 (сцена для обхода). **Разблокируешь:** E3 (панель-вьюпорт), E4 (плеер рисует).
**Готово:** демо-сцена рендерится с освещением.

## Недели 3–4 (M2) — пикинг, проекция, Game-вью, скайбокс
- `[H] editor/shell/src/editor_camera.hpp` — `EditorCamera` (orbit/pan/zoom/pose/orthoHeight); `pick(x,y,w,h)`, `project(world,w,h)`.
- `[S] vulkan_renderer.cpp` (дополнить): обработка `SetSky` — купол-скайбокс со `skyMode`.
- Второй offscreen для Game-вью (через Main Camera).
**Разблокируешь:** E3 (гизмо через project, клик через pick).

## Недели 5–6 (M3) — текстуры, PBR-слоты, примитивы
- `[S] vulkan_renderer.cpp` (дополнить): `uploadTexture` → `GpuTexture`; дескрипторные сеты 1..6 (albedo/normal/roughness/metallic/occlusion/height); `bindMaterial` с нейтральными дефолтами.
- `[S] scene/scene_authoring.cpp` (совместно с E1): процедурная геометрия `Sphere`/`Plane`.
- Оверлей статистики (tri-count из команд).
**Зависишь:** E5 (импортеры текстур).

## Недели 7–8 (M4) — тени
- `[H+B] shaders/shadow.vert` + `.spv.h` — depth-only, `ShadowPush{lightVP, model}`.
- `[S] vulkan_renderer.cpp` (дополнить): `initShadowResources()` (D32 2048², clamp-to-border, отдельный pass, set 7); в `renderFrame` — сперва shadow pass, потом основной; `FrameUbo` растёт `lightViewProjection`+`counts.y/z`.
- `[S] mesh.frag` (дополнить): `shadowVisibility(worldPos,ndl)` — 3×3 PCF + normal-bias.
**Готово:** ящики отбрасывают тени в редакторе и в плеере (скриншот).

## Твои личные ворота
- M1: `triangle.png` + освещённая демо-сцена.
- M2: пикинг и проекция для гизмо E3.
- M4: тени в кадре.
